#include <iostream>

#include "xbson/bson.h"
#include "xpack/json.h"

using namespace std;

struct test {
    string a;
    bson_date_time_t dt1;
    bson_date_time_t dt2;
    bson_date_time_t dt3;
    bson_oid_t o;
    XPACK(O(a, dt1, dt2, dt3, o));
};

int main(int argc, char *argv[]) {
    // use static so parse only once
    if (argc == 1) {
        static xpack::BsonBuilder bd("{?:?, 'users':?}");
        cout<<"error:"<<bd.Error()<<endl;

        vector<int> v(3);
        v[0] = 1; v[1] = 2; v[2] = 3;
        cout<<"json1:"<<bd.EncodeAsJson("hi", true, v)<<endl;

        cout<<"json2:"<<bd.EncodeAsJson("uid", 123.0, "LiLei/HanMeimei/Jim")<<endl;

        cout<<"json3:"<<bd.EncodeAsJson("Lang", "C++", "")<<endl;

        test t;
        t.a = "good";
        t.dt1 = 1669096917012;
        t.dt2 = 1669096917000;
        t.dt3 = -1669096917456;
        bson_oid_init_from_string(&t.o, "5d505646cf6d4fe581014ab2");
        string s = xpack::json::encode(t);
        cout<<s<<endl;

        test t1;
        xpack::json::decode(s, t1);
        cout<<xpack::json::encode(t1)<<endl;

        xpack::BsonEncoder en;
        en.encode(NULL, t, NULL);
        string js = en.Json();
        cout<<"bson:"<<js<<endl;

        test t2;
        xpack::json::decode(js, t2);
        cout<<xpack::json::encode(t2)<<endl;

        bson_date_time_t dt;
        bool pok = dt.parse_rfc3339("2022-11-22T06:01:57.012Z");
        cout<<pok<<','<<dt.ts<<endl;

        pok = dt.parse_rfc3339("2022-11-22T06:01:57Z");
        cout<<pok<<','<<dt.ts<<endl;

        pok = dt.parse_rfc3339("2022-11-22T06:01:57Z-07:00");
        cout<<pok<<','<<dt.ts<<endl;

        pok = dt.parse_rfc3339("2022-11-22T06:01:57Z+07:00");
        cout<<pok<<','<<dt.ts<<endl;

        pok = dt.parse_rfc3339("2022-11-22T06:01:57Z07:00");
        cout<<pok<<','<<dt.ts<<endl;

        pok = dt.parse_rfc3339("2022-11-22T06:01:57.067Z07:00");
        cout<<pok<<','<<dt.ts<<endl;

        test tdt1;
        string dts = "{\"dt1\":{\"$date\":\"2022-11-21T22:01:57.012Z\"}}";
        xpack::json::decode(dts, tdt1);
        cout<<tdt1.dt1.ts<<endl;
    }

    if (argc > 1) {
        string bin(argv[1]);
        string b64;
        string bino;
        bson_binary_t::b64_ntop((const uint8_t*)bin.data(), bin.length(), b64);
        cout<<b64<<endl;
        bool ret = bson_binary_t::b64_pton(b64, bino);
        cout<<ret<<':'<<bino<<endl;
    }

    return 0;
}
