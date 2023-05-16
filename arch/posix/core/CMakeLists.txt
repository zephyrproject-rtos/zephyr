# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)
zephyr_library_sources(
	cpuhalt.c
	fatal.c
	irq.c
	posix_core.c
	swap.c
	thread.c
	)

if(CONFIG_ARCH_HAS_TIMING_FUNCTIONS AND
		NOT CONFIG_SOC_HAS_TIMING_FUNCTIONS AND
    NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
endif()
