#pragma once
#include <list>
#include <map>
#include "ps_common.h"
#include "pspacket.h"
#include "Common.h"
#include "Config.h"
#include "ClientRequest.h"
#include "ConnectionPool.h"
#include "rdkafkacpp.h"
#include "sms-cdr-avro.hh"


class KafkaEventCallback : public RdKafka::EventCb
{
public:
    KafkaEventCallback();
    void event_cb (RdKafka::Event &event);
private:
    bool allBrokersDown;
};


class Server
{
public:
    Server();
    bool Initialize(const Config &config, ConnectionPool *cp, std::string &errDescription);
	~Server();
	void Run();
    void Stop();

private:
	static const int MAX_CLIENT_CONNECTIONS = 10;
	static const int PARSE_ERROR = -1;

    std::string kafkaTopic;
    std::unique_ptr<RdKafka::Conf> kafkaGlobalConf;
    std::unique_ptr<RdKafka::Conf> kafkaTopicConf;
    std::unique_ptr<RdKafka::Producer> kafkaProducer;
    KafkaEventCallback eventCb;
    int udpSocket;
    bool shutdownInProgress;
    ConnectionPool* connectionPool;
    void ProcessIncomingData(const char* buffer, int bufferSize, sockaddr_in &senderAddr);
    int ProcessNextRequestFromBuffer(const char* buffer, int maxLen, sockaddr_in& senderAddr);
    bool SendNotAcceptedResponse(sockaddr_in &senderAddr, uint32_t requestNum, std::string errDescr);
	bool SendIAMAliveResponse(sockaddr_in& senderAddr, uint32_t requestNum, std::string errDescr);
    void SendClientResponses();
    void SendSmsToKafka(ClientRequest* request, bool responseSendSuccess);
    std::vector<uint8_t> EncodeCdr(const SMS_CDR &avroCdr);
    std::string IPAddr2Text(const in_addr& pinAddr);
    void WaitForKafkaQueue();
};

