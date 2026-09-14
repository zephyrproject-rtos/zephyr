# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named("nrfx")

# The nrfx source directory can be override through the definition of the NRFX_DIR symbol
# during the invocation of the build system
zephyr_get(NRFX_DIR SYSBUILD GLOBAL)

if(NOT DEFINED NRFX_DIR)
  set(NRFX_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/nrfx CACHE PATH "nrfx Directory")
endif()

set(INC_DIR ${NRFX_DIR}/drivers/include)
set(SRC_DIR ${NRFX_DIR}/drivers/src)
set(HELPERS_DIR ${NRFX_DIR}/helpers)

if(NOT CONFIG_NRF_DONT_USE_STABLE_BSP)
  set(BSP_DIR ${NRFX_DIR}/bsp/stable)
  set(MDK_DIR ${BSP_DIR}/mdk)

  zephyr_include_directories(${BSP_DIR})
  zephyr_include_directories(${BSP_DIR}/templates)
endif()

zephyr_include_directories(${NRFX_DIR})
zephyr_include_directories(${INC_DIR})
zephyr_include_directories(.)

if(NOT CONFIG_NRF_DONT_USE_STABLE_BSP)
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF51
                               ${MDK_DIR}/nrf51/system_nrf51.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52805
                               ${MDK_DIR}/nrf52/nrf52805/system_nrf52805.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52810
                               ${MDK_DIR}/nrf52/nrf52810/system_nrf52810.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52811
                               ${MDK_DIR}/nrf52/nrf52811/system_nrf52811.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52820
                               ${MDK_DIR}/nrf52/nrf52820/system_nrf52820.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52832
                               ${MDK_DIR}/nrf52/system_nrf52.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52833
                               ${MDK_DIR}/nrf52/nrf52833/system_nrf52833.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF52840
                               ${MDK_DIR}/nrf52/nrf52840/system_nrf52840.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUAPP
                               ${MDK_DIR}/nrf53/nrf5340/system_nrf5340_application.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUNET
                               ${MDK_DIR}/nrf53/nrf5340/system_nrf5340_network.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF54H
                               ${MDK_DIR}/nrf54h/system_nrf54h.c)
  if(CONFIG_SOC_SERIES_NRF54L OR CONFIG_SOC_SERIES_BSIM_NRF54LX)
    zephyr_library_sources(${MDK_DIR}/nrf54l/system_nrf54l.c)
  endif()
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF71
                               ${MDK_DIR}/nrf71/system_nrf71.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF91
                               ${MDK_DIR}/nrf91/system_nrf91.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF92
                               ${MDK_DIR}/nrf92/system_nrf92.c)
endif()

zephyr_library_sources(nrfx_glue.c)
zephyr_library_sources(${HELPERS_DIR}/nrfx_flag32_allocator.c)
zephyr_library_sources_ifdef(CONFIG_HAS_NORDIC_RAM_CTRL       ${HELPERS_DIR}/nrfx_ram_ctrl.c)

if(CONFIG_NRFX_GPPI AND NOT CONFIG_NRFX_GPPI_V1)
  zephyr_library_sources_ifdef(CONFIG_HAS_HW_NRF_PPI ${HELPERS_DIR}/nrfx_gppi_ppi.c)
  zephyr_library_sources_ifndef(CONFIG_HAS_HW_NRF_PPI ${HELPERS_DIR}/nrfx_gppi_dppi.c)
  if(NOT CONFIG_NRF_DONT_USE_STABLE_BSP)
    if(CONFIG_SOC_COMPATIBLE_NRF54LX OR CONFIG_SOC_SERIES_NRF71)
      zephyr_library_sources(${BSP_DIR}/soc/interconnect/nrfx_gppi_d2ppi.c)
    endif()
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_NRFX_PRS     ${SRC_DIR}/prs/nrfx_prs.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_ADC     ${SRC_DIR}/nrfx_adc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_BELLBOARD ${SRC_DIR}/nrfx_bellboard.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK   ${SRC_DIR}/nrfx_clock.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_HFCLK      ${SRC_DIR}/nrfx_clock_hfclk.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_HFCLKAUDIO ${SRC_DIR}/nrfx_clock_hfclkaudio.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_HFCLK192M  ${SRC_DIR}/nrfx_clock_hfclk192m.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_LFCLK      ${SRC_DIR}/nrfx_clock_lfclk.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_XO         ${SRC_DIR}/nrfx_clock_xo.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK_XO24M      ${SRC_DIR}/nrfx_clock_xo24m.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_COMP    ${SRC_DIR}/nrfx_comp.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CRACEN  ${SRC_DIR}/nrfx_cracen.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_EGU     ${SRC_DIR}/nrfx_egu.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_GPIOTE  ${SRC_DIR}/nrfx_gpiote.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_GRTC    ${SRC_DIR}/nrfx_grtc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_I2S     ${SRC_DIR}/nrfx_i2s.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_IPC     ${SRC_DIR}/nrfx_ipc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_KMU     ${SRC_DIR}/nrfx_kmu.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_LPCOMP  ${SRC_DIR}/nrfx_lpcomp.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_MRAMC   ${SRC_DIR}/nrfx_mramc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_NFCT    ${SRC_DIR}/nrfx_nfct.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_NVMC    ${SRC_DIR}/nrfx_nvmc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PDM     ${SRC_DIR}/nrfx_pdm.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_POWER   ${SRC_DIR}/nrfx_power.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PWM     ${SRC_DIR}/nrfx_pwm.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_QDEC    ${SRC_DIR}/nrfx_qdec.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_QSPI    ${SRC_DIR}/nrfx_qspi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_RNG     ${SRC_DIR}/nrfx_rng.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_RRAMC   ${SRC_DIR}/nrfx_rramc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_RTC     ${SRC_DIR}/nrfx_rtc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_SAADC   ${SRC_DIR}/nrfx_saadc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_SPI     ${SRC_DIR}/nrfx_spi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_SPIM    ${SRC_DIR}/nrfx_spim.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_SPIS    ${SRC_DIR}/nrfx_spis.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_SYSTICK ${SRC_DIR}/nrfx_systick.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TBM     ${SRC_DIR}/nrfx_tbm.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TDM     ${SRC_DIR}/nrfx_tdm.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TEMP    ${SRC_DIR}/nrfx_temp.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TIMER   ${SRC_DIR}/nrfx_timer.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TWI     ${SRC_DIR}/nrfx_twi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TWIM    ${SRC_DIR}/nrfx_twim.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TWIS    ${SRC_DIR}/nrfx_twis.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_UART    ${SRC_DIR}/nrfx_uart.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_UARTE   ${SRC_DIR}/nrfx_uarte.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_USBREG  ${SRC_DIR}/nrfx_usbreg.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_WDT     ${SRC_DIR}/nrfx_wdt.c)

if(CONFIG_NRFX_TWI OR CONFIG_NRFX_TWIM)
  zephyr_library_sources(${SRC_DIR}/nrfx_twi_twim.c)
endif()

if(CONFIG_SOC_SERIES_NRF54H AND CONFIG_NRFX_GPPI_V1)
  zephyr_library_sources(${HELPERS_DIR}/internal/nrfx_gppiv1_ipct.c)
  zephyr_library_sources(${HELPERS_DIR}/internal/nrfx_gppiv1_shim.c)
endif()

# Get the SVD file for the current SoC
macro(mdk_svd_ifdef feature_toggle filename)
  if(${feature_toggle})
    set(SOC_SVD_FILE ${MDK_DIR}/${filename} CACHE FILEPATH "Path to a CMSIS-SVD file")
  endif()
endmacro()

mdk_svd_ifdef(CONFIG_SOC_SERIES_NRF51         nrf51/nrf51.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52805             nrf52/nrf52805/nrf52805.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52810             nrf52/nrf52810/nrf52810.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52811             nrf52/nrf52811/nrf52811.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52820             nrf52/nrf52820/nrf52820.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52832             nrf52/nrf52.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52833             nrf52/nrf52833/nrf52833.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52840             nrf52/nrf52840/nrf52840.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF5340_CPUAPP       nrf53/nrf5340/nrf5340_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF5340_CPUNET       nrf53/nrf5340/nrf5340_network.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUAPP      nrf54h/nrf54h20/nrf54h20_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUPPR      nrf54h/nrf54h20/nrf54h20_ppr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUFLPR     nrf54h/nrf54h20/nrf54h20_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPURAD      nrf54h/nrf54h20/nrf54h20_radiocore.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L05_CPUAPP      nrf54l/nrf54l05/nrf54l05_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L05_CPUFLPR     nrf54l/nrf54l05/nrf54l05_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L10_CPUAPP      nrf54l/nrf54l10/nrf54l10_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L10_CPUFLPR     nrf54l/nrf54l10/nrf54l10_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L15_CPUAPP      nrf54l/nrf54l15/nrf54l15_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L15_CPUFLPR     nrf54l/nrf54l15/nrf54l15_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LM20A_CPUAPP    nrf54l/nrf54lm20a/nrf54lm20a_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LM20A_CPUFLPR   nrf54l/nrf54lm20a/nrf54lm20a_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LM20B_CPUAPP    nrf54l/nrf54lm20b/nrf54lm20b_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LM20B_CPUFLPR   nrf54l/nrf54lm20b/nrf54lm20b_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP  nrf71/nrf7120_enga/nrf7120_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR nrf71/nrf7120_enga/nrf7120_enga_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120E_ENGA_CPUAPP nrf71/nrf7120e_enga/nrf7120e_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120E_ENGA_CPUFLPR nrf71/nrf7120e_enga/nrf7120e_enga_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9120              nrf91/nrf9120/nrf9120.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9160              nrf91/nrf9160/nrf9160.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUAPP  nrf92/nrf9230_engb/nrf9230_engb_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUPPR  nrf92/nrf9230_engb/nrf9230_engb_ppr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPURAD  nrf92/nrf9230_engb/nrf9230_engb_radiocore.svd)
build_info(vendor-specific nordic svdfile VALUE ${SOC_SVD_FILE})
