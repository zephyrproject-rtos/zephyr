# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_FPGA_SHELL  fpga_shell.c)

zephyr_library_sources_ifdef(CONFIG_ALTERA_AGILEX_BRIDGE_FPGA     fpga_altera_agilex_bridge.c)
zephyr_library_sources_ifdef(CONFIG_EOS_S3_FPGA fpga_eos_s3.c)
zephyr_library_sources_ifdef(CONFIG_ICE40_FPGA  fpga_ice40.c)
zephyr_library_sources_ifdef(CONFIG_MPFS_FPGA fpga_mpfs.c)
zephyr_library_sources_ifdef(CONFIG_ZYNQMP_FPGA fpga_zynqmp.c)
zephyr_library_sources_ifdef(CONFIG_SLG471X5_FPGA fpga_slg471x5.c)
