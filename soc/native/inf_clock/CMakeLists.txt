# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2023 Nordic Semiconductor ASA

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

zephyr_library_sources(
	soc.c
	native_tasks.c
	)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/posix/linker.ld CACHE INTERNAL "")
