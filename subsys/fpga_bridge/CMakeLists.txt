# Copyright (c) 2023, Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_FPGA_BRIDGE)
zephyr_library_sources(fpga_bridge.c)
zephyr_library_sources_ifdef(CONFIG_FPGA_BRIDGE_INTEL fpga_bridge_intel.c)
zephyr_library_sources_ifdef(CONFIG_FPGA_BRIDGE_SHELL fpga_bridge_shell.c)
endif()
