#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

class ConnectionPool
{
public:
    ConnectionPool();
    ~ConnectionPool();
    bool Initialize(const Config& config, std::string& errDescription);
    bool TryAcquire(unsigned int& index);
    int8_t ExecRequest(unsigned int index, ClientRequest& clientRequest);
private:
    std::string connectString;
    bool initialized;
    bool stopFlag;
};

