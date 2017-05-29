#include <string>
#include <cctype>
#include <vector>
#include <sstream>
#include <iostream>
#include <cassert>
#include <http/uri.hpp>

using namespace http;
using namespace std;

auto URI::read_pct(common::DisplacedArray<char>::iterator& iter)
{
    auto ret = '\0';

    for (int k = 0; k < 2; k++, ++iter) {
	if (isdigit(*iter)) {
	    ret |= *iter - '0';
	}
	else if (isalpha(*iter)) {
	    ret |= tolower(*iter) - 'a' + 10;
	}

	if (k == 0)
	    ret <<= 4;
    }

    return ret;
}

std::unique_ptr<URI> URI::make_uri(const std::string& s)
{
    common::DisplacedArray<char> disp((char *)s.c_str(), 0, s.length());
    return make_uri(disp);
}

std::unique_ptr<URI> URI::make_uri(common::DisplacedArray<char>& arr)
{
    auto tmp = '\0';
    auto ret = make_unique<URI>();
    enum State { start, scheme, slash_sep, host, port, path, params, done };
    auto state = start;
    ostringstream buf[7];

    for (auto it = arr.begin(); it != arr.end(); ++it) {
	auto curr = *it != '%' ? *it : read_pct(++it);

    retake:
	switch (state) {
	case start:
	    if (curr == '/') {
		state = path;
		goto retake;
	    }

	    state = scheme;
	    // fall through

	case scheme:
	    switch (curr) {
	    case ':':
		ret->components[SCHEME] = buf[state].str();
		state = slash_sep;
		break;

	    default:
		buf[state] << *it;
	    }
	    break;

	case slash_sep:
	    switch (curr) {
	    case '/':
		++it;
		if (*it == '/') {
		    state = host;
		} else {
		    throw runtime_error("missing slash separator");
		}
		break;
	    default:
		throw runtime_error("missing slash separator");
	    }
	    break;

	case host:
	    switch (curr) {
	    case ':':
		ret->components[HOST] = buf[state].str();
		state = port;
		break;

	    case '/':
		ret->components[HOST] = buf[state].str();
		state = path;
		break;

	    default:
		buf[state] << *it;
	    }
	    break;

	case port:
	    switch (curr) {
	    case '/':
		ret->components[PORT] = buf[state].str();
		state = path;
		goto retake;

	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		buf[state] << *it;
		break;

	    default:
		throw "invalid port";
	    }
	    break;

	case path:
	    switch (curr) {
	    case ' ':
		ret->components[PATH] = buf[state].str();
		state = done;
		break;

	    case '?':
		ret->components[PATH] = buf[state].str();
		state = params;
		break;

	    default:
		buf[state] << *it;
	    }
	    break;

	case params:
	    switch (curr) {
	    case '#':
		ret->components[PARAMS] = buf[state].str();
		break;

	    default:
		buf[state] << *it;
	    }
	    break;

	case done:
	    break;
	}
    }

    auto val = buf[state].str();
    switch (state) {
    case host:
	ret->components[HOST] = val;
	break;

    case port:
	ret->components[PORT] = val;
	break;

    case scheme:
         // done processing input and still stuck on scheme?
        // this is probably a path then...
    case path:
	ret->components[PATH] = val;
	break;

    case params:
	ret->components[PARAMS] = val;
	break;

    default:
	break;
    }

    return ret;
}

const string& URI::scheme() const noexcept
{
    return components[SCHEME];
}

const string& URI::host() const noexcept
{
    return components[HOST];
}

const string& URI::port() const noexcept
{
    return components[PORT];
}

const string& URI::path() const noexcept
{
    return components[PATH];
}

const string& URI::params() const noexcept
{
    return components[PARAMS];
}

std::ostream& http::operator<<(std::ostream& out, const URI& uri)
{
    out << "scheme=" << uri.scheme()
	<< ", host=" << uri.host()
	<< ", port=" << uri.port()
	<< ", path=" << uri.path()
	<< ", params=" << uri.params();
    return out;
}
