# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

zephyr_library_sources(
	soc.c
	)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
  )
