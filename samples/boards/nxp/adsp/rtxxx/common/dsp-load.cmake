# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/dsp.c" "${CMAKE_CURRENT_LIST_DIR}/src/dspimgs.S")
include_directories("${CMAKE_CURRENT_LIST_DIR}/src/")

set(HIFI4_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr")

set(HIFI4_BIN_SYMS
    "HIFI4_BIN_RESET=\"${HIFI4_BUILD_DIR}/zephyr.reset.bin\""
    "HIFI4_BIN_TEXT=\"${HIFI4_BUILD_DIR}/zephyr.text.bin\""
    "HIFI4_BIN_DATA=\"${HIFI4_BUILD_DIR}/zephyr.data.bin\""
)

set_source_files_properties(
    "${CMAKE_CURRENT_LIST_DIR}/src/dspimgs.S"
    PROPERTIES
    COMPILE_DEFINITIONS
    "${HIFI4_BIN_SYMS}"
)

set_source_files_properties(
    "${CMAKE_CURRENT_LIST_DIR}/src/dspimgs.S"
    OBJECT_DEPENDS
    ${HIFI4_BUILD_DIR}/zephyr.elf
)
