#include <sstream>
#include "CallFinishRequest.h"
#include "Common.h"
#include "rdkafkacpp.h"


CallFinishRequest::CallFinishRequest(sockaddr_in& senderAddr) :
    ClientRequest(senderAddr)
{}


bool CallFinishRequest::ValidateAndSetParams(uint32_t reqNum, const psAttrMap& requestAttrs,
                                                std::string& errorDescr)
{
    requestNum = reqNum;
    if (!SetRequiredIntParam(requestAttrs, CAMEL_IMSI, "IMSI", imsi, errorDescr)) {
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
    if (!SetRequiredIntParam(requestAttrs, CAMEL_TOTAL_DURATION, "Total duration (seconds)",
                             totalDurationSeconds, errorDescr)) {
        return false;
    }
    return true;
}


void CallFinishRequest::Process(DBConnect* dbConnect)
{
    otl_stream dbStream;
    dbStream.open(1,
        "call M2M.CallFinishRequest(:imsi /*ubigint,in*/, "
        ":call_ref_num /*ubigint,in*/, :event_type /*short,in*/, :service_key /*short,in*/, "
        ":total_secs /*long,in*/, :start_time /*timestamp,out*/, :calling /*ubigint,out*/, "
        ":called /*ubigint,out*/)", *dbConnect);
    dbStream
           << imsi
           << callReferenceNumber
           << static_cast<short>(eventType)
           << static_cast<short>(serviceKey)
           << totalDurationSeconds;
    otl_datetime callStart;
    unsigned long long callingParty, calledParty;
    dbStream >> callStart >> callingParty >> calledParty;
}


bool CallFinishRequest::SendResultToClient(int socket, std::string& errorDescr)
{
    // no response is sent for this type of request
    return true;
}


void CallFinishRequest::LogToKafka(RdKafka::Producer* producer, const std::string& topic,
                            bool responseSendSuccess)
{
    CAMEL_Request req;

   //   TODO: what to log ?

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


std::vector<uint8_t> CallFinishRequest::EncodeAvro(const CAMEL_Request &req)
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


