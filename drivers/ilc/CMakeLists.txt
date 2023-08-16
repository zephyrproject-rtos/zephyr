# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ILC_INTEL_SOC_FPGA ilc_intel_soc_fpga.c)
zephyr_library_sources_ifdef(CONFIG_ILC_SHELL ilc_shell.c)
