# Copyright (c) 2024 Realtek Semiconductor, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

set(RTS_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}.rts5817.bin)
string(TOUPPER "${SOC_NAME}" soc_name_upper)

if(APPVERSION)
  set(VERSION ${APPVERSION})
else()
  set(VERSION 0)
endif()

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${SOC_${soc_name_upper}_DIR}/common/tool/gen_download_bin.py
  -i ${KERNEL_BIN_NAME}
  -o ${RTS_BIN_NAME}
  -v ${VERSION}
)
