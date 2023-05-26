# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)
zephyr_library_sources(
	cpuhalt.c
	fatal.c
	irq.c
	nsi_compat/nsi_compat.c
	nsi_compat/nce.c
	posix_core.c
	swap.c
	thread.c
	)

zephyr_include_directories(
	nsi_compat/
)
