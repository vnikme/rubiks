cmake_minimum_required(VERSION 2.8)

set(TARGET_FILE_NAME "rubiks_server")
project(${TARGET_FILE_NAME})
#set( CMAKE_VERBOSE_MAKEFILE on )

INCLUDE(CMakeLists.clean.txt)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(Boost 1.54.0 COMPONENTS system program_options locale REQUIRED)
find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS ON)

find_library(SSL_LIB libssl.a REQUIRED)
find_library(CRYPTO_LIB libcrypto.a REQUIRED)

add_subdirectory(
    ../lib rubiks_lib
)

SET(USED_LIBS
    ${Boost_LIBRARIES}
    ${SSL_LIB}
    ${CRYPTO_LIB}
    ${CMAKE_DL_LIBS}
    pthread
    rubiks_lib
)

if(NOT Boost_FOUND)
    message(SEND_ERROR "Boost not found! :(")
    return()
endif()

include_directories(
    ${Boost_INCLUDE_DIRS}
    ../lib
)

link_directories(
    ${Boost_LIBRARY_DIRS}
)

set(HDRS
    rubiks/rubiks.h

    network/echo.h
    network/http_request.h
    network/server.h
    network/session.h
    network/session_http.h
    network/session_https.h
    network/worker_pool.h

    util/base64.h
    util/datetime.h
    util/json.h
    util/md5.h
    util/random_util.h
    util/synchronizable.h
    util/url.h
    util/utf.h
)

set(SRCS
    rubiks/rubiks.cpp

    dist/jsoncpp.cpp

    main.cpp

    network/echo.cpp
    network/http_request.cpp
    network/server.cpp
    network/session.cpp
    network/session_http.cpp
    network/session_https.cpp
    network/worker_pool.cpp

    util/base64.cpp
    util/datetime.cpp
    util/json.cpp
    util/md5.cpp
    util/random_util.cpp
    util/url.cpp
    util/utf.cpp
)

add_executable(${TARGET_FILE_NAME} WIN32 ${HDRS} ${SRCS})

set_target_properties(${TARGET_FILE_NAME} PROPERTIES DEBUG_POSTFIX _d)

target_link_libraries(
    ${TARGET_FILE_NAME}
    ${USED_LIBS}
)

### debug/release config
if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

install(TARGETS ${TARGET_FILE_NAME}
    RUNTIME DESTINATION bin
    CONFIGURATIONS All
)
