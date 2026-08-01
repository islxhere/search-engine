#include "server/SearchEngineServer.h"

#include "core/EnvLoader.h"
#include "webpage/DocLibrary.h"
#include "webpage/InvertedIndex.h"
#include "webpage/WebSearcher.h"

using namespace std::placeholders;

SearchEngineServer::SearchEngineServer(EventLoop *loop, const InetAddress &listenAddr)
    : server_(loop, listenAddr, "SE"),
      tlv_codec_(bind(&SearchEngineServer::onEntireMessage, this, _1, _2, _3)),
      docs_(EnvLoader::se_data_webpage_dir() + "/pages.dat", EnvLoader::se_data_webpage_dir() + "/offsets.dat"),
      index_(EnvLoader::se_data_webpage_dir() + "/inverted_index.dat"),
      searcher_(docs_, index_),
      cn_dictionary_(EnvLoader::se_data_dict_dir() + "/dict_cn.dat"),
      cn_index_(EnvLoader::se_data_dict_dir() + "/index_cn.dat", cn_dictionary_.size()),
      en_dictionary_(EnvLoader::se_data_dict_dir() + "/dict_en.dat"),
      en_index_(EnvLoader::se_data_dict_dir() + "/index_en.dat", en_dictionary_.size()),
      recommender_(cn_dictionary_, cn_index_, en_dictionary_, en_index_) {
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
    const auto response = recommender_.recommend(msg.value);
    tlv_codec_.send(conn, {msg.type, static_cast<uint32_t>(response.size()), response});
}

void SearchEngineServer::searchWebpages(const TcpConnectionPtr &conn, const TLVCodec::Message &msg) {
    const auto response = searcher_.search(msg.value);
    tlv_codec_.send(conn, {msg.type, static_cast<uint32_t>(response.size()), response});
}
