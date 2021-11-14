cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmp100)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
##############################
# Below line is new addition #
##############################
add_subdirectory(../../../drivers/sensor/tmp100 tmp100)
