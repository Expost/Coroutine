cmake_minimum_required(VERSION 3.5)
project (xco)

add_subdirectory(basic basic_demo)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_subdirectory(iocp iocp_demo)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(epoll epoll_demo)
    add_subdirectory(proxy simple_proxy)
endif()
