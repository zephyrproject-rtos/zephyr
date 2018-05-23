# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m4)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language(C ASM)

# Location of external dependencies:
set(ZEPHYR_KERNEL_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zephyr/kernel/libkernel.a")
set(PLATFORM_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/platform")

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)

target_sources(app PRIVATE src/main_master.c ${PLATFORM_DIR}/platform.c
	       ${PLATFORM_DIR}/platform_ops.c ${PLATFORM_DIR}/resource_table.c)

include($ENV{ZEPHYR_BASE}/ext/lib/ipc/open-amp/open-amp.cmake)

add_dependencies(app open-amp)

ExternalProject_Add(
  openamp_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
)
add_dependencies(core_m0_inc_target openamp_remote)

target_include_directories(app PRIVATE ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})

target_link_libraries(app
	${OPENAMP_LIBRARY}
	${ZEPHYR_KERNEL_LIBRARY})

