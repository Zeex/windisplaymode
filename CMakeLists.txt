cmake_minimum_required(VERSION 2.8)
project(windisplaymode)

add_executable(windisplaymode windisplaymode.cpp)

install(TARGETS windisplaymode RUNTIME DESTINATION .)

set(CPACK_GENERATOR ZIP)
include(CPack)
