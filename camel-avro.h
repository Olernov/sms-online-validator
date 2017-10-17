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


#ifndef CAMEL_AVRO_HH_4187440818__H_
#define CAMEL_AVRO_HH_4187440818__H_


#include <sstream>
#include "boost/any.hpp"
#include "avro/Specific.hh"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"

struct Call_CDR {
    int64_t imsi;
    int64_t callingPartyNumber;
    int64_t calledPartyNumber;
    int64_t startTime;
    int64_t finishTime;
    int32_t totalDurationSeconds;
    int32_t quotaResult;
    Call_CDR() :
        imsi(int64_t()),
        callingPartyNumber(int64_t()),
        calledPartyNumber(int64_t()),
        startTime(int64_t()),
        finishTime(int64_t()),
        totalDurationSeconds(int32_t()),
        quotaResult(int32_t())
        { }
};

namespace avro {
template<> struct codec_traits<Call_CDR> {
    static void encode(Encoder& e, const Call_CDR& v) {
        avro::encode(e, v.imsi);
        avro::encode(e, v.callingPartyNumber);
        avro::encode(e, v.calledPartyNumber);
        avro::encode(e, v.startTime);
        avro::encode(e, v.finishTime);
        avro::encode(e, v.totalDurationSeconds);
        avro::encode(e, v.quotaResult);
    }
    static void decode(Decoder& d, Call_CDR& v) {
        if (avro::ResolvingDecoder *rd =
            dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
            for (std::vector<size_t>::const_iterator it = fo.begin();
                it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.imsi);
                    break;
                case 1:
                    avro::decode(d, v.callingPartyNumber);
                    break;
                case 2:
                    avro::decode(d, v.calledPartyNumber);
                    break;
                case 3:
                    avro::decode(d, v.startTime);
                    break;
                case 4:
                    avro::decode(d, v.finishTime);
                    break;
                case 5:
                    avro::decode(d, v.totalDurationSeconds);
                    break;
                case 6:
                    avro::decode(d, v.quotaResult);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.imsi);
            avro::decode(d, v.callingPartyNumber);
            avro::decode(d, v.calledPartyNumber);
            avro::decode(d, v.startTime);
            avro::decode(d, v.finishTime);
            avro::decode(d, v.totalDurationSeconds);
            avro::decode(d, v.quotaResult);
        }
    }
};

}
#endif
