IF(WIN32)
set(DEMUXER_SRCS IOCPDemultiplexer.cpp IOCPDemultiplexer.hpp)
set(REACTOR_SRCS)
set(PROACTOR_SRCS Proactor.cpp Proactor.hpp)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(PROACTOR_SRCS)
set(REACTOR Reactor.cpp Reactor.hpp)
set(DEMUXER_SRCS KqueueDemultiplexer.cpp KqueueDemultiplexer.hpp)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
set(DEMUXER_SRCS KqueueDemultiplexer.cpp KqueueDemultiplexer.hpp)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(DEMUXER_SRCS EpollDemultiplexer.cpp EpollDemultiplexer.hpp)
ELSE()
ENDIF()
add_library(io
  OBJECT
  AcceptHandler.cpp
  AcceptHandler.hpp
  ApplicationHandler.hpp
  Socket.hpp
  Demultiplexer.cpp
  Dispatcher.cpp
  Dispatcher.hpp
  Demultiplexer.hpp
  AcceptHandler.cpp
  IOHandler.cpp
  IOHandler.hpp
  Platform.hpp
  Event.hpp
  Connector.hpp
  PlatformNetworking.hpp
  PlatformDemultiplexer.hpp
  ${PROACTOR_SRCS}
  ${REACTOR_SRCS}
  ${DEMUXER_SRCS})

target_include_directories(io
PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
../
../../)
