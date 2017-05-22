#pragma once

namespace io
{
namespace net
{
enum EventType : unsigned int {
    Read = 0x0001,
    Write = 0x0002,
    Accept = 0x0004,
    Close = 0x0008,
    Signal = 0x0010,
    Timeout = 0x0020,
    Error = 0x0040
};
}
}
