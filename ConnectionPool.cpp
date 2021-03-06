#include <sstream>
#include "ConnectionPool.h"
#include "otl_utils.h"


extern LogWriterOtl logWriter;

ConnectionPool::ConnectionPool() :
    initialized(false),
    stopFlag(false),
    incomingRequests(queueSize)
{
}


ConnectionPool::~ConnectionPool()
{
    stopFlag = true;
    conditionVar.notify_all();
    for(auto& thr : workerThreads) {
        if (thr.joinable()) {
            thr.join();
        }
    }
    for(auto& db : dbConnectPool) {
        delete db;
    }
}


bool ConnectionPool::Initialize(const Config& config, int socket, RdKafka::Producer* kafkaProd,
                                std::string& errDescription)
{
    if (initialized) {
        errDescription = "Connection pool is already initialized";
        return false;
    }
    const int OTL_MULTITHREADED_MODE = 1;
    otl_connect::otl_initialize(OTL_MULTITHREADED_MODE);
    dbConnectPool.resize(config.connectionCount);
    for(auto& db : dbConnectPool) {
        try {
            db = new DBConnect;
            db->rlogon(config.connectString.c_str());
        }
        catch(const otl_exception& ex) {
            errDescription = "**** DB ERROR while logging to DB: **** " +
                crlf + OTL_Utils::OtlExceptionToText(ex);
            return false;
        }
    }
    for (unsigned int i = 0; i < config.connectionCount; ++i) {
        workerThreads.push_back(std::thread(&ConnectionPool::WorkerThread, this, i, dbConnectPool[i]));
    }
    udpSocket = socket;
    kafkaProducer = kafkaProd;
    logWriter << "Connection pool initialized successfully";
    initialized = true;
    return true;
}


void ConnectionPool::PushRequest(ClientRequest *clientRequest)
{
    incomingRequests.push(clientRequest);
    conditionVar.notify_one();

}


void ConnectionPool::WorkerThread(unsigned int index, DBConnect* dbConnect)
{
    while (!stopFlag) {
        std::unique_lock<std::mutex> ul(lock);
        conditionVar.wait(ul);
        ul.unlock();

        ClientRequest* request;
        while (incomingRequests.pop(request)) {
            double requestAgeSec = duration<double>(system_clock::now() - request->accepted).count();
            if (requestAgeSec < maxRequestAgeSec) {
                ProcessRequest(index, request, dbConnect);
            }
            else {
                std::stringstream ss;
                ss << "Request #" << request->requestNum << " discarded due to max age exceeding ("
                   << round(requestAgeSec * 1000) << " ms)";
                logWriter << ss.str();
            }
            delete request;
        }
    }
}


void ConnectionPool::ProcessRequest(unsigned int index, ClientRequest *request, DBConnect* dbConnect)
{
    try {
        std::stringstream ss;
        ss << "Started processing of request #" << request->requestNum << " by thread #" << index;
        logWriter.Write(ss.str(), index, debug);

        request->Process(dbConnect);
        SendResponseAndLogToKafka(request);

        ss.str(std::string());
        ss << "Request #" << request->requestNum << " processed by thread #" << index
           << " in " << round(duration<double>(system_clock::now() - request->accepted).count() * 1000)
           << " ms. ";
        logWriter << ss.str();
        request->DumpResults();

    }
    catch(const otl_exception& ex) {
        request->resultDescr = "**** DB ERROR ****" + crlf + OTL_Utils::OtlExceptionToText(ex);
        request->resultCode = ClientRequest::resultCodeDbException;
        logWriter.Write("Error while processing request #" + std::to_string(request->requestNum)
                        + ": " + request->resultDescr);
        dbConnect->reconnect();
    }
}


void ConnectionPool::SendResponseAndLogToKafka(ClientRequest *request)
{
    std::string errorDescr;
    bool responseSendSuccess;
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        responseSendSuccess = request->SendResultToClient(udpSocket, errorDescr);
    }
    if (responseSendSuccess) {
        logWriter.Write("Response #" + std::to_string(request->requestNum)
                        + " sent to client. ", mainThreadIndex, notice);
    }
    else {
        logWriter.Write("Send response #" + std::to_string(request->requestNum)
                        + " error: " + errorDescr, mainThreadIndex, error);
    }

    if (kafkaProducer != nullptr) {
        request->LogToKafka(responseSendSuccess);
    }
}





