#include <web/StaticPageController.hpp>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include <unicode/chariter.h>
#include <unicode/ustream.h>

using namespace OptimizerSite;
using namespace std;
using namespace http;

StaticPageController::StaticPageController(const string & resourceName)
    : resource(resourceName)
{
}

StaticPageController::~StaticPageController()
{
}

pair<HttpCode, string> StaticPageController::process(
    const request& req,
    multimap<string, string>& headers,
    multimap<string, string>&& params,
    map<string, string>& vars,
    const string payload)
{
    auto rsrc = resources::load(resource);
    if (rsrc.empty()) {
      cout << "resource " << resource << " doesn't exist" << endl;

        return { HttpCode::HTTP_NO_CONTENT, "" };
    }

    auto unistr = icu::UnicodeString(rsrc.c_str());
    ostringstream response;
    response << unistr;
    return { HttpCode::HTTP_OK, response.str() };
}
