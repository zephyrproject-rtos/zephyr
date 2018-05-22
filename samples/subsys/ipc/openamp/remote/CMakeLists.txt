# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m0)

set(ZEPHYR_KERNEL_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zephyr/kernel/libkernel.a")
set(PLATFORM_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main_remote.c ${PLATFORM_DIR}/platform.c ${PLATFORM_DIR}/resource_table.c ${PLATFORM_DIR}/platform_ops.c)

include($ENV{ZEPHYR_BASE}/ext/lib/ipc/open-amp.cmake)

target_include_directories(app PRIVATE ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})

add_dependencies(app open-amp)

target_link_libraries(app
	${OPENAMP_LIBRARY}
	${ZEPHYR_KERNEL_LIBRARY})

