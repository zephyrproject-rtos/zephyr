# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/mspi.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MSPI_AMBIQ_AP3   mspi_ambiq_ap3.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_AMBIQ_AP5   mspi_ambiq_ap5.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_AMBIQ_TIMING_SCAN mspi_ambiq_timing_scan.c)
zephyr_library_sources_ifdef(CONFIG_MSPI_EMUL        mspi_emul.c)
