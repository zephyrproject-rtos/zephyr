#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

if(FILE_SUFFIX STREQUAL "nrf5340_bt")
  if(SB_CONFIG_SOC_NRF5340_CPUAPP)
    # nRF5340 with Bluetooth configuration, add partition manager configuration to MCUboot image
    list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/app_nrf5340_bt.overlay)
    list(REMOVE_DUPLICATES mcuboot_EXTRA_DTC_OVERLAY_FILE)
    set(mcuboot_EXTRA_DTC_OVERLAY_FILE ${mcuboot_EXTRA_DTC_OVERLAY_FILE} CACHE INTERNAL "" FORCE)
  else()
    message(FATAL_ERROR "File suffix 'nrf5340_bt' can only be used on an nRF5340 CPUAPP board target")
  endif()
endif()
