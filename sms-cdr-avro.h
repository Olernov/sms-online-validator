/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef SMS_CDR_AVRO_HH_4026632438__H_
#define SMS_CDR_AVRO_HH_4026632438__H_


#include <sstream>
#include "boost/any.hpp"
#include "avro/Specific.hh"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"

struct SMS_CDR {
    int64_t originationImsi;
    std::string originationMsisdn;
    std::string destinationMsisdn;
    int32_t oaflags;
    int32_t daflags;
    int32_t referenceNum;
    int32_t totalParts;
    int32_t partNumber;
    std::string servingMSC;
    int64_t validationTime;
    int32_t validationRes;
    bool responseSendSuccess;
    SMS_CDR() :
        originationImsi(int64_t()),
        originationMsisdn(std::string()),
        destinationMsisdn(std::string()),
        oaflags(int32_t()),
        daflags(int32_t()),
        referenceNum(int32_t()),
        totalParts(int32_t()),
        partNumber(int32_t()),
        servingMSC(std::string()),
        validationTime(int64_t()),
        validationRes(int32_t()),
        responseSendSuccess(bool())
        { }
};

namespace avro {
template<> struct codec_traits<SMS_CDR> {
    static void encode(Encoder& e, const SMS_CDR& v) {
        avro::encode(e, v.originationImsi);
        avro::encode(e, v.originationMsisdn);
        avro::encode(e, v.destinationMsisdn);
        avro::encode(e, v.oaflags);
        avro::encode(e, v.daflags);
        avro::encode(e, v.referenceNum);
        avro::encode(e, v.totalParts);
        avro::encode(e, v.partNumber);
        avro::encode(e, v.servingMSC);
        avro::encode(e, v.validationTime);
        avro::encode(e, v.validationRes);
        avro::encode(e, v.responseSendSuccess);
    }
    static void decode(Decoder& d, SMS_CDR& v) {
        if (avro::ResolvingDecoder *rd =
            dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
            for (std::vector<size_t>::const_iterator it = fo.begin();
                it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.originationImsi);
                    break;
                case 1:
                    avro::decode(d, v.originationMsisdn);
                    break;
                case 2:
                    avro::decode(d, v.destinationMsisdn);
                    break;
                case 3:
                    avro::decode(d, v.oaflags);
                    break;
                case 4:
                    avro::decode(d, v.daflags);
                    break;
                case 5:
                    avro::decode(d, v.referenceNum);
                    break;
                case 6:
                    avro::decode(d, v.totalParts);
                    break;
                case 7:
                    avro::decode(d, v.partNumber);
                    break;
                case 8:
                    avro::decode(d, v.servingMSC);
                    break;
                case 9:
                    avro::decode(d, v.validationTime);
                    break;
                case 10:
                    avro::decode(d, v.validationRes);
                    break;
                case 11:
                    avro::decode(d, v.responseSendSuccess);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.originationImsi);
            avro::decode(d, v.originationMsisdn);
            avro::decode(d, v.destinationMsisdn);
            avro::decode(d, v.oaflags);
            avro::decode(d, v.daflags);
            avro::decode(d, v.referenceNum);
            avro::decode(d, v.totalParts);
            avro::decode(d, v.partNumber);
            avro::decode(d, v.servingMSC);
            avro::decode(d, v.validationTime);
            avro::decode(d, v.validationRes);
            avro::decode(d, v.responseSendSuccess);
        }
    }
};

}
#endif
