#include <io/IOHandler.hpp>
#include <io/ApplicationHandler.hpp>
#include <io/Platform.hpp>
#include <io/PlatformNetworking.hpp>

using namespace std;
using namespace io::net;

IOHandler::IOHandler(
    const DispatcherPtr& disp,
    const SocketBasePtr& ptr)
    : socket(ptr), dispatcher(disp)
{
#if defined(_WIN32) || defined(_WIN64)
    ::memset(&rxCtx, 0, sizeof(rxCtx));
    ::memset(&txCtx, 0, sizeof(txCtx));
    rxCtx.event = EventType::Read;
    txCtx.event = EventType::Write;
#endif
}

IOHandler::~IOHandler()
{
#if defined(_WIN32) || defined(_WIN64)
    WSACloseEvent(rxCtx.overlapped.hEvent);
    WSACloseEvent(txCtx.overlapped.hEvent);
#endif
}

socket_type IOHandler::GetHandle() const
{
    return socket->GetHandle();
}

bool IOHandler::HandleEvent(const uint32_t eventMask)
{
    bool ok = true;
    auto appHandler = static_pointer_cast<ApplicationHandler>(shared_from_this());

    if (eventMask & EventType::Read) {
        ok &= receive();
    }

    if (eventMask & EventType::Write) {
        ok &= transmitLoop();
    }

    if (eventMask & EventType::Close) {
        ok &= false;
    }

    if (eventMask & EventType::Error) {
        ok &= false;
    }

#if defined(_WIN32) || defined(_WIN64)
    transmitLoop();
#endif

    return ok;
}

bool IOHandler::receive(const size_t sizeHint)
{
    size_t bufsz = sizeHint == 0 ? 1024 : sizeHint;

#if defined(_WIN32) || defined(_WIN64)
    DWORD ret = ~0;
    if (rxCtx.overlapped.hEvent != nullptr) {
        ret = WaitForSingleObject(rxCtx.overlapped.hEvent, 0);
    }

    if (ret == WAIT_OBJECT_0) {
        DWORD nread = 0;
        DWORD flags = 0;

        WSAGetOverlappedResult(socket->GetHandle(), &rxCtx.overlapped, &nread, false, &flags);
        if (nread == 0) {
            return false;
        }

        WSAResetEvent(rxCtx.overlapped.hEvent);
        rxdata->resize(nread);

        auto handler = dynamic_cast<ApplicationHandler*>(this);
        auto response = handler->HandleEvent(EventType::Read, *rxdata);
        if (response != nullptr) {
            transmit(std::move(response));
        }
    }

    if (rxCtx.overlapped.hEvent != nullptr) {
        if (ret == WAIT_TIMEOUT) {
            return true;
        }

        if (ret == WAIT_FAILED) {
            throw runtime_error("Waiting for read to complete failed");
        }
    }
    else {
        ::memset(&rxCtx.overlapped, 0, sizeof(rxCtx.overlapped));
        rxCtx.overlapped.hEvent = WSACreateEvent();
    }

    int err = 0;

    rxdata = make_unique<vector<uint8_t>>(bufsz, 0);
    DWORD nread = 0, flags = 0;
    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<CHAR*>(rxdata->data());
    wsaBuf.len = static_cast<ULONG>(rxdata->size());

    WSAResetEvent(rxCtx.overlapped.hEvent);

    err = WSARecv(
        socket->GetHandle(),
        &wsaBuf,
        1,
        nullptr,
        &flags,
        &rxCtx.overlapped,
        nullptr);

    if (err == 0) {
        BOOL ret = WSAGetOverlappedResult(socket->GetHandle(), &rxCtx.overlapped, &nread, false, &flags);
        if (!ret) {
            int wsaErr = WSAGetLastError();
            if (wsaErr == WSA_IO_INCOMPLETE) {
                while (true);
            }
            else {
                while (true);
            }
        }

        if (nread == 0) {
            cerr << "CONNECTION CLOSED" << endl;
            return false;
            //throw runtime_error("connection closed");
        }
        rxdata->resize(nread);

        auto handler = dynamic_cast<ApplicationHandler*>(this);
        auto response = handler->HandleEvent(EventType::Read, *rxdata);
        if (response != nullptr) {
            transmit(std::move(response));
        }
        WSACloseEvent(rxCtx.overlapped.hEvent);
        ::memset(&rxCtx.overlapped, 0, sizeof(rxCtx.overlapped));
    }
    else {
        if (err == INVALID_SOCKET) {
            int error = WSAGetLastError();

            if (error == WSAECONNABORTED || error == WSAECONNRESET) {
                cerr << "Connection aborted/reset" << endl;
                return false;
            }

            if (error != WSA_IO_PENDING) {
                throw runtime_error("Unknown error returned when attempting to read");
            }

            return true;
        }
    }

    return err == 0;
#else
retry:
    rxdata = make_unique<vector<uint8_t>>(bufsz, 0);
    auto nread = ::read(
        socket->GetHandle(),
        &(*rxdata)[0],
        bufsz);

    if (nread == -1) {
        if (errno == EINTR) {
            cout << "RX EINTR" << endl;
            goto retry;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            cout << "RX EAGAIN || EWOULDBLOCK" << endl;
            return true;
        }
    }

    if (nread == 0) {
        return false;
    }

    rxdata->resize(nread);

    auto appHandler = dynamic_cast<ApplicationHandler*>(this);
    auto response = appHandler->HandleEvent(EventType::Read, *rxdata);
    if (response != nullptr) {
        transmit(std::move(response));
    }

    return true;
#endif
}

bool IOHandler::transmit(DataPtr pkt)
{
    txq.emplace(0, std::move(pkt));
#if defined(_WIN32) || defined(_WIN64)
    transmitLoop();
    if (txq.empty()) {
        return true;
    } else {
        return false;
    }
#else
    auto sharedThis = shared_from_this();
    if (!dispatcher->ModifyHandler(
        sharedThis, EventType::Read | EventType::Write)) {
        cerr << "Error modifying handler " << GetHandle() << " when sending" << endl;
    }
#endif
    return true;
}

bool IOHandler::transmitLoop()
{
    IOHandlerPtr sharedThis = shared_from_this();

    if (txq.empty()) {
        //cout << "TX queue empty." << endl;
#if defined(_WIN32) || defined(_WIN64)
        return true;
#else
	dispatcher->RemoveHandler(sharedThis, EventType::Write);
        return dispatcher->ModifyHandler(sharedThis, EventType::Read);
#endif
    }

#if defined(_WIN32) || defined(_WIN64)
    if (txCtx.overlapped.hEvent == nullptr) {
        txCtx.overlapped.hEvent = WSACreateEvent();
    }

    while (!txq.empty()) {
        auto& curr = txq.front();
        const auto sofar = curr.first;

        DWORD nsent = 0;
        DWORD flags = 0;
        WSABUF buffer;
        buffer.buf = reinterpret_cast<CHAR*>(curr.second->data());
        buffer.len = static_cast<ULONG>(curr.second->size() - sofar);

        WSAResetEvent(txCtx.overlapped.hEvent);
        txCtx.event = EventType::Write;

        int ret = WSASend(socket->GetHandle(),
            &buffer,
            1,
            &nsent,
            flags,
            &txCtx.overlapped,
            nullptr);

        if (ret == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSA_IO_PENDING) {
                return true;
            }

            return false;
        }

        curr.first += nsent;
        if (curr.first == curr.second->size()) {
            txq.pop();
        }
    }

    return true;
#else
    while (!txq.empty()) {
        auto& curr = txq.front();
        const auto sofar = curr.first;
        auto nwritten = ::write(
            socket->GetHandle(),
            &(*curr.second)[sofar],
            curr.second->size() - sofar);

        if (nwritten == -1 && errno == EINTR) {
            cerr << "EINTR" << endl;
            continue;
        }

        if (nwritten == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                cerr << "errno is EAGAIN || EWOULDBLOCK " << errno << endl;
                return true;
            }

            cerr << "other error " << errno << endl;
            return false;
        }

        curr.first += nwritten;
        if (curr.first == curr.second->size()) {
            txq.pop();
        }
    }

    return true;
#endif
}
