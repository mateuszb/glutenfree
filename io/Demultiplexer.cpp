#include <io/Demultiplexer.hpp>
#include <io/PlatformDemultiplexer.hpp>

using namespace std;
using namespace io::net;

DemultiplexerPtr Demultiplexer::Make()
{
#if defined(_WIN32) || defined(_WIN64)
    return make_unique<IOCPDemultiplexer>();
#elif defined(__linux__)
    return make_unique<EpollDemultiplexer>();
#elif defined APPLE
#else
    return make_unique<KqueueDemultiplexer>();
#endif
}
