#include <sstream>
#include "CamelRequest.h"
#include "Common.h"
#include "rdkafkacpp.h"


CamelRequest::CamelRequest(sockaddr_in& senderAddr) :
    ClientRequest(senderAddr)
{}


bool CamelRequest::ValidateAndSetParams(uint32_t reqNum, const psAttrMap& requestAttrs,
                                                std::string& errorDescr)
{
    requestNum = reqNum;
    if (!SetRequiredIntParam(requestAttrs, CAMEL_IMSI, "IMSI", imsi, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, CAMEL_CALLING_PARTY, "Calling Party Number",
                        callingPartyNumber, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, CAMEL_CALLED_PARTY, "Called Party Number",
                        calledPartyNumber, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, CAMEL_CALL_REF_NUM, "Call Reference Number",
                             callReferenceNumber, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, CAMEL_EVENT_TYPE, "Event type",
                             eventType, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, CAMEL_SERVICE_KEY, "Service key",
                             serviceKey, errorDescr)) {
        return false;
    }
    return true;
}


void CamelRequest::Process(DBConnect* dbConnect)
{
    otl_stream dbStream;
    dbStream.open(1,
        "call M2M.CallQuotaRequest(:imsi /*ubigint,in*/, :calling /*ubigint,in*/, :called /*ubigint,in*/, "
        ":call_ref_num /*ubigint,in*/, :event_type /*short,in*/, :service_key /*short,in*/, "
        ":quota_res /*short,out*/, :quota_sec /*long,out*/)"
        " into :res /*short,out*/",
        *dbConnect);
    dbStream
           << imsi
           << callingPartyNumber
           << calledPartyNumber
           << callReferenceNumber
           << static_cast<short>(eventType)
           << static_cast<short>(serviceKey);
    short successCode, res;
    dbStream >> res >> quotaSeconds >> successCode;
    resultCode = successCode;
    quotaResult = res;
}


bool CamelRequest::SendResultToClient(int socket, std::string& errorDescr)
{
    CPSPacket pspResponse;
    char buffer[2014];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
                        requestNum, QUOTA_RESP) != 0) {
        errorDescr = "PSPacket init failed";
        return false;
    }

    unsigned long len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
            PS_RESULT, &resultCode, sizeof(resultCode));
    if (resultCode == OPERATION_SUCCESS) {
        len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
            CAMEL_QUOTA_RESULT, &quotaResult, sizeof(quotaResult));
        if (quotaSeconds > 0) {
            uint32_t quotaSecondsNO = htonl(quotaSeconds);
            len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
                CAMEL_QUOTA_SECONDS, &quotaSecondsNO, sizeof(quotaSecondsNO));
        }
    }
    else if(!resultDescr.empty()) {
        len = pspResponse.AddAttr(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
            PS_DESCR, resultDescr.data(), resultDescr.size());
    }

    if(sendto(socket, buffer, len, 0, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr)) <= 0) {
        errorDescr = "socket error " + std::to_string(errno);
        return false;
    }
    return true;
}


void CamelRequest::LogToKafka(RdKafka::Producer* producer, const std::string& topic,
                            bool responseSendSuccess)
{
    CAMEL_Request req;
//    req.originationImsi = originationImsi;
//    req.originationMsisdn = originationMsisdn;
//    req.destinationMsisdn = destinationMsisdn;
//    req.oaflags = originationFlags;
//    req.daflags = destinationFlags;
//    req.referenceNum = referenceNum;
//    req.totalParts = totalParts;
//    req.partNumber = partNum;
//    req.servingMSC = servingMSC;
//    req.validationTime = system_clock::to_time_t(accepted) * 1000;
//    req.validationRes = resultCode;
//    req.responseSendSuccess = responseSendSuccess;

    std::vector<uint8_t> rawData = EncodeAvro(req);
    std::string errstr;
    RdKafka::ErrorCode resp = producer->produce(topic, RdKafka::Topic::PARTITION_UA,
                               RdKafka::Producer::RK_MSG_COPY,
                               rawData.data(), rawData.size(), nullptr, 0,
                               time(nullptr) * 1000 /*milliseconds*/, nullptr);

    if (resp != RdKafka::ERR_NO_ERROR) {
        logWriter << "Kafka produce failed: " + RdKafka::err2str(resp);
    }
}


std::vector<uint8_t> CamelRequest::EncodeAvro(const CAMEL_Request &req)
{
    std::unique_ptr<avro::OutputStream> out(avro::memoryOutputStream());
    avro::EncoderPtr encoder(avro::binaryEncoder());
    encoder->init(*out);
    avro::encode(*encoder, req);
    encoder->flush();
    std::vector<uint8_t> rawData(out->byteCount());
    std::unique_ptr<avro::InputStream> in = avro::memoryInputStream(*out);
    avro::StreamReader reader(*in);
    reader.readBytes(&rawData[0], out->byteCount());
    return rawData;
}


