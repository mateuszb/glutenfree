#pragma once

#include <map>
#include <regex>
#include <string>
#include <initializer_list>
#include <http/request.hpp>
#include <resources/resource.hpp>
#include <sstream>
#include <util/json.hpp>
#include <http/HttpHandler.hpp>

namespace http
{
using namespace std;

class PageController
{
public:
  virtual pair<HttpCode, std::string> process(
    const http::request& req,
    std::multimap<std::string, std::string>& headers,
    std::multimap<std::string, std::string>&& params,
    std::map<std::string, std::string>& vars,
    const std::string payload = "") = 0;

  struct MethodNotImplemented : public std::runtime_error {
      MethodNotImplemented(const char* msg)
          : runtime_error(msg) {}
  };

};

inline const std::string getParam(std::multimap<std::string, std::string>& dict, const std::string& key)
{
    auto range = dict.equal_range(key);
    ostringstream value;

    for (auto it = range.first; it != range.second; ++it) {
        value << it->second;

        auto itcopy = it;
        if (++itcopy != range.second) {
            value << ";";
        }
    }

    return value.str();
}

}
