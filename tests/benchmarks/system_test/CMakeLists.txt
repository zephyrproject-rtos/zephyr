cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(system_test)

FILE(GLOB app_sources src/*.c)
include_directories(include)
target_sources(app PRIVATE ${app_sources})
