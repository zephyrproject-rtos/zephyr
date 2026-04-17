# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_INFINEON_EDGE)
if(CONFIG_BUILD_WITH_TFM)
  set(zephyr_ifx_cycfg_dir ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/platform/ext/target/infineon/pse84/epc2/board/KIT_PSOCE84_EVK/shared/design/default/GeneratedSource)

  zephyr_include_directories(${zephyr_ifx_cycfg_dir})
  zephyr_library_sources(${zephyr_ifx_cycfg_dir}/cycfg_qspi_memslot.c)

  zephyr_include_directories(${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/platform/ext/target/infineon/pse84/shared/device/include)

  set(zephyr_ifx_cy_dev_headers_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/kit_pse84_eval/device_headers)
  zephyr_include_directories(${zephyr_ifx_cy_dev_headers_dir})
else()

  set(zephyr_ifx_cycfg_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/kit_pse84_eval)

  zephyr_include_directories(${zephyr_ifx_cycfg_dir})
  zephyr_include_directories(${zephyr_ifx_cycfg_dir}/device_headers)

  # Select the correct external flash component configuration based on the
  # compatible string of the flash node (i.e. "infineon,s25fs128s").
  dt_comp_path(flash_s25fs128s_path COMPATIBLE "infineon,s25fs128s" INDEX 0)
  dt_comp_path(flash_s25hs512t_path COMPATIBLE "infineon,s25hs512t" INDEX 0)

  if(DEFINED flash_s25fs128s_path)
    zephyr_library_sources(${zephyr_ifx_cycfg_dir}/cycfg_qspi_memslot_s25fs128s.c)
  elseif(DEFINED flash_s25hs512t_path)
    zephyr_library_sources(${zephyr_ifx_cycfg_dir}/cycfg_qspi_memslot_s25hs512t.c)
  else()
    message(FATAL_ERROR "No recognised external flash compatible found.")
  endif()

  zephyr_library_sources(${zephyr_ifx_cycfg_dir}/ifx_cycfg_init.c)
  if(CONFIG_USE_INFINEON_AUTANALOG_SAR_ADC AND NOT CONFIG_MFD_INFINEON_AUTANALOG)
    zephyr_library_sources(${zephyr_ifx_cycfg_dir}/ifx_autanalog.c)
  endif()
endif()
endif()

if(CONFIG_SOC_SERIES_PSC3)
  set(zephyr_ifx_cycfg_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr-ifx-cycfg/soc_psc3)

  zephyr_include_directories(${zephyr_ifx_cycfg_dir})
  zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_HPPASS_SAR_ADC ${zephyr_ifx_cycfg_dir}/ifx_hppass_analog.c)
  zephyr_library_sources(${zephyr_ifx_cycfg_dir}/ifx_cycfg_init.c)
endif()
