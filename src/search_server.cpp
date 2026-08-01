#include "server/SearchEngineServer.h"

#include <exception>
#include <iostream>

int main() {
    try {
        muduo::net::EventLoop loop;
        SearchEngineServer server(&loop, muduo::net::InetAddress(8888));
        server.start();
        std::cout << "搜索服务已启动：0.0.0.0:8888\n";
        loop.loop();
    } catch (const std::exception &error) {
        std::cerr << "搜索服务启动失败: " << error.what() << '\n';
        return 1;
    }
}
