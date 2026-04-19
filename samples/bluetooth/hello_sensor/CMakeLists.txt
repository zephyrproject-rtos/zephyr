cmake_minimum_required(VERSION 3.20.0)

project(hello_sensor)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE
    src/main.c
)
