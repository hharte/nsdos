# CMakeList.txt : CMake project for nsdos, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("nsdos")

# Add source to this project's executable.
add_executable (nsdos "nsdos.c" )

set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
if (MSVC)
    target_compile_options(nsdos PRIVATE /MT)
    target_link_options(nsdos PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
endif (MSVC)

# TODO: Add tests and install targets if needed.
