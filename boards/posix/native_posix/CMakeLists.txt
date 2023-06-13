# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

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
	find_package(PkgConfig REQUIRED)
	pkg_search_module(SDL2 REQUIRED sdl2)
	zephyr_include_directories(${SDL2_INCLUDE_DIRS})
	zephyr_link_libraries(${SDL2_LIBRARIES})
	zephyr_compile_options(${SDL2_CFLAGS_OTHER})
	zephyr_library_sources(sdl_events.c)
endif()

zephyr_ld_options(
  -lm
)
