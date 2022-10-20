#Copyright (c) 2023, Intel Corporation.
#SPDX-License-Identifier: Apache-2.0


zephyr_library()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_FPGA_MANAGER)
zephyr_library_sources(fpga_manager.c)
zephyr_library_sources_ifdef(CONFIG_FPGA_MANAGER_INTEL fpga_manager_intel.c)
zephyr_library_sources_ifdef(CONFIG_FPGA_MANAGER_SHELL fpga_manager_shell.c)
endif()
