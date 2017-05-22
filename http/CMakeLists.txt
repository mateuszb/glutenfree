add_library(http
  OBJECT
  request.cpp
  request.hpp
  uri.cpp
  uri.hpp
  HttpHandler.cpp
  HttpHandler.hpp
  urlencode.cpp
  urlencode.hpp
  PageController.hpp)

add_dependencies(http resources)
target_include_directories(
  http
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ../../
  ../)
