#pragma once

#include <regex>
#include <string>
#include <initializer_list>
#include <memory>
#include <http/PageController.hpp>
#include <iostream>
#include <list>
#include <sstream>

namespace http
{
class SiteConfig {
public:
    SiteConfig(std::initializer_list<std::pair<std::string, std::shared_ptr<PageController>>> paths)
    {
        for (auto it = paths.begin(); it != paths.end(); ++it) {
            std::regex var_re(R"(:([^:]+):)");

            std::smatch match;

            auto input = it->first;
            std::ostringstream translated;
            std::list<std::string> vars;

            while (std::regex_search(input, match, var_re)) {
                translated << match.prefix();
                translated << R"(([a-zA-Z0-9]+))";
                auto varname = match.str();
                vars.emplace_back(varname.substr(1, varname.size() - 2));
                input = match.suffix();
            }
            translated << input;
            std::regex final_re(translated.str());

            m_routes[it->first] = make_tuple(final_re, vars, it->second);
        }
    }

    std::shared_ptr<PageController> find(const std::string& path, std::map<std::string, std::string>& vars)
    {
        for (auto it = m_routes.begin(); it != m_routes.end(); ++it) {
            auto re = std::get<0>(it->second);
            auto v = std::get<1>(it->second);
            auto controller = std::get<2>(it->second);
            std::smatch matches;

            auto result = std::regex_match(path, matches, re);
            if (result) {
                auto var_it = v.cbegin();
                for (auto k = 1; k < matches.size(); ++k, ++var_it) {
                    vars[*var_it] = matches[k].str();
                }

                return controller;
            }
        }

        return nullptr;
    }

private:
    std::map<std::string,
        std::tuple<std::regex,
        std::list<std::string>,
        std::shared_ptr<PageController>>> m_routes;
};

extern SiteConfig SITE_CONFIG;
}
