# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpi_mailbox_vc_if
    VERSION 1.0.0
    DESCRIPTION "Raspberry Pi VideoCore (GPU) interface through mailbox"
    LANGUAGES C ASM
)

if(CONFIG_BOARD_RPI_4B)
    message(STATUS "Build app for: ${BOARD}${BOARD_QUALIFIERS}")
else()
    message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

target_sources(app PRIVATE
    src/main.c
)
