# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr
  COMPONENTS zephyr_default:boards
  REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(uicr LANGUAGES NONE)

# Override the runners.yaml path to use CMAKE_CURRENT_BINARY_DIR/zephyr
# instead of PROJECT_BINARY_DIR, this ensures runners.yaml is generated
# at <build>/uicr/zephyr where west expects it
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(gen_script ${CMAKE_CURRENT_LIST_DIR}/gen_uicr.py)
set(uicr_hex ${PROJECT_BINARY_DIR}/zephyr.hex)

zephyr_get(DEFAULT_IMAGE_EDT_PICKLE SYSBUILD GLOBAL)

if(NOT DEFAULT_IMAGE_EDT_PICKLE)
  message(FATAL_ERROR
    "DEFAULT_IMAGE_EDT_PICKLE not found. "
    "The UICR generator must be built via sysbuild."
  )
endif()

add_custom_command(
  OUTPUT ${uicr_hex}
  COMMAND ${PYTHON_EXECUTABLE} ${gen_script}
    --zephyr-base ${ZEPHYR_BASE}
    --edt-pickle ${DEFAULT_IMAGE_EDT_PICKLE}
    --output ${uicr_hex}
  DEPENDS ${DEFAULT_IMAGE_EDT_PICKLE} ${gen_script}
  COMMENT "Generating UICR hex: ${uicr_hex}"
)

add_custom_target(gen_uicr ALL DEPENDS ${uicr_hex})

# Create the runners_yaml_props_target that flash system expects
add_custom_target(runners_yaml_props_target)
set_target_properties(runners_yaml_props_target PROPERTIES
  hex_file "zephyr.hex"
)

# Copy over Kconfig and dts files from the main image
get_filename_component(default_image_binary_dir ${DEFAULT_IMAGE_EDT_PICKLE} DIRECTORY)
zephyr_file_copy(${default_image_binary_dir}/.config ${PROJECT_BINARY_DIR}/.config
  ONLY_IF_DIFFERENT
)
zephyr_file_copy(${default_image_binary_dir}/edt.pickle ${PROJECT_BINARY_DIR}/edt.pickle
  ONLY_IF_DIFFERENT
)
import_kconfig(CONFIG_ ${default_image_binary_dir}/.config)

# Manually include board configuration to enable automatic runners.yaml generation
foreach(dir ${BOARD_DIRECTORIES})
  include(${dir}/board.cmake OPTIONAL)
endforeach()

# Include flash support to automatically generate runners.yaml
include(${ZEPHYR_BASE}/cmake/flash/CMakeLists.txt)

# Silent unused variable warnings
set(EXTRA_KCONFIG_TARGETS)
set(FORCED_CONF_FILE)
