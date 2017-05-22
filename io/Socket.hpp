#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <io/Platform.hpp>
#include <io/PlatformNetworking.hpp>
#include <io/NetworkTraits.hpp>
#include <vector>

namespace io {
namespace net {
class SocketBase : public std::enable_shared_from_this<SocketBase> {
public:
    virtual socket_type GetHandle() const noexcept = 0;
    virtual ~SocketBase() {}
};
typedef std::shared_ptr<SocketBase> SocketBasePtr;

template<typename protocol, typename carrier, typename SOCKTYPE>
class Socket : public SocketBase {
public:
    using socket_type = SOCKTYPE;

    std::shared_ptr<Socket<protocol, carrier, SOCKTYPE>> ptr()
    {
        return this->shared_from_this();
    }

    explicit Socket()
    {
        fd = ::socket(carrier::value, protocol::value, protocol::proto);
        std::cout << "created socket with fd " << fd << std::endl;
    }

    explicit Socket(SOCKTYPE desc) : fd(desc) {}

    Socket(int desc, const typename carrier::sockaddr_type& addr)
        : fd(desc), local_addr(addr)
    {}

    Socket(const typename carrier::sockaddr_type& saddr)
        : Socket()
    {
        memcpy(&local_addr, &saddr, sizeof(saddr));

        if (!set_options()) {
            throw std::runtime_error("Failed to set socket options");
        }

        auto err = ::bind(fd, &saddr, carrier::address_size());
        if (err < 0) {
            throw std::runtime_error("Failed to bind()");
        }

        err = ::listen(fd, 10);
        if (err < 0) {
            throw std::runtime_error("Failed to listen()");
        }
    }

    Socket(const ipv4::addr_type& addr, const std::uint16_t port)
        : Socket()
    {
        if (!set_options()) {
            throw std::runtime_error("Failed to set socket options");
        }

        memcpy(&local_addr.sin_addr.s_addr, &addr, sizeof(ipv4::addr_type));

        struct sockaddr_in saddr;
        saddr.sin_family = carrier::value;
        saddr.sin_port = htons(port);
        memcpy(&saddr.sin_addr.s_addr, &local_addr.sin_addr.s_addr, sizeof(ipv4::addr_type));

        auto err = ::bind(fd, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(saddr));
        if (err < 0) {
            throw std::runtime_error("Failed to bind()");
        }

        err = ::listen(fd, 10);
        if (err == -1) {
            throw std::runtime_error("Failed to listen()");
        }
    }

    Socket(const typename ipv6::addr_type& addr, const std::uint16_t port)
        : Socket()
    {
        memcpy(&local_addr.sin6_addr.s6_addr, &addr, sizeof(ipv6::addr_type));

        struct sockaddr_in6 saddr;
        saddr.sin6_family = carrier::value;
        saddr.sin6_port = htons(port);
        memcpy(&saddr.sin6_addr.s6_addr, &local_addr.sin6_addr.s6_addr, sizeof(ipv6::addr_type));

        addrinfo hints, *results = nullptr;
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = carrier::value;
        hints.ai_socktype = protocol::value;
        hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;

	std::ostringstream portStr;
	portStr << port;
        auto ret = getaddrinfo(nullptr, portStr.str().c_str(), &hints, &results);
        assert(ret > -1);
        assert(results[0].ai_family == AF_INET6);

        auto err = ::bind(fd, results[0].ai_addr, static_cast<int>(results[0].ai_addrlen));
        if (err < 0) {
            throw std::runtime_error("Failed to bind()");
        }

        err = ::listen(fd, 10);
        if (err == -1) {
            perror(nullptr);
            assert(!"can't listen");
        }
    }

    socket_type GetHandle() const noexcept { return fd; }

    Socket(const Socket& rhs) = delete;

    ~Socket()
    {
        std::cout << "~Socket() !!!" << std::endl;
        if (fd != -1) {
#if defined _WIN32 || defined  _WIN64
            closesocket(fd);
#else
            close(fd);
#endif
            fd = -1;
        }
    }

    size_t send(const std::unique_ptr<std::vector<uint8_t>>& data)
    {
#if defined(_WIN32) || defined(_WIN64)
        WSABUF buf;
        buf.buf = data->data();
        buf.len = data->size();
        int err = WSASend(fd, &buf, 1, &nsent, 0, nullptr, nullptr);
        //err = WSASend(socket->id(), &wsaBuf, 1, nullptr, 0, &txContext.overlapped, nullptr);
#else
#endif
    }

    void receive(const std::unique_ptr<std::vector<uint8_t>>& buf)
    {
    }

private:
    SOCKTYPE fd;
    typename carrier::sockaddr_type local_addr{};

    bool set_options()
    {
        struct {
            int opt;
            int val;
        } options[] = {
    #if !defined(_WIN32) && !defined(_WIN64)
            { SO_REUSEPORT, 1 },
    #endif
            { SO_REUSEADDR, 1 },
            { SO_KEEPALIVE, 1 },
        };

        for (auto k = 0; k < sizeof(options) / sizeof(options[0]); k++) {
            auto err = setsockopt(
                fd,
                SOL_SOCKET,
                options[k].opt,
                reinterpret_cast<char *>(&options[k].val),
                sizeof(options[k].val));
            if (err == -1) {
                return false;
            }
        }

#if !defined(_WIN32) && !defined(_WIN64)
        auto flags = ::fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            flags = 0;
        }

        auto err = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (err == -1) {
            return false;
        }
#endif
        return true;
    }
};
}
}
