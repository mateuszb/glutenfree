#pragma once

#include <io/Socket.hpp>
#include <io/EventHandler.hpp>
#include <io/NetworkTraits.hpp>
#include <io/Dispatcher.hpp>
#include <io/PlatformDemultiplexer.hpp>

namespace io
{

namespace net
{
class IOHandler
    : public EventHandler,
    public std::enable_shared_from_this<IOHandler>
{
public:
    IOHandler(
        const DispatcherPtr& dispatcher,
        const SocketBasePtr& ptr);
    ~IOHandler();
    socket_type GetHandle() const override;
    bool HandleEvent(const uint32_t eventMask) override;

protected:
    using DataPtr = std::unique_ptr<std::vector<std::uint8_t>>;
    using DataBuffer = std::pair<std::size_t, DataPtr>;

    virtual bool receive(const size_t sizeHint = 0);
    virtual bool transmit(DataPtr pkt);

private:
    SocketBasePtr socket;
    DispatcherPtr dispatcher;

    using ReceiveQueue = std::queue<DataBuffer>;
    using TransmitQueue = std::queue<DataBuffer>;

    TransmitQueue txq;
    ReceiveQueue rxq;

    bool transmitLoop();
#if defined(_WIN32) || defined(_WIN64)
    IOCPDemultiplexer::IOContext rxCtx;
    IOCPDemultiplexer::IOContext txCtx;
#endif
    DataPtr rxdata;
};

typedef std::shared_ptr<IOHandler> IOHandlerPtr;
}
}
