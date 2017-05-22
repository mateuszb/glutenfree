#include <string>
#include <resource.hpp>
#include <map>
#include <tuple>

using namespace std;
using namespace resources;

extern "C" {
#include <externs.hpp>
}

map<string, resources::resource_descriptor> resources::resource_map {
#include <rsrc.hpp>
};
