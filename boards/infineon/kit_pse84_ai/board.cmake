# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
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
