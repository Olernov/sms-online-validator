#include <sstream>
#include <signal.h>
#include "Server.h"
#include "LogWriterOtl.h"
#include "ClientRequest.h"
#include "SmsRequest.h"
#include "CamelRequest.h"
#include "CallFinishRequest.h"

extern LogWriterOtl logWriter;

void CloseSocket(int socket)
{
#ifdef WIN32
    shutdown(socket, SD_BOTH);
    closesocket(socket);
#else
    shutdown(socket, SHUT_RDWR);
    close(socket);
#endif
}


Server::Server() :
    shutdownInProgress(false),
    connectionPool(nullptr),
    kafkaGlobalConf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
    kafkaTopicConf(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC))
{}


bool Server::Initialize(const Config& config,  ConnectionPool* cp, std::string& errDescription)
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket < 0) {
        errDescription = "Unable to create server socket SOCK_DGRAM, IPPROTO_UDP.";
        return false;
	}
#ifndef _WIN32
    int optval = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
	struct sockaddr_in serverAddr;
	memset((char *) &serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(config.serverPort);
    if (bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0) {
        errDescription = "Failed to call bind on server socket. Error #" + std::to_string(errno);
        return false;
	}
    connectionPool = cp;
    if (!config.kafkaBroker.empty()) {
        std::string errstr;
        if (kafkaGlobalConf->set("bootstrap.servers", config.kafkaBroker, errstr) != RdKafka::Conf::CONF_OK
                || kafkaGlobalConf->set("group.id", "pcrf-emitter", errstr) != RdKafka::Conf::CONF_OK
                || kafkaGlobalConf->set("api.version.request", "true", errstr) != RdKafka::Conf::CONF_OK) {
            errDescription = "Failed to set kafka global conf: " + errstr;
            return false;
        }
        kafkaGlobalConf->set("event_cb", &eventCb, errstr);

        kafkaProducer = std::unique_ptr<RdKafka::Producer>(RdKafka::Producer::create(kafkaGlobalConf.get(), errstr));
        if (!kafkaProducer) {
            errDescription = "Failed to create kafka producer: " + errstr;
            return false;
        }
        kafkaTopic = config.kafkaTopic;
    }
    else {
        kafkaProducer = nullptr;
        logWriter << "Kafka broker is not set. SMS Validator will not log events to Kafka";
    }
        
    return true;
}


Server::~Server()
{
    CloseSocket(udpSocket);
}


void Server::Run()
{
	while (!shutdownInProgress) {
        SendClientResponses();
        char receiveBuffer[65000];
        sockaddr_in senderAddr;
#ifndef _WIN32
        socklen_t senderAddrSize = sizeof(senderAddr);
#else
		int senderAddrSize = sizeof(senderAddr);
#endif
        int recvBytes = recvfrom(udpSocket, receiveBuffer, sizeof(receiveBuffer), 0,
                                 reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrSize);
        if (recvBytes > 0) {
            logWriter.Write(std::to_string(recvBytes) + " bytes received from " + IPAddr2Text(senderAddr.sin_addr), 
				mainThreadIndex, debug);
            ProcessIncomingData(receiveBuffer, recvBytes, senderAddr);
        }
        else if (recvBytes == SOCKET_ERROR && errno != EAGAIN) {
            logWriter << "Error receiving data from socket (error code "
                    + std::to_string(errno) + "). ";
        }
    }
    SendClientResponses();
    WaitForKafkaQueue();
}


void Server::ProcessIncomingData(const char* buffer, int bufferSize, sockaddr_in& senderAddr)
{
	int bytesProcessed = 0;
    while(bytesProcessed < bufferSize) {
      int requestLen = ProcessNextRequestFromBuffer(buffer + bytesProcessed, bufferSize - bytesProcessed, senderAddr);
	  if (requestLen >= 0) {
		  bytesProcessed += requestLen;
	  }
	  else {
		  break;
	  }
    }
}

/* Function returns length of next successfully processed request from buffer 
	or PARSE_ERROR in case of failure.
*/
int Server::ProcessNextRequestFromBuffer(const char* buffer, int maxLen, sockaddr_in& senderAddr)
{
	std::multimap<__uint16_t, SPSReqAttrParsed> requestAttrs;
	CPSPacket pspRequest;
	uint32_t requestNum;
    uint16_t requestType;
    uint16_t packetLen;
    const int VALIDATE_PACKET = 1;
    
    int parseRes = pspRequest.Parse((SPSRequest *)buffer, maxLen,
                      requestNum, requestType, packetLen, requestAttrs, VALIDATE_PACKET);
	if (parseRes == PARSE_ERROR) {
		logWriter.Write("Unable to parse incoming data.", mainThreadIndex, error);
        return PARSE_ERROR;
	}
    logWriter.Write("request length: " + std::to_string(packetLen), mainThreadIndex, debug);
	std::string errorDescr;
    if(requestType != VALIDATEEX_REQ && requestType != ARE_Y_ALIVE && requestType != QUOTA_REQ
            && requestType != CALL_FINISH_INFO) {
		errorDescr = "Unsupported request type " + std::to_string(requestType);
        logWriter.Write(errorDescr, mainThreadIndex, error);
        SendNotAcceptedResponse(senderAddr, requestNum, errorDescr);
        return packetLen;
    }
	if (requestType == ARE_Y_ALIVE) {
		logWriter.Write("ARE_YOU_ALIVE_REQUEST received, sending I_AM_ALIVE response", mainThreadIndex, debug);
		SendIAMAliveResponse(senderAddr, requestNum, errorDescr);
		return packetLen;
	}

    logWriter.Write("Request #" + std::to_string(requestNum) + " received from " + IPAddr2Text(senderAddr.sin_addr) 
		+ ". Size: " + std::to_string(packetLen) + " bytes.", mainThreadIndex, notice);
    ClientRequest* clientRequest = nullptr;
    switch (requestType) {
    case VALIDATEEX_REQ:
        clientRequest = new SmsRequest(senderAddr);
        break;
    case QUOTA_REQ:
        clientRequest = new CamelRequest(senderAddr);
        break;
    case CALL_FINISH_INFO:
        clientRequest = new CallFinishRequest(senderAddr);
        break;
    }

    if (!clientRequest->ValidateAndSetParams(requestNum, requestAttrs, errorDescr)) {
        logWriter.Write("Request #" + std::to_string(requestNum) + " rejected due to: " + errorDescr,
                        mainThreadIndex, error);
        SendNotAcceptedResponse(senderAddr, requestNum, errorDescr);
        return packetLen;
	}
	
    connectionPool->PushRequest(clientRequest);
    return packetLen;
}


bool Server::SendNotAcceptedResponse(sockaddr_in& senderAddr, uint32_t requestNum,  std::string errDescr)
{
	CPSPacket pspResponse;
	char buffer[2014];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), requestNum, VALIDATEEX_RESP) != 0) {
        logWriter.Write("SendNotAcceptedResponse error: initializing response buffer failed", mainThreadIndex, error);
        return false;
    }
    int32_t errorCode = BAD_REQUEST;
    int32_t errorCodeN = htonl(errorCode);

	unsigned long len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), 
            PS_RESULT, &errorCodeN, sizeof(errorCodeN));
	len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), 
			PS_DESCR, errDescr.data(), errDescr.size());
    if(sendto(udpSocket, buffer, len, 0, reinterpret_cast<sockaddr*>(&senderAddr), sizeof(senderAddr)) <= 0) {
        logWriter.Write("SendNotAcceptedResponse error: socket error " + std::to_string(errno), mainThreadIndex, error);
        return false;
    }
	return true;
}


void Server::SendClientResponses()
{
    ClientRequest* request;
    std::string errorDescr;
    bool responseSendSuccess;
    while((request = connectionPool->PopProcessedRequest()) != nullptr) {
        if (request->SendResultToClient(udpSocket, errorDescr)) {
            responseSendSuccess = true;
            logWriter.Write("Response #" + std::to_string(request->requestNum)
                            + " sent to client. ", mainThreadIndex, notice);
        }
        else {
            responseSendSuccess = false;
            logWriter.Write("SendRequestResultToClient error: " + errorDescr, mainThreadIndex, error);
        }

        if (kafkaProducer != nullptr) {
            request->LogToKafka(kafkaProducer.get(), kafkaTopic, responseSendSuccess);
        }
        delete request;
    }
}


bool Server::SendIAMAliveResponse(sockaddr_in& senderAddr, uint32_t requestNum,  std::string errDescr)
{
	CPSPacket pspResponse;
	char buffer[2014];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), requestNum, I_AM_ALIVE) != 0) {
        logWriter.Write("SendIAMAliveResponse error: initializing response buffer failed", mainThreadIndex, error);
        return false;
    }
	if(sendto(udpSocket, buffer, sizeof(SPSRequest), 0, reinterpret_cast<sockaddr*>(&senderAddr), sizeof(senderAddr)) <= 0) {
        logWriter.Write("SendIAMAliveResponse error: socket error " + std::to_string(errno), mainThreadIndex, error);
        return false;
    }
	return true;
}

std::string Server::IPAddr2Text(const in_addr& inAddr)
{
	char buffer[64];
#ifdef WIN32
    _snprintf_s(buffer, sizeof(buffer) - 1, "%d.%d.%d.%d", inAddr.S_un.S_un_b.s_b1, inAddr.S_un.S_un_b.s_b2, 
		inAddr.S_un.S_un_b.s_b3, inAddr.S_un.S_un_b.s_b4);
#else
    inet_ntop(AF_INET, (const void*) &inAddr, buffer, sizeof(buffer) - 1);
#endif
	return std::string(buffer);
}

void Server::Stop()
{
    shutdownInProgress = true;

}



KafkaEventCallback::KafkaEventCallback() :
    allBrokersDown(false)
{}

void KafkaEventCallback::event_cb (RdKafka::Event &event)
{
    switch (event.type())
    {
      case RdKafka::Event::EVENT_ERROR:
        logWriter << "Kafka ERROR (" + RdKafka::err2str(event.err()) + "): " + event.str();
        if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
            allBrokersDown = true;
        break;
      case RdKafka::Event::EVENT_STATS:
        logWriter << "Kafka STATS: " + event.str();
        break;
      case RdKafka::Event::EVENT_LOG:
        logWriter << "Kafka LOG-" + std::to_string(event.severity()) + "-" + event.fac() + ":" + event.str();
        break;
      default:
        logWriter << "Kafka EVENT " + std::to_string(event.type()) + " (" +
                     RdKafka::err2str(event.err()) + "): " + event.str();
        break;
    }
}



void Server::WaitForKafkaQueue()
{
    const int producerPollTimeoutMs = 1000;
    std::string lastErrorMessage;
    while (kafkaProducer->outq_len() > 0)   {
        std::string message = std::to_string(kafkaProducer->outq_len()) + " message(s) are in Kafka producer queue. "
                "Waiting to be sent...";
        if (message != lastErrorMessage) {
            logWriter << message;
            lastErrorMessage = message;
        }
        kafkaProducer->poll(producerPollTimeoutMs);
    }
}

