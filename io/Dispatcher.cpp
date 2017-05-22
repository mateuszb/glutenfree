#include <memory>
#include <cstddef>
#include <set>
#include <io/EventHandler.hpp>
#include <io/Dispatcher.hpp>

using namespace std;
using namespace io::net;

DispatcherPtr Dispatcher::Instance()
{
    static DispatcherPtr dispatcher = shared_ptr<Dispatcher>(new Dispatcher());
    return dispatcher;
}

Dispatcher::Dispatcher()
{
    demuxer = std::move(Demultiplexer::Make());
}

Dispatcher::~Dispatcher()
{
}

bool Dispatcher::AddHandler(const EventHandlerPtr& handler, const uint32_t mask)
{
    auto handle = handler->GetHandle();
    auto handlerIter = eventHandlers.find(handle);
    if (handlerIter == eventHandlers.end()) {
        if (demuxer->add(handle, EventType(mask))) {
            eventHandlers.insert(make_pair(handle, handler));
            eventHandlerMasks.insert(make_pair(handle, mask)); 
            return true;
        }
        return false;
    }
    return false;
}

bool Dispatcher::RemoveHandler(const EventHandlerPtr& handler, const uint32_t mask)
{
    auto handle = handler->GetHandle();
    auto handlerIter = eventHandlers.find(handle);
    if (handlerIter == eventHandlers.end()) {
        return false;
    }

    demuxer->remove(handle, EventType(mask));
    auto it = eventHandlerMasks.find(handle);
    if (it == eventHandlerMasks.end()) {
      return false;
    }

    it->second &= ~mask;
    if (it->second == 0) {
      eventHandlerMasks.erase(it);
    }
    return true;
}

bool Dispatcher::ModifyHandler(const EventHandlerPtr& handler, const uint32_t mask)
{
    auto handle = handler->GetHandle();
    auto handlerIter = eventHandlerMasks.find(handle);
    if (handlerIter == eventHandlerMasks.end()) {
        return false;
    }

    auto currentMask = handlerIter->second;
    if (!demuxer->modify(handle, mask)) {
        return false;
    }

    handlerIter->second = mask;

#if defined(_WIN32) || defined(_WIN64)
    if (mask & EventType::Read) {
        handler->HandleEvent(EventType::Read);
    }
#endif

    return true;
}

uint32_t Dispatcher::HandleEvents(const uint32_t timeoutMs)
{
    set<socket_type> toRemove;

#if defined(_WIN32) || defined(_WIN64)
    for (auto& handler : eventHandlers) {
        auto sockId = handler.first;
        auto mask = eventHandlerMasks[sockId];
        if (!handler.second->HandleEvent(mask)) {
            toRemove.insert(sockId);
        }
    }
#endif

    auto events = demuxer->WaitForEvents();

    cout << "Got " << events.size() << " events" << endl;

    for (auto& evt : events) {
        socket_type socketHandle;
        uint32_t eventMask;
        tie(eventMask, socketHandle) = evt;

	cout << "fd " <<socketHandle << " mask " <<eventMask <<endl;
        if (toRemove.find(socketHandle) != toRemove.end()) {
            continue;
        }

        auto handlerIter = eventHandlers.find(socketHandle);
        if (handlerIter == eventHandlers.end()) {
            continue;
        }

        EventHandlerPtr handler = handlerIter->second;

        if (handler == nullptr) {
            continue;
        }

        if (!handler->HandleEvent(eventMask)) {
            toRemove.insert(socketHandle);
        }
    }

    for (auto& socketHandle : toRemove) {
        demuxer->remove(socketHandle, 0);

        auto it = eventHandlers.find(socketHandle);
        eventHandlers.erase(it);

        auto maskIt = eventHandlerMasks.find(socketHandle);
        eventHandlerMasks.erase(maskIt);
    }

    return events.size();
}
