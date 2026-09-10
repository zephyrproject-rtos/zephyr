# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_COVERAGE_GCOV coverage.c)

zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RAM_SECTIONS coverage_ram.ld)
zephyr_linker_sources_ifdef(CONFIG_COVERAGE_GCOV RODATA coverage_rodata.ld)
