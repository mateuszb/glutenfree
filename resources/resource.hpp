#pragma once

#include <regex>
#include <string>
#include <map>

namespace resources
{
  struct resource_descriptor {
    int   size;
    void* vaddr;
  };
  
  extern std::map<std::string, resource_descriptor> resource_map;
  extern std::map<std::string, std::string> alias_map;

  std::string load(const std::string& name);
  std::string get_mime_type(const std::string& name);
  resource_descriptor*
  get_resource(const std::string& name);
  
  resource_descriptor*
  register_resource(const std::string& path, void* ptr);

  resource_descriptor*
  register_alias(const std::string& name, const std::string& alias);
}
