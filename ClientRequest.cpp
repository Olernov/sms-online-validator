#include <sstream>
#include "ClientRequest.h"

ClientRequest::ClientRequest(sockaddr_in& senderAddr) :
    accepted(system_clock::now()),
    resultCode(resultCodeUnknown),
    clientAddr(senderAddr)
{
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

