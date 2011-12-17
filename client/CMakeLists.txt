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

# Agar configuration
# Here be dragons.  I can almost guarantee you this will not work on your system.
set(AGAR_INCLUDE_DIR "C:/Users/Alex Mayfield/Documents/Workspace/lib/mingw/agar-1.4.1-mingw32/include/agar")
set(AGAR_LIBRARY "C:/Users/Alex Mayfield/Documents/Workspace/lib/mingw/agar-1.4.1-mingw32/lib/libag_core.a")
include_directories(${AGAR_INCLUDE_DIR})

# Client target
if(SDL_FOUND AND SDLMIXER_FOUND)
	add_executable(odamex WIN32 ${COMMON_SOURCES} ${COMMON_HEADERS} ${TEXTSCREEN_SOURCES} ${TEXTSCREEN_HEADERS} ${CLIENT_SOURCES} ${CLIENT_HEADERS} ${CLIENT_WIN32_RESOURCES})
	target_link_libraries(odamex ${AGAR_LIBRARY} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
	if(WIN32)
		target_link_libraries(odamex winmm wsock32)
	endif()
endif()
