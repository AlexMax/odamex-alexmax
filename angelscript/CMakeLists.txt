# Compiles Angelscript as a static library for Odamex.
#
# Based on ./projects/cmake/CMakeLists.txt

set(ANGELSCRIPT_LIBRARY "angelscript")
set(ANGELSCRIPT_DIR .)
file(GLOB ANGELSCRIPT_HEADERS ${ANGELSCRIPT_DIR}/source/*.h)
file(GLOB ANGELSCRIPT_SOURCES ${ANGELSCRIPT_DIR}/source/*.cpp)

# Visual C++ x64 needs assembler
if(MSVC AND CMAKE_CL_64)
  enable_language(ASM_MASM)
  if(CMAKE_ASM_MASM_COMPILER_WORKS)
    set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} ${ANGELSCRIPT_DIR}/source/as_callfunc_x64_msvc_asm.asm)
  else()
    message(FATAL ERROR "MSVC x86_64 target requires a working assembler")
  endif()
endif()

include_directories(${ANGELSCRIPT_DIR}/include)

add_definitions("-D_CRT_SECURE_NO_WARNINGS -DANGELSCRIPT_EXPORT -D_LIB")

# Fix x64 issues on Linux
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT APPLE)
  add_definitions(-fPIC)
endif()

add_library(${ANGELSCRIPT_LIBRARY} STATIC ${ANGELSCRIPT_SOURCES} ${ANGELSCRIPT_HEADERS})
