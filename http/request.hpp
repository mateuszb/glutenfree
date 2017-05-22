#pragma once

#include <memory>
#include <string>
#include <map>
#include <cstddef>
#include <array>
#include <http/uri.hpp>

namespace http {
using std::string;
using std::multimap;
using std::size_t;

using HttpHeaders = std::multimap<string, string>;

class request {
public:
    enum class Method { OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT, UNSUPPORTED };

    static std::unique_ptr<request> make_request(const Method m, std::unique_ptr<URI>& uri);
    static std::unique_ptr<request> make_request(const uint8_t* input, const size_t len);

    using group_key = std::array<string, 1>;

    request() = default;
    request(const Method& m, std::unique_ptr<URI>& u);

    auto operator[](const string& key) noexcept
    {
        static const string empty;
        if (headers.count(key) > 0) {
            return headers.find(key)->second;
        }
        else {
            return empty;
        }
    }

    const auto operator[](const string& key) const noexcept
    {
        static const string empty;
        if (headers.count(key) > 0) {
            return headers.find(key)->second;
        }
        else {
            return empty;
        }
    }

    auto operator[](const group_key& key) noexcept
    {
        return headers.equal_range(key[0]);
    }

    const auto operator[](const group_key& key) const noexcept
    {
        return headers.equal_range(key[0]);
    }

    const URI& url() const
    {
        return *uri;
    }

    const Method get_method() const noexcept { return method; }

private:
    Method method;
    std::unique_ptr<URI> uri;
    string version;
    multimap<string, string> headers;
};
}
