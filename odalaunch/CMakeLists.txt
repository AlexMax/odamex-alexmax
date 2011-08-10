# API
set(API_DIR ../odalpapi)
file(GLOB API_HEADERS ${API_DIR}/*.h)
file(GLOB API_SOURCES ${API_DIR}/*.cpp)

# Odalaunch
set(LAUNCH_DIR src)
file(GLOB LAUNCH_HEADERS ${LAUNCH_DIR}/*.h)
file(GLOB LAUNCH_SOURCES ${LAUNCH_DIR}/*.cpp)

# Odalaunch definitions
include_directories(${API_DIR} ${LAUNCH_DIR})

# wxWidgets configuration
find_package(wxWidgets)
if(wxWidgets_FOUND)
	include_directories(${wxWidgets_INCLUDE_DIRS})
else()
	if(wxWidgets_ROOT_DIR)
		message(WARNING "wxWidgets root directory exists but no compiled libraries were found, odalaunch target not generated.")
	else()
		message(WARNING "wxWidgets not found, odalaunch target not generated.")
	endif()
endif()

# Odalaunch target
if(wxWidgets_FOUND)
	add_executable(odalaunch ${API_SOURCES} ${API_HEADERS} ${LAUNCH_SOURCES} ${LAUNCH_HEADERS})
	target_link_libraries(odalaunch ${wxWidgets_LIBRARY_DIRS})
endif()