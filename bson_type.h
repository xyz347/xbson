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
#include <ctime>
#include <time.h>
#include <stdint.h>
#include "xpack/xpack.h"

#include "thirdparty/libbson/include/libbson-1.0/bson.h"

/*
// define in this file
bson_date_time_t
bson_timestamp_t
bson_binary_t
bson_regex_t

// define in bson.h
bson_oid_t
bson_decimal128_t
*/

// define in global namespace for consistency with bson
struct bson_timestamp_t {
    uint32_t timestamp;
    uint32_t increment;
    bson_timestamp_t(const uint32_t&t=0, const uint32_t&i=0):timestamp(t),increment(i){}
    XPACK(A(timestamp, "t", increment, "i"));
};
struct bson_binary_t {
    std::string data;
    bson_subtype_t subType;
};
struct bson_regex_t {
    std::string pattern;
    std::string options;
    // ver==1 {"$regularExpression": {pattern: <string>, "options": <string>"}}
    // ver==2 {"$regex: <string>, $options: <string>"}
    int ver; // ver==1 
    bson_regex_t():ver(1){}
};
struct bson_date_time_t {
    int64_t ts; // milliseconds
    bson_date_time_t(const int64_t&t=0):ts(t){}
    operator int64_t() const {return ts;}

    // parse rfc3339 time format(milliseconds). C++ doesn't even have a full-featured standard time library T_T
    // "2000-01-01T01:01:01[.001]Z[07:00]" string should ends with \0
    bool parse_rfc3339(const char buf[]) {
        struct tm tm;
        memset((void*)&tm, 0, sizeof(tm));

        const char *end = strptime(buf, "%Y-%m-%dT%H:%M:%S", &tm);
        if (end == NULL) {
            return false;
        }

        int msec = 0;
        int offset = 0;
        try {
            size_t pos;
            if (*end == '.') {
                msec = std::stoi(end+1, &pos);
                if (pos==0 || pos > 3) {
                    return false;
                }
                end += pos+1;
            }
            if (*end != 'Z') {
                return false;
            }
            ++end;
            if (*end != '\0') {
                pos = 0;
                int hour = std::stoi(end, &pos);
                if (pos == 0 || pos > 3) {
                    return false;
                }
                end += pos;
                if (*end != ':') {
                    return false;
                }
                int min = std::stoi(end+1, &pos);
                if (hour > 0) {
                    offset = hour*3600 + min*60;
                } else {
                    offset = -(-hour*3600 + min*60);
                }
            }
        } catch(...) {
            return false;
        }

        time_t tmp = mktime(&tm);
        ts = tmp*1000 + msec + offset;
        return true;
    }
};

namespace xpack {

// ref https://github.com/mongodb/specifications/blob/master/source/extended-json.rst#conversion-table
// Relaxed Extended JSON Format Only

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ObjectId ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
template<>
struct is_xpack_json_type<bson_oid_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_oid_t &val, const Extend *ext) {
    bool ret;
    OBJ *o = obj.find(key, ext);
    if (o != NULL) {
        char buf[25];
        ret = o->decode("$oid", buf, NULL);
        if (ret) {
            bson_oid_init_from_string(&val, buf);
        }
    } else {
        ret = false;
    }
    return ret;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_oid_t &val, const Extend *ext) {
    char buf[25];
    bson_oid_to_string(&val, buf);
    obj.ObjectBegin(key, ext);
    obj.encode("$oid", buf, NULL);
    obj.ObjectEnd(key, ext);
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ date time ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
template<>
struct is_xpack_json_type<bson_date_time_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_date_time_t &val, const Extend *ext) {
    OBJ *o = obj.find(key, ext);
    if (o == NULL || NULL == (o = o->find("$date", NULL))) {
        return false;
    }

    bool ret;
    try {
        ret = o->decode(NULL, val.ts, NULL);
        return ret;
    } catch (...){}

    char buf[25];
    try {
        ret = o->decode(NULL, buf, NULL);
        if (ret) {
            ret = val.parse_rfc3339(buf);
        }
        return ret;
    } catch (...) {}

    try {
        ret = o->decode("$numberLong", buf, NULL);
        if (ret) {
            ret = Util::atoi(buf, val.ts);
        }
        return ret;
    } catch (...) {}

    return false;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_date_time_t &val, const Extend *ext) {
    obj.ObjectBegin(key, ext);
    if (val.ts >= 0) {
        char buf[32];
        int msecs = int(val.ts % 1000);
        time_t secs = (time_t)(val.ts/1000);

        #ifndef _MSC_VER
        struct tm posix_date;
        size_t sz = strftime(buf, sizeof buf, "%FT%T", gmtime_r(&secs, &posix_date));
        #else
        size_t sz = strftime(buf, sizeof buf, "%FT%T", gmtime(&secs));
        #endif

        if (msecs > 0) {
            sprintf(&buf[sz], ".%03dZ", msecs);
        } else {
            strcpy(&buf[sz], "Z");
        }
        obj.encode("$date", buf, NULL);
    } else { // key:{"$date":{"$numberLong":ts}} ts use string not int 
        obj.ObjectBegin("$date", NULL);
        obj.encode("$numberLong", Util::itoa(val.ts), NULL);
        obj.ObjectEnd("$date", NULL);
    }
    obj.ObjectEnd(key, ext);
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ timestamp ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
template<>
struct is_xpack_json_type<bson_timestamp_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_timestamp_t &val, const Extend *ext) {
    bool ret;
    OBJ *o = obj.find(key, ext);
    if (o != NULL) {
        ret = o->decode("$timestamp", val, NULL);
    } else {
        ret = false;
    }
    return ret;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_timestamp_t &val, const Extend *ext) {
    obj.ObjectBegin(key, ext);
    obj.encode("$timestamp", val, NULL);
    obj.ObjectEnd(key, ext);
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ numberDecimal ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
template<>
struct is_xpack_json_type<bson_decimal128_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_decimal128_t &val, const Extend *ext) {
    bool ret;
    OBJ *o = obj.find(key, ext);
    if (o != NULL) {
        char buf[BSON_DECIMAL128_STRING];
        ret = o->decode("$numberDecimal", buf, NULL);
        if (ret) {
            bson_decimal128_from_string(buf, &val);
        }
    } else {
        ret = false;
    }
    return ret;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_decimal128_t &val, const Extend *ext) {
    char buf[BSON_DECIMAL128_STRING];
    bson_decimal128_to_string(&val, buf);
    obj.ObjectBegin(key, ext);
    obj.encode("$numberDecimal", buf, NULL);
    obj.ObjectEnd(key, ext);
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ binary ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
template<>
struct is_xpack_json_type<bson_binary_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_binary_t &val, const Extend *ext) {
    return true;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_binary_t &val, const Extend *ext) {
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ regex ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
template<>
struct is_xpack_json_type<bson_regex_t> {static bool const value = true;};
template<class OBJ>
bool xpack_json_type_decode(OBJ &obj, const char*key, bson_regex_t &val, const Extend *ext) {
    bool ret;
    OBJ *o = obj.find(key, ext);
    if (o == NULL) {
        return false;
    }
    OBJ *v1 = o->find("$regularExpression", NULL);
    try {
        if (v1 != NULL) {
            v1->decode("pattern", val.pattern, NULL);
            v1->decode("options", val.options, NULL);
            val.ver = 1;
        } else {
            o->decode("$regex", val.pattern, NULL);
            o->decode("$options", val.options, NULL);
            val.ver = 2;
        }
    } catch(...){}
    return true;
}
template<class OBJ>
bool xpack_json_type_encode(OBJ &obj, const char*key, const bson_regex_t &val, const Extend *ext) {
    obj.ObjectBegin(key, ext);
    if (val.ver == 1) {
        obj.ObjectBegin("$regularExpression", NULL);
        obj.encode("pattern", val.pattern, NULL);
        obj.encode("options", val.options, NULL);
        obj.ObjectEnd("$regularExpression", NULL);
    } else {
        obj.encode("$regex", val.pattern, NULL);
        obj.encode("$options", val.options, NULL);
    }
    obj.ObjectEnd(key, ext);
    return true;
}


}

#endif
