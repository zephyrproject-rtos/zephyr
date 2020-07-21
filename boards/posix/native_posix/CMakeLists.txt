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

# Override the C standard used for compilation to C 2011
# This is due to some tests using _Static_assert which is a 2011 feature, but
# otherwise relying on compilers supporting it also when set to C99.
# This was in general ok, but with some host compilers and C library versions
# it led to problems. So we override it to 2011 for native_posix.
set_property(GLOBAL PROPERTY CSTD c11)
