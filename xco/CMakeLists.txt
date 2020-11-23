cmake_minimum_required(VERSION 3.5)
project(xco CXX C ASM_MASM)

file(GLOB_RECURSE SRC_FILES_CPP *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    file(GLOB_RECURSE SRC_FILES_ASM *.asm)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES 
      ${SRC_FILES_CPP}
      ${HEADER_FILES}
      ${SRC_FILES_ASM})
else()
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source" FILES 
      ${SRC_FILES_CPP}
      ${HEADER_FILES})
endif()


add_library(libxco STATIC
    ${SRC_FILES_CPP}
    ${HEADER_FILES}
    ${SRC_FILES_ASM})