# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2017 Oticon A/S

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)
zephyr_library_compile_definitions(_POSIX_C_SOURCE=200809L _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED)

zephyr_library_sources(
	hw_models_top.c
	timer_model.c
	native_rtc.c
	irq_handler.c
	irq_ctrl.c
	main.c
	tracing.c
	cmdline_common.c
	cmdline.c
	cpu_wait.c
	hw_counter.c
	)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
  )

if(CONFIG_HAS_SDL)
	add_subdirectory(${ZEPHYR_BASE}/boards/native/common/sdl/ ${CMAKE_CURRENT_BINARY_DIR}/sdl)
endif()

zephyr_ld_options(
  -lm
)
