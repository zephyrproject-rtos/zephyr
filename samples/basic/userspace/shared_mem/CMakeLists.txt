cmake_minimum_required(VERSION 3.9.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
#add_definitions( -DALTMSG)
#add_definitions( -DDBUG)
