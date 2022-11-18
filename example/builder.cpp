#include <iostream>

#include "xbson/bson.h"
#include "xpack/json.h"

using namespace std;

struct test {
    string a;
    xpack::BsonDate b;
    XPACK(O(a, b));
};

int main(int argc, char *argv[]) {
    // use static so parse only once
    static xpack::BsonBuilder bd("{?:?, 'users':?}");
    cout<<"error:"<<bd.Error()<<endl;

    vector<int> v(3);
    v[0] = 1; v[1] = 2; v[2] = 3;
    cout<<"json1:"<<bd.EncodeAsJson("hi", true, v)<<endl;

    cout<<"json2:"<<bd.EncodeAsJson("uid", 123.0, "LiLei/HanMeimei/Jim")<<endl;

    cout<<"json3:"<<bd.EncodeAsJson("Lang", "C++", "")<<endl;

    test t;
    t.a = "good";
    t.b = 1668768118000;
    string s = xpack::json::encode(t);
    cout<<s<<endl;

    xpack::BsonEncoder en;
    en.encode(NULL, t, NULL);
    string js = en.Json();
    cout<<"bson:"<<js<<endl;
}
