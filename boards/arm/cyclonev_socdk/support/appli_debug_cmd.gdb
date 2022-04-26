# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
# Description:
# helper file for "west debug" command to load the .elf
# works if the .elf is located in "build" directory

set confirm off
set pagination off
set remote interrupt-on-connect off

restore ./build/zephyr/zephyr.elf
symbol-file -readnow ./build/zephyr/zephyr.elf
thbreak main
jump z_arm_reset
