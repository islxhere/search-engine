#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include "protocol/TLVCodec.h"
#include "server/SearchEngineBackend.h"

using namespace muduo::net;

class SearchEngineServer {
public:
    SearchEngineServer(EventLoop *, const InetAddress &);

    void start();

private:
    void onConnection(const TcpConnectionPtr &);

    void onEntireMessage(const TcpConnectionPtr &, const TLVCodec::Message &, Timestamp);

    void recommendKeywords(const TcpConnectionPtr&, const TLVCodec::Message &);

    void searchWebpages(const TcpConnectionPtr&, const TLVCodec::Message &);

    TcpServer server_;
    TLVCodec tlv_codec_;
    SearchEngineBackend backend_;
};
