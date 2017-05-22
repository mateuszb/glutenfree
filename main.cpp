#include <http/HttpHandler.hpp>
#include <io/Socket.hpp>
#include <io/ApplicationHandler.hpp>
#include <io/Demultiplexer.hpp>
#include <io/AcceptHandler.hpp>
#include <io/Dispatcher.hpp>

#if defined _WIN32 || defined _WIN64
#pragma comment(lib, "Ws2_32.lib")
#else
extern "C" {
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
}
#endif
#include <iostream>

int main(int argc, char *argv[])
{
    using namespace std;
    using namespace io;
    using namespace io::net;

#if defined _WIN32 || defined _WIN64
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

#if !defined(_WIN32) && !defined(_WIN64)
    signal(SIGPIPE, SIG_IGN);
#endif

    const string hostname{ "invalidopcode.org" };
    AcceptHandler<http::HttpHandlerFactory> http(hostname, 80, io::net::tcp(), io::net::ipv4());
    //AcceptHandler<http::HttpHandlerFactory> httpv6(hostname, 80, io::net::tcp(), io::net::ipv6());
   
    EventHandlerPtr ptr(&http);
    //EventHandlerPtr ptrv6(&httpv6);

    auto dispatcher = Dispatcher::Instance();
    dispatcher->AddHandler(ptr, EventType::Accept);
    //dispatcher->AddHandler(ptrv6, EventType::Accept);

    for (;;) {
	    auto nevents = dispatcher->HandleEvents(-1);
    }

    return 0;
}
