# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/ipm_esp32_net-prefix/src/ipm_esp32_net-build/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipm_esp32)

set_source_files_properties(${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c PROPERTIES GENERATED TRUE)
target_sources(app PRIVATE src/main.c ${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c)

include(ExternalProject)

ExternalProject_Add(
	ipm_esp32_net
	SOURCE_DIR ${APPLICATION_SOURCE_DIR}/ipm_esp32_net
	INSTALL_COMMAND ""
	CMAKE_CACHE_ARGS -DBOARD:STRING=esp32_net
	BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
	BUILD_ALWAYS True
)

add_dependencies(app ipm_esp32_net)
