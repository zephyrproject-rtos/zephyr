# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usb_printer)

target_sources(app PRIVATE
  src/main.c
  src/printer_pcl.c
  src/printer_pjl.c
  src/printer_queue.c
  )

# Add include directory
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

# Generate version header
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/generated/version.h
  )