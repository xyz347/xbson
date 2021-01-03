xbson
====
* 用于在C++结构体和bson之间互相转换
* 依赖于[xpack](https://github.com/xyz347/xpack)
* 使用方法请参考[xpack](https://github.com/xyz347/xpack)
* xbson自身只有头文件, 无需额外编译库文件，依赖于libbson，在thirdparty目录下。

------
* [重要说明](#重要说明)

重要说明
----
- bson的反序列化用的是[libbson](https://github.com/mongodb/libbson/tree/1.0.0)
- bson的序列化是我自己写的，没有参考相应标准，可能有和标准不一样的地方.
- libbson是预编译的. 环境是: Ubuntu12.04 g++4.9.2. 其他环境如果想使用该库文件可能需要自行编译
