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

# JsonCpp
set(JSONCPP_DIR ../jsoncpp)
file(GLOB JSONCPP_HEADERS ${JSONCPP_DIR}/json/*.h)
set(JSONCPP_SOURCE ${JSONCPP_DIR}/jsoncpp.cpp)

# Lua
set(LUA_DIR $ENV{LUA_DIR} CACHE PATH
  "Path to Lua base directory.  Only necessary if you get errors about Lua not being found, or if you want to use a different installation of Lua.")
set(ENV{LUA_DIR} ${LUA_DIR})
find_package(Lua 5.2)
if(LUA_FOUND)
  include_directories(${LUA_INCLUDE_DIR})
else()
  message(WARNING, "Lua not found, server target not generated.")
endif()

# Platform definitions
define_platform()

# Server definitions
add_definitions(-DNOASM -DJSON_IS_AMALGAMATION)
if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()
include_directories(${JSONCPP_DIR} ${COMMON_DIR} ${SERVER_DIR} ${SERVER_WIN32_DIR})

# Server target
if(LUA_FOUND)
  add_executable(odasrv
    ${JSONCPP_SOURCE} ${JSONCPP_HEADERS}
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${SERVER_SOURCES} ${SERVER_HEADERS}
    ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})
  target_link_libraries(odasrv ${LUA_LIBRARIES})
  include(CheckIncludeFiles)
  check_include_files(dlfcn.h HAVE_DL_H)
  if(HAVE_DL_H)
    target_link_libraries(odasrv dl)
  endif()
  if(WIN32)
    target_link_libraries(odasrv winmm wsock32)
  elseif(SOLARIS)
    target_link_libraries(odasrv socket nsl)
  endif()
endif()
