# SPDX-License-Identifier: Apache-2.0

set(NSI_DIR ${ZEPHYR_BASE}/scripts/native_simulator)

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

set(zephyr_build_path ${CMAKE_BINARY_DIR}/zephyr)

set(nsi_config_content
  "NSI_BUILD_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_COMPILE_OPTIONS>,\ >"
  "NSI_BUILD_PATH:=${zephyr_build_path}/NSI"
  "NSI_CC:=${CMAKE_C_COMPILER}"
  "NSI_EMBEDDED_CPU_SW:=${zephyr_build_path}/${KERNEL_ELF_NAME}"
  "NSI_EXE:=${zephyr_build_path}/${KERNEL_EXE_NAME}"
  "NSI_EXTRA_SRCS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_SOURCES>,\ >"
  "NSI_LINK_OPTIONS:=$<JOIN:$<TARGET_PROPERTY:native_simulator,INTERFACE_LINK_OPTIONS>,\ >"
  "NSI_PATH:=${NSI_DIR}/"
  "NSI_NATIVE=1"
)

string(REPLACE ";" "\n" nsi_config_content "${nsi_config_content}")

file(GENERATE OUTPUT "${zephyr_build_path}/NSI/nsi_config"
  CONTENT "${nsi_config_content}"
)
