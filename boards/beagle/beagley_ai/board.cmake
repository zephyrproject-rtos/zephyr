# BeagleBoard.org Beagle Y-AI
# Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
# SPDX-License-Identifier: Apache-2.0

# Select the OpenOCD debug interface. Empty (default) attaches to a separately
# launched OpenOCD session on the board Linux host. Any other value is treated
# as an OpenOCD interface script stem under interface/, e.g.:
#   -DOPENOCD_INTERFACE=ftdi/tumpa
#   -DOPENOCD_INTERFACE=buspirate

if(CONFIG_SOC_J722S_MAIN_R5F0_0)
  set(OPENOCD_GDB_PORT 3339)
  set(OPENOCD_TARGET_HANDLE "j722s.cpu.main0_r5.0")
elseif(CONFIG_SOC_J722S_MCU_R5F0_0)
  set(OPENOCD_GDB_PORT 3340)
  set(OPENOCD_TARGET_HANDLE "j722s.cpu.mcu0_r5.0")
endif()

board_runner_args(openocd
  "--gdb-client-port=${OPENOCD_GDB_PORT}"
  "--target-handle=${OPENOCD_TARGET_HANDLE}")

if("${OPENOCD_INTERFACE}" STREQUAL "")
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets")
else()
  board_runner_args(openocd
    --cmd-pre-init "source [find interface/${OPENOCD_INTERFACE}.cfg]"
    --cmd-pre-init "transport select jtag"
    --cmd-pre-init "reset_config srst_only srst_push_pull"
    --cmd-pre-init "adapter srst delay 20"
    --cmd-pre-init "set SOC j722s"
    --cmd-pre-init "source [find target/ti/k3.cfg]"
    --cmd-pre-init "adapter speed 2500")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
