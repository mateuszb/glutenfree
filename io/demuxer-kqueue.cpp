#include <io/demuxer.hpp>
#include <string>
#include <cassert>
#include <iostream>
#include <io/handler.hpp>

extern "C" {
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string.h>
}

using namespace io;
using std::string;

demuxer::demuxer()
{
    fd = kqueue();
    if (fd == -1) {
        throw string(::strerror(errno));
    }
}

demuxer::~demuxer()
{
    ::close(fd);
}

bool demuxer::modify(const operation op, const filter filt, int desc, void *data)
{
    auto idx = 0;

    struct kevent kevt[2]{};
    auto action = EV_ADD;
    struct timespec zero {};

    switch (op) {
    case operation::add:
        action = EV_ADD | EV_ENABLE;
        break;

    case operation::del:
        action = EV_DELETE | EV_DISABLE;
        break;

    case operation::enable:
        action = EV_ENABLE;
        break;

    case operation::disable:
        action = EV_DISABLE;
        break;

    default:
        break;
    }

    if (filt == filter::timer) {
        EV_SET(&kevt[idx], desc, EVFILT_TIMER, action, 0, desc, 0);
        ++idx;
    }

    if (filt == filter::read || filt == filter::rw) {
        EV_SET(&kevt[idx], desc, EVFILT_READ, action, 0, 0, data);
        ++idx;
    }

    if (filt == filter::write || filt == filter::rw) {
        EV_SET(&kevt[idx], desc, EVFILT_WRITE, action, 0, 0, data);
        ++idx;
    }

    auto err = kevent(fd, kevt, idx, nullptr, 0, &zero);
    if (err == -1) {
        auto s = strerror(errno);
        perror(nullptr);
    }

    return err == 0;
}

bool demuxer::modify(const operation op, const filter filt, int desc, int interval, void *data)
{
    auto idx = 0;

    struct kevent kevt[2]{};
    auto action = EV_ADD;
    struct timespec zero {};

    switch (op) {
    case operation::add:
        action = EV_ADD | EV_ENABLE;
        break;

    case operation::del:
        action = EV_DELETE | EV_DISABLE;
        break;

    case operation::enable:
        action = EV_ENABLE;
        break;

    case operation::disable:
        action = EV_DISABLE;
        break;

    default:
        break;
    }

    if (filt == filter::timer) {
        EV_SET(&kevt[idx], desc, EVFILT_TIMER, action, 0, interval, data);
        ++idx;
    }

    auto err = kevent(fd, kevt, idx, nullptr, 0, &zero);
    if (err == -1) {
        auto s = strerror(errno);
        perror(nullptr);
    }

    return err == 0;
}


bool demuxer::operator()()
{
    struct kevent evts[512];
    struct timespec timeout { 0 };

    auto n = kevent(fd, nullptr, 0, evts, sizeof(evts) / sizeof(evts[0]), &timeout);
    if (n > 0) {
        for (auto k = 0; k < n; ++k) {
            if (evts[k].filter == EVFILT_READ || evts[k].filter == EVFILT_WRITE) {
                int style = 0;
                int accepting = 0;

                auto data_ptr = reinterpret_cast<void *>(evts[k].udata);
                auto flt = evts[k].filter;
                struct sockaddr_storage storage;

                socklen_t len = sizeof(style);
                getsockopt(evts[k].ident, SOL_SOCKET, SO_TYPE, &style, &len);

                len = sizeof(accepting);
                getsockopt(evts[k].ident, SOL_SOCKET, SO_ACCEPTCONN, &accepting, &len);

                len = sizeof(storage);
                getsockname(evts[k].ident, reinterpret_cast<struct sockaddr *>(&storage), &len);

                auto listening = (accepting == 1);
                auto tcp = (style == SOCK_STREAM);
                auto inet6 = (storage.ss_family == PF_INET6);

                // TODO: different handlers, e.g. acceptor handler, connector, conn in progress handler, etc.
                // they work on IOVs and pass those to parsers...
                if (flt == EVFILT_READ) {
                    if (data_ptr != nullptr) {
                        io::handler_base * h = reinterpret_cast<io::handler_base *>(data_ptr);
                        auto shared = h->ptr();

                        std::shared_ptr<io::handler_base> newh{};
                        auto ret = (*h)(filter::read, evts[k].data, &newh);

                        if (ret && newh.get() != nullptr) {
                            handler_map.insert(newh);

                            if (!modify(operation::add, filter::read, newh->id(), newh.get())) {
                                assert(!"failure");
                            }
                        }
                        else if (!ret) {
                            handler_map.erase(shared);
                        }
                    }
                }

                if (flt == EVFILT_WRITE) {
                    if (data_ptr != nullptr) {
                        auto h = reinterpret_cast<io::handler_base *>(data_ptr);
                        auto ret = (*h)(filter::write, evts[k].data);
                    }
                }
            }
            else if (evts[k].filter == EVFILT_TIMER) {
                auto h = reinterpret_cast<io::handler_base *>(evts[k].udata);
                auto shared = h->ptr();
                if (shared != nullptr) {
                    (*shared)(filter::timer, evts[k].data, nullptr);
                }
            }
        }
    }
    return n >= 0;

    return false;
}
