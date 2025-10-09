# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_AIROC_WIFI6)
    set(whd_wifi_ver COMPONENT_WIFI6)
else()
    set(whd_wifi_ver COMPONENT_WIFI5)
endif()

set(hal_dir                  ${ZEPHYR_HAL_INFINEON_MODULE_DIR})
set(hal_wifi_dir             ${hal_dir}/whd-expansion)
set(hal_wifi_dir_resources   ${hal_dir}/whd-expansion/WHD/${whd_wifi_ver}/resources)

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

zephyr_compile_definitions(WHD_ZEPHYR)

# Add WHD includes
zephyr_include_directories(${hal_wifi_dir})
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/inc)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/include)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/resources)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols)
zephyr_include_directories(${hal_wifi_dir}/WHD/${whd_wifi_ver}/resources/resource_imp)

# common/inc
zephyr_include_directories(${hal_wifi_dir}/WHD/COMMON/inc)

# common/src
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_bus_common.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_cdc_bdc.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_clm.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_sdpcm.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_utils.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_wifi.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_logging.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/COMMON/src/whd_debug.c)

# src
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_ap.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_buffer_api.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_chip.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_chip_constants.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_events.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_management.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_network_if.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_proto.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_resource_if.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_thread.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_wifi_api.c)
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/whd_wifi_p2p.c)

# src/bus_protocols
zephyr_library_sources(${hal_wifi_dir}/WHD/${whd_wifi_ver}/src/bus_protocols/whd_bus.c)
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
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0-mfgtest.bin)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43012/43012C0.bin)
  endif()
endif()

# CYW43022 firmware
if(CONFIG_CYW43022 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43022/COMPONENT_SM/43022C1-mfgtest.trxs)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43022/COMPONENT_SM/43022C1.trxs)
  endif()
endif()

# CYW4343W firmware
if(CONFIG_CYW4343W AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1-mfgtest.bin)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_4343W/4343WA1.bin)
  endif()
endif()

# CYW43438 firmware
if(CONFIG_CYW43438 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1-mfgtest.bin)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43438/43438A1.bin)
  endif()
endif()

# CYW43439 firmware
if(CONFIG_CYW43439 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43439/43439A0-mfgtest.bin)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_43439/43439A0.bin)
  endif()
endif()

# CYW4373 firmware
if(CONFIG_CYW4373 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0-mfgtest.bin)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_4373/4373A0.bin)
  endif()
endif()

# CYW43012_MURATA_1LV clm/nvram
if(CONFIG_CYW43012_MURATA_1LV AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43012/COMPONENT_MURATA-1LV/43012C0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43012/COMPONENT_MURATA-1LV/43012C0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_43012/COMPONENT_MURATA-1LV/cyw9cy8ckit_062S2_43012.txt)
endif()

# CYW43022CUB clm/nvram
if(CONFIG_CYW43022CUB AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43022/COMPONENT_CYW43022CUB/43022C1-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43022/COMPONENT_CYW43022CUB/43022C1.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_43022/COMPONENT_CYW43022CUB/cyw943022sdm2wlipa_rev3.1.txt)
endif()

# CYW4343W_MURATA_1DX clm/nvram
if(CONFIG_CYW4343W_MURATA_1DX AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4343W/COMPONENT_MURATA-1DX/4343WA1-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4343W/COMPONENT_MURATA-1DX/4343WA1.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_4343W/COMPONENT_MURATA-1DX/cyw94343cy8cmod.txt)
endif()

# CYW43439_MURATA_1YN clm/nvram
if(CONFIG_CYW43439_MURATA_1YN AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43439/COMPONENT_MURATA-1YN/43439A0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43439/COMPONENT_MURATA-1YN/43439A0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_43439/COMPONENT_MURATA-1YN/cyfmac43439-1YN.txt)
endif()

# CYW43439_STERLING_LWBPLUS
if(CONFIG_CYW43439_STERLING_LWBPLUS AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43439/COMPONENT_STERLING-LWBplus/43439A0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_43439/COMPONENT_STERLING-LWBplus/43439A0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_43439/COMPONENT_STERLING-LWBplus/cyfmac43439-1YN.txt)
endif()

# CYW4373_STERLING_LWB5PLUS
if(CONFIG_CYW4373_STERLING_LWB5PLUS AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/4373A0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_STERLING-LWB5plus/cyfmac4373-2AE.txt)
endif()

# CYW4373_MURATA-2AE
if(CONFIG_CYW4373_MURATA_2AE AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2AE/4373A0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2AE/4373A0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_MURATA-2AE/cyfmac4373-2AE.txt)
endif()

# CYW4373_MURATA-2BC
if(CONFIG_CYW4373_MURATA_2BC AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2BC/4373A0-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_4373/COMPONENT_MURATA-2BC/4373A0.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_4373/COMPONENT_MURATA-2BC/cyfmac4373-2BC.txt)
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
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_55500/COMPONENT_SM/55500A1-mfgtest.trxcse)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_55500/COMPONENT_SM/55500A1.trxcse)
  endif()
endif()

# CYW55572 firmware
if(CONFIG_CYW55572 AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_55572/COMPONENT_SM/55572A1-mfgtest.trxse)
  else()
    set(FW_IMAGE_NAME ${hal_blobs_dir}/firmware/COMPONENT_55572/COMPONENT_SM/55572A1-mfgtest.trxse)
  endif()
endif()

# CYW955513SDM2WLIPA_SM clm, nvram
if(CONFIG_CYW955513SDM2WLIPA_SM AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55500/COMPONENT_CYW955513SDM2WLIPA/55500A1-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55500/COMPONENT_CYW955513SDM2WLIPA/55500A1.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_55500/COMPONENT_CYW955513SDM2WLIPA/cyw955513sdm2wlipa.txt)
endif()

# CYW55513IUBG_SM clm, nvram
if(CONFIG_CYW55513IUBG_SM AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55500/COMPONENT_CYW55513IUBG/55500A1-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55500/COMPONENT_CYW55513IUBG/55500A1.clm_blob)
  endif()
  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_55500/COMPONENT_CYW55513IUBG/cyw955513wlipa_rev103.txt)
endif()

# CYW955573M2IPA1_SM
if(CONFIG_CYW955573M2IPA1_SM AND NOT CONFIG_AIROC_WIFI_CUSTOM)
  # clm
  if(CONFIG_AIROC_WLAN_MFG_FIRMWARE)
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55572/COMPONENT_CYW955573M2IPA1/55572A1-mfgtest.clm_blob)
  else()
    set(CLM_IMAGE_NAME ${hal_blobs_dir}/clm/COMPONENT_55572/COMPONENT_CYW955573M2IPA1/55572A1.clm_blob)
  endif()

  # nvram
  set(NVRAM_IMAGE_NAME ${hal_wifi_dir_resources}/nvram/COMPONENT_55572/COMPONENT_CYW955573M2IPA1/cyw955573m2ipa1_rev1.01.txt)
endif()

# set fw size
file(SIZE ${FW_IMAGE_NAME} FW_IMAGE_SIZE)
zephyr_compile_definitions(FW_IMAGE_NAME="${FW_IMAGE_NAME}")
zephyr_compile_definitions(FW_IMAGE_SIZE=${FW_IMAGE_SIZE})

# set clm size
file(SIZE ${CLM_IMAGE_NAME} CLM_IMAGE_SIZE)
zephyr_compile_definitions(CLM_IMAGE_NAME="${CLM_IMAGE_NAME}")
zephyr_compile_definitions(CLM_IMAGE_SIZE=${CLM_IMAGE_SIZE})

# set nvram size
file(SIZE ${NVRAM_IMAGE_NAME} NVRAM_IMAGE_SIZE)
zephyr_compile_definitions(NVRAM_IMAGE_NAME="${NVRAM_IMAGE_NAME}")
zephyr_compile_definitions(NVRAM_IMAGE_SIZE=${NVRAM_IMAGE_SIZE})

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
