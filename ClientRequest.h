#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "Common.h"
#include "OTL_Header.h"
#include "DBConnect.h"
#include "rdkafkacpp.h"

using namespace std::chrono;

class ClientRequest
{
public:
    ClientRequest(sockaddr_in& senderAddr);
    virtual ~ClientRequest() {};

    virtual bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
                                             std::string& errorDescr) = 0;
    virtual void Process(DBConnect* dbConnect) = 0;
    virtual bool SendResultToClient(int socket, std::string& errorDescr) = 0;
    virtual void LogToKafka(RdKafka::Producer* producer, const std::string& topic,
                            bool responseSendSuccess) = 0;
    std::string DumpResults();

    static const int32_t resultCodeUnknown = -12;
    static const int8_t resultCodeDbException = -100;

    sockaddr_in clientAddr;
    uint32_t requestNum;
    system_clock::time_point accepted;
    int8_t resultCode;
    std::string resultDescr;
};
