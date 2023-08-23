# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

zephyr_library_sources(
	cmdline.c
	cpu_wait.c
	nsi_if.c
	irq_handler.c
	misc.c
	posix_arch_if.c
	)

zephyr_include_directories(
  ${NSI_DIR}/common/src/include
  ${NSI_DIR}/native/src/include
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
)

if(CONFIG_HAS_SDL)
  add_subdirectory(${ZEPHYR_BASE}/boards/${ARCH}/common/sdl/ ${CMAKE_CURRENT_BINARY_DIR}/sdl)
endif()

set(nsi_config_content
  ${nsi_config_content}
  "NSI_NATIVE=1"
)

include(../common/natsim_config.cmake)
