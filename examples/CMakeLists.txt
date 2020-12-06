cmake_minimum_required(VERSION 3.5)
project (xco)

add_subdirectory(basic basic_demo)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_subdirectory(iocp iocp_demo)
endif()
