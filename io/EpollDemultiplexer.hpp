#pragma once

#include <io/Demultiplexer.hpp>

namespace io
{
namespace net
{
class EpollDemultiplexer : public Demultiplexer {
public:
    EpollDemultiplexer();
    ~EpollDemultiplexer();
    std::vector<Demultiplexer::EventHandlePair> WaitForEvents() override;

    bool add(const socket_type handle, const uint32_t mask) override;
    bool remove(const socket_type handle, const uint32_t mask) override;
    bool modify(const socket_type handle, const uint32_t mask) override;

private:
    int epollfd;
};
}
}
