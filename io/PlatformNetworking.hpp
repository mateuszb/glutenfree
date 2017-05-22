#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <MSWSock.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
}
#endif
