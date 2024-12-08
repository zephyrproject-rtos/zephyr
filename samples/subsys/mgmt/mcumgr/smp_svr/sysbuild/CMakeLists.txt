# SPDX-License-Identifier: Apache-2.0

if("${FILE_SUFFIX}" STREQUAL "ram_load")
  set(mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/nrf52840dk_nrf52840_mcuboot_ram_load.overlay" CACHE INTERNAL "" FORCE)
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
