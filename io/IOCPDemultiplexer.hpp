#pragma once

#include <io/Demultiplexer.hpp>

namespace io
{
namespace net
{

class IOCPDemultiplexer : public Demultiplexer
{
public:
    struct IOContext
    {
        WSAOVERLAPPED overlapped;
        uint32_t    event;
    };

    IOCPDemultiplexer();
    ~IOCPDemultiplexer();

    bool add(const socket_type handle, const uint32_t mask) override;
    bool remove(const socket_type handle, const uint32_t mask) override;
    bool modify(const socket_type handle, const uint32_t mask) override;

    std::vector<Demultiplexer::EventHandlePair> WaitForEvents();

private:
    HANDLE iocp;
};
}
}
