# - Try to find PortMidi
# Once done, this will define
#
#  PortMidi_FOUND - system has PortMidi
#  PortMidi_INCLUDE_DIRS - the PortMidi include directories
#  PortMidi_LIBRARIES - link these to use PortMidi
#  PortMidi_VERSION - detected version of PortMidi
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

find_path(PORTMIDI_INCLUDE_DIR portmidi.h
  HINTS
  $ENV{PORTMIDI_DIR}
)

find_library(PORTMIDI_LIBRARY portmidi
  HINTS
  $ENV{PORTMIDI_DIR}
)

find_library(PORTRIME_LIBRARY porttime
  HINTS
  $ENV{PORTMIDI_DIR}
)

# Porttime library is merged to Portmidi in new versions, so
# we work around problems by adding it only if it's present
if(${PORTRIME_LIBRARY} MATCHES "PORTRIME_LIBRARY-NOTFOUND")
  set(PORTMIDI_LIBRARIES ${PORTMIDI_LIBRARY})
else()
  set(PORTMIDI_LIBRARIES ${PORTMIDI_LIBRARY} ${PORTRIME_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortMidi
  REQUIRED_VARS PORTMIDI_INCLUDE_DIR PORTMIDI_LIBRARIES)

mark_as_advanced(PORTMIDI_LIBRARY PORTRIME_LIBRARY)