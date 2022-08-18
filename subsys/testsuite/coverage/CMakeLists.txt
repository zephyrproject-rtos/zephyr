# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_COVERAGE_GCOV coverage.c)

if(CONFIG_DEMAND_PAGING AND CONFIG_COVERAGE)
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV PINNED_RAM_SECTIONS coverage_ram.ld)
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV PINNED_RODATA coverage_rodata.ld)
else()
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RAM_SECTIONS coverage_ram.ld)
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RODATA coverage_rodata.ld)
endif()
