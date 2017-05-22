#pragma once

#include <vector>
#include <memory>
#include <io/IOHandler.hpp>

namespace io
{
class ApplicationHandler : public net::IOHandler {
public:
    ApplicationHandler(
        const net::DispatcherPtr& dispatcher,
        const net::SocketBasePtr& ptr)
        : IOHandler(dispatcher, ptr) {}
    
    virtual std::unique_ptr<std::vector<uint8_t>> HandleEvent(
	    const uint32_t eventMask,
	    const std::vector<uint8_t>& input) = 0;
};

typedef std::shared_ptr<ApplicationHandler> ApplicationHandlerPtr;
}
