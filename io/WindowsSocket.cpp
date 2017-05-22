#include <io/Socket.hpp>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <WinSock2.h>
#include <MSWSock.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

