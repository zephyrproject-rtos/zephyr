#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#

if (EXISTS "${BOARD_DIR}/${BOARD}-qemu.dts")
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND dtc -I dts -O dtb -q ${BOARD_DIR}/${BOARD}-qemu.dts -o ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
  )
endif()
