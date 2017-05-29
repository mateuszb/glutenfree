project(blog CXX)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
IF(${WIN32})
ELSE()
set(CMAKE_CXX_FLAGS "-gdwarf-2 -ggdb")
ENDIF()

set(DEPENDENCY_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

IF(${WIN32})
set(EXTRA_LIBS Ws2_32 MSWsock)
set(ZLIB_NAME zlib)
set(DEPENDENCY_DIR ${DEPENDENCY_ROOT_DIR}/win10/)
link_directories(
${DEPENDENCY_DIR}/zlib/lib64
${DEPENDENCY_DIR}/icu/lib64
)

include_directories(
${DEPENDENCY_DIR}/icu/include
${DEPENDENCY_DIR}/zlib/include
${CMAKE_CURRENT_SOURCE_DIR}
)

ELSE()
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(DEPENDENCY_DIR ${DEPENDENCY_ROOT_DIR}/linux/)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DEPENDENCY_DIR ${DEPENDENCY_ROOT_DIR}/osx/)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(DEPENDENCY_DIR ${DEPENDENCY_ROOT_DIR}/freebsd/)
ENDIF()

set(EXTRA_LIBS)
set(ZLIB_NAME z)

link_directories(
  /opt/local/lib
  /usr/local/lib
)

include_directories(
/opt/local/include/
/usr/local/include/
${CMAKE_CURRENT_SOURCE_DIR}
)
ENDIF()

add_subdirectory(util)
add_subdirectory(io)
add_subdirectory(http)
add_subdirectory(resources)
add_subdirectory(web)

add_executable(blog main.cpp
$<TARGET_OBJECTS:io>
$<TARGET_OBJECTS:http>
$<TARGET_OBJECTS:util>
)

IF(WIN32)

add_custom_command(TARGET blog POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/zlib/lib64/zlib.dll $<TARGET_FILE_DIR:blog>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/icu/lib64/icudt58.dll $<TARGET_FILE_DIR:blog>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/icu/lib64/icuin58.dll $<TARGET_FILE_DIR:blog>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/icu/lib64/icuio58.dll $<TARGET_FILE_DIR:blog>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/icu/lib64/icutu58.dll $<TARGET_FILE_DIR:blog>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/win10/icu/lib64/icuuc58.dll $<TARGET_FILE_DIR:blog>
)
ENDIF()

target_link_libraries(blog 
resources
${ZLIB_NAME}
icuuc
icuio
site
${EXTRA_LIBS}
)

target_include_directories(blog PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
