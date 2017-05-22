#include <io/IOCPDemultiplexer.hpp>
#include <io/PlatformNetworking.hpp>
#include <io/Platform.hpp>
#include <string>
#include <cassert>
#include <iostream>
#include <vector>

using namespace io::net;
using namespace std;

IOCPDemultiplexer::IOCPDemultiplexer()
{
    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp == nullptr) {
        throw runtime_error("Failed to create IO completion port");
    }
}

IOCPDemultiplexer::~IOCPDemultiplexer()
{
    CloseHandle(iocp);
}

bool IOCPDemultiplexer::add(const socket_type handle, const uint32_t mask)
{
    HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(handle), iocp, handle, 0);
    return result != nullptr;
}

bool IOCPDemultiplexer::remove(const socket_type handle, const uint32_t mask)
{
    return true;
}

bool IOCPDemultiplexer::modify(const socket_type handle, const uint32_t mask)
{
    return true;
}

std::vector<Demultiplexer::EventHandlePair> IOCPDemultiplexer::WaitForEvents()
{
    using namespace std;
    vector<OVERLAPPED_ENTRY> overlapped(512);
    ULONG numEntries = 0;
    vector<Demultiplexer::EventHandlePair> result;

    auto ok = GetQueuedCompletionStatusEx(
        iocp,
        overlapped.data(),
        static_cast<ULONG>(overlapped.size()),
        &numEntries,
        INFINITE,
        false);

    if (!ok) {
        auto err = GetLastError();
        if (err != WAIT_TIMEOUT) {
            vector<char> errbuf(512);
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                err,
                0,
                errbuf.data(),
                static_cast<DWORD>(errbuf.size()),
                nullptr);
            cerr << errbuf.data() << endl;
            throw runtime_error(errbuf.data());
        }

        return {};
    }

    for (auto k = 0ul; k < numEntries; ++k) {
        DWORD bytesTransferred = 0, flags = 0;
        auto cplKey = overlapped[k].lpCompletionKey;
        socket_type socket = static_cast<socket_type>(cplKey);
        IOContext* ctx = reinterpret_cast<IOContext*>(overlapped[k].lpOverlapped);

        WSAGetOverlappedResult(socket, overlapped[k].lpOverlapped, &bytesTransferred, false, &flags);
        
        result.emplace_back(ctx->event, socket);
    }

    return result;
}

#if 0
bool IOCPDemultiplexer::modify(const operation op, const filter filt, socket_type desc, void *data)
{
    auto h = CreateIoCompletionPort((HANDLE)desc, iocp, reinterpret_cast<ULONG_PTR>(data), 0);
    return h == nullptr;
}

bool IOCPDemultiplexer::modify(const operation op, const filter filt, int desc, int interval, void *data)
{
    return false;
}

bool IOCPDemultiplexer::operator()()
{
    using namespace std;
    vector<OVERLAPPED_ENTRY> overlapped(512);
    ULONG numEntries = 0;
    auto ok = GetQueuedCompletionStatusEx(iocp, overlapped.data(), static_cast<ULONG>(overlapped.size()), &numEntries, INFINITE, false);
    if (!ok) {
        auto err = GetLastError();
        vector<char> errbuf(512);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, errbuf.data(), static_cast<DWORD>(errbuf.size()), nullptr);
        cerr << (char *)errbuf.data() << endl;
        return false;
    }

    unordered_set<shared_ptr<handler_base>> toRemove;

    for (auto k = 0ul; k < numEntries; ++k) {
        auto cplKey = overlapped[k].lpCompletionKey;
        auto h = reinterpret_cast<handler_base*>(cplKey);
        auto shared = h->ptr();

        auto ctx = reinterpret_cast<io::handler_base::Context*>(overlapped[k].lpOverlapped);
        DWORD bytesTransferred = 0, flags = 0;
        WSAGetOverlappedResult(h->id(), overlapped[k].lpOverlapped, &bytesTransferred, false, &flags);
        std::shared_ptr<io::handler_base> newh{};

        auto ret = (*h)(ctx->filter, bytesTransferred, &newh);

        if (ret && newh.get() != nullptr) {
            handler_map.insert(newh);

            if (!modify(operation::add, filter::rw, newh->id(), newh.get())) {
                assert(!"failure");
            }
        }
        else if (!ret) {
            toRemove.insert(shared);
        }
    }

    for (auto& handler : toRemove) {
        handler_map.erase(handler);
    }

    return numEntries >= 0;
}
#endif
