# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

# Resolve required led0 alias
dt_alias(led0_path PROPERTY "led0")
if(NOT DEFINED led0_path)
  message(FATAL_ERROR
    "\n"
    "Blinky requires a devicetree `led0` alias. Add an overlay;\n"
    "See `samples/basic/blinky/README.rst` (Adding board support).\n"
  )
endif()

# Check if alias node is enabled
dt_node_has_status(led0_ok PATH "${led0_path}" STATUS okay)
if(NOT led0_ok)
  message(FATAL_ERROR
    "\n"
    "Blinky requires the `led0` node to have status \"okay\".\n"
  )
endif()

target_sources(app PRIVATE src/main.c)
