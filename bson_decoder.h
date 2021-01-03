/*
* Copyright (C) 2021 Duowan Inc. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#ifndef __X_PACK_BSON_DECODER_H
#define __X_PACK_BSON_DECODER_H


#include "thirdparty/libbson/include/libbson-1.0/bson.h"

#include "string.h"
#include "xpack/util.h"
#include "xpack/xdecoder.h"

namespace xpack {


class BsonDecoder:public XDecoder<BsonDecoder> {
public:
    friend class XDecoder<BsonDecoder>;
    using xdoc_type::decode;

    BsonDecoder(const uint8_t*data, size_t length, bool copy):xdoc_type(0, "") {
        parse(data, length, copy);
    }
    BsonDecoder(const std::string&data, bool copy):xdoc_type(0, "") {
        parse((const uint8_t*)data.data(), data.length(), copy);
    }
    ~BsonDecoder() {
    }

    inline const char * Type() const {
        return "bson";
    }

    std::string json() const {
        if (NULL == _data) {
            return "";
        }
        size_t length=BSON_UINT32_TO_LE(*(int32_t*)_data);
        bson_t b; // local is ok
        bson_init_static(&b, _data, length);

        size_t s;
        char *jstr = bson_as_json(&b, &s);
        if (jstr) {
            std::string j(jstr);
            bson_free(jstr);
            return j;
        } else {
            return "";
        }
    }
public:
    #define XPACK_BSON_DECODE_CHECK()                             \
        const bson_iter_t* it = get_val(key);                     \
        if (NULL == it) {                                         \
            if (Extend::Mandatory(ext)) {                         \
                decode_exception("mandatory key not found", key); \
            }                                                     \
            return false;                                         \
        }

    bool decode(const char *key, std::string &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()

        uint32_t length;
        const char* data = bson_iter_utf8(it, &length);
        if (NULL != data) {
            val = std::string(data, length);
        }
        return true;
    }
    bool decode(const char *key, bool &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        val = bson_iter_as_bool(it);
        return true;
    }

    // vtype is type of val, and f is function name, all use int64, or will fail if type not match
    #define XPACK_BSON_DECODE_NUMBER(vtype, f)      \
    inline bool decode(const char *key, vtype &val, const Extend *ext) {\
        const bson_iter_t* it = get_val(key);                     \
        if (NULL == it) {                                         \
            if (Extend::Mandatory(ext)) {                         \
                decode_exception("mandatory key not found", key); \
            }                                                     \
            return false;                                         \
        } else {                                                  \
            val = (vtype)f(it); return true;                      \
        }                                                         \
    }

    XPACK_BSON_DECODE_NUMBER(char, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(signed char, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(unsigned char, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(short, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(unsigned short, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(int, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(unsigned int, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(long, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(unsigned long, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(long long, bson_iter_as_int64)
    XPACK_BSON_DECODE_NUMBER(unsigned long long, bson_iter_as_int64)

    XPACK_BSON_DECODE_NUMBER(float, bson_iter_double)
    XPACK_BSON_DECODE_NUMBER(double, bson_iter_double)
    XPACK_BSON_DECODE_NUMBER(long double, bson_iter_double)

    size_t Size() const {
            return _childs.size();
    }
    BsonDecoder At(size_t index) {
        if (index < _childs.size()) {
            return BsonDecoder(&_childs[index], this, index);
        } else {
            throw std::runtime_error("Out of index");
        }
        return BsonDecoder(NULL, NULL, "");
    }
    BsonDecoder* Find(const char*key, BsonDecoder*tmp) {
        node_index::iterator iter;
        if (_childs_index.end() != (iter=_childs_index.find(key))) {
            tmp->_key = key;
            tmp->_parent = this;
            tmp->_node = &_childs[iter->second];
            tmp->init();
            return tmp;
        } else {
            return NULL;
        }
    }
    BsonDecoder Begin() {
        if (_childs.size() > 0) {
            return BsonDecoder(&_childs[0], this, bson_iter_key(&_childs[0]), 0);
        } else {
            return BsonDecoder(NULL, this, "");
        }
    }
    BsonDecoder Next() {
        if (NULL == _parent) {
            throw std::runtime_error("parent null");
        } else {
            size_t iter = _iter+1;
            if (iter < _parent->_childs.size()) {
                const bson_iter_t *nnode = &_parent->_childs[iter];
                return BsonDecoder(nnode, _parent, bson_iter_key(nnode), iter);
            } else {
                return BsonDecoder(NULL, _parent, "");
            }
        }
    }
    operator bool() const {
        return NULL != _node;
    }

private:
    typedef std::map<const char*, size_t, cmp_str> node_index; // index of _childs

    void parse(const uint8_t*data, size_t length, bool copy){
        bson_t b; // local is ok
        length = (length>0)?length:BSON_UINT32_TO_LE(*(int32_t*)data);

        if (copy) {
            std::string tmp(std::string((const char*)data, length));
            _tmp.swap(tmp);
            _data = (const uint8_t*)_tmp.data();
        } else {
            _data = data;
        }
        bson_init_static(&b, _data, length);

        bson_iter_init(&_root, &b);
        _node = &_root;
        init(true);
    }

    // get object or array info
    void init(bool top = false) {
        bson_iter_t sub;
        if (NULL == _node) {
            return;
        } else { // is array or object
            bool isobj;
            if (top) {
                isobj = true;
                memcpy((void*)&sub, (void*)_node, sizeof(sub));
            } else  if (bson_iter_recurse(_node, &sub)) {
                isobj = BSON_TYPE_ARRAY != bson_iter_type(_node);
            } else {
                return;
            }
            size_t i = 0;
            while (bson_iter_next(&sub)) {
                _childs.push_back(sub);
                if (isobj) {
                    _childs_index[bson_iter_key(&sub)] = i++;
                }
            }
        }
    }

    BsonDecoder():xdoc_type(0, ""),_node(NULL) {
        _data = NULL;
    }
    BsonDecoder(const bson_iter_t* val, const BsonDecoder*parent, const char*key, size_t iter=0):xdoc_type(parent, key),_node(val),_iter(iter) {
        _data = NULL;
        init();
    }
    BsonDecoder(const bson_iter_t* val, const BsonDecoder*parent, size_t index):xdoc_type(parent, index),_node(val) {
        _data = NULL;
        init();
    }

    const bson_iter_t* get_val(const char *key) {
        if (NULL == key) {
            return _node;
        } else if (NULL != _node) {
            node_index::iterator iter = _childs_index.find(key);
            if (iter != _childs_index.end()) {
                return &_childs[iter->second];
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    // only for parse
    std::string _tmp;
    const uint8_t* _data;
    bson_iter_t _root;

    const bson_iter_t* _node;          // current node
    std::vector<bson_iter_t> _childs;  // childs
    node_index _childs_index;
    size_t _iter;
};

}

#endif
