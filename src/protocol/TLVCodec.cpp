#include "protocol/TLVCodec.h"


void TLVCodec::send(const TcpConnectionPtr &conn, const Message &msg) {
    Buffer buf;
    uint8_t type = msg.type;
    uint32_t length = htobe32(msg.length);
    buf.append(&type, 1);
    buf.append(&length, sizeof length);
    buf.append(msg.value.data(), msg.value.size());
    conn->send(&buf);
}

void TLVCodec::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp timestamp) {
    Message parsed_msg{};
    while (buf->readableBytes() >= 5) {
        parsed_msg.type = buf->peekInt8();
        uint32_t netlen = 0;
        memcpy(&netlen, buf->peek() + 1, 4);
        parsed_msg.length = be32toh(netlen);

        if (buf->readableBytes() < parsed_msg.length + 5) break;

        buf->retrieve(5);
        parsed_msg.value = buf->retrieveAsString(parsed_msg.length);

        messageCallback_(conn, parsed_msg, timestamp);
    }
}
