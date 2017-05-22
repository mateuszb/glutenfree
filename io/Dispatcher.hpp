#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <list>
#include <numeric>
#include <functional>
#include <map>
#include <io/Socket.hpp>
#include <io/Event.hpp>
#include <io/EventHandler.hpp>
#include <io/Demultiplexer.hpp>

namespace io
{
namespace net
{
class Dispatcher {
public:
    static std::shared_ptr<Dispatcher> Instance();
    ~Dispatcher();
    
    bool AddHandler(const EventHandlerPtr& handler, const uint32_t mask);
    bool ModifyHandler(const EventHandlerPtr& handler, const uint32_t mask);
    bool RemoveHandler(const EventHandlerPtr& handler, const uint32_t mask);
    uint32_t HandleEvents(const uint32_t timeoutMs);
private:
    std::map<socket_type, uint32_t> eventHandlerMasks;
    std::map<socket_type, EventHandlerPtr> eventHandlers;
    DemultiplexerPtr demuxer;
    
private:
    Dispatcher();
};

typedef std::shared_ptr<Dispatcher> DispatcherPtr;
}
}
