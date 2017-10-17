#ifdef _WIN32
	#include <Winsock2.h>
#endif
#include <sstream>
#include "SmsRequest.h"
#include "Common.h"
#include "rdkafkacpp.h"


SmsRequest::SmsRequest(sockaddr_in& senderAddr, RdKafka::Producer *producer,
                       const std::string &topic) :
    ClientRequest(senderAddr, producer, topic),
    originationImsi(0)
{}


bool SmsRequest::ValidateAndSetParams(uint32_t reqNum, const psAttrMap& requestAttrs,
                                                std::string& errorDescr)
{
	requestNum = reqNum;
    if (!SetOptionalIntParam(requestAttrs, VLD_IMSI, "Origination IMSI",
                                 originationImsi, origImsiNotGiven, errorDescr, 8)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_OA, "Origination address", originationMsisdn, errorDescr)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_DA, "Destination address", destinationMsisdn, errorDescr)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, VLD_OAFLAGS, "OAFLAGS", originationFlags, errorDescr, 1)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, VLD_DAFLAGS, "DAFLAGS", destinationFlags, errorDescr, 1)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, VLD_REFNUM, "Reference number", referenceNum, errorDescr, 2)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, VLD_TOTAL, "Total", totalParts, errorDescr, 1)) {
        return false;
    }
    if (!SetRequiredIntParam(requestAttrs, VLD_PART, "Part number", partNum, errorDescr, 1)) {
        return false;
    }
    if (!SetStringParam(requestAttrs, VLD_SERVINGMSC, "Serving MSC", servingMSC, errorDescr)) {
        return false;
    }
    return true;
}


void SmsRequest::Process(DBConnect* dbConnect)
{
    otl_stream dbStream;
    dbStream.open(1,
            "call M2M.ValidateSMS(:oa_imsi /*ubigint,inout*/, :oa /*char[100],in*/, :oa_flags/*short,in*/, "
            ":da /*char[100],in*/, :da_flags/*short,in*/, "
            ":ref_num /*short,in*/, :total /*short,in*/, :part_num /*short,in*/, :serving_msc /*char[100],in*/)"
            " into :res /*long,out*/",
            *dbConnect);
    dbStream
           << static_cast<unsigned long long>(originationImsi)
           << originationMsisdn
           << static_cast<short>(originationFlags)
           << destinationMsisdn
           << static_cast<short>(destinationFlags)
           << static_cast<short>(referenceNum)
           << static_cast<short>(totalParts)
           << static_cast<short>(partNum)
           << servingMSC;
    long result;
    unsigned long long imsiOutOfProc;
    dbStream >> imsiOutOfProc >> result;
    resultCode = result;
    originationImsi = imsiOutOfProc;
}


bool SmsRequest::SendResultToClient(int socket, std::string& errorDescr)
{
	CPSPacket pspResponse;
	char buffer[2014];
    if(pspResponse.Init(reinterpret_cast<SPSRequest*>(buffer), sizeof(buffer), requestNum, VALIDATEEX_RESP) != 0) {
		errorDescr = "PSPacket init failed";
        return false;
    }

    int32_t resultCodeN = htonl(static_cast<int32_t>(resultCode)); // 4 bytes required for SMS response
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


void SmsRequest::LogToKafka(bool responseSendSuccess)
{
    if (originationImsi == SmsRequest::origImsiNotGiven) {
        return;
    }
    SMS_CDR cdr;
    cdr.originationImsi = originationImsi;
    cdr.originationMsisdn = originationMsisdn;
    cdr.destinationMsisdn = destinationMsisdn;
    cdr.oaflags = originationFlags;
    cdr.daflags = destinationFlags;
    cdr.referenceNum = referenceNum;
    cdr.totalParts = totalParts;
    cdr.partNumber = partNum;
    cdr.servingMSC = servingMSC;
    cdr.validationTime = system_clock::to_time_t(accepted) * 1000;
    cdr.validationRes = resultCode;
    cdr.responseSendSuccess = responseSendSuccess;

    std::vector<uint8_t> rawData = EncodeCdr(cdr);
    std::string errstr;
    RdKafka::ErrorCode resp = kafkaProducer->produce(kafkaTopic, RdKafka::Topic::PARTITION_UA,
                               RdKafka::Producer::RK_MSG_COPY,
                               rawData.data(), rawData.size(), nullptr, 0,
                               time(nullptr) * 1000 /*milliseconds*/, nullptr);

    if (resp != RdKafka::ERR_NO_ERROR) {
        logWriter << "Kafka produce failed: " + RdKafka::err2str(resp);
    }
}


std::vector<uint8_t> SmsRequest::EncodeCdr(const SMS_CDR &avroCdr)
{
    std::unique_ptr<avro::OutputStream> out(avro::memoryOutputStream());
    avro::EncoderPtr encoder(avro::binaryEncoder());
    encoder->init(*out);
    avro::encode(*encoder, avroCdr);
    encoder->flush();
    std::vector<uint8_t> rawData(out->byteCount());
    std::unique_ptr<avro::InputStream> in = avro::memoryInputStream(*out);
    avro::StreamReader reader(*in);
    reader.readBytes(&rawData[0], out->byteCount());
    return rawData;
}


