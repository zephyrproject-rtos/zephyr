# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)

# Add a custom command to create a test disk image on the fly.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test.img
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/test.img
    ${CONFIG_RAMDISK_DISKIMAGE_SAMPLE_DISK_SIZE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_custom_target(test_img_target DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.img)
add_dependencies(app test_img_target)
