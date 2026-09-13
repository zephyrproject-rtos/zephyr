# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

# Resolve required led0 alias
dt_alias(led0_path PROPERTY "led0")
if(NOT DEFINED led0_path)
  message(FATAL_ERROR
    "\n"
    "Blinky requires a devicetree 'led0' alias, which this board does not define.\n"
    "Define it in the board devicetree or in an overlay. See the sample\n"
    "documentation, section 'Adding board support'.\n"
  )
endif()

# Check if led0 alias node is enabled
dt_node_has_status(led0_ok PATH "${led0_path}" STATUS okay)
if(NOT led0_ok)
  message(FATAL_ERROR
    "\n"
    "The node behind the devicetree 'led0' alias, ${led0_path}, is not enabled.\n"
    "Set its status to \"okay\" in an overlay. See the sample documentation,\n"
    "section 'Adding board support'.\n"
  )
endif()

target_sources(app PRIVATE src/main.c)
