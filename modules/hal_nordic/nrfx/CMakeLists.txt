# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# The nrfx source directory can be override through the definition of the NRFX_DIR symbol
# during the invocation of the build system
if(NOT DEFINED NRFX_DIR)
  set(NRFX_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/nrfx CACHE PATH "nrfx Directory")
endif()

set(INC_DIR ${NRFX_DIR}/drivers/include)
set(SRC_DIR ${NRFX_DIR}/drivers/src)
set(MDK_DIR ${NRFX_DIR}/mdk)
set(HELPERS_DIR ${NRFX_DIR}/helpers)

zephyr_include_directories(${NRFX_DIR})
zephyr_include_directories(${INC_DIR})
zephyr_include_directories(${MDK_DIR})
zephyr_include_directories(.)

# Define MDK defines globally
zephyr_compile_definitions_ifdef(CONFIG_SOC_SERIES_NRF51X       NRF51)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF51822_QFAA       NRF51422_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF51822_QFAB       NRF51422_XXAB)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF51822_QFAC       NRF51422_XXAC)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52805            NRF52805_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52810            NRF52810_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52811            NRF52811_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52820            NRF52820_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52832            NRF52832_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52833            NRF52833_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF52833 NRF52833_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52840            NRF52840_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF5340_CPUAPP      NRF5340_XXAA_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP NRF5340_XXAA_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF5340_CPUNET      NRF5340_XXAA_NETWORK)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET NRF5340_XXAA_NETWORK)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54H20_CPUAPP     NRF54H20_XXAA
                                                                NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54H20_CPURAD     NRF54H20_XXAA
                                                                NRF_RADIOCORE)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54H20_CPUPPR     NRF54H20_XXAA
                                                                NRF_PPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54H20_CPUFLPR    NRF54H20_XXAA
                                                                NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54L15            NRF54L15_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54L15_CPUAPP     NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54L15_CPUFLPR    NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF54L15 NRF54L15_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF54L15_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54L20_ENGA       NRF54L20_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54L20_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9120             NRF9120_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9160             NRF9160_XXAA)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUAPP NRF9230_ENGB_XXAA
                                                                NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9230_ENGB_CPURAD NRF9230_ENGB_XXAA
                                                                NRF_RADIOCORE)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUPPR NRF9230_ENGB_XXAA
                                                                NRF_PPR)

zephyr_compile_definitions_ifdef(CONFIG_NRF_APPROTECT_LOCK
                                 ENABLE_APPROTECT)
zephyr_compile_definitions_ifdef(CONFIG_NRF_APPROTECT_USER_HANDLING
                                 ENABLE_APPROTECT_USER_HANDLING
                                 ENABLE_AUTHENTICATED_APPROTECT)
zephyr_compile_definitions_ifdef(CONFIG_NRF_SECURE_APPROTECT_LOCK
                                 ENABLE_SECURE_APPROTECT
                                 ENABLE_SECUREAPPROTECT)
zephyr_compile_definitions_ifdef(CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING
                                 ENABLE_SECURE_APPROTECT_USER_HANDLING
                                 ENABLE_AUTHENTICATED_SECUREAPPROTECT)
zephyr_library_compile_definitions_ifdef(CONFIG_NRF_TRACE_PORT
                                 ENABLE_TRACE)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF5340_CPUAPP
                                 NRF_SKIP_FICR_NS_COPY_TO_RAM)
zephyr_compile_definitions_ifdef(CONFIG_SOC_SERIES_NRF91X
                                 NRF_SKIP_FICR_NS_COPY_TO_RAM)

# Connect Kconfig compilation option for Non-Secure software with option required by MDK/nrfx
zephyr_compile_definitions_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE NRF_TRUSTZONE_NONSECURE)
zephyr_compile_definitions_ifdef(CONFIG_LOG_BACKEND_SWO ENABLE_SWO)

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF51X  ${MDK_DIR}/system_nrf51.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52805       ${MDK_DIR}/system_nrf52805.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52810       ${MDK_DIR}/system_nrf52810.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52811       ${MDK_DIR}/system_nrf52811.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52820       ${MDK_DIR}/system_nrf52820.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52832       ${MDK_DIR}/system_nrf52.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52833       ${MDK_DIR}/system_nrf52833.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF52840       ${MDK_DIR}/system_nrf52840.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUAPP ${MDK_DIR}/system_nrf5340_application.c)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUNET ${MDK_DIR}/system_nrf5340_network.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF54HX ${MDK_DIR}/system_nrf54h.c)
if(CONFIG_SOC_SERIES_NRF54LX OR CONFIG_SOC_SERIES_BSIM_NRF54LX)
  zephyr_library_sources(${MDK_DIR}/system_nrf54l.c)
endif()
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF91X  ${MDK_DIR}/system_nrf91.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF92X  ${MDK_DIR}/system_nrf92.c)

zephyr_library_sources(nrfx_glue.c)
zephyr_library_sources(${HELPERS_DIR}/nrfx_flag32_allocator.c)
zephyr_library_sources_ifdef(CONFIG_RETAINED_MEM_NRF_RAM_CTRL ${HELPERS_DIR}/nrfx_ram_ctrl.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_GPPI                 ${HELPERS_DIR}/nrfx_gppi_dppi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PPI                  ${HELPERS_DIR}/nrfx_gppi_ppi.c)

zephyr_library_sources_ifdef(CONFIG_NRFX_PRS     ${SRC_DIR}/prs/nrfx_prs.c)

zephyr_library_sources_ifdef(CONFIG_NRFX_ADC     ${SRC_DIR}/nrfx_adc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_CLOCK   ${SRC_DIR}/nrfx_clock.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_COMP    ${SRC_DIR}/nrfx_comp.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_DPPI    ${SRC_DIR}/nrfx_dppi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_EGU     ${SRC_DIR}/nrfx_egu.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_GPIOTE  ${SRC_DIR}/nrfx_gpiote.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_GRTC    ${SRC_DIR}/nrfx_grtc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_I2S     ${SRC_DIR}/nrfx_i2s.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_IPC     ${SRC_DIR}/nrfx_ipc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_LPCOMP  ${SRC_DIR}/nrfx_lpcomp.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_NFCT    ${SRC_DIR}/nrfx_nfct.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_NVMC    ${SRC_DIR}/nrfx_nvmc.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PDM     ${SRC_DIR}/nrfx_pdm.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_POWER   ${SRC_DIR}/nrfx_power.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PPI     ${SRC_DIR}/nrfx_ppi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_PPIB    ${SRC_DIR}/nrfx_ppib.c)
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

# Inject HAL "CONFIG_NFCT_PINS_AS_GPIOS" definition if user requests to
# configure the NFCT pins as GPIOS. Do the same with "CONFIG_GPIO_AS_PINRESET"
# to configure the reset GPIO as nRESET. This way, the HAL will take care of
# doing the proper configuration sequence during system init

dt_nodelabel(uicr_path NODELABEL "uicr")
if(DEFINED uicr_path)
  dt_prop(nfct_pins_as_gpios PATH ${uicr_path} PROPERTY "nfct-pins-as-gpios")
  if(${nfct_pins_as_gpios})
    zephyr_library_compile_definitions(
      CONFIG_NFCT_PINS_AS_GPIOS
      NRF_CONFIG_NFCT_PINS_AS_GPIOS
    )
  endif()

  dt_prop(gpio_as_nreset PATH ${uicr_path} PROPERTY "gpio-as-nreset")
  if(${gpio_as_nreset})
    zephyr_library_compile_definitions(CONFIG_GPIO_AS_PINRESET)
  endif()
endif()

if(CONFIG_SOC_NRF54L15_CPUAPP OR CONFIG_SOC_NRF54L20_ENGA_CPUAPP)
	dt_prop(clock_frequency PATH "/cpus/cpu@0" PROPERTY "clock-frequency")
	math(EXPR clock_frequency_mhz "${clock_frequency} / 1000000")
	zephyr_compile_definitions("NRF_CONFIG_CPU_FREQ_MHZ=${clock_frequency_mhz}")
endif()

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LX_SKIP_CLOCK_CONFIG NRF_SKIP_CLOCK_CONFIGURATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LX_DISABLE_FICR_TRIMCNF NRF_DISABLE_FICR_TRIMCNF)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LX_SKIP_GLITCHDETECTOR_DISABLE NRF_SKIP_GLITCHDETECTOR_DISABLE)

# Inject code to skip TAMPC setup for nRF54L20. It is not supported for now.
# It needs to be removed when support is provided.
if(CONFIG_SOC_NRF54L20_ENGA_CPUAPP)
  zephyr_compile_definitions(NRF_SKIP_TAMPC_SETUP)
endif()

if(CONFIG_SOC_SERIES_NRF54LX AND CONFIG_NRFX_GPPI)
  zephyr_library_sources(${HELPERS_DIR}/nrfx_gppi_dppi_ppib_lumos.c)
  zephyr_library_sources(${NRFX_DIR}/soc/interconnect/dppic_ppib/nrfx_interconnect_dppic_ppib.c)
endif()

if(CONFIG_SOC_SERIES_NRF54HX AND
   (CONFIG_DT_HAS_NORDIC_NRF_DPPIC_LOCAL_ENABLED OR
    CONFIG_DT_HAS_NORDIC_NRF_DPPIC_GLOBAL_ENABLED))
  zephyr_library_sources(${HELPERS_DIR}/nrfx_gppi_dppi_ppib.c)
  zephyr_library_sources(${NRFX_DIR}/soc/interconnect/apb/nrfx_interconnect_apb.c)
  zephyr_library_sources(${NRFX_DIR}/soc/interconnect/ipct/nrfx_interconnect_ipct.c)
endif()

# Get the SVD file for the current SoC
macro(mdk_svd_ifdef feature_toggle filename)
  if(${feature_toggle})
    set(SOC_SVD_FILE ${MDK_DIR}/${filename} CACHE FILEPATH "Path to a CMSIS-SVD file")
  endif()
endmacro()

mdk_svd_ifdef(CONFIG_SOC_SERIES_NRF51X        nrf51.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52805             nrf52805.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52810             nrf52810.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52811             nrf52811.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52820             nrf52820.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52832             nrf52.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52833             nrf52833.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF52840             nrf52840.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF5340_CPUAPP       nrf5340_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF5340_CPUNET       nrf5340_network.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUAPP      nrf54h20_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUPPR      nrf54h20_ppr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPUFLPR     nrf54h20_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54H20_CPURAD      nrf54h20_radiocore.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L15_CPUAPP      nrf54l15_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L15_CPUFLPR     nrf54l15_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54L20_ENGA_CPUAPP nrf54l20_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9120              nrf9120.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9160              nrf9160.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUAPP  nrf9230_engb_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPUPPR  nrf9230_engb_ppr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF9230_ENGB_CPURAD  nrf9230_engb_radiocore.svd)
build_info(vendor-specific nordic svdfile VALUE ${SOC_SVD_FILE})
