project(libes)
cmake_minimum_required(VERSION 3.12)

set( EXECUTABLE_OUTPUT_PATH ../bin)
add_link_options(-s -Wl,--gc-sections)

add_executable(libres object.cc libres.cc)
target_link_libraries(libres -lstdshit)
