# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(openocd)
board_set_flasher_ifnset(openocd)

if(CONFIG_CPU_CORTEX_M55)
  # Connect to the second port for CM55 (default port is 3333)
  board_runner_args(openocd "--gdb-init=disconnect")
  board_runner_args(openocd "--gdb-init=target extended-remote :3334")
  board_runner_args(openocd "--target-handle=CHIPNAME.cm55")
else()
  board_runner_args(openocd "--target-handle=CHIPNAME.cm33")
endif()

board_runner_args(openocd --no-load --no-targets --no-halt)
# 'west flash --erase' invokes the vendor 'erase_all' OpenOCD proc, which
# erases the on-die CM33 main_ns RRAM bank (which includes the non-reserved
# memory portion) and every external SMIF/QSPI bank attached to the device. The
# 'main_s' bank is a virtual alias of 'main_ns' so a single sector erase
# covers both secure and non-secure views.
board_runner_args(openocd "--cmd-erase=erase_all")
board_runner_args(openocd "--gdb-init=maint flush register-cache")
board_runner_args(openocd "--gdb-init=tb main")
board_runner_args(openocd "--gdb-init=continue")

board_runner_args(probe-rs "--chip=PSE846GPS4DBZC4A")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
  set_property(TARGET runners_yaml_props_target
    PROPERTY hex_file ${KERNEL_NAME}.signed.hex)
endif()

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_NONSECURE)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()
