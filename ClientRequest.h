#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "Common.h"
#include "OTL_Header.h"
#include "DBConnect.h"
#include "rdkafkacpp.h"
#include "LogWriterOtl.h"

using namespace std::chrono;

extern LogWriterOtl logWriter;

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

    template<typename T>
    bool SetRequiredIntParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                            T& value, std::string& errorDescr, size_t requiredSize = 0)
    {
        auto iter = requestAttrs.find(paramType);
        if (iter == requestAttrs.end()) {
            errorDescr  = paramName + " is missing in request";
            return false;
        }

        if (requiredSize != 0 && iter->second.m_usDataLen != requiredSize) {
            errorDescr  = paramName + " has incorrect size " + std::to_string(iter->second.m_usDataLen) +
                ". Its size must be " + std::to_string(requiredSize) + " bytes.";
            return false;
        }
        switch(iter->second.m_usDataLen) {
        case 1:
            value = *static_cast<uint8_t*>(iter->second.m_pvData);
            break;
        case 2:
            value = ntohs(*static_cast<uint16_t*>(iter->second.m_pvData));
            break;
        // TODO: parse other lengths

        case 8:
            value = ntohll(*static_cast<uint64_t*>(iter->second.m_pvData));
            break;
        default:
            errorDescr = "SetIntegerParam: unexpected integer size " +
                    std::to_string(iter->second.m_usDataLen);
            return false;
        }
        logWriter.Write(paramName + ": " + std::to_string(value), mainThreadIndex, debug);
        return true;
    }


    template<typename T>
    bool SetOptionalIntParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                           T& value, T defaultValue, std::string& errorDescr, size_t requiredSize = 0)
    {
        bool res = SetRequiredIntParam(requestAttrs, paramType, paramName,
                                           value, errorDescr, requiredSize);
        if (!res && errorDescr == paramName + " is missing in request") {
            errorDescr.clear();
            value = defaultValue;
            return true;
        }
        return res;
    }

    bool SetStringParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                        std::string& value, std::string& errorDescr);

};
