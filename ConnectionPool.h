#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "OTL_Header.h"
#include "DBConnect.h"
#include "Config.h"
#include "ClientRequest.h"

class ConnectionPool
{
public:
    ConnectionPool();
    ~ConnectionPool();
    bool Initialize(const Config& config, std::string& errDescription);
    void PushRequest(ClientRequest *clientRequest);
    ClientRequest* PopProcessedRequest();
private:
    static const int queueSize = 1024;
    static const int maxRequestAgeSec = 3.0;
    std::string connectString;
    bool initialized;
    bool stopFlag;
    std::vector<DBConnect*> dbConnectPool;
    std::vector<std::thread> workerThreads;
    std::condition_variable conditionVar;
    std::mutex lock;
    boost::lockfree::queue<ClientRequest*> incomingRequests;
    boost::lockfree::queue<ClientRequest*> processedRequests;

    void WorkerThread(unsigned int index, DBConnect* dbConnect);
    void ProcessRequest(unsigned int index, ClientRequest* request, DBConnect* dbConnect);
};

