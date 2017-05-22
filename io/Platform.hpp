#pragma once

#if defined _WIN32 || defined _WIN64
#include <WinSock2.h>
#include <MSWSock.h>
#include <Ws2tcpip.h>
namespace io
{
namespace net
{
using socket_type = SOCKET;
}
}
#else
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
}
namespace io
{
namespace net
{
using socket_type = int;
}
}
#endif
