# Copyright (c) 2024-2025 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
include(west)

add_subdirectory(siwg917)

# Necessary to not overwrite NWP Firmware
math(EXPR FLASH_LOAD_ADDRESS "(${CONFIG_FLASH_BASE_ADDRESS}) + (${CONFIG_FLASH_LOAD_OFFSET})")

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/siwx91x_isp_prepare.py
    --load-addr ${FLASH_LOAD_ADDRESS}
    ${KERNEL_BIN_NAME}
    ${KERNEL_NAME}.rps
)

# runners_yaml_props_target controls the file used by "west flash"
if(CONFIG_SIWX91X_SIGN_KEY OR CONFIG_SIWX91X_MIC_KEY)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
    ${WEST} sign -t silabs_commander --build-dir ${CMAKE_BINARY_DIR}
  )
  set_target_properties(runners_yaml_props_target
    PROPERTIES bin_file ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.signed.rps
  )
else()
  set_target_properties(runners_yaml_props_target
    PROPERTIES bin_file ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.rps
  )
endif()
