#include <sstream>
#include "CallFinishRequest.h"
#include "Common.h"
#include "rdkafkacpp.h"
#include "otl_utils.h"


CallFinishRequest::CallFinishRequest(sockaddr_in& senderAddr, RdKafka::Producer *producer,
                                     const std::string &topic) :
    ClientRequest(senderAddr, producer, topic)
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
    if (!SetRequiredIntParam(requestAttrs, CAMEL_TOTAL_DURATION, "Total duration (milliseconds)",
                             totalDurationMilliseconds, errorDescr)) {
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
        ":called /*ubigint,out*/, :quota_res /*long,out*/)", *dbConnect);
    dbStream
           << imsi
           << callReferenceNumber
           << static_cast<short>(eventType)
           << static_cast<short>(serviceKey)
           << totalDurationMilliseconds;
    otl_datetime callStartOtl;
    dbStream >> callStartOtl >> callingPartyNumber >> calledPartyNumber >> lastQuotaRes;
    callStartTime = OTL_Utils::OTL_datetime_to_Time_t(callStartOtl);
    resultCode = OPERATION_SUCCESS;
}


bool CallFinishRequest::SendResultToClient(int socket, std::string& errorDescr)
{
    CPSPacket pspResponse;
    char buffer[20];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer),
                        requestNum, CALL_FINISH_ACK) != 0) {
        errorDescr = "PSPacket init failed";
        return false;
    }
    int len = sizeof(SPSRequest);
    if(sendto(socket, buffer, len, 0, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr)) <= 0) {
        errorDescr = "socket error " + std::to_string(errno);
        return false;
    }
    return true;
}


void CallFinishRequest::LogToKafka(bool responseSendSuccess)
{
    Call_CDR cdr;
    cdr.imsi = imsi;
    cdr.callingPartyNumber = callingPartyNumber;
    cdr.calledPartyNumber = calledPartyNumber;
    cdr.startTime = callStartTime * 1000;
    cdr.finishTime = system_clock::to_time_t(accepted) * 1000;
    cdr.totalDurationSeconds = totalDurationMilliseconds;
    cdr.quotaResult = lastQuotaRes;
    std::vector<uint8_t> rawData = EncodeAvro(cdr);
    std::string errstr;
    RdKafka::ErrorCode resp = kafkaProducer->produce(kafkaTopic, RdKafka::Topic::PARTITION_UA,
                               RdKafka::Producer::RK_MSG_COPY,
                               rawData.data(), rawData.size(), nullptr, 0,
                               time(nullptr) * 1000 /*milliseconds*/, nullptr);

    if (resp != RdKafka::ERR_NO_ERROR) {
        logWriter << "Kafka produce failed: " + RdKafka::err2str(resp);
    }
}


std::vector<uint8_t> CallFinishRequest::EncodeAvro(const Call_CDR &req)
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


