# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

include(west)  # Add this at the top or before using ${WEST}

add_subdirectory(siwg917)

# Necessary to not overwrite NWP Firmware
math(EXPR FLASH_LOAD_ADDRESS "(${CONFIG_FLASH_BASE_ADDRESS}) + (${CONFIG_FLASH_LOAD_OFFSET})")

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/siwx91x_isp_prepare.py
    --load-addr ${FLASH_LOAD_ADDRESS}
    ${KERNEL_BIN_NAME}
    ${KERNEL_BIN_NAME}.rps
)

# Custom signing with Silicon Labs Commander
if(CONFIG_SIWX91X_SILABS_OTA_SIGN_ENABLE OR CONFIG_SIWX91X_SILABS_OTA_MIC_ENABLE OR CONFIG_SIWX91X_SILABS_OTA_ENCRYPT_ENABLE)
    # Necessary to invoke west sign after build
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND ${WEST} sign -t silabs_commander --build-dir ${CMAKE_BINARY_DIR} --sbin ${PROJECT_BINARY_DIR}/${KERNEL_NAME}_signed.bin.rps
        DEPENDS zephyr_final
    )
endif()