#pragma once

#include <memory>
#include <io/handler.hpp>
#include <io/socket.hpp>
#include <io/demuxer-defs.hpp>
#include <io/demuxer-base.hpp>
#include <unordered_set>

namespace io
{
  namespace net {
    class handler_base;
  }
  
  class demuxer : public demuxer_base, public std::enable_shared_from_this<demuxer> {
  public:
    explicit demuxer();
    ~demuxer();

    bool modify(const operation op, const filter dir, int fd, void * udata = nullptr) override;
    
    bool modify(const operation op, const filter filt, int desc, int interval,
		void * udata = nullptr) override;
    
    bool modify(const operation op, const filter filt, int desc, int interval,
		const std::shared_ptr<handler_base>& h) override {
      return modify(op, filt, desc, interval, h.get());
    }
    
    bool modify(const operation op, const filter dir,
		const std::shared_ptr<handler_base>& h) override
    {
      return modify(op, dir, h->id(), h.get());
    }

    bool operator()();

    void add_handler(std::shared_ptr<handler_base> h)
    {
      handler_map.insert(h);
    }

  private:
    inline std::shared_ptr<demuxer> ptr()
    {
      return this->shared_from_this();
    }

  private:
    int fd;
    std::unordered_set<std::shared_ptr<handler_base>> handler_map;
  };
}
