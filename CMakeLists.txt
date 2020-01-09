#
# This file is subject to the terms and conditions defined in
# file 'LICENSE', which is part of this source code package.
#

cmake_minimum_required(VERSION 3.1...3.14)

project(ccc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif(MSVC)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
add_definitions(-D_WIN32_WINNT=0x0501)
find_package(Boost COMPONENTS system unit_test_framework program_options date_time regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

macro(boosttest_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME} PRIVATE ${Boost_FILESYSTEM_LIBRARY}
                                PRIVATE ${Boost_SYSTEM_LIBRARY}
                                PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

############
# Chapters #
############

include_directories(headers)
add_subdirectory(src)
