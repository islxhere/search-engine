#include "server/SearchEngineServer.h"

using namespace std::placeholders;

SearchEngineServer::SearchEngineServer(EventLoop *loop, const InetAddress &listenAddr)
    : server_(loop, listenAddr, "SE"),
      tlv_codec_(bind(&SearchEngineServer::onEntireMessage, this, _1, _2, _3)) {
    server_.setConnectionCallback(bind(&SearchEngineServer::onConnection, this, _1));
    server_.setMessageCallback(bind(&TLVCodec::onMessage, tlv_codec_, _1, _2, _3));
}

void SearchEngineServer::start() { server_.start(); }

void SearchEngineServer::onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
        // todo
    }
}

void SearchEngineServer::onEntireMessage(const TcpConnectionPtr &conn, const TLVCodec::Message &msg, Timestamp) {
    switch (msg.type) {
        case 1: {
            recommendKeywords(conn, msg);
            break;
        }
        case 2: {
            searchWebpages(conn, msg);
            break;
        }
        default: break;
    }
}

void SearchEngineServer::recommendKeywords(const TcpConnectionPtr &conn, const TLVCodec::Message &msg) {
    const auto response = backend_.recommend(msg.value);
    tlv_codec_.send(conn, {msg.type, static_cast<uint32_t>(response.size()), response});
}

void SearchEngineServer::searchWebpages(const TcpConnectionPtr &conn, const TLVCodec::Message &msg) {
    const auto response = backend_.search(msg.value);
    tlv_codec_.send(conn, {msg.type, static_cast<uint32_t>(response.size()), response});
}
