#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <chrono>
#include "LogWriterOtl.h"
#include "Common.h"

extern LogWriterOtl logWriter;

using namespace std::chrono;

class ClientRequest
{
public:
    ClientRequest(sockaddr_in& senderAddr);
    bool ValidateAndSetRequestParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    bool SendRequestResultToClient(int socket, std::string& errorDescr);
	std::string DumpResults();

    uint32_t requestNum;
    uint64_t originationImsi;
    std::string originationMsisdn;
    std::string destinationMsisdn;
    uint8_t originationFlags;
    uint8_t destinationFlags;
    uint16_t referenceNum;
    uint8_t partNum;
    uint8_t totalParts;
    std::string servingMSC;

    system_clock::time_point accepted;
    int32_t resultCode;
    std::string resultDescr;

    static const int32_t resultCodeUnknown = -12;
    static const int32_t resultCodeDbException = -999;
    static const uint64_t origImsiNotGiven = 0;
private:
    sockaddr_in clientAddr;

    bool SetStringParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                        std::string& value, std::string& errorDescr);

    template<typename T>
    bool SetRequiredIntegerParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                                        size_t requiredSize, T& value, std::string& errorDescr)
    {
        auto iter = requestAttrs.find(paramType);
        if (iter == requestAttrs.end()) {
            errorDescr  = paramName + " is missing in request";
            return false;
        }

        if (iter->second.m_usDataLen != requiredSize) {
            errorDescr  = paramName + " has incorrect size " + std::to_string(iter->second.m_usDataLen) +
                ". Its size must be " + std::to_string(requiredSize) + " bytes.";
            return false;
        }
        switch(requiredSize) {
        case 1:
            value = *static_cast<uint8_t*>(iter->second.m_pvData);
            break;
        case 2:
            value = ntohs(*static_cast<uint16_t*>(iter->second.m_pvData));
            break;
        case 8:
            value = ntohll(*static_cast<uint64_t*>(iter->second.m_pvData));
            break;
        default:
            errorDescr = "SetIntegerParam: unexpected integer size " + std::to_string(requiredSize);
            return false;
        }
        logWriter.Write(paramName + ": " + std::to_string(value), mainThreadIndex, debug);
        return true;
    }

    template<typename T>
    bool SetOptionalIntegerParam(const psAttrMap& requestAttrs, int paramType, const std::string& paramName,
                           size_t requiredSize, T& value, T defaultValue, std::string& errorDescr)
    {
        bool res = SetRequiredIntegerParam(requestAttrs, paramType, paramName, requiredSize,
                                           value, errorDescr);
        if (!res && errorDescr == paramName + " is missing in request") {
            errorDescr.clear();
            value = defaultValue;
            return true;
        }
        return res;
    }
};
