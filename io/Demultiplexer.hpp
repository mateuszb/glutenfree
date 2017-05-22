#pragma once

#include <io/Event.hpp>
#include <io/Platform.hpp>
#include <vector>
#include <utility>
#include <memory>

namespace io
{
namespace net
{
class Demultiplexer {
public:
    virtual ~Demultiplexer() {}

    using EventHandlePair = std::pair<uint32_t, socket_type>;
    virtual std::vector<EventHandlePair> WaitForEvents() = 0;
    static std::unique_ptr<Demultiplexer> Make();

    virtual bool add(const socket_type handle, const uint32_t mask) = 0;
    virtual bool remove(const socket_type handle, const uint32_t mask) = 0;
    virtual bool modify(const socket_type handle, const uint32_t mask) = 0;

protected:
    Demultiplexer() {}
};

typedef std::unique_ptr<Demultiplexer> DemultiplexerPtr;

}
}
