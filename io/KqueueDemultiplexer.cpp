#include <io/KqueueDemultiplexer.hpp>
#include <io/Event.hpp>
#include <vector>
#include <iostream>
#include <cstring>

using namespace io::net;
using namespace std;

KqueueDemultiplexer::KqueueDemultiplexer()
{
    kqueuefd = kqueue();
}

KqueueDemultiplexer::~KqueueDemultiplexer()
{
    close(kqueuefd);
}

vector<Demultiplexer::EventHandlePair> KqueueDemultiplexer::WaitForEvents()
{
    vector<Demultiplexer::EventHandlePair> result;
    vector<struct kevent> events(512);

    int cnt = kevent(kqueuefd, nullptr, 0, events.data(), events.size(), nullptr);
    if (cnt == -1) {
        throw runtime_error("kqueue demultiplexing operation failed");
    }

    for (int k = 0; k < cnt; ++k) {
        uint32_t eventMask = 0;

        if (events[k].filter == EVFILT_READ) {
            eventMask |= EventType::Read;
        }
        if (events[k].filter == EVFILT_WRITE) {
            eventMask |= EventType::Write;
        }
        if (events[k].fflags & EV_EOF) {
            eventMask |= EventType::Close;
        }
        if (events[k].fflags & EV_ERROR) {
            eventMask |= EventType::Error;
        }

        result.emplace_back(eventMask, events[k].ident);
    }

    return result;
}

bool KqueueDemultiplexer::add(const socket_type handle, const uint32_t mask)
{
    vector<struct kevent> event(2);
    size_t idx = 0;

    using namespace std;
    
    if (mask & (EventType::Read | EventType::Accept)) {
        event[idx].filter = EVFILT_READ;
	event[idx].flags = EV_ADD | EV_ENABLE;
	event[idx].ident = handle;
	++idx;
    }
    if (mask & EventType::Write ) {
        event[idx].filter = EVFILT_WRITE;
	event[idx].flags = EV_ADD | EV_ENABLE;
	event[idx].ident = handle;
	++idx;
    }

    int err = kevent(kqueuefd, event.data(), idx, nullptr, 0, nullptr);
    if (err == -1) {
        throw runtime_error("Failed to register a file descriptor with kqueue");
    }
    return true;
}

bool KqueueDemultiplexer::remove(const socket_type handle, const uint32_t mask)
{
    vector<struct kevent> event(2);
    size_t idx = 0;
    event[idx].ident = handle;

    if (mask & (EventType::Read | EventType::Accept)) {
        event[idx].filter = EVFILT_READ;
	event[idx].flags = EV_DELETE;
	++idx;
    }
    if (mask & EventType::Write) {
        event[idx].filter = EVFILT_WRITE;
	event[idx].flags = EV_DELETE;
	++idx;
    }

    int err = kevent(kqueuefd, event.data(), idx, nullptr, 0, nullptr);
    if (err == -1) {
        throw runtime_error("Failed to register a file descriptor with kqueue");
    }
    return true;
}

bool KqueueDemultiplexer::modify(const socket_type handle, const uint32_t mask)
{
  return add(handle, mask);
}
