# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
#

zephyr_include_directories(.)

if (DEFINED CONFIG_REALTEK_RTS5912_OUTPUT_BIN)
  set(RTS5912_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}.rts5912.bin)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/realtek/ec/common/rts5912_imgtool/img_gen.py
    --load-addr ${CONFIG_REALTEK_RTS5912_IMG_HEADER_LOAD_ADDRESS}
    --spi-freq ${CONFIG_REALTEK_RTS5912_IMG_HEADER_SPI_FREQ_MHZ}
    --spi-mode ${CONFIG_REALTEK_RTS5912_IMG_HEADER_SPI_MODE}
    --spi-rdcmd ${CONFIG_REALTEK_RTS5912_IMG_HEADER_SPI_READ_CMD}
    ${KERNEL_BIN_NAME}
    ${RTS5912_BIN_NAME}
  )
endif()