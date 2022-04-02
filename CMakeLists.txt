# CMakeList.txt : CMake project for nsdos, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("nsdos")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(COMMAND git describe --dirty --always --tags
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "Unknown")
endif()

string(STRIP "${GIT_REV}" GIT_REV)
add_definitions(-DVERSION=\"${GIT_REV}\")

# Add source to this project's executable.
add_executable (nsdos "nsdos.c" "nsdos.h")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
if (MSVC)
    target_compile_options(nsdos PRIVATE /MT)
    target_link_options(nsdos PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
endif (MSVC)

# Install
if (NOT MSVC)
install(TARGETS nsdos DESTINATION /usr/local/bin)
else()
install(FILES LICENSE DESTINATION /)
install(FILES README.md DESTINATION /)
install(TARGETS nsdos DESTINATION /)
install(DIRECTORY disk_images DESTINATION /)

set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_VERSION ${GIT_REV})
SET(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
include(CPack)
endif()
