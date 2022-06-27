# Copyright (c) 2021 ASPEED Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)
zephyr_include_directories(${ZEPHYR_BASE}/soc/arm/common/cortex_m)
zephyr_linker_sources(ROM_START SORT_KEY 0x1sboot sboot.ld)
zephyr_linker_sources(RAM_SECTIONS nocache.ld)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/${SOC_SERIES}/tools/gen_uart_booting_image.py
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  ${PROJECT_BINARY_DIR}/uart_${CONFIG_KERNEL_BIN_NAME}.bin
)
