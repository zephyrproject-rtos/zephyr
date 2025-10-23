# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CPU_CORTEX_M55)
  # Connect to the second port for CM55 (default port is 3333)
  board_runner_args(openocd "--gdb-init=target extended-remote :3334")
endif()

board_runner_args(openocd --no-load --no-targets --no-halt)
board_runner_args(openocd "--gdb-init=maint flush register-cache")
board_runner_args(openocd "--gdb-init=tb main")
board_runner_args(openocd "--gdb-init=continue")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
  set_property(TARGET runners_yaml_props_target
    PROPERTY hex_file ${KERNEL_NAME}.signed.hex)
endif()
