cmake_minimum_required(VERSION 3.5)
project(xco CXX C)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  file(GLOB_RECURSE SRC_FILES_ASM *.S)
  set_property(SOURCE ${SRC_FILES_ASM} PROPERTY LANGUAGE C)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
  file(GLOB_RECURSE SRC_FILES_ASM *.asm)
endif()

file(GLOB_RECURSE SRC_FILES_CPP *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

message("src file is ${SRC_FILES_ASM}")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES 
  ${SRC_FILES_CPP}
  ${HEADER_FILES}
  ${SRC_FILES_ASM})

set(CMAKE_CXX_FLAGS -std=c++11)

add_library(libxco STATIC
    ${SRC_FILES_CPP}
    ${HEADER_FILES}
    ${SRC_FILES_ASM})