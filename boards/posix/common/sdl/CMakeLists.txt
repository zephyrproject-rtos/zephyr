# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)

if (CONFIG_NATIVE_APPLICATION)
	zephyr_link_libraries(${SDL2_LIBRARIES})
	zephyr_include_directories(${SDL2_INCLUDE_DIRS})
	zephyr_compile_options(${SDL2_CFLAGS_OTHER})
else()
	target_link_options(native_simulator INTERFACE "-l${SDL2_LIBRARIES}")
	target_compile_options(native_simulator INTERFACE "-I${SDL2_INCLUDE_DIRS}" ${SDL2_CFLAGS_OTHER})
endif()

zephyr_library_sources(sdl_events.c)
if (CONFIG_NATIVE_APPLICATION)
	zephyr_library_sources(sdl_events_bottom.c)
else()
	target_sources(native_simulator INTERFACE sdl_events_bottom.c)
endif()
