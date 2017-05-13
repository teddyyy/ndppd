cmake_minimum_required(VERSION 2.8.11)
project(ndppd)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(
        "C:/Users/Daniel Adolfsson/AppData/Local/Lxss/rootfs/usr/include"
        "C:/Users/Daniel Adolfsson/AppData/Local/Lxss/rootfs/usr/include/x86_64-linux-gnu"
)

add_executable(ndppd
        src/ndppd.cpp       src/ndppd.hpp
        src/address.cpp     src/address.hpp
        src/config.cpp      src/config.hpp
        src/interface.cpp   src/interface.hpp
        src/logger.cpp      src/logger.hpp
        src/socket.cpp      src/socket.hpp
        src/proxy.cpp       src/proxy.hpp
        #src/route.cc
        #src/route.h
        #src/rule.cc
        #src/rule.h
        #src/session.cpp
        #src/session.hpp

        #src/hwaddress.cpp src/hwaddress.hpp
        )



