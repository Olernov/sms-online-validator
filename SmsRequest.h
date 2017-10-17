#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "LogWriterOtl.h"
#include "Common.h"
#include "OTL_Header.h"
#include "DBConnect.h"
#include "ClientRequest.h"
#include "sms-cdr-avro.h"

extern LogWriterOtl logWriter;

class SmsRequest : public ClientRequest
{
public:
    SmsRequest(sockaddr_in& senderAddr, RdKafka::Producer *producer, const std::string &topic);
    bool ValidateAndSetParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    void Process(DBConnect* dbConnect);
    bool SendResultToClient(int socket, std::string& errorDescr);
    void LogToKafka(bool responseSendSuccess);
private:
    std::vector<uint8_t> EncodeCdr(const SMS_CDR &avroCdr);

    uint64_t originationImsi;
    std::string originationMsisdn;
    std::string destinationMsisdn;
    uint8_t originationFlags;
    uint8_t destinationFlags;
    uint16_t referenceNum;
    uint8_t partNum;
    uint8_t totalParts;
    std::string servingMSC;

    static const uint64_t origImsiNotGiven = 0;
};
