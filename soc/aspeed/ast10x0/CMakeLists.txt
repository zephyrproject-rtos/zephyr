# Copyright (c) 2021 ASPEED Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)
zephyr_include_directories(.)
zephyr_linker_sources(ROM_START SORT_KEY 0x1sboot sboot.ld)
zephyr_linker_sources(RAM_SECTIONS nocache.ld)

string(TOUPPER "${SOC_NAME}" soc_name_upper)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${SOC_${soc_name_upper}_DIR}/${SOC_SERIES}/tools/gen_uart_booting_image.py
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  ${PROJECT_BINARY_DIR}/uart_${CONFIG_KERNEL_BIN_NAME}.bin
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
