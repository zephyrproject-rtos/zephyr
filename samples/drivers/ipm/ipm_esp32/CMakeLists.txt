# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/ipm_esp32_appcpu-prefix/src/ipm_esp32_appcpu-build/zephyr)

if("${BOARD}" STREQUAL "esp32_devkitc_wrover/esp32/procpu")
  set(BOARD_REMOTE "esp32_devkitc_wrover/esp32/appcpu")
elseif("${BOARD}" STREQUAL "esp32_devkitc_wroom/esp32/procpu")
  set(BOARD_REMOTE "esp32_devkitc_wroom/esp32/appcpu")
elseif("${BOARD}" STREQUAL "esp32s3_devkitm/esp32s3/procpu")
  set(BOARD_REMOTE "esp32s3_devkitm/esp32s3/appcpu")
else()
  message(FATAL_ERROR "${BOARD} was not supported for this sample")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipm_esp32)

set_source_files_properties(${REMOTE_ZEPHYR_DIR}/esp32_appcpu_firmware.c PROPERTIES GENERATED TRUE)
target_sources(app PRIVATE src/main.c ${REMOTE_ZEPHYR_DIR}/esp32_appcpu_firmware.c)

include(ExternalProject)

ExternalProject_Add(
	ipm_esp32_appcpu
	SOURCE_DIR ${APPLICATION_SOURCE_DIR}/ipm_esp_appcpu
	INSTALL_COMMAND ""
	CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
	BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
	BUILD_ALWAYS True
)

add_dependencies(app ipm_esp32_appcpu)
