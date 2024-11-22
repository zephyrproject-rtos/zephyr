# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
#

zephyr_include_directories(.)

if (DEFINED CONFIG_REALTEK_RTS5912_BOOTROM_HEADER)
  math(EXPR adjustment "${CONFIG_REALTEK_RTS5912_BOOTROM_HEADER_LOAD_ADDRESS}" OUTPUT_FORMAT DECIMAL)
  set(RTS5912_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}.rts5912.bin)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/realtek/ec/common/rts5912_imgtool/img_gen.py
    -L ${adjustment}
    -I ${KERNEL_BIN_NAME}
    -O ${RTS5912_BIN_NAME}
    -V
  )
endif()
