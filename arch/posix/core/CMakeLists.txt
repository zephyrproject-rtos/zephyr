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

if(CONFIG_NATIVE_APPLICATION)
	zephyr_include_directories(
		nsi_compat/
	)
	zephyr_library_sources(
		posix_core.c
		nsi_compat/nsi_compat.c
		nsi_compat/nce.c
	)
else()
	zephyr_library_sources(
		posix_core_nsi.c
	)
endif()
