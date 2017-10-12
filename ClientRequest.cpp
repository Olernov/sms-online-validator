#include <sstream>
#include "ClientRequest.h"

ClientRequest::ClientRequest(sockaddr_in& senderAddr) :
    accepted(system_clock::now()),
    resultCode(resultCodeUnknown),
    clientAddr(senderAddr)
{
}


bool ClientRequest::SetStringParam(const psAttrMap &requestAttrs, int paramType, const std::string& paramName,
                                   std::string& value, std::string& errorDescr)
{
    auto iter = requestAttrs.find(paramType);
    if (iter == requestAttrs.end()) {
        errorDescr  = paramName + " is missing in request";
        return false;
    }
    value.resize(iter->second.m_usDataLen);
    std::copy(static_cast<char*>(iter->second.m_pvData),
              static_cast<char*>(iter->second.m_pvData) + iter->second.m_usDataLen, value.begin());
    logWriter.Write(paramName + ": " + value, mainThreadIndex, debug);
    return true;
}


std::string ClientRequest::DumpResults()
{
    std::stringstream ss;
    ss << "Request #" + std::to_string(requestNum) + " result code: " << std::to_string(resultCode);
    if (resultCode != OPERATION_SUCCESS) {
        ss << " (" << resultDescr << ")";
    }
    return ss.str();
}

