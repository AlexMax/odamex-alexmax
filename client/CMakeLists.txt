# Common
set(COMMON_DIR ../common)
file(GLOB COMMON_HEADERS ${COMMON_DIR}/*.h)
file(GLOB COMMON_SOURCES ${COMMON_DIR}/*.cpp)

# Textscreen
set(TEXTSCREEN_DIR ../textscreen)
file(GLOB TEXTSCREEN_HEADERS ${TEXTSCREEN_DIR}/*.h)
file(GLOB TEXTSCREEN_SOURCES ${TEXTSCREEN_DIR}/*.cpp)

# Client (FIXME: Can the CLIENT_DIR be used for globbing?)
set(CLIENT_DIR sdl src)
file(GLOB CLIENT_HEADERS sdl/*.h src/*.h)
file(GLOB CLIENT_SOURCES sdl/*.cpp src/*.cpp)
if(WIN32)
	set(CLIENT_WIN32_RESOURCES sdl/client.rc)
endif()

# Client definitions
add_definitions(-DNOASM)
include_directories(${COMMON_DIR} ${TEXTSCREEN_DIR} ${CLIENT_DIR})

# SDL configuration
set(SDLDIR $ENV{SDLDIR} CACHE PATH "Path to SDL (and optionally SDL_mixer) base directory.  Only necessary if you get errors about SDL not being found, or if you want to use a different installation of SDL.")
set(ENV{SDLDIR} ${SDLDIR})
find_package(SDL)
if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
else()
	message(WARNING "SDL not found, client target not generated.")
endif()

# SDL_Mixer configuration
set(SDLMIXERDIR $ENV{SDLMIXERDIR} CACHE PATH "Path to SDL_mixer base directory.  Only necessary if you get errors about SDL not being found, or if you want to use a different installation of SDL_mixer.  If SDL_mixer is in the location specified by SDLMAIN then you can leave this blank.")
set(ENV{SDLMIXERDIR} ${SDLMIXERDIR})
find_package(SDL_mixer)
if(SDLMIXER_FOUND)
	include_directories(${SDLMIXER_INCLUDE_DIR})
else()
	message(WARNING "SDL_mixer not found, client target not generated.")
endif()

# Freetype configuration
set(FREETYPE_DIR $ENV{FREETYPE_DIR} CACHE PATH "Path to Freetype base directory.")
set(ENV{FREETYPE_DIR} ${FREETYPE_DIR})
if(WIN32)
	# FindFreetype makes bad assumptions on where to find things if you just
	# compile the library and point FREETYPE_DIR at the basedir on Windows.
	# TODO: Only tested with MinGW, what does MSVC do?  Or an installer?
	find_path(FREETYPE_INCLUDE_DIR_ft2build ft2build.h HINTS ${FREETYPE_DIR}/include)
	find_path(FREETYPE_INCLUDE_DIR_freetype2 freetype/config/ftheader.h HINTS ${FREETYPE_DIR}/include)
	find_library(FREETYPE_LIBRARY NAMES freetype HINTS ${FREETYPE_DIR}/objs)
endif()
find_package(Freetype)
if(FREETYPE_FOUND)
	include_directories(${FREETYPE_INCLUDE_DIRS})
else()
	message(WARNING "Freetype not found, client target not generated.")
endif()

# Agar configuration
# Here be dragons.  I can almost guarantee you this will not work on your system.
if(WIN32)
	set(AGAR_DIR $ENV{AGAR_DIR} CACHE PATH "Path to AGAR base directory.")
	set(ENV{AGAR_DIR} ${AGAR_DIR})
	set(AGAR_INCLUDE_DIR "${AGAR_DIR}/include/agar")
	set(AGAR_LIBRARY "${AGAR_DIR}/lib/libag_gui.a;${AGAR_DIR}/lib/libag_core.a")
	include_directories(${AGAR_INCLUDE_DIR})
else()
	set(AGAR_INCLUDE_DIR "$ENV{HOME}/local/include/agar")
	set(AGAR_LIBRARY "$ENV{HOME}/local/lib/libag_gui.a;$ENV{HOME}/local/lib/libag_core.a")
	include_directories(${AGAR_INCLUDE_DIR})
endif()

# Client target
if(SDL_FOUND AND SDLMIXER_FOUND AND FREETYPE_FOUND)
	add_executable(odamex WIN32 ${COMMON_SOURCES} ${COMMON_HEADERS} ${TEXTSCREEN_SOURCES} ${TEXTSCREEN_HEADERS} ${CLIENT_SOURCES} ${CLIENT_HEADERS} ${CLIENT_WIN32_RESOURCES})
	target_link_libraries(odamex ${AGAR_LIBRARY} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${FREETYPE_LIBRARIES})
	if(WIN32)
		target_link_libraries(odamex winmm wsock32)
	endif()
endif()
