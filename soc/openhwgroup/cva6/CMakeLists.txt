# Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_CVA6_PROVIDE_NONSTANDARD_CACHE_OPTIONS soc_cache_management.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
