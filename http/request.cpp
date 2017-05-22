#include <request.hpp>
#include <containers/DisplacedArray.hpp>
#include <cctype>
#include <sstream>
#include <iostream>
#include <cassert>

using namespace http;
using namespace std;

request::request(const Method& m, unique_ptr<URI>& u)
    : method(m), uri(std::move(u))
{}

unique_ptr<request> request::make_request(const Method m, unique_ptr<URI>& uri)
{
    auto req = make_unique<request>(m, uri);
    return req;
}

unique_ptr<request> request::make_request(const uint8_t* input, const size_t len)
{
    auto req = make_unique<request>();
    auto curr = '\0';

    enum State {
        start, method,
        uri_start, uri,
        version_start, version,
        header_start, header,
        field_start, field,
        msg_body
    };

    ostringstream method_buf, uri_buf, version_buf, hdr_buf, fld_buf;
    auto state = start;

    for (size_t k = 0; k < len; k++) {
        curr = input[k];
        switch (state) {
        case start:
            if (isspace(curr))
                continue;

            state = method;

        case method:
            switch (curr) {
            case '\r':
            case '\n':
                throw "malformed method in request";

            case '\t':
            case ' ': {
                auto tmp = method_buf.str();
                if (tmp == "OPTIONS")      req->method = Method::OPTIONS;
                else if (tmp == "GET")     req->method = Method::GET;
                else if (tmp == "HEAD")    req->method = Method::HEAD;
                else if (tmp == "POST")    req->method = Method::POST;
                else if (tmp == "PUT")     req->method = Method::PUT;
                else if (tmp == "DELETE")  req->method = Method::DELETE;
                else if (tmp == "TRACE")   req->method = Method::TRACE;
                else if (tmp == "CONNECT") req->method = Method::CONNECT;
                else req->method = Method::UNSUPPORTED;

                state = uri_start;
                break;
            }

            default:
                method_buf << curr;
                break;
            }
            break;

        case uri_start:
            if (isspace(curr)) {
                continue;
            }
            state = uri;

        case uri:
            switch (curr) {
            case '\r':
            case '\n':
                throw "malformed uri in request";

            case ' ':
            case '\t':
                req->uri = URI::make_uri(uri_buf.str());
                state = version_start;
                break;

            default:
                uri_buf << curr;
            }
            break;

        case version_start:
            if (isspace(curr)) {
                continue;
            }
            state = version;

        case version:
            switch (curr) {
            case ' ':
            case '\t':
                continue;

            case '\r':
                if (input[k + 1] == '\n') {
                    req->version = version_buf.str();
                    state = header_start;
                }
                else {
                    throw std::runtime_error("malformed version");
                }
                break;

            default:
                version_buf << curr;
            }
            break;

        case header_start:
            if (isspace(curr))
                continue;

            state = header;

        case header:
            switch (curr) {
            case ':':
                state = field_start;
                break;

            default:
                hdr_buf << curr;
            }

            break;

        case field_start:
            if (isspace(curr))
                continue;

            state = field;

        case field:
            switch (curr) {
            case '\r':
                continue;

            case '\n':
                req->headers.emplace(make_pair(hdr_buf.str(), fld_buf.str()));
                hdr_buf.str("");
                fld_buf.str("");

                state = header_start;
                break;

            default:
                fld_buf << curr;
            }
            break;

        default:
            assert(!"wtf? houston?");
            break;
        }
    }

    return req;
}
