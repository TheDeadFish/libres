project(libes)
cmake_minimum_required(VERSION 3.12)

set( EXECUTABLE_OUTPUT_PATH ../bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)

add_compile_options(-Wno-address-of-packed-member)
add_link_options(-s -Wl,--gc-sections)

add_library(res object.cc arFile.cc)
link_libraries(-lstdshit res)
add_executable(libres libres.cc)
add_executable(libmerge libmerge.cc)
