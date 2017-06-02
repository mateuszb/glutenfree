#pragma once

#include <http/request.hpp>
#include <io/ApplicationHandler.hpp>
#include <io/Dispatcher.hpp>
#include <io/Socket.hpp>
#include <map>
#include <memory>
#include <unordered_set>

namespace http
{

using namespace std;

const string make_header(
    const uint16_t code,
    const string& reason,
    const multimap<string, string>& headers,
    const size_t content_size);

unique_ptr<vector<uint8_t>> gzip(const string& input);

enum class HttpCode : uint16_t {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_PERMANENT_REDIRECT = 308,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_PAYMENT_REQUIRED = 402,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
};

class HttpHandler : public io::ApplicationHandler {
public:
    struct HostConfig {
        unordered_set<string> static_dirs;
        unordered_set<string> dynamic_dirs;
    };

    HttpHandler(
        const std::string& hostname,
        const io::net::DispatcherPtr& dispatcher,
        const io::net::SocketBasePtr& ptr);

    virtual unique_ptr<vector<uint8_t>> HandleEvent(
        const uint32_t eventMask,
        const vector<uint8_t>& input) override;

    void register_url(const string& url);

    void register_host(
        const string& host,
        const HostConfig& cfg);

private:
    const size_t MAX_HDR_SIZE = 1024;
    const size_t MAX_BODY_SIZE = 8192;
    size_t idx;
    size_t crlf_count;
    unique_ptr<vector<uint8_t>> data;
    unique_ptr<vector<uint8_t>> request_data;
    unique_ptr<http::request> request;
    bool full_hdr;
    bool full_msg;

    map<string, HostConfig> hostmap;
};

class HttpHandlerFactory {
public:
    static io::ApplicationHandlerPtr Make(
        const std::string& hostname,
        io::net::SocketBasePtr socket);
};

class HttpsRedirectorHandler : public io::ApplicationHandler {
public:
    HttpsRedirectorHandler(
        const std::string& hostname,
        const io::net::DispatcherPtr& dispatcher,
        const io::net::SocketBasePtr& ptr)
        : ApplicationHandler(dispatcher, ptr), targetHost(hostname) {
    }

    unique_ptr<vector<uint8_t>> HandleEvent(
        const uint32_t eventMask,
        const vector<uint8_t>& input) override {

        ostringstream hostStream;
        hostStream << "https://" << targetHost;
        multimap<string, string> defhdrs = {
            { "Host", targetHost },
            { "Connection", "close" },
            { "Location", hostStream.str() }
        };

        const auto hdr = make_header(
            uint32_t(HttpCode::HTTP_MOVED_PERMANENTLY),
            "Moved Permanently",
            defhdrs,
            0);
        return make_unique<vector<uint8_t>>(hdr.cbegin(), hdr.cend());
    }
private:
    const string targetHost;
};

class HttpsRedirectorHandlerFactory {
public:
    static io::ApplicationHandlerPtr Make(
        const std::string& hostname,
        io::net::SocketBasePtr socket);
};
}
