#pragma once

#include <io/demuxer-defs.hpp>

namespace io {
  class handler_base;
  class demuxer_base {
  public:
#if defined _WIN32 || defined _WIN64
      using socket_type = SOCKET;
#else
      using socket_type = int;
#endif

    virtual bool modify(const operation op, const filter dir, socket_type fd, void *udata = nullptr) = 0;
    virtual bool modify(const operation op, const filter dir, const std::shared_ptr<handler_base>& h) = 0;
    virtual bool modify(const operation op, const filter filt, int desc, int interval, void *udata = nullptr) = 0;
    virtual bool modify(const operation op, const filter filt, int desc, int interval, const std::shared_ptr<handler_base>&h) = 0;
  };
}
