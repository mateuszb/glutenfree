#pragma once

#include <io/Demultiplexer.hpp>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

namespace io
{
namespace net
{
class KqueueDemultiplexer : public Demultiplexer {
public:
    KqueueDemultiplexer();
    ~KqueueDemultiplexer();
    std::vector<Demultiplexer::EventHandlePair> WaitForEvents() override;

    bool add(const socket_type handle, const uint32_t mask) override;
    bool remove(const socket_type handle, const uint32_t mask) override;
    bool modify(const socket_type handle, const uint32_t mask) override;

private:
    int kqueuefd;
};
}
}
