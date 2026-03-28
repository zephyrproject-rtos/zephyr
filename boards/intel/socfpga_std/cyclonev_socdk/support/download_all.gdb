# Copyright (c) 2022-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
# Description:
# Download preloader and .elf using GDB commands

set confirm off
set pagination off

#Download and Run preloader
source boards/intel/socfpga_std/cyclonev_socdk/support/preloader_dl_cmd.txt

#Stop watchdog timer
#permodrst Reg , reset watch dog timer
set $permodrst = (int *)0xffd05014
set *$permodrst = (*$permodrst) | (1<<6)
set *$permodrst = (*$permodrst) & ~(1<<6)

quit
