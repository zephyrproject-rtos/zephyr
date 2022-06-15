cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_master)

file(GLOB APP_SOURCES src/*.c)
target_sources(app PRIVATE ${APP_SOURCES})
