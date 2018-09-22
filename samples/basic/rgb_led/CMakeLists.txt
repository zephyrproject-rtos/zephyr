cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

add_definitions(-DCONFIG_TICKLESS_IDLE=1 -DCONFIG_TICKLESS_IDLE_THRESH=100)
project(NONE)

target_sources(app PRIVATE src/main.c)
