# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
# Description:
# Helper file to use "west flash" with the .elf file
# this file is called by the runner for Cyclone V SoC DeKit

set confirm off
set pagination off

restore ./build/zephyr/zephyr.elf
symbol-file -readnow ./build/zephyr/zephyr.elf
thbreak main
jump z_arm_reset

# Execute OpenOCD "resume" command to keep the target running, then quit gdb.
mon resume
quit
