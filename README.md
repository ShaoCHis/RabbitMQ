# RabbitMQ
使用C++仿造 RabbitMQ 实现消息队列

# 开发环境
- Linux（ubuntu22.04）
- g++/gdb
- CMake

# 技术选型
- 序列化框架：Protobuf
- 网络通信：自定义应用层协议+muduo库
- 源数据信息数据库：SQLite3
- 单元测试框架：Gtest