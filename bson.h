/*
* Copyright (C) 2021 Duowan Inc. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __X_PACK_BSON_H
#define __X_PACK_BSON_H

#include "bson_decoder.h"
#include "bson_encoder.h"
#include "bson_builder.h"
#include "xpack/xpack.h"

namespace xpack {

class bson {
public:
    template <class T>
    static void decode(const std::string &data, T &val) {
        BsonDecoder doc(data, false);
        doc.decode(NULL, val, NULL);
    }

    template <class T>
    static void decode(const uint8_t* data, size_t len, T &val) {
        BsonDecoder doc(data, len, false);
        doc.decode(NULL, val, NULL);
    }

    template <class T>
    static std::string encode(const T &val) {
        BsonEncoder doc;
        doc.encode(NULL, val, NULL);
        return doc.String();
    }
};

}

#endif
