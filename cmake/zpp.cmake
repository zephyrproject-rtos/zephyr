# Copyright (c) 2024 Basalte bv
#
# SPDX-License-Identifier: Apache-2.0

# Generate compile_commands.json in the output directory
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL
    "Export CMake compile commands. Used by zpp.py script"
    FORCE
)

set(annotations_json "${CMAKE_BINARY_DIR}/zephyr/annotations.json")

add_custom_command(
  OUTPUT ${annotations_json}
  COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zpp.py
    -j ${CMAKE_BUILD_PARALLEL_LEVEL}
    -o ${annotations_json}
    -d ${annotations_json}.d
    ${CMAKE_BINARY_DIR}/compile_commands.json
  DEPENDS
    ${ZEPHYR_BASE}/scripts/zpp.py
    ${CMAKE_BINARY_DIR}/compile_commands.json
  DEPFILE ${annotations_json}.d
)

add_custom_target(zpp DEPENDS ${annotations_json})
