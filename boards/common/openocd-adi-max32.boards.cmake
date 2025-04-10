#
# Copyright (c) 2025 Analog Devices, Inc
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARCH STREQUAL "riscv")
  set(MAX32_TARGET_CFG "${CONFIG_SOC}_riscv.cfg")
  set(MAX32_INTERFACE_CFG "olimex-arm-usb-ocd-h.cfg")
else()
  set(MAX32_TARGET_CFG "${CONFIG_SOC}.cfg")
  set(MAX32_INTERFACE_CFG "cmsis-dap.cfg")
endif()

# MAX32666 share the same target configuration file with MAX32665
if(CONFIG_SOC_MAX32666)
  set(MAX32_TARGET_CFG "max32665.cfg")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${MAX32_INTERFACE_CFG}]")
board_runner_args(openocd --cmd-pre-init "source [find target/${MAX32_TARGET_CFG}]")
board_runner_args(openocd "--target-handle=_CHIPNAME.cpu")

if(CONFIG_SOC_FAMILY_MAX32_M4)
  board_runner_args(openocd --cmd-pre-init "allow_low_pwr_dbg")
  board_runner_args(openocd "--cmd-erase=max32xxx mass_erase 0")
endif()
