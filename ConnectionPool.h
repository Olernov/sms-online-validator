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
    bool TryAcquire(unsigned int& acquiredIndex);
    void ExecRequest(ClientRequest *clientRequest);
private:
    std::string connectString;
    bool initialized;
    bool stopFlag;
    std::vector<DBConnect*> dbConnects;
    std::vector<std::thread> workerThreads;
    std::atomic_bool busy[MAX_THREADS];
    std::atomic_int lastUsed;
    bool finished[MAX_THREADS];
    std::condition_variable condVars[MAX_THREADS];
    std::mutex mutexes[MAX_THREADS];
    ClientRequest* clientRequests[MAX_THREADS];

    void WorkerThread(unsigned int index);
};

