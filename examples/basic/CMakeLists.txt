cmake_minimum_required(VERSION 3.5)
project(xco)

file(GLOB_RECURSE SRC_FILES_CPP *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES 
  ${SRC_FILES_CPP}
  ${HEADER_FILES})

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
endif()

set(CMAKE_CXX_FLAGS "-std=c++11 -g")

add_executable(basic_demo  
    ${SRC_FILES_CPP}
    ${HEADER_FILES})
    
target_link_libraries(basic_demo libxco)