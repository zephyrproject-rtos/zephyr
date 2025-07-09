# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

add_custom_command(
    OUTPUT ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.reset.bin
    DEPENDS ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.reset.bin
    --only-section=.ResetVector.text
)

add_custom_command(
    OUTPUT ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.text.bin
    DEPENDS ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.text.bin
    --only-section=.WindowVectors.text
    --only-section=.*Vector.text
    --only-section=!.ResetVector.text
    --only-section=.iram.text
    --only-section=.text
)

add_custom_command(
    OUTPUT ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.data.bin
    DEPENDS ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.data.bin
    --only-section=.rodata
    --only-section=initlevel
    --only-section=sw_isr_table
    --only-section=device_area
    --only-section=device_states
    --only-section=service_area
    --only-section=.noinit
    --only-section=.data
    --only-section=.bss
    --only-section=log_*_area
    --only-section=k_*_area
    --only-section=*_api_area
)

add_custom_target(
    dsp_bin ALL
    DEPENDS
    ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.reset.bin
    ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.text.bin
    ${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.data.bin
)
