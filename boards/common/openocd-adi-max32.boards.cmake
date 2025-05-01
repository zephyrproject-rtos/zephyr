#
# Copyright (c) 2025 Analog Devices, Inc
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARCH STREQUAL "riscv")
  set(MAX32_TARGET_CFG "${CONFIG_SOC}_riscv.cfg")
  set(MAX32_INTERFACE_CFG "ftdi/olimex-arm-usb-ocd-h.cfg")
  set(MAX32_FLASH_TARGET_CFG "${CONFIG_SOC}.cfg")
  set(MAX32_FLASH_INTERFACE_CFG "cmsis-dap.cfg")
else()
  set(MAX32_TARGET_CFG "${CONFIG_SOC}.cfg")
  set(MAX32_INTERFACE_CFG "cmsis-dap.cfg")
endif()

# MAX32666 share the same target configuration file with MAX32665
if(CONFIG_SOC_MAX32666)
  set(MAX32_TARGET_CFG "max32665.cfg")
elseif(CONFIG_SOC_MAX32657)
  set(MAX32_INTERFACE_CFG "jlink.cfg")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${MAX32_INTERFACE_CFG}]")
board_runner_args(openocd --cmd-pre-init "source [find target/${MAX32_TARGET_CFG}]")
board_runner_args(openocd "--target-handle=_CHIPNAME.cpu")

if(MAX32_FLASH_INTERFACE_CFG)
  board_runner_args(openocd --flash-cmd-pre-init "source [find interface/${MAX32_FLASH_INTERFACE_CFG}]")
endif()

if(MAX32_FLASH_TARGET_CFG)
  board_runner_args(openocd --flash-cmd-pre-init "source [find target/${MAX32_FLASH_TARGET_CFG}]")
endif()

if(CONFIG_SOC_FAMILY_MAX32_M4 OR CONFIG_SOC_FAMILY_MAX32_M33)
  board_runner_args(openocd --cmd-pre-init "allow_low_pwr_dbg")
  board_runner_args(openocd "--cmd-erase=max32xxx mass_erase 0")
endif()

if(CONFIG_SOC_MAX32690 AND CONFIG_SOC_FAMILY_MAX32_RV32)
  board_runner_args(openocd "--cmd-erase=max32xxx mass_erase 1")
endif()
