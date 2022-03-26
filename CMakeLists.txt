# CMakeList.txt : CMake project for nsdos, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("nsdos")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
endif()
