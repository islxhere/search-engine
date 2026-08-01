#pragma once
#include <functional>
#include <utility>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>

using namespace muduo::net;
using muduo::Timestamp;

// 协议格式：| Tag(1 字节) | Length (4 字节，网络字节序) | Value (Length 字节) |
class TLVCodec {
public:
    struct Message {
        uint8_t type; // 消息的类型: 1=关键字推荐, 2=网页搜索
        uint32_t length; // value 的长度
        std::string value; // 消息的内容
    };

    using MessageCallback = std::function<void(const TcpConnectionPtr &, Message &, Timestamp)>;

    explicit TLVCodec(MessageCallback cb)
        : messageCallback_(std::move(cb)) {}


    void send(const TcpConnectionPtr &, const Message &);

    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

private:
    MessageCallback messageCallback_;
};
