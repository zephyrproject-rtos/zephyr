# Copyright (c) 2023 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

set(hal_dir                  ${ZEPHYR_HAL_INFINEON_MODULE_DIR})
set(hal_wifi_dir             ${hal_dir}/wifi-host-driver)
set(hal_wifi_dir_resources   ${hal_dir}/wifi-host-driver/WiFi_Host_Driver/resources)

set(hal_blobs_dir            ${hal_dir}/zephyr/blobs/img/whd/resources)
set(blob_gen_dir             ${ZEPHYR_BINARY_DIR}/include/generated)

set(cyw43xx_fw_bin_gen_inc   ${blob_gen_dir}/cyw43xx_fw_blob.inc)
set(cyw43xx_clm_bin_gen_inc  ${blob_gen_dir}/cyw43xx_clm_blob.inc)

#########################################################################################
# Wi-Fi Host driver
#########################################################################################
if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
  zephyr_compile_definitions(WLAN_MFG_FIRMWARE)
endif()

# Add WHD includes
zephyr_include_directories(${hal_wifi_dir})
zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/inc)
zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/src)
zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/src/include)
zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/resources/resource_imp)

# src
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_ap.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_buffer_api.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_cdc_bdc.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_chip.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_chip_constants.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_clm.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_debug.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_events.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_logging.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_management.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_network_if.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_resource_if.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_sdpcm.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_thread.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_utils.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_wifi.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_wifi_api.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/whd_wifi_p2p.c)

# src/bus_protocols
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/bus_protocols/whd_bus.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/bus_protocols/whd_bus_common.c)
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/src/bus_protocols/whd_bus_sdio_protocol.c)

# resources/resource_imp
zephyr_library_sources(${hal_wifi_dir}/WiFi_Host_Driver/resources/resource_imp/whd_resources.c)

# CYW43012 firmware
if(CONFIG_CYW43012 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/resources/firmware/COMPONENT_43012)

  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0-mfgtest.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43012/43012C0-mfgtest_bin.c)
  else()
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43012/43012C0_bin.c)
  endif()
endif()

# CYW4343W firmware
if(CONFIG_CYW4343W AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/firmware/COMPONENT_4343W)

  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1-mfgtest.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_4343W/4343WA1-mfgtest_bin.c)
  else()
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_4343W/4343WA1_bin.c)
  endif()
endif()

# CYW43438 firmware/clm
if(CONFIG_CYW43438 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/firmware/COMPONENT_43438)
  zephyr_include_directories(${hal_wifi_dir_resources}/clm/COMPONENT_43438)

  # firmware/clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1-mfgtest.bin)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43438/43438A1-mfgtest.clm_blob)

    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43438/43438A1-mfgtest_bin.c)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43438/43438A1-mfgtest_clm_blob.c)
  else()
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1.bin)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43438/43438A1.clm_blob)

    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43438/43438A1_bin.c)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43438/43438A1_clm_blob.c)
  endif()

endif()

# CYW43439 firmware
if(CONFIG_CYW43439 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/firmware/COMPONENT_43439)

  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43439/43439A0-mfgtest.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43439/43439A0-mfgtest_bin.c)
  else()
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_43439/43439A0.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_43439/43439A0_bin.c)
  endif()
endif()

# CYW4373 firmware
if(CONFIG_CYW4373 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/firmware/COMPONENT_4373)

  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0-mfgtest.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_4373/4373A0-mfgtest_bin.c)
  else()
    set(cyw43xx_fw_bin     ${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0.bin)
    zephyr_library_sources(${hal_wifi_dir_resources}/firmware/COMPONENT_4373/4373A0_bin.c)
  endif()
endif()


# CYW43012_MURATA_1LV clm/nvram
if(CONFIG_CYW43012_MURATA_1LV AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir}/WiFi_Host_Driver/resources/clm/COMPONENT_43012)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43012/43012C0-mfgtest.clm_blob)

    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43012/43012C0-mfgtest_clm_blob.c)
  else()
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43012/43012C0.clm_blob)

    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43012/43012C0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43012/COMPONENT_MURATA-1LV)
endif()

# CYW4343W_MURATA_1DX clm/nvram
if(CONFIG_CYW4343W_MURATA_1DX AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/clm/COMPONENT_4343W)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_4343W/4343WA1-mfgtest.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_4343W/4343WA1-mfgtest_clm_blob.c)
  else()
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_4343W/4343WA1.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_4343W/4343WA1_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_4343W/COMPONENT_MURATA-1DX)
endif()

# CYW43439_MURATA_1YN
if(CONFIG_CYW43439_MURATA_1YN AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/clm/COMPONENT_43439)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43439/43439A0-mfgtest.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43439/43439A0-mfgtest_clm_blob.c)
  else()
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_43439/43439A0.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_43439/43439A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43439/COMPONENT_MURATA-1YN)
endif()

# CYW4373_STERLING_LWB5PLUS
if(CONFIG_CYW4373_STERLING_LWB5PLUS AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  zephyr_include_directories(${hal_wifi_dir_resources}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0-mfgtest.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0-mfgtest_clm_blob.c)
  else()
    set(cyw43xx_clm_bin    ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0.clm_blob)
    zephyr_library_sources(${hal_wifi_dir_resources}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories_ifdef(${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_STERLING-LWB5plus)
endif()

# generate FW inc_blob from fw_bin
if(EXISTS ${cyw43xx_fw_bin})
  message(INFO " generate include of blob FW file: ${cyw43xx_fw_bin}")
  generate_inc_file_for_target(app ${cyw43xx_fw_bin} ${cyw43xx_fw_bin_gen_inc})
endif()

# generate CLM inc_blob from clm_bin
if(EXISTS ${cyw43xx_clm_bin})
  message(INFO " generate include of blob CLM file: ${cyw43xx_clm_bin}")
  generate_inc_file_for_target(app ${cyw43xx_clm_bin} ${cyw43xx_clm_bin_gen_inc})
endif()
