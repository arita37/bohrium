cmake_minimum_required(VERSION 2.8)
set(FILTER_FUSEPRINTER true CACHE BOOL "FILTER-FUSEPRINTER: Build the FUSEPRINTER filter.")
if(NOT FILTER_FUSEPRINTER)
    return()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

file(GLOB SRC main.cpp)

add_library(bh_filter_fuseprinter SHARED ${SRC})

#We depend on bh.so
target_link_libraries(bh_filter_fuseprinter bh)

install(TARGETS bh_filter_fuseprinter DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT bohrium)
