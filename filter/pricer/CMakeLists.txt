cmake_minimum_required(VERSION 2.8)
set(FILTER_PRICER true CACHE BOOL "FILTER-PRICER: Build the PRICER filter.")
if(NOT FILTER_PRICER)
    return()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

file(GLOB SRC main.cpp)

add_library(bh_filter_pricer SHARED ${SRC})

#We depend on bh.so
target_link_libraries(bh_filter_pricer bh)

install(TARGETS bh_filter_pricer DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT bohrium)
