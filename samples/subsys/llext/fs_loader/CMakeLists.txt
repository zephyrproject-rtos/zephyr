# Copyright (c) 2024 Schneider Electric
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(fs_loader)

target_sources(app PRIVATE src/main.c src/log_extension.c src/export.c)

# declare extension
zephyr_llext(hello_world)
zephyr_llext_sources(hello_world.llext/hello_world.c)

zephyr_llext(hello_world_pic PIC)
zephyr_llext_sources(hello_world.llext/hello_world.c)

set(FAT_IMG ${CMAKE_CURRENT_BINARY_DIR}/llext_vfat.img)

# generate image
add_custom_target(vfat.img ALL
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/makeimg.sh ${FAT_IMG} ${CMAKE_CURRENT_BINARY_DIR}/hello_world.llext ${CMAKE_CURRENT_BINARY_DIR}/hello_world_pic.llext
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/makeimg.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hello_world.llext ${CMAKE_CURRENT_BINARY_DIR}/hello_world_pic.llext
)
