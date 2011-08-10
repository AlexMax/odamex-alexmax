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

# Server definitions
add_definitions(-DNOASM)
include_directories(${COMMON_DIR} ${SERVER_DIR} ${SERVER_WIN32_DIR})

# Server target
add_executable(odasrv ${COMMON_SOURCES} ${COMMON_HEADERS} ${SERVER_SOURCES} ${SERVER_HEADERS} ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})
if(WIN32)
	target_link_libraries(odasrv winmm wsock32)
endif()
