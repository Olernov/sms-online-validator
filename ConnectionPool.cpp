#include <sstream>
#include "ConnectionPool.h"
#include "otl_utils.h"


extern LogWriterOtl logWriter;

ConnectionPool::ConnectionPool() :
    initialized(false),
    stopFlag(false),
    incomingRequests(queueSize),
    processedRequests(queueSize)
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


bool ConnectionPool::Initialize(const Config& config, std::string& errDescription)
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
                processedRequests.push(request);
            }
            else {
                std::stringstream ss;
                ss << "Request #" << request->requestNum << " discarded due to max age exceeding ("
                   << round(requestAgeSec * 1000) << " ms)";
                logWriter << ss.str();
            }
        }
    }
}


void ConnectionPool::ProcessRequest(unsigned int index, ClientRequest* request, DBConnect* dbConnect)
{
    try {
        otl_stream dbStream;
        dbStream.open(1,
                "call ValidateSMS(:oa /*char[100],in*/, :oa_flags/*short,in*/, "
                ":da /*char[100],in*/, :da_flags/*sort,in*/, "
                ":ref_num /*short,in*/, :total /*short,in*/, :part_num /*short,in*/, :serving_msc /*char[100],in*/)"
                " into :res /*long,out*/",
                *dbConnect);
        dbStream
               << request->origination
               << static_cast<short>(request->originationFlags)
               << request->destination
               << static_cast<short>(request->destinationFlags)
               << static_cast<short>(request->referenceNum)
               << static_cast<short>(request->totalParts)
               << static_cast<short>(request->partNum)
               << request->servingMSC;
        long result;
        dbStream >> result;
        request->resultCode = result;
        std::stringstream ss;
        ss << "Request #" << request->requestNum << " processed by thread #" << index
           << " in " << round(duration<double>(system_clock::now() - request->accepted).count() * 1000)  << " ms."
           << " Result: " << result;
        logWriter << ss.str();
    }
    catch(const otl_exception& ex) {
        request->resultDescr = "**** DB ERROR ****" + crlf + OTL_Utils::OtlExceptionToText(ex);
        request->resultCode = ClientRequest::resultCodeDbException;
        logWriter.Write("Error while processing request #" + std::to_string(request->requestNum)
                        + ": " + request->resultDescr);
        dbConnect->reconnect();
    }
}

ClientRequest* ConnectionPool::PopProcessedRequest()
{
    ClientRequest* request;
    if (processedRequests.pop(request)) {
        return request;
    }
    else {
        return nullptr;
    }
}



