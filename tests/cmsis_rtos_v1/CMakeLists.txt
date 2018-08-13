cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/include/cmsis_rtos_v1)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
