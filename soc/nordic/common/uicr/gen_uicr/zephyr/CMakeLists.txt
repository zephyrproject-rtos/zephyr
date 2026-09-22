# SPDX-License-Identifier: Apache-2.0

# Flash configuration for UICR domain
# This subdirectory ensures runners.yaml is generated in the correct location

# Manually include board configuration to enable automatic runners.yaml generation
include(${BOARD_DIR}/board.cmake OPTIONAL)

# Create the runners_yaml_props_target that flash system expects
add_custom_target(runners_yaml_props_target)

# Set hex_file property to point to zephyr.hex in this directory
set_target_properties(runners_yaml_props_target PROPERTIES
  hex_file "zephyr.hex"
)

# Override the runners.yaml path to use CMAKE_CURRENT_BINARY_DIR instead of PROJECT_BINARY_DIR
# This ensures runners.yaml is generated at build/uicr/zephyr/ where west expects it
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Include flash support to automatically generate runners.yaml
include(${ZEPHYR_BASE}/cmake/flash/CMakeLists.txt)
