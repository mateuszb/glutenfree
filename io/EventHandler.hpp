#pragma once
#include <memory>
#include <io/Platform.hpp>
#include <io/Event.hpp>

namespace io
{
namespace net
{
class EventHandler {
public:
    virtual bool HandleEvent(const uint32_t eventMask) = 0;
    virtual socket_type GetHandle() const = 0;
    virtual ~EventHandler() {}
};
typedef std::shared_ptr<EventHandler> EventHandlerPtr;
}
}
