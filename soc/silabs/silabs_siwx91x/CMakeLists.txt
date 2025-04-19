# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

include(west)  # Add this at the top or before using ${WEST}

add_subdirectory(siwg917)
zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_WISECONNECT_NETWORK_STACK nwp_init.c)

# Necessary to not overwrite NWP Firmware
math(EXPR FLASH_LOAD_ADDRESS "(${CONFIG_FLASH_BASE_ADDRESS}) + (${CONFIG_FLASH_LOAD_OFFSET})")

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/siwx91x_isp_prepare.py
    --load-addr ${FLASH_LOAD_ADDRESS}
    ${KERNEL_BIN_NAME}
    ${KERNEL_BIN_NAME}.rps
)

# Custom signing with Silicon Labs Commander
if(CONFIG_SILABS_COMMANDER_SIGN)

    # Define input and output files
    set(ZEPHYR_BINARY ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}.bin.rps)
    set(SIGNED_RPS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}.bin.rps)

    # Define the west sign command with explicit working directory and environment
    set(WEST_SIGN_CMD
        ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}  # Explicitly set working directory
        ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH}:$ENV{WEST_ROOT}/bin" "ZEPHYR_WORKSPACE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        ${WEST} sign
        -t silabs_rps
        -d ${CMAKE_BINARY_DIR}
        --sbin ${SIGNED_RPS}
    )

    # Add custom command to invoke west sign during build
    add_custom_command(
        OUTPUT ${SIGNED_RPS}
        COMMAND ${CMAKE_COMMAND} -E echo "Forcing execution in ${CMAKE_BINARY_DIR}"
        COMMAND ${WEST_SIGN_CMD}
        DEPENDS zephyr_final
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}  # Ensure working directory is set
        COMMENT "Signing binary with 'west sign -t silabs_rps' to generate ${KERNEL_BIN_NAME}.bin.rps"
        VERBATIM
    )

    # Add custom target to ensure signing is part of the build
    add_custom_target(silabs_rps_sign_target
        ALL
        DEPENDS ${SIGNED_RPS}
        COMMENT "Ensuring signing with Silicon Labs Commander is complete"
    )
else()
    message(STATUS "CONFIG_SILABS_COMMANDER_SIGN is not enabled")
endif()
