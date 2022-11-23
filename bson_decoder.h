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

#ifndef __X_PACK_BSON_DECODER_H
#define __X_PACK_BSON_DECODER_H


#include "thirdparty/libbson/include/libbson-1.0/bson.h"

#include "string.h"
#include "xpack/util.h"
#include "xpack/xdecoder.h"
#include "bson_type.h"

namespace xpack {


class BsonDecoder:public XDecoder<BsonDecoder>, private noncopyable {
    friend class XDecoder<BsonDecoder>;

    class MemberIterator {
        friend class BsonDecoder;
    public:
        MemberIterator(size_t iter, BsonDecoder* parent):_iter(iter),_parent(parent){}
        bool operator != (const MemberIterator &that) const {
            return _iter != that._iter;
        }
        MemberIterator& operator ++ () {
            ++_iter;
            return *this;
        }
        const char* Key() const {
            if (NULL != _parent) {
                return _parent->get_iter_key(_iter);
            }
            return "";
        }
        BsonDecoder& Val() const {
            return _parent->member(*this, *_parent->alloc());
        }
    private:
        size_t _iter;
        BsonDecoder* _parent;
    };
public:
    using xdoc_type::decode;
    typedef MemberIterator Iterator;

    // if copy is false, BsonDecoder will parse bson in data. data's life time must >= BsonDecoder
    BsonDecoder(const uint8_t*data, size_t length, bool copy):xdoc_type(0, "") {
        parse(data, length, copy);
    }
    // if copy is false, BsonDecoder will parse bson in data. data's life time must >= BsonDecoder
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
    BsonDecoder& operator[](size_t index) {
        BsonDecoder *d = alloc();
        member(index, *d, NULL);
        return *d;
    }
    BsonDecoder& operator[](const char* key) {
        BsonDecoder *d = alloc();
        member(key, *d, NULL);
        return *d;
    }
    Iterator Begin() {
        return Iterator(0, this);
    }
    Iterator End() {
        return Iterator(_childs.size(), this);
    }
    operator bool() const {
        return NULL != _node;
    }

    // bson type
    bool decode(const char *key, bson_oid_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        const bson_oid_t *t = bson_iter_oid(it);
        if (t != NULL) {
            bson_oid_init_from_data(&val, t->bytes);
            return true;
        } else {
            return false;
        }
    }
    bool decode(const char *key, bson_date_time_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        val.ts = bson_iter_date_time(it);
        return true;
    }
    bool decode(const char *key, bson_timestamp_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        bson_iter_timestamp(it, &val.timestamp, &val.increment);
        return true;
    }
    bool decode(const char *key, bson_decimal128_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        return bson_iter_decimal128(it, &val);
    }
    bool decode(const char *key, bson_regex_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        const char *options = NULL;
        const char *regex = bson_iter_regex(it, &options);
        if (NULL != regex) {
            val.pattern = regex;
        }
        if (NULL != options) {
            val.options = options;
        }
        return true;
    }
    bool decode(const char *key, bson_binary_t &val, const Extend *ext) {
        XPACK_BSON_DECODE_CHECK()
        uint32_t len = 0;
        const uint8_t *data = NULL;
        bson_iter_binary(it, &val.subType, &len, &data);
        if (data != NULL && len > 0) {
            val.data = std::string((const char*)data, size_t(len));
        }
        return true;
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
         _childs.clear();
         _childs_index.clear();

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

    BsonDecoder& member(size_t index, BsonDecoder&d, const Extend *ext) {
        (void)ext;
        if (index < _childs.size()) {
            d.init_base(this, index);
            d._node = &_childs[index];
            d.init();
        } else {
            decode_exception("Out of index", NULL);
        }
        return d;
    }
    BsonDecoder& member(const char*key, BsonDecoder&d, const Extend *ext) {
        (void)ext;
        node_index::iterator iter;
        if (_childs_index.end() != (iter=_childs_index.find(key))) {
            d.init_base(this, key);
            d._node = &_childs[iter->second];
            d.init();
        }
        return d;
    }
    BsonDecoder& member(const Iterator &iter, BsonDecoder&d) {
        const bson_iter_t* node = &_childs[iter._iter];
        d.init_base(iter._parent, bson_iter_key(node));
        d._node = node;
        d.init();
        return d;
    }

    const char *get_iter_key(size_t index) const {
        return bson_iter_key(&_childs[index]);
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
