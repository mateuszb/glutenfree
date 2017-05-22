#include <io/EpollDemultiplexer.hpp>
#include <io/Event.hpp>
#include <vector>
#include <iostream>
#include <cstring>

extern "C" {
#include <sys/epoll.h>
}

using namespace io::net;
using namespace std;

EpollDemultiplexer::EpollDemultiplexer()
{
    epollfd = epoll_create(42);
}

EpollDemultiplexer::~EpollDemultiplexer()
{
    close(epollfd);
}

vector<Demultiplexer::EventHandlePair> EpollDemultiplexer::WaitForEvents()
{
    vector<Demultiplexer::EventHandlePair> result;
    vector<epoll_event> events(512);

    int cnt = epoll_wait(epollfd, events.data(), events.size(), -1);
    if (cnt == -1) {
        throw runtime_error("Epoll demultiplexing operation failed");
    }

    for (int k = 0; k < cnt; ++k) {
        uint32_t eventMask = 0;

        if (events[k].events & EPOLLIN) {
            eventMask |= EventType::Read;
        }
        if (events[k].events & EPOLLOUT) {
            eventMask |= EventType::Write;
        }
        if (events[k].events & (EPOLLHUP | EPOLLRDHUP)) {
            eventMask |= EventType::Close;
        }
        if (events[k].events & EPOLLERR) {
            eventMask |= EventType::Error;
        }

        result.emplace_back(eventMask, events[k].data.fd);
    }

    return result;
}

bool EpollDemultiplexer::add(const socket_type handle, const uint32_t mask)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.data.fd = handle;

    if (mask & EventType::Read) {
        event.events |= EPOLLIN;
    }
    if (mask & EventType::Write) {
        event.events |= EPOLLOUT;
    }
    if (mask & EventType::Accept) {
        event.events |= EPOLLIN;
    }
    if (mask & EventType::Close) {
        event.events |= EPOLLHUP | EPOLLRDHUP;
    }

    int err = epoll_ctl(epollfd, EPOLL_CTL_ADD, handle, &event);
    if (err == -1) {
        throw runtime_error("Failed to register a file descriptor with epoll_ctl()");
    }
    return true;
}

bool EpollDemultiplexer::remove(const socket_type handle, const uint32_t mask)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = handle;

    int err = epoll_ctl(epollfd, EPOLL_CTL_DEL, handle, &event);
    if (err == -1) {
        perror(nullptr);
        throw runtime_error("Failed to unregister a file descriptor with epoll_ctl()");
    }
    return true;
}

bool EpollDemultiplexer::modify(const socket_type handle, const uint32_t mask)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    event.data.fd = handle;

    if (mask & EventType::Read) {
        event.events |= EPOLLIN;
    }
    if (mask & EventType::Write) {
        event.events |= EPOLLOUT;
    }
    if (mask & EventType::Accept) {
        event.events |= EPOLLIN;
    }
    if (mask & EventType::Close) {
        event.events |= EPOLLHUP | EPOLLRDHUP;
    }

    int err = epoll_ctl(epollfd, EPOLL_CTL_MOD, handle, &event);
    if (err == -1) {
        if (errno == ENOENT) {
            cerr << "Warning: attempting to modify a closed or non-existent descriptor " << handle << endl;
            return false;
        }
        throw runtime_error("Failed to modify a file descriptor with epoll_ctl()");
    }
    return true;
}
