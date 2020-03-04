project(libes)
cmake_minimum_required(VERSION 3.12)

set( EXECUTABLE_OUTPUT_PATH ../bin)
add_compile_options(-Wno-address-of-packed-member)
add_link_options(-s -Wl,--gc-sections)

add_executable(libres object.cc arFile.cc libres.cc)
target_link_libraries(libres -lstdshit)

