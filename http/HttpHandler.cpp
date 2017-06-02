#include <http/HttpHandler.hpp>
#include <http/PageController.hpp>
#include <resources/resource.hpp>
#include <http/site_config.hpp>
#include <http/urlencode.hpp>
#include <algorithm>
#include <regex>

extern "C" {
#include <sys/stat.h>
#include <zlib.h>
}

using namespace http;
using namespace std;

const string _500_error = R"(<html>
<head>
<title>Internal server error</title>
</head>
</html>)";

const string _404_error = R"(<html>
<head>
<title>Page not found</title>
</head>
</html>)";

HttpHandler::HttpHandler(
    const std::string& hostname,
    const io::net::DispatcherPtr& dispatcher,
    const io::net::SocketBasePtr& ptr)
    : ApplicationHandler(dispatcher, ptr),
    idx(0), crlf_count(0),
    data(make_unique<vector<uint8_t>>()),
    request_data(make_unique<vector<uint8_t>>()),
    full_hdr(false),
    request(nullptr)
{
    HostConfig cfg;

    cfg.static_dirs.emplace("static");
    cfg.dynamic_dirs.emplace("dynamic");

    register_host(hostname, cfg);

    // register localhost variants
    register_host("localhost", cfg);
    register_host("127.0.0.1", cfg);
    register_host("::1", cfg);
}

unique_ptr<vector<uint8_t>>
HttpHandler::HandleEvent(
    const uint32_t eventMask,
    const vector<uint8_t>& input)
{
    bool gzip_supported = false;
    copy(input.begin(), input.end(), back_inserter(*data));

    if (!full_hdr) {
        for (; idx < data->size(); ++idx) {
            auto c = (*data)[idx];
            switch (c) {
            case '\r':
                switch (crlf_count) {
                case 0:
                    ++crlf_count;
                    continue;

                case 2:
                    ++crlf_count;
                    continue;

                default:
                    crlf_count = 0;
                    continue;
                }
                break;

            case '\n':
                switch (crlf_count) {
                case 1:
                    ++crlf_count;
                    continue;

                case 3:
                    ++crlf_count;
                    break;

                default:
                    crlf_count = 0;
                    continue;
                }
                break;

            default:
                crlf_count = 0;
                continue;
            }

            if (crlf_count == 4) {
                request = http::request::make_request(&(*data)[0], idx + 1);

                auto whole = data->begin() + idx + 1 == data->end();
                if (whole) {
                    data->clear();
                }
                else {
                    data->erase(data->begin(), data->begin() + idx + 1);
                    data->shrink_to_fit();
                }

                full_hdr = true;
                idx = 0;
                crlf_count = 0;

                auto thisHandle = IOHandler::GetHandle();
                struct sockaddr_storage saddr;
                socklen_t namlen = sizeof(saddr);

                int tmp = getpeername(IOHandler::GetHandle(),
                    reinterpret_cast<struct sockaddr*>(&saddr),
                    &namlen);
                vector<char> addrData;
                void * src = nullptr;

                if (saddr.ss_family == AF_INET) {
                    src = &reinterpret_cast<sockaddr_in*>(&saddr)->sin_addr;
                    addrData.resize(INET_ADDRSTRLEN);
                }
                else if (saddr.ss_family == AF_INET6) {
                    src = &reinterpret_cast<sockaddr_in6*>(&saddr)->sin6_addr;
                    addrData.resize(INET6_ADDRSTRLEN);
                }

                inet_ntop(saddr.ss_family, src, addrData.data(), addrData.size());

                cout << "Originator: " << addrData.data() << endl;
                break;
            }
        }

        if (!full_hdr) {
            if (data->size() >= MAX_HDR_SIZE) {
                throw runtime_error("Header size exceeded");
            }
            return nullptr;
        }
    }

    // if request header has Transfer-Encoding or Content-Length headers
    // then we need to read message body. Otherwise, we have a complete message

    auto& req = *request;

    if (req["Content-Length"].size() > 0 ||
        req["Transfer-Encoding"].size() > 0) {

        size_t len = 0;
        if (req["Content-Length"].size() > 0) {
            len = stoi(req["Content-Length"]);
        }
        else {
            len = stoi(req["Transfer-Encoding"]);
        }

        if (len > MAX_BODY_SIZE) {
            throw runtime_error("Body size too large");
        }

        copy(data->cbegin(), data->cend(), back_inserter(*request_data));
        data->erase(data->begin(), data->end());
        data->shrink_to_fit();

        if (request_data->size() < len) {
            return nullptr;
        }
        full_msg = true;
    }
    else {
        full_msg = true;
    }

    full_hdr = full_msg = false;
    idx = crlf_count = 0;

    const auto host = req.url().host();
    const auto hdr_host = [](const string&& s)
    {
        auto it = s.find(":");
        if (it < s.size()) {
            return s.substr(0, it);
        }
        else {
            return s;
        }
    }(req["Host"]);

    multimap<string, string> defhdrs =
    { {"Host", host.size() > 0 ? host : req["Host"]} };

    if (req["Connection"].size() > 0) {
        defhdrs.emplace("Connection", req["Connection"]);
    }
    else {
        defhdrs.emplace("Connection", "keep-alive");
    }

    if (req["Accept-Encoding"].find("gzip") != string::npos) {
        gzip_supported = true;
    }

    if (hostmap.find(host) == hostmap.end() &&
        hostmap.find(hdr_host) == hostmap.end()) {

        string hdr = make_header(404, "Not Found", defhdrs, _404_error.size());
        auto final_str = hdr + _404_error;

        return make_unique<vector<uint8_t>>(final_str.begin(), final_str.end());
    }
    else {
        const auto path = req.url().path();

        clog << "Request for path " << path << endl;
        string payload;

        // step 1. analyze path through routing and dispatch accordingly
        map<string, string> vars;
        auto ctl = SITE_CONFIG.find(path, vars);
        if (ctl != nullptr) {
            defhdrs.emplace("Content-Type", "text/html");

            auto param_str = req.url().params();
            if ((request_data->size() > 0 && req.get_method() == request::Method::POST)) {
                const string contentType = req["Content-Type"];
                const string formParametersType = "application/x-www-form-urlencoded";

                if (contentType == formParametersType) {
                    param_str = string(request_data->cbegin(), request_data->cend());
                }
                else {
                    payload = string(request_data->cbegin(), request_data->cend());
                }

                request_data->clear();
            }

            multimap<string, string> params;

            if (param_str.size() > 0) {
                // transform param string into a multimap
                enum class token_state { name, value } state = token_state::name;

                auto it = param_str.cbegin();
                auto c = it;
                string param[2];

                for (; c != param_str.cend(); ++c) {
                    switch (*c) {
                    case '&':
                        if (state == token_state::name) {
                            param[0] = urldecode(param_str.substr(it - param_str.cbegin(), c - it));
                            //cout << "param name: " << param[0] << endl;
                            params.emplace(param[0], "");
                        }
                        else {
                            param[1] = urldecode(param_str.substr(it - param_str.cbegin(), c - it));
                            //cout << "param value: " << param[1] << endl;
                            params.emplace(param[0], param[1]);
                        }
                        param[0] = "";
                        param[1] = "";
                        it = c + 1;
                        state = token_state::name;
                        break;

                    case '=':
                        param[0] = urldecode(param_str.substr(it - param_str.cbegin(), c - it));
                        //cout << "param name: " << param[0] << endl;
                        it = c + 1;
                        state = token_state::value;
                        break;

                    default:
                        break;
                    }
                }

                if (state == token_state::name) {
                    param[0] = urldecode(param_str.substr(it - param_str.cbegin(), c - it));
                    params.emplace(param[0], "");
                }
                else {
                    param[1] = urldecode(param_str.substr(it - param_str.cbegin(), c - it));
                    params.emplace(param[0], param[1]);
                }
            }

            try {
                HttpCode responseCode;
                string responseData;
                tie(responseCode, responseData) = ctl->process(req, defhdrs, move(params), vars, payload);
                string response;
                if (gzip_supported && !responseData.empty()) {
                    auto compressed = gzip(responseData);
                    defhdrs.emplace("Content-Encoding", "gzip");
                    response = string(compressed->cbegin(), compressed->cend());
                }
                else {
                    response = responseData;
                }

                if (responseCode == HttpCode::HTTP_FOUND ||
                    responseCode == HttpCode::HTTP_MOVED_PERMANENTLY ||
                    responseCode == HttpCode::HTTP_TEMPORARY_REDIRECT) {
                    defhdrs.emplace("Location", responseData);
                }

                // XXX
                const auto hdr = make_header(uint32_t(responseCode), "OK", defhdrs, response.size());
                const auto final = hdr + response;
                return make_unique<vector<uint8_t>>(final.cbegin(), final.cend());
            }
            catch (exception&) {
                const string hdr = make_header(500, "Internal error", defhdrs, _500_error.size());
                const auto response = hdr + _500_error;
                return make_unique<vector<uint8_t>>(response.cbegin(), response.cend());
            }
        }
        else {
            auto res = resources::get_resource(path);
            if (res != nullptr) {
                string response;
                auto ptr = reinterpret_cast<uint8_t*>(res->vaddr);
                auto raw = string(ptr, ptr + res->size);

                if (gzip_supported) {
                    auto compressed = gzip(raw);
                    defhdrs.emplace("Content-Encoding", "gzip");
                    response = string(compressed->cbegin(), compressed->cend());
                }
                else {
                    response = raw;
                }

                auto mimeType = resources::get_mime_type(path);
                if (!mimeType.empty()) {
                    defhdrs.emplace("Content-Type", mimeType);
                }
                const string hdr = make_header(200, "OK", defhdrs, response.size());
                auto final_str = hdr + response;
                return make_unique<vector<uint8_t>>(final_str.begin(), final_str.end());
            }

            string hdr = make_header(404, "Not Found", defhdrs, _404_error.size());
            auto final_str = hdr + _404_error;
            return make_unique<vector<uint8_t>>(final_str.begin(), final_str.end());
        }
    }

    return nullptr;
}

void HttpHandler::register_url(const string& url)
{
}

void HttpHandler::register_host(const string& host, const HostConfig& cfg)
{
    hostmap[host] = cfg;
}

const string http::make_header(
    const uint16_t code,
    const string& reason,
    const multimap<string, string>& headers,
    const size_t content_size)
{
    ostringstream oss;

    oss << "HTTP/1.1 " << code << " " << reason << "\r\n"
        << "Content-Length: " << content_size << "\r\n";

    for (auto&& val : headers) {
        oss << val.first << ": " << val.second << "\r\n";
    }
    oss << "\r\n";

    return oss.str();
}

unique_ptr<vector<uint8_t>> http::gzip(const string& input)
{
    auto buffer = make_unique<vector<uint8_t>>();
    const size_t bufsz = 128 * 1024;
    vector<uint8_t> tmpbuf(bufsz);

    z_stream zstream;
    zstream.zalloc = 0;
    zstream.zfree = 0;
    zstream.next_in = (uint8_t*)(input.c_str());
    zstream.avail_in = static_cast<uint32_t>(input.size());
    zstream.next_out = &tmpbuf[0];
    zstream.avail_out = bufsz;

    deflateInit2(&zstream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    while (zstream.avail_in != 0) {
        auto res = deflate(&zstream, Z_NO_FLUSH);
        if (zstream.avail_out == 0) {
            buffer->insert(buffer->end(), &tmpbuf[0], &tmpbuf[0] + bufsz);
            zstream.next_out = &tmpbuf[0];
            zstream.avail_out = bufsz;
        }
    }

    auto res = Z_OK;
    while (res == Z_OK) {
        if (zstream.avail_out == 0) {
            buffer->insert(buffer->end(), &tmpbuf[0], &tmpbuf[0] + bufsz);
            zstream.next_out = &tmpbuf[0];
            zstream.avail_out = bufsz;
        }
        res = deflate(&zstream, Z_FINISH);
    }

    buffer->insert(buffer->end(), &tmpbuf[0], &tmpbuf[0] + bufsz - zstream.avail_out);
    deflateEnd(&zstream);

    return buffer;
}

io::ApplicationHandlerPtr HttpHandlerFactory::Make(
    const std::string& hostname,
    io::net::SocketBasePtr socket)
{
    using namespace io;
    using namespace io::net;
    ApplicationHandlerPtr handler;

    auto dispatcher = Dispatcher::Instance();
    handler = make_shared<HttpHandler>(hostname, dispatcher, socket);
    dispatcher->AddHandler(handler, EventType::Read | EventType::Write);
    return handler;
}

io::ApplicationHandlerPtr HttpsRedirectorHandlerFactory::Make(
    const std::string& hostname,
    io::net::SocketBasePtr socket)
{
    using namespace io;
    using namespace io::net;
    ApplicationHandlerPtr handler;

    auto dispatcher = Dispatcher::Instance();
    handler = make_shared<HttpsRedirectorHandler>(hostname, dispatcher, socket);
    dispatcher->AddHandler(handler, EventType::Read | EventType::Write);
    return handler;
}
