#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "Common.h"
#include "OTL_Header.h"
#include "DBConnect.h"
#include "ClientRequest.h"
#include "camel-avro.h"

class CallFinishRequest : public ClientRequest
{
public:
    CallFinishRequest(sockaddr_in& senderAddr, RdKafka::Producer* producer,
                      const std::string& topic);
    bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    void Process(DBConnect* dbConnect);
    bool SendResultToClient(int socket, std::string& errorDescr);
    void LogToKafka(bool responseSendSuccess);
private:
    unsigned long long imsi;
    unsigned long long callReferenceNumber;
    unsigned long long callingPartyNumber;
    unsigned long long calledPartyNumber;
    uint8_t eventType;
    uint8_t serviceKey;
    long totalDurationMilliseconds;
    time_t callStartTime;
    long lastQuotaRes;

    std::vector<uint8_t> EncodeAvro(const Call_CDR &avroCdr);
};


