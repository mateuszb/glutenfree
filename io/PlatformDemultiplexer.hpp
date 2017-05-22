#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <io/IOCPDemultiplexer.hpp>
#elif defined(__linux__)
#include <io/EpollDemultiplexer.hpp>
#elif defined APPLE
#include <io/KqueueDemultiplexer.hpp>
#else
#include <io/KqueueDemultiplexer.hpp>
#endif
