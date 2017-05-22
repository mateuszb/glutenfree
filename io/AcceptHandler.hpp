#pragma once

#include <io/Socket.hpp>
#include <io/EventHandler.hpp>
#include <io/NetworkTraits.hpp>
#include <io/IOHandler.hpp>
#include <io/ApplicationHandler.hpp>
#include <io/Dispatcher.hpp>
#include <string>

namespace io
{
namespace net
{

class BaseSocketFactory
{
public:
    virtual SocketBasePtr Make() = 0;
};

using BaseSocketFactoryPtr = std::shared_ptr<BaseSocketFactory>;

template<typename PROTO, typename CARRIER>
class SocketFactory : public BaseSocketFactory
{
public:
    SocketBasePtr Make()
    {
        return std::make_shared<Socket<PROTO, CARRIER, socket_type>>();
    }
};

template<typename HandlerFactory>
class AcceptHandler : public EventHandler {
public:
    AcceptHandler(
        const std::string& hostname,
        const uint16_t port,
        const tcp&,
        const ipv6&)
        : hostName(hostname)
    {
        ipv6::addr_type addr = IN6ADDR_ANY_INIT;
        socket = std::make_shared<Socket<tcp, ipv6, socket_type>>(addr, port);
        socketFactory = std::make_shared<SocketFactory<tcp, ipv6>>();
    }

    AcceptHandler(
        const std::string& hostname,
        const uint16_t port,
        const udp&,
        const ipv6&)
    {
        throw std::runtime_error("Not implemented");
    }

    AcceptHandler(
        const std::string& hostname,
        const uint16_t port,
        const tcp&,
        const ipv4&)
        : hostName(hostname)
    {
        ipv4::addr_type addr = { INADDR_ANY };
        socket = std::make_shared<Socket<tcp, ipv4, socket_type>>(addr, port);
        socketFactory = std::make_shared<SocketFactory<tcp, ipv4>>();
    }

    AcceptHandler(
        const std::string& hostname,
        const uint16_t port,
        const udp&,
        const ipv4&)
    {
        throw std::runtime_error("Not implemented");
    }

    ~AcceptHandler() {}

    socket_type GetHandle() const
    {
        return socket->GetHandle();
    }

    bool HandleEvent(const uint32_t eventMask)
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD ret = ~0;
        if (acceptCtx.overlapped.hEvent != nullptr) {
            ret = WaitForSingleObject(acceptCtx.overlapped.hEvent, 0);
        }

        // accept event has been triggered. this means new connection is accepted
        if (ret == WAIT_OBJECT_0) {
            ApplicationHandlerPtr appHandler =
                HandlerFactory::Make(hostName, std::move(newSocket));
        }

        if (ret == WAIT_TIMEOUT) {
            return true;
        }

        if (acceptCtx.overlapped.hEvent == nullptr) {
            acceptCtx.overlapped.hEvent = WSACreateEvent();
        }
        BOOL result = false;
        DWORD dwAddrLen = sizeof(addrBuf);

        do {
            acceptCtx.event = EventType::Accept;
            newSocket = socketFactory->Make();

            result = AcceptEx(
                socket->GetHandle(),
                newSocket->GetHandle(),
                addrBuf,
                0,
                dwAddrLen,
                dwAddrLen,
                nullptr,
                &acceptCtx.overlapped);

            if (result) {
                ApplicationHandlerPtr appHandler =
                    HandlerFactory::Make(hostName, std::move(newSocket));
            }
        } while (result);

        return true;
#else
        sockaddr_storage remote;
        socklen_t len = sizeof(remote);

        socket_type ret = ::accept(
            socket->GetHandle(),
            reinterpret_cast<sockaddr*>(&remote),
            &len);
        if (ret == -1) {
            perror(nullptr);
            throw std::runtime_error("Failed to accept() a new connection");
        }
        SocketBasePtr newsocket;
        if (remote.ss_family == ipv6::value) {
            newsocket = std::make_shared<Socket<tcp, ipv6, socket_type>>(ret);
        }
        else {
            newsocket = std::make_shared<Socket<tcp, ipv4, socket_type>>(ret);
        }

        ApplicationHandlerPtr appHandler =
            HandlerFactory::Make(hostName, newsocket);

        return true;
#endif
    }

private:
    const std::string& hostName;
    SocketBasePtr socket;
#if defined(_WIN32) || defined(_WIN64)
    IOCPDemultiplexer::IOContext acceptCtx {};
    SocketBasePtr newSocket;
    char addrBuf[256 * 2];
#endif
	BaseSocketFactoryPtr socketFactory;
};
}
}
