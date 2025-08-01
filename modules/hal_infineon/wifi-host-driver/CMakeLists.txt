# Copyright (c) 2023 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_AIROC_WIFI6)
    set(whd_wifi_ver COMPONENT_WIFI6)
else()
    set(whd_wifi_ver COMPONENT_WIFI5)
endif()

set(hal_dir                  ${ZEPHYR_HAL_INFINEON_MODULE_DIR})
set(hal_wifi_dir             ${hal_dir}/wifi-host-driver)
set(hal_wifi_dir_resources   ${hal_dir}/wifi-host-driver/WHD/${whd_wifi_ver}/resources)

set(hal_blobs_dir            ${hal_dir}/zephyr/blobs/img/whd/resources)
set(blob_gen_dir             ${ZEPHYR_BINARY_DIR}/include/generated)

set(airoc_wifi_fw_bin_gen_inc   ${blob_gen_dir}/airoc_wifi_fw_blob.inc)
set(airoc_wifi_clm_bin_gen_inc  ${blob_gen_dir}/airoc_wifi_clm_blob.inc)

#########################################################################################
# Wi-Fi Host driver
#########################################################################################
if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
  zephyr_compile_definitions(WLAN_MFG_FIRMWARE)
endif()

zephyr_compile_definitions(CY_WIFI_COUNTRY=$<UPPER_CASE:${CONFIG_WHD_WIFI_COUNTRY}>)

if(CONFIG_WHD_DISABLE_SDIO_PULLUP_DURING_SPI_SLEEP)
  zephyr_compile_definitions(WHD_DISABLE_SDIO_PULLUP_DURING_SPI_SLEEP)
endif()

# Add WHD includes
zephyr_include_directories(${hal_wifi_dir})
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/inc)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/include)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/resources/resource_imp)

# src
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_ap.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_buffer_api.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_cdc_bdc.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_chip.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_chip_constants.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_clm.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_debug.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_events.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_logging.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_management.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_network_if.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_proto.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_resource_if.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_sdpcm.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_thread.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_utils.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_wifi.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_wifi_api.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_wifi_p2p.c)

# src/bus_protocols
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols/whd_bus.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols/whd_bus_common.c)
zephyr_library_sources_ifdef(CONFIG_AIROC_WIFI_BUS_SDIO  ${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols/whd_bus_sdio_protocol.c)
zephyr_library_sources_ifdef(CONFIG_AIROC_WIFI_BUS_SPI  ${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols/whd_bus_spi_protocol.c)

# resources/resource_imp
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/resources/resource_imp/whd_resources.c)

############################################################################################################
## WIFI 5 devices
############################################################################################################

# Specific defines for CYW43022 devices
if(CONFIG_CYW43022)
  zephyr_compile_definitions(BLHS_SUPPORT)
  zephyr_compile_definitions(ULP_SUPPORT)
  zephyr_compile_definitions(DM_43022C1)
endif()

# CYW43012 firmware
if(CONFIG_CYW43012 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0_bin.c)
  endif()
endif()

# CYW43022 firmware
if(CONFIG_CYW43022 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43022/COMPONENT_SM/43022C1-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43022/COMPONENT_SM/43022C1_bin.c)
  endif()
endif()

# CYW4343W firmware
if(CONFIG_CYW4343W AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1_bin.c)
  endif()
endif()

# CYW43438 firmware/clm
if(CONFIG_CYW43438 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware/clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1-mfgtest_bin.c)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43438/43438A1-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1_bin.c)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43438/43438A1_clm_blob.c)
  endif()

endif()

# CYW43439 firmware
if(CONFIG_CYW43439 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43439/43439A0-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_43439/43439a0_bin.c)
  endif()
endif()

# CYW4373 firmware
if(CONFIG_CYW4373 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0_bin.c)
  endif()
endif()

# CYW43012_MURATA_1LV clm/nvram
if(CONFIG_CYW43012_MURATA_1LV AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43012/43012C0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43012/43012C0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43012/COMPONENT_MURATA-1LV)
endif()

# CYW43022CUB clm/nvram
if(CONFIG_CYW43022CUB AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43022/43022C1-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43022/43022C1_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43022/COMPONENT_CYW43022CUB)
endif()

# CYW4343W_MURATA_1DX clm/nvram
if(CONFIG_CYW4343W_MURATA_1DX AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4343W/4343WA1-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4343W/4343WA1_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_4343W/COMPONENT_MURATA-1DX)
endif()

# CYW43439_MURATA_1YN
if(CONFIG_CYW43439_MURATA_1YN AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43439/43439A0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43439/43439A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43439/COMPONENT_MURATA-1YN)
endif()

# CYW43439_STERLING_LWBPLUS
if(CONFIG_CYW43439_STERLING_LWBPLUS AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43439/43439A0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_43439/43439A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_43439/COMPONENT_STERLING-LWBplus)
endif()

# CYW4373_STERLING_LWB5PLUS
if(CONFIG_CYW4373_STERLING_LWB5PLUS AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_STERLING-LWB5plus)
endif()

# CYW4373_MURATA-2AE
if(CONFIG_CYW4373_MURATA_2AE AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2AE/4373A0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2AE/4373A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_MURATA-2AE)
endif()

# CYW4373_MURATA-2BC
if(CONFIG_CYW4373_MURATA_2BC AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2BC/4373A0-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2BC/4373A0_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_MURATA-2BC)
endif()

############################################################################################################
## WIFI 6 devices
############################################################################################################

# Specific defines for CYW555xx devices
if(CONFIG_CYW55500 OR CONFIG_CYW55572)
  zephyr_compile_definitions(BLHS_SUPPORT)
endif()

# CYW55500 firmware
if(CONFIG_CYW55500 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_55500/COMPONENT_SM/55500A1-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_55500/COMPONENT_SM/55500A1_bin.c)
  endif()
endif()

# CYW55572 firmware
if(CONFIG_CYW55572 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # firmware
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_55572/COMPONENT_SM/55572A1-mfgtest_bin.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/firmware/COMPONENT_55572/COMPONENT_SM/55572A1_bin.c)
  endif()
endif()


# CYW955513SDM2WLIPA_SM
if(CONFIG_CYW955513SDM2WLIPA_SM AND NOT CONFIG_AIROC_WIFI_CUSTOM)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_55500/55500A1-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_55500/55500A1_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_55500/COMPONENT_CYW955513SDM2WLIPA)
endif()

# CYW955573M2IPA1_SM
if(CONFIG_CYW955573M2IPA1_SM AND NOT CONFIG_AIROC_WIFI_CUSTOM)

  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_55572/55572A1-mfgtest_clm_blob.c)
  else()
    zephyr_library_sources(${hal_blobs_dir}/clm/COMPONENT_55572/55572A1_clm_blob.c)
  endif()

  # nvram
  zephyr_include_directories(${hal_wifi_dir_resources}/nvram/COMPONENT_55572/COMPONENT_CYW955573M2IPA1)
endif()

# generate FW inc_blob from fw_bin
if(EXISTS ${airoc_wifi_fw_bin})
  message(INFO " generate include of blob FW file: ${airoc_wifi_fw_bin}")
  generate_inc_file_for_target(app ${airoc_wifi_fw_bin} ${airoc_wifi_fw_bin_gen_inc})
endif()

# generate CLM inc_blob from clm_bin
if(EXISTS ${airoc_wifi_clm_bin})
  message(INFO " generate include of blob CLM file: ${airoc_wifi_clm_bin}")
  generate_inc_file_for_target(app ${airoc_wifi_clm_bin} ${airoc_wifi_clm_bin_gen_inc})
endif()
