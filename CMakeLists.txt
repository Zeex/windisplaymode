cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(windisplaymode VERSION 0.1)

add_executable(windisplaymode windisplaymode.cpp)

install(TARGETS windisplaymode RUNTIME DESTINATION .)

set(CPACK_GENERATOR ZIP)
include(CPack)
