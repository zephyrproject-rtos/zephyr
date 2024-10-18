# Copyright 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_CV64A6_PROVIDE_TEST_POWEROFF soc_poweroff.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_CV64A6_PROVIDE_NONSTANDARD_CACHE_OPTIONS soc_cache_management.c)
zephyr_include_directories_ifdef(CONFIG_SOC_SERIES_CV64A6 ".")


zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_CV64A6_PROVIDE_FPGA_POWEROFF soc_poweroff_fpga.c)
