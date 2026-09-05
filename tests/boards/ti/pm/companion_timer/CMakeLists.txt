cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pm_companion_timer_test)

target_sources(app PRIVATE src/main.c)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
