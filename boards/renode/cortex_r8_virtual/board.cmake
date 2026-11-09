# Copyright (c) 2024 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode)
if(CONFIG_BOARD_CORTEX_R8_VIRTUAL_CORTEX_R8_VIRTUAL_SMP)
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/cortex_r8_virtual_smp.resc)
else()
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/cortex_r8_virtual.resc)
endif()
set(RENODE_UART sysbus.uart0)
