#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include "LogWriter.h"
#include "Common.h"

extern LogWriter logWriter;

class ClientRequest
{
public:
    ClientRequest(sockaddr_in& senderAddr);
    bool ValidateAndSetRequestParams(uint32_t reqNum, const psAttrMap &requestAttrs,
            std::string& errorDescr);
    bool SendRequestResultToClient(int socket, std::string& errorDescr);
	std::string DumpResults();

    uint32_t requestNum;
    std::string origination;
    std::string destination;
    uint8_t originationFlags;
    uint8_t destinationFlags;
    uint16_t referenceNum;
    uint8_t partNum;
    uint8_t totalParts;
    std::string servingMSC;

    int32_t resultCode;
    std::string resultDescr;
private:
    static const int32_t resultCodeUnknown = -12;
    sockaddr_in clientAddr;

    bool SetStringParam(const psAttrMap& requestAttrs, int paramType, std::string paramName, std::string& value, std::string& errorDescr);

    template<typename T>
    bool SetIntegerParam(const psAttrMap& requestAttrs, int paramType, std::string paramName,
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
        default:
            errorDescr = "SetIntegerParam: unexpected integer size " + std::to_string(requiredSize);
            return false;
        }
        logWriter.Write(paramName + ": " + std::to_string(value), mainThreadIndex, notice);
        return true;
    }
};
