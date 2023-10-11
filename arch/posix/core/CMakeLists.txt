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
  if(CONFIG_NATIVE_LIBRARY)
    target_sources(native_simulator INTERFACE fatal_trap.c)
  else()
    zephyr_library_sources(fatal_trap.c)
  endif()
endif()

if(CONFIG_NATIVE_APPLICATION)
	zephyr_include_directories(
		nsi_compat/
	)
	zephyr_library_sources(
		posix_core.c
		nsi_compat/nsi_compat.c
		${ZEPHYR_BASE}/scripts/native_simulator/common/src/nce.c
		${ZEPHYR_BASE}/scripts/native_simulator/common/src/nsi_host_trampolines.c
	)
else()
	zephyr_library_sources(
		posix_core_nsi.c
	)
endif()
