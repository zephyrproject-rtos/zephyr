#
# Copyright (c) 2025 Analog Devices, Inc
#
# SPDX-License-Identifier: Apache-2.0

# Default cmsis-dap, it will be overwritten below if requires
set(MAX32_INTERFACE_CFG "cmsis-dap.cfg")

if(CONFIG_SOC_MAX32655_M4)
  set(MAX32_TARGET_CFG "max32655.cfg")
elseif(CONFIG_SOC_MAX32662)
  set(MAX32_TARGET_CFG "max32662.cfg")
elseif(CONFIG_SOC_MAX32666)
  set(MAX32_TARGET_CFG "max32665.cfg")
elseif(CONFIG_SOC_MAX32670)
  set(MAX32_TARGET_CFG "max32670.cfg")
elseif(CONFIG_SOC_MAX32672)
  set(MAX32_TARGET_CFG "max32672.cfg")
elseif(CONFIG_SOC_MAX32675)
  set(MAX32_TARGET_CFG "max32675.cfg")
elseif(CONFIG_SOC_MAX32680_M4)
  set(MAX32_TARGET_CFG "max32680.cfg")
elseif(CONFIG_SOC_MAX32690_M4)
  set(MAX32_TARGET_CFG "max32690.cfg")
elseif(CONFIG_SOC_MAX78000_M4)
  set(MAX32_TARGET_CFG "max78000.cfg")
elseif(CONFIG_SOC_MAX78002_M4)
  set(MAX32_TARGET_CFG "max78002.cfg")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${MAX32_INTERFACE_CFG}]")
board_runner_args(openocd --cmd-pre-init "source [find target/${MAX32_TARGET_CFG}]")

if(CONFIG_SOC_FAMILY_MAX32_M4)
  board_runner_args(openocd --cmd-pre-init "allow_low_pwr_dbg")
  board_runner_args(openocd "--cmd-erase=max32xxx mass_erase 0")
endif()
