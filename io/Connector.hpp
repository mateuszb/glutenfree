#pragma once

#include <string>

namespace io
{
namespace net
{
template<typename PROTOCOL, typename CARRIER>
class Connector {
public:
    Connector(
        const std::string& remoteNumericAddr,
        const uint16_t port)
        : remoteAddr(remoteNumericAddr), remotePort(port)
    {
        socket = std::make_shared<Socket<PROTOCOL, CARRIER, socket_type>>();
    }

    bool connect()
    {
        int err = 0;
        if (CARRIER::value == ipv6::value) {
            ipv6::sockaddr_type remote;
            memset(&remote, 0, sizeof(remote));
            inet_pton(ipv6::value, remoteAddr.c_str(), &remote.sin6_addr);
            remote.sin6_family = ipv6::value;
            remote.sin6_port = htons(remotePort);
            err = ::connect(socket->GetHandle(), reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
        }
        else {
            ipv4::sockaddr_type remote;
            memset(&remote, 0, sizeof(remote));
            inet_pton(ipv4::value, remoteAddr.c_str(), &remote.sin_addr);
            remote.sin_family = ipv4::value;
            remote.sin_port = htons(remotePort);
            err = ::connect(socket->GetHandle(), reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
        }
        if (err == -1) {
            throw std::runtime_error(
                "Can't connect to remote host " + remoteAddr + " on port " + std::to_string(remotePort));
        }

        return true;
    }

    size_t send(const std::vector<uint8_t>& data)
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD nsent = 0;
        WSABUF buffers[1];
        buffers[0].buf = const_cast<char*>(reinterpret_cast<const char*>(data.data()));
        buffers[0].len = static_cast<ULONG>(data.size());
        int err = WSASend(socket->GetHandle(), buffers, 1, &nsent, 0, nullptr, nullptr);
#else
        ssize_t nsent = 0;
        int err = ::write(socket->GetHandle(), data.data(), data.size());

        if (err == -1) {
            if (errno == EAGAIN) {
                std::cout << "EAGAIN" << std::endl;
            }
            if (errno == EWOULDBLOCK) {
                std::cout << "EWOULDBLOCK" << std::endl;
            }
        }
#endif
        if (err == -1) {
            throw std::runtime_error("Error sending data");
        }
        return nsent;
    }

    size_t receive(std::vector<uint8_t>& rxbuf)
    {
#if defined _WIN32 || defined _WIN64
        DWORD nrecvd = 0, flags = 0;
        WSABUF buffer;
        buffer.buf = const_cast<char*>(reinterpret_cast<const char*>(rxbuf.data()));
        buffer.len = static_cast<ULONG>(rxbuf.size());
        int err = WSARecv(socket->GetHandle(), &buffer, 1, &nrecvd, &flags, nullptr, nullptr);
#else
        ssize_t nrecvd, err;
        nrecvd = err = recv(socket->GetHandle(), rxbuf.data(), rxbuf.size(), MSG_WAITALL);
#endif
        if (err == -1) {
            throw std::runtime_error("Error receiving data");
        }

        return nrecvd;
    }

    ~Connector()
    {
    }

private:
    const uint16_t remotePort;
    const std::string remoteAddr;
    SocketBasePtr socket;
};
}
}
