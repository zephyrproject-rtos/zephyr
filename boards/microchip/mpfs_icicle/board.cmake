# Copyright (c) 2025 Antmicro
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode)
if(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_U54)
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_u54.resc)
  set(RENODE_UART sysbus.mmuart1)
elseif(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_U54_SMP)
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_u54_smp.resc)
  set(RENODE_UART sysbus.mmuart1)
elseif(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_E51)
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_e51.resc)
  set(RENODE_UART sysbus.mmuart0)
endif()

set(OPENOCD_USE_LOAD_IMAGE NO)
