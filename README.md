# xbson
* 用于在C++结构体和bson之间互相转换
* 依赖于[xpack](https://github.com/xyz347/xpack)
* 使用方法请参考[xpack](https://github.com/xyz347/xpack)
* xbson自身只有头文件, 无需额外编译库文件，依赖于libbson，在thirdparty目录下。
* 这个库可以配合[mongoxclient](https://github.com/xyz347/mongoxclient)使用

# 目录
- [encoder](#encoder)
- [decoder](#decoder)
- [builder](#builder)
- [重要说明](#重要说明)

## encoder
用于将结构体转成bson格式。范例：
```C++
struct Test {
	int64_t     id;
	std::string name;
	std::string email;
	XPACK(A(id, "bson:_id"), O(name, email));
};

Test t;
std::string data = xpack::bson::encode(t); // data就是t转成bson的数据
```
关于XPACK的使用，请参考[xpack](https://github.com/xyz347/xpack)

## decoder
用于将bson数据转成结构体
用于将结构体转成bson格式。范例：
```C++
struct Test {
	int64_t     id;
	std::string name;
	std::string email;
	XPACK(A(id, "bson:_id"), O(name, email));
};

std::string data;
// data = xxxx; data存储了相应的bson数据
Test t;
xpack::bson::decode(data, t);
// xpack::bson::decode 还有另一个参数方式：static void decode(const uint8_t* data, size_t len, T &val)
```
关于XPACK的使用，请参考[xpack](https://github.com/xyz347/xpack)

## builder
**需要C++11或以上版本支持**。用json的格式来构造bson数据。查询接口中的query，一般只需要指定很少几个字段，这个场景用结构体表达不是很方便。用builder就会简单很多。基本用法是：
1. 定义一个BsonBuilder：
	- 参数是一个json格式的字符串
	- 里面的双引号用单引号替代
	- 变量用问号作为占位符，问号后面可以加一个单词用来作为助记符，比如"?"和"?uid"是等效的
	- key也可以是变量，key作为变量，问号不用加引号
	- BsonBuilder是线程安全的，建议定义为static变量，这样json字符串只需解析一次
范例：
```C++
static xpack::BsonBuilder bd("{'_id':?, ?:?}"); // 带占位符的builder
static xpack::BsonBuilder empty("{}");   // 空bson
static xpack::BsonBuilder student("{'type':'student'}"); // 不带占位符的builder
```

2. 调用Encode函数来构造bson数据，Encode的参数对应的是各个占位符，因此参数个数应该和占位符一致
比如对于上述的BsonBuilder
```C++
std::string data1 = bd.Encode(12345, "type", "master");
std::string data2 = empty.Encode();
std::string data3 = student.Encode();
// 这些bson的数据可以用在mongoxclient的各个接口中
```

3. 其他的一些API
	- EncodeAsJson 和Encode类似，但是输出的是json格式的数据，可以用于调试等场景（所以这个其实也是个JsonBuilder）
	- Error 如果构造BsonBuilder的json字符串有误，这个接口可以返回错误信息
	- static std::string En(const std::string&fmt, Args... args)。这个是把构造BsonBuilder和Encode合在一起了，不是很建议使用，建议用于调试，因为每次都要构造一次BsonBuilder


## 重要说明
- bson的反序列化用的是[libbson](https://github.com/mongodb/libbson/tree/1.0.0)
- bson的序列化是我自己写的，没有参考相应标准，可能有和标准不一样的地方.
- libbson是预编译的. 环境是: Ubuntu12.04 g++4.9.2. 其他环境如果想使用该库文件可能需要自行编译
