#pragma once
#include <http/PageController.hpp>
#include <string>

namespace OptimizerSite {
using namespace std;
using namespace http;

class StaticPageController : public http::PageController
{
public:
    StaticPageController(const string& resourceName);
    ~StaticPageController();

    // Inherited via PageController
    virtual pair<HttpCode, string> process(
        const http::request& req,
        multimap<string, string>& headers,
        multimap<string, string>&& params,
        map<string, string>& vars,
        const string payload = "") override;
private:
    string resource;
};
}
