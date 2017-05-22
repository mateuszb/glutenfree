#include <http/site_config.hpp>
#include <web/web.hpp>
#include <web/StaticPageController.hpp>

using namespace std;
using namespace http;
using namespace OptimizerSite;

SiteConfig http::SITE_CONFIG
{
    { "/", make_shared<StaticPageController>("/blog.html") },
};

void init()
{
}
