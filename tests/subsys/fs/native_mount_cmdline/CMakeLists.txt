cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(native_mount_cmdline)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${NATIVE_MOUNT_HOST_DIR_A}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${NATIVE_MOUNT_HOST_DIR_B}")

target_sources(app PRIVATE src/main.c)
