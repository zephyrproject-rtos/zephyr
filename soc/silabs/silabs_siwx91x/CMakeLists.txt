# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(siwg917)

# Necessary to not overwrite NWP Firmware
math(EXPR FLASH_LOAD_ADDRESS "(${CONFIG_FLASH_BASE_ADDRESS}) + (${CONFIG_FLASH_LOAD_OFFSET})")

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/siwx91x_isp_prepare.py
    --load-addr ${FLASH_LOAD_ADDRESS}
    ${KERNEL_BIN_NAME}
    ${KERNEL_BIN_NAME}.rps
)
