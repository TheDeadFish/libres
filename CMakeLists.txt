project(libes)
cmake_minimum_required(VERSION 3.12)

set( EXECUTABLE_OUTPUT_PATH ../bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)
include_directories(src)

add_compile_options(-Wno-address-of-packed-member)
add_link_options(-s -Wl,--gc-sections)

add_library(res src/object.cc src/arFile.cc src/arMerge.cc)
link_libraries(-lstdshit res)
add_executable(libres src/app/libres.cc)
add_executable(libmerge src/app/libmerge.cc)
add_executable(dfobjdmp src/app/dfobjdmp.cc)

# libview
link_libraries(-lwin32hlp)
add_link_options(-mwindows)
add_executable(libview 
	src/app/libview/libview.cc 
	src/app/libview/resource.rc
	src/app/libview/tempfile.cc
)
