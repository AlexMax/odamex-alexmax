# Common
set(COMMON_DIR ../common)
file(GLOB COMMON_HEADERS ${COMMON_DIR}/*.h)
file(GLOB COMMON_SOURCES ${COMMON_DIR}/*.cpp)

# Server
set(SERVER_DIR src)
file(GLOB SERVER_HEADERS ${SERVER_DIR}/*.h)
file(GLOB SERVER_SOURCES ${SERVER_DIR}/*.cpp)
if(WIN32)
  set(SERVER_WIN32_DIR win32)
  file(GLOB SERVER_WIN32_HEADERS ${SERVER_WIN32_DIR}/*.h)
  set(SERVER_WIN32_RESOURCES ${SERVER_WIN32_DIR}/server.rc)
endif()

# Jim
# TODO: This deserves its own file
set(JIM_DIR ../jim)
file(GLOB JIM_HEADERS ${JIM_DIR}/*.h)
file(GLOB JIM_SOURCES ${JIM_DIR}/*.c)

include(CheckFunctionExists)
check_function_exists("backtrace", HAVE_BACKTRACE)
check_function_exists("_NSGetEnviron", HAVE__NSGETENVIRON)

include(CheckIncludeFiles)
check_include_files("sys/time.h", HAVE_SYS_TIME_H)
check_include_files("crt_externs.h", HAVE_CRT_EXTERNS_H)

include(CheckTypeSize)
check_type_size("long long" LONG_LONG)

set(JIMCFG_DIR ${CMAKE_CURRENT_BINARY_DIR}/jim)
configure_file(
  ${JIM_DIR}/jim-config.h.in
  ${JIMCFG_DIR}/jim-config.h)
configure_file(
  ${JIM_DIR}/jimautoconf.h.in  
  ${JIMCFG_DIR}/jimautoconf.h)

# JsonCpp
set(JSONCPP_DIR ../jsoncpp)
file(GLOB JSONCPP_HEADERS ${JSONCPP_DIR}/json/*.h)
set(JSONCPP_SOURCE ${JSONCPP_DIR}/jsoncpp.cpp)

# Platform definitions
define_platform()

# Server definitions
add_definitions(-DNOASM -DJSON_IS_AMALGAMATION)
if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()
include_directories(${JIM_DIR} ${JIMCFG_DIR} ${JSONCPP_DIR} ${COMMON_DIR} ${SERVER_DIR} ${SERVER_WIN32_DIR})

# Server target
add_executable(odasrv
  ${JIM_SOURCES} ${JIM_HEADERS}
  ${JSONCPP_SOURCE} ${JSONCPP_HEADERS}
  ${COMMON_SOURCES} ${COMMON_HEADERS}
  ${SERVER_SOURCES} ${SERVER_HEADERS}
  ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})
if(WIN32)
  target_link_libraries(odasrv winmm wsock32)
elseif(SOLARIS)
  target_link_libraries(odasrv socket nsl)
endif()
