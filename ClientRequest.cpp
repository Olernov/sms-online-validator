#ifdef _WIN32
	#include <Winsock2.h>
#endif
#include <sstream>
#include "ClientRequest.h"
#include "Common.h"

ClientRequest::ClientRequest(sockaddr_in& senderAddr) :
    accepted(system_clock::now()),
    resultCode(resultCodeUnknown),
    clientAddr(senderAddr)
{}


bool ClientRequest::ValidateAndSetRequestParams(uint32_t reqNum, const psAttrMap& requestAttrs,
                                                std::string& errorDescr)
{
	requestNum = reqNum;
    if (!SetIntegerParam(requestAttrs, VLD_IMSI, "Origination IMSI", 8, originationImsi, errorDescr)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_OA, "Origination address", originationMsisdn, errorDescr)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_DA, "Destination address", destinationMsisdn, errorDescr)) {
        return false;
    }

    if (!SetIntegerParam(requestAttrs, VLD_OAFLAGS, "OAFLAGS", 1, originationFlags, errorDescr)) {
        return false;
    }
    if (!SetIntegerParam(requestAttrs, VLD_DAFLAGS, "DAFLAGS", 1, destinationFlags, errorDescr)) {
        return false;
    }
    if (!SetIntegerParam(requestAttrs, VLD_REFNUM, "Reference number", 2, referenceNum, errorDescr)) {
        return false;
    }
    if (!SetIntegerParam(requestAttrs, VLD_TOTAL, "Total", 1, totalParts, errorDescr)) {
        return false;
    }
    if (!SetIntegerParam(requestAttrs, VLD_PART, "Part number", 1, partNum, errorDescr)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_SERVINGMSC, "Serving MSC", servingMSC, errorDescr)) {
        return false;
    }
    return true;
}


bool ClientRequest::SetStringParam(const psAttrMap &requestAttrs, int paramType, std::string paramName,
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


bool ClientRequest::SendRequestResultToClient(int socket, std::string& errorDescr)
{
	CPSPacket pspResponse;
	char buffer[2014];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), requestNum, VALIDATEEX_RESP) != 0) {
		errorDescr = "PSPacket init failed";
        return false;
    }

    int32_t resultCodeN = htonl(resultCode);
    unsigned long len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
            PS_RESULT, &resultCodeN, sizeof(resultCodeN));
    if (resultCode != OPERATION_SUCCESS && !resultDescr.empty()) {
		len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
			PS_DESCR, resultDescr.data(), resultDescr.size());
	}

    if(sendto(socket, buffer, len, 0, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr)) <= 0) {
        errorDescr = "socket error " + std::to_string(errno);
        return false;
    }
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
