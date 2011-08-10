project(Odamex)
cmake_minimum_required(VERSION 2.8)

# Default build type
if(NOT MSVC)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel.")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
endif()

# Build type definitions
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()

# Platform definitions
if(UNIX)
	add_definitions(-DUNIX)
elseif(WIN32)
	add_definitions(-DWIN32)
endif()

# Subdirectories for individual projects
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(master)
#add_subdirectory(odalaunch)