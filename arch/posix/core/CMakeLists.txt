# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_compile_definitions(NO_POSIX_CHEATS)
zephyr_library_sources(
	cpuhalt.c
	fatal.c
	irq.c
	swap.c
	thread.c
	)

if(CONFIG_ARCH_POSIX_TRAP_ON_FATAL)
  target_sources(native_simulator INTERFACE fatal_trap.c)
endif()

zephyr_library_sources(
	posix_core_nsi.c
)
