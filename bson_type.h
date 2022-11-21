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

#ifndef __X_PACK_BSON_TYPE_H
#define __X_PACK_BSON_TYPE_H

#include <map>
#include <string>
#include <stdint.h>
#include "xpack/extend.h"

namespace xpack {

class BsonDate {
public:
    BsonDate(const int64_t &t = 0):ts(t){}
    operator int64_t() const {
        return ts;
    }
    int64_t ts;
};

template<>
struct is_xpack_xtype<BsonDate> {static bool const value = true;};

template<class OBJ>
inline bool xpack_xtype_decode(OBJ &obj, const char*key, BsonDate &val, const Extend *ext) {
    OBJ *o = obj.find(key, ext);
    if (NULL != o) {
        return o->decode("$date", val.ts, NULL);
    } else {
        return false;
    }
}
template<class OBJ>
inline bool xpack_xtype_encode(OBJ &obj, const char*key, const BsonDate &val, const Extend *ext) {
    std::map<std::string, int64_t> m;
    m["$date"] = val.ts;
    return obj.encode(key, m, ext);
}

}

#endif
