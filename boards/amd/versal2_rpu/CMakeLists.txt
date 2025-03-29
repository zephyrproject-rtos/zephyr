#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#

find_package(Dtc 1.4.6 REQUIRED)

# Check if the board-specific qemu.dts file exists
if(EXISTS "${BOARD_DIR}/${BOARD}-qemu.dts")
  # Ensure DTC executable is available
  if(DTC_FOUND)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${DTC} -I dts -O dtb -q "${BOARD_DIR}/${BOARD}-qemu.dts" -o "${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb"
    )
  else()
    message(FATAL_ERROR "DTC not found, but required for compiling ${BOARD}-qemu.dts")
  endif()
endif()
