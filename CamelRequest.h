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


class CamelRequest : public ClientRequest
{
public:
    CamelRequest(sockaddr_in& senderAddr, RdKafka::Producer *producer, const std::string &topic);
    bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    void Process(DBConnect* dbConnect);
    bool SendResultToClient(int socket, std::string& errorDescr);
    void LogToKafka(bool responseSendSuccess);
    void DumpResults();
private:
    enum {
        allowCallAndRequestAgain = 0,
        allowCallAndDropAfterQuota = 1
    };
    unsigned long long imsi;
    unsigned long long callingPartyNumber;
    short callingNatureOfAddress;
    unsigned long long calledPartyNumber;
    short calledNatureOfAddress;
    unsigned long long callReferenceNumber;
    short eventType;
    short serviceKey;
    int8_t quotaResult;
    long quotaChunks;

    std::vector<uint8_t> EncodeAvro(const Call_CDR &avroCdr);
};
