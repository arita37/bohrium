cmake_minimum_required(VERSION 2.8)
set(FUSER_OPTIMAL true CACHE BOOL "FUSER-OPTIMAL: Build OPTIMAL fuser.")
if(NOT FUSER_OPTIMAL)
    return()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

find_package(OpenMP COMPONENTS bohrium)
if(OPENMP_FOUND)

	file(GLOB SRC *.cpp)

	add_library(bh_fuser_optimal SHARED ${SRC})

	#We depend on bh.so
	target_link_libraries(bh_fuser_optimal bh)

	#We depend on boost
	target_link_libraries(bh_fuser_optimal ${Boost_LIBRARIES})

	install(TARGETS bh_fuser_optimal DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT bohrium)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
