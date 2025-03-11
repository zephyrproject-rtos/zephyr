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
if(CONFIG_SiWX91X_SILABS_COMMANDER_SIGN)
    # Define the west sign command to run along with west build command
    set(WEST_SIGN_CMD
        ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH}:$ENV{WEST_ROOT}/bin" "ZEPHYR_WORKSPACE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        ${WEST} sign
        -t silabs_commander
        -d ${CMAKE_BINARY_DIR}
        --sbin ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}.rps
    )

    # Add custom command to invoke west sign during build
    add_custom_command(
        OUTPUT ${KERNEL_BIN_NAME}.rps
        COMMAND ${CMAKE_COMMAND} -E echo "Forcing execution in ${PROJECT_BINARY_DIR}"
        COMMAND ${WEST_SIGN_CMD}
        DEPENDS zephyr_final
        VERBATIM
    )

    # Add custom target to ensure signing is part of the build
    add_custom_target(silabs_commander_sign
        ALL
        DEPENDS ${KERNEL_BIN_NAME}.rps
        COMMENT "Ensuring signing with Silicon Labs Commander is complete"
    )
endif()
