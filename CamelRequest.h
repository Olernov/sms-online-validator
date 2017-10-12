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


class CamelRequest : public ClientRequest
{
public:
    CamelRequest(sockaddr_in& senderAddr);
    bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    void Process(DBConnect* dbConnect);
    bool SendResultToClient(int socket, std::string& errorDescr);
    void LogToKafka(RdKafka::Producer* producer, const std::string &topic,
                    bool responseSendSuccess);
private:
    unsigned long long imsi;
    unsigned long long callingPartyNumber;
    unsigned long long calledPartyNumber;
    unsigned long long callReferenceNumber;
    uint8_t eventType;
    uint8_t serviceKey;
    int8_t quotaResult;
    long quotaSeconds;

    std::vector<uint8_t> EncodeAvro(const CAMEL_Request &avroCdr);
};
