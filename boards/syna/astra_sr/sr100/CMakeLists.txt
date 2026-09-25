# SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_SR100_RDK_SR100_M55 AND NOT DEFINED SYSBUILD)
  set(SR100_IMAGEGEN_M55_IMG ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf)
  include(${CMAKE_CURRENT_LIST_DIR}/image_generation.cmake)
endif()
