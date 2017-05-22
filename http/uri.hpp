#pragma once

#include <memory>
#include <string>
#include <cstddef>
#include <tuple>
#include <containers/DisplacedArray.hpp>

namespace http {
using std::string;
  
class URI {
public:
    static std::unique_ptr<URI> make_uri(common::DisplacedArray<char>& arr);
    static std::unique_ptr<URI> make_uri(const std::string& s);

    const std::string& scheme() const noexcept;
    const std::string& host() const noexcept;
    const std::string& port() const noexcept;
    const std::string& path() const noexcept;
    const std::string& params() const noexcept;

private:

    enum component { SCHEME, HOST, PORT, PATH, PARAMS };
    string components[5];
    
private:
    static auto read_pct(common::DisplacedArray<char>::iterator& iter);
    friend std::ostream& operator<<(std::ostream& out, const URI& o);
};

std::ostream& operator<<(std::ostream& out, const URI& uri);
}
