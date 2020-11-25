cmake_minimum_required(VERSION 3.5)
project(xco)

file(GLOB_RECURSE SRC_FILES_CPP *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES 
  ${SRC_FILES_CPP}
  ${HEADER_FILES})

add_executable(iocp  
    ${SRC_FILES_CPP}
    ${HEADER_FILES})
    
target_link_libraries(iocp libxco)