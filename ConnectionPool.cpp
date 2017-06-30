#include "ConnectionPool.h"
#include "otl_utils.h"


extern LogWriterOtl logWriter;

ConnectionPool::ConnectionPool() :
    initialized(false),
    stopFlag(false)
{
    lastUsed.store(0);
}


ConnectionPool::~ConnectionPool()
{
    for(auto& db : dbConnects) {
        delete db;
    }
    stopFlag = true;
    for (int i = 0; i < workerThreads.size(); ++i) {
        condVars[i].notify_one();
    }
    for(auto& thr : workerThreads) {
        if (thr.joinable()) {
            thr.join();
        }
    }

}


bool ConnectionPool::Initialize(const Config& config, std::string& errDescription)
{
    if (initialized) {
        errDescription = "Connection pool is already initialized";
        return false;
    }
    connectString = config.connectString;
    dbConnects.resize(config.connectionCount);
    for(auto& db : dbConnects) {
        try {
            db = new DBConnect;
            db->rlogon(connectString.c_str());
        }
        catch(const otl_exception& ex) {
            errDescription = "**** DB ERROR while logging to DB: **** " +
                crlf + OTL_Utils::OtlExceptionToText(ex);
            return false;
        }
    }
    for (unsigned int index = 0; index < config.connectionCount; ++index) {
        busy[index] = false;
        finished[index] = false;
    }
    for (unsigned int i = 0; i < config.connectionCount; ++i) {
        workerThreads.push_back(std::thread(&ConnectionPool::WorkerThread, this, i));
    }
    logWriter << "Connection pool initialized successfully";
    initialized = true;
    return true;
}


void ConnectionPool::ExecRequest(ClientRequest* clientRequest)
{
    unsigned int connIndex;
    if (!TryAcquire(connIndex)) {
        logWriter.Write("Unable to acqure connection to database to execute request #"
                        + std::to_string(clientRequest->requestNum), mainThreadIndex, error);
        return;
    }
    logWriter.Write("Acquired connection #" + std::to_string(connIndex), mainThreadIndex, debug);
    clientRequests[connIndex] = clientRequest;
    condVars[connIndex].notify_one();
}


bool ConnectionPool::TryAcquire(unsigned int& acquiredIndex)
{
    using namespace std::chrono;
    if (!initialized || stopFlag) {
        logWriter.Write("Attempt to acquire on not initialized or being in shutdown connection pool",
                        mainThreadIndex, error);
        return false;
    }

    const int maxMilliSecondsToAcquire = 800;
    int cycleCounter = 0;
    auto startTime =  high_resolution_clock::now();
    bool firstCycle = true;
    while (true) {
        // Start looping from last used connection + 1 to ensure consequent connection usage and
        // to avoid suspending rarely used connections
        for (size_t i = (firstCycle ? ((lastUsed + 1) %  dbConnects.size()) : 0); i < dbConnects.size(); ++i) {
            bool oldValue = busy[i];
            if (!oldValue) {
                if (busy[i].compare_exchange_weak(oldValue, true)) {
                    acquiredIndex = i;
                    finished[i] = false;
                    lastUsed.store(i);
                    return true;
                }
            }
        }
        firstCycle = false;
        ++cycleCounter;
        if (cycleCounter > 1000) {
            auto now = high_resolution_clock::now();
            auto timeSpan = duration_cast<milliseconds>(now - startTime);
            if (timeSpan.count() > maxMilliSecondsToAcquire) {
                return false;
            }
            cycleCounter = 0;
        }
    }
}

void ConnectionPool::WorkerThread(unsigned int index)
{
    while (!stopFlag) {
        std::unique_lock<std::mutex> locker(mutexes[index]);
        while(!stopFlag && !busy[index])  {
            condVars[index].wait(locker);
        }
        if (!stopFlag && busy[index] && !finished[index]) {
            std::string errDescription;
            try {
                otl_stream dbStream;
                dbStream.open(1,
                        "call ValidateSMS(:oa /*char[100],in*/, :oa_flags/*short,in*/, "
                        ":da /*char[100],in*/, :da_flags/*sort,in*/, "
                        ":ref_num /*short,in*/, :total /*short,in*/, :part_num /*short,in*/, :serving_msc /*char[100],in*/)"
                        " into :res /*long,out*/",
                        *dbConnects[index]);
                dbStream
                       << clientRequests[index]->origination
                       << static_cast<short>(clientRequests[index]->originationFlags)
                       << clientRequests[index]->destination
                       << static_cast<short>(clientRequests[index]->destinationFlags)
                       << static_cast<short>(clientRequests[index]->referenceNum)
                       << static_cast<short>(clientRequests[index]->totalParts)
                       << static_cast<short>(clientRequests[index]->partNum)
                       << clientRequests[index]->servingMSC;
                long result;
                dbStream >> result;
                clientRequests[index]->resultCode = result;

                logWriter.Write("Request #" + std::to_string(clientRequests[index]->requestNum)
                                + " processing finished. Sending result to client", index);
                //clientRequests[index]->SendRequestResultToClient();
                //delete clientRequests[index];
                //m_resultCodes[index] = res;
                //m_results[index] = errDescription;
            }
            catch(const otl_exception& ex) {
                logWriter << "**** DB ERROR ****" + crlf + OTL_Utils::OtlExceptionToText(ex);
                dbConnects[index]->reconnect();
            }
            finished[index] = true;
            condVars[index].notify_one();
            busy[index] = false;
        }
    }
}




