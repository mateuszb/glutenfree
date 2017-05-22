add_library(site
  web.cpp
  web.hpp
  StaticPageController.hpp
  StaticPageController.cpp
)

target_include_directories(
  site
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ../../
  ../
  /opt/local/include)
