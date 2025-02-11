# Copyright (c) 2022 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

set(hal_dir             ${ZEPHYR_HAL_INFINEON_MODULE_DIR})
set(btstack_integration ${hal_dir}/btstack-integration)
set(hal_blobs_dir       ${hal_dir}/zephyr/blobs/img/bluetooth/firmware)
set(blob_gen_inc_file   ${ZEPHYR_BINARY_DIR}/include/generated/bt_firmware.hcd.inc)

#########################################################################################
# btstack integration assets sources for cyw208xx
#########################################################################################

if(CONFIG_BT_CYW208XX)

  zephyr_include_directories(${btstack_integration}/../btstack/wiced_include)
  zephyr_include_directories(${btstack_integration}/COMPONENT_BTSS-IPC/platform/include)
  zephyr_include_directories(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/include)

  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/cybt_platform_freertos.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/cybt_platform_hci.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/cybt_bt_task.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/cybt_host_stack_platform_interface.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/ipc/cybt_platform_task.c)

  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/common/cybt_platform_main.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/common/cybt_platform_trace.c)
  zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/common/cybt_prm.c)

  #zephyr_library_sources(${btstack_integration}/COMPONENT_BTSS-IPC/platform/common/cybt_patchram_download.c)
endif()

#########################################################################################
# BT firmware
#########################################################################################

# HCD files for CYW43xx devices
# CYW43012 modules
if(CONFIG_CYW43012_MURATA_1LV)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_43012/COMPONENT_MURATA-1LV/bt_firmware.hcd)
endif()

# CYW4343W modules
if(CONFIG_CYW4343W_MURATA_1DX)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_4343W/COMPONENT_MURATA-1DX/bt_firmware.hcd)
endif()

# CYW43439 modules
if(CONFIG_CYW43439_MURATA_1YN)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_43439/COMPONENT_MURATA-1YN/bt_firmware.hcd)
endif()

# CYW4373 modules
if(CONFIG_CYW4373_STERLING_LWB5PLUS)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/bt_firmware.hcd)
endif()

# HCD files for CYW208xx

# CYW20829 device (FW patch for 0dBm TX Power)
if(CONFIG_CYW20829_BT_FW_TX0)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-TX0/bt_firmware.hcd)
endif()

# CYW20829 device (FW patch for 10dBm TX Power)
if(CONFIG_CYW20829_BT_FW_TX10)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-TX10/bt_firmware.hcd)
endif()

# CYW20829 device (FW patch with PAwR support for 0dBm TX Power)
if(CONFIG_CYW20829_BT_FW_PAWR_TX0)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-PAWR-TX0/bt_firmware.hcd)
endif()

# CYW20829 device (FW patch with PAwR support for 10dBm TX Power)
if(CONFIG_CYW20829_BT_FW_PAWR_TX10)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-PAWR-TX10/bt_firmware.hcd)
endif()

# CYW20829 device (FW patch with ISOC support for 0dBm TX Power)
if(CONFIG_CYW20829_BT_FW_ISOC_TX0)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-ISOC-TX0/bt_firmware.hcd)
endif()

# CYW20829 device (FW patch with ISOC support for 10dBm TX Power)
if(CONFIG_CYW20829_BT_FW_ISOC_TX10)
  set(blob_hcd_file  ${hal_blobs_dir}/COMPONENT_CYW20829B0/COMPONENT_BTFW-ISOC-TX10/bt_firmware.hcd)
endif()

# use user provided FIRMWARE HCD file (path must be defined in CONFIG_AIROC_CUSTOM_FIRMWARE_HCD_BLOB)
if(CONFIG_AIROC_CUSTOM_FIRMWARE_HCD_BLOB)
  # Allowed to pass absolute path to HCD blob file, or relative path from Application folder.
  if(IS_ABSOLUTE ${CONFIG_AIROC_CUSTOM_FIRMWARE_HCD_BLOB})
    set(blob_hcd_file  ${CONFIG_AIROC_CUSTOM_FIRMWARE_HCD_BLOB})
  else()
    set(blob_hcd_file  ${APPLICATION_SOURCE_DIR}/${CONFIG_AIROC_CUSTOM_FIRMWARE_HCD_BLOB})
  endif()
endif()

# generate Bluetooth include blob from HCD binary
if(EXISTS ${blob_hcd_file})
  message(INFO " generate include of blob Bluetooth file: ${blob_hcd_file}")

  generate_inc_file_for_target(app ${blob_hcd_file} ${blob_gen_inc_file})
  zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/w_bt_firmware_controller.c)
endif()
