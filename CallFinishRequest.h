#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "Common.h"
#include "OTL_Header.h"
#include "DBConnect.h"
#include "ClientRequest.h"
#include "camel-avro.hh"

class CallFinishRequest : public ClientRequest
{
public:
    CallFinishRequest(sockaddr_in& senderAddr);
    bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    void Process(DBConnect* dbConnect);
    bool SendResultToClient(int socket, std::string& errorDescr);
    void LogToKafka(RdKafka::Producer* producer, const std::string &topic,
                    bool responseSendSuccess);
private:
    unsigned long long imsi;
    unsigned long long callReferenceNumber;
    uint8_t eventType;
    uint8_t serviceKey;
    long totalDurationSeconds;

    std::vector<uint8_t> EncodeAvro(const CAMEL_Request &avroCdr);
};


