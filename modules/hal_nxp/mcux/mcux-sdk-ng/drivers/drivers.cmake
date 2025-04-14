# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

# fsl_common driver
zephyr_library_sources(${MCUX_SDK_NG_DIR}/drivers/common/fsl_common.c
                       ${MCUX_SDK_NG_DIR}/drivers/common/fsl_common_arm.c)
zephyr_include_directories(${MCUX_SDK_NG_DIR}/drivers/common)

set_variable_ifdef(CONFIG_HWINFO_MCUX_SRC_V2    CONFIG_MCUX_COMPONENT_driver.src_2)
set_variable_ifdef(CONFIG_GPIO_MCUX_IGPIO       CONFIG_MCUX_COMPONENT_driver.igpio)
set_variable_ifdef(CONFIG_ADC_MCUX_LPADC        CONFIG_MCUX_COMPONENT_driver.lpadc)
set_variable_ifdef(CONFIG_COUNTER_MCUX_CTIMER   CONFIG_MCUX_COMPONENT_driver.ctimer)
set_variable_ifdef(CONFIG_COUNTER_MCUX_LPC_RTC  CONFIG_MCUX_COMPONENT_driver.lpc_rtc)
set_variable_ifdef(CONFIG_GLIKEY_MCUX_GLIKEY    CONFIG_MCUX_COMPONENT_driver.glikey)

if(CONFIG_NXP_LP_FLEXCOMM)
  set_variable_ifdef(CONFIG_I2C_MCUX_LPI2C      CONFIG_MCUX_COMPONENT_driver.lpflexcomm)
  set_variable_ifdef(CONFIG_I2C_MCUX_LPI2C      CONFIG_MCUX_COMPONENT_driver.lpflexcomm_lpi2c)
  set_variable_ifdef(CONFIG_UART_MCUX_LPUART    CONFIG_MCUX_COMPONENT_driver.lpflexcomm)
  set_variable_ifdef(CONFIG_UART_MCUX_LPUART    CONFIG_MCUX_COMPONENT_driver.lpflexcomm_lpuart)
  set_variable_ifdef(CONFIG_SPI_MCUX_LPSPI      CONFIG_MCUX_COMPONENT_driver.lpflexcomm)
  set_variable_ifdef(CONFIG_SPI_MCUX_LPSPI      CONFIG_MCUX_COMPONENT_driver.lpflexcomm_lpspi)
else()
  set_variable_ifdef(CONFIG_I2C_MCUX_LPI2C      CONFIG_MCUX_COMPONENT_driver.lpi2c)
  set_variable_ifdef(CONFIG_UART_MCUX_LPUART    CONFIG_MCUX_COMPONENT_driver.lpuart)
  set_variable_ifdef(CONFIG_SPI_MCUX_LPSPI      CONFIG_MCUX_COMPONENT_driver.lpspi)
endif()

set_variable_ifdef(CONFIG_DMA_MCUX_LPC          CONFIG_MCUX_COMPONENT_driver.lpc_dma)
set_variable_ifdef(CONFIG_GPIO_MCUX_LPC         CONFIG_MCUX_COMPONENT_driver.lpc_gpio)
set_variable_ifdef(CONFIG_NXP_PINT              CONFIG_MCUX_COMPONENT_driver.pint)
set_variable_ifdef(CONFIG_NXP_PINT              CONFIG_MCUX_COMPONENT_driver.inputmux)
set_variable_ifdef(CONFIG_DMA_MCUX_SMARTDMA     CONFIG_MCUX_COMPONENT_driver.inputmux)
set_variable_ifdef(CONFIG_I2C_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm)
set_variable_ifdef(CONFIG_I2C_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm_i2c)
set_variable_ifdef(CONFIG_I2S_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm)
set_variable_ifdef(CONFIG_I2S_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm_i2s)
set_variable_ifdef(CONFIG_SPI_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm)
set_variable_ifdef(CONFIG_SPI_MCUX_FLEXCOMM     CONFIG_MCUX_COMPONENT_driver.flexcomm_spi)
set_variable_ifdef(CONFIG_UART_MCUX_FLEXCOMM    CONFIG_MCUX_COMPONENT_driver.flexcomm)
set_variable_ifdef(CONFIG_UART_MCUX_FLEXCOMM    CONFIG_MCUX_COMPONENT_driver.flexcomm_usart)
set_variable_ifdef(CONFIG_MCUX_OS_TIMER         CONFIG_MCUX_COMPONENT_driver.ostimer)
set_variable_ifdef(CONFIG_PWM_MCUX_SCTIMER      CONFIG_MCUX_COMPONENT_driver.sctimer)
set_variable_ifdef(CONFIG_PWM_MCUX_CTIMER       CONFIG_MCUX_COMPONENT_driver.ctimer)
set_variable_ifdef(CONFIG_SOC_FLASH_LPC         CONFIG_MCUX_COMPONENT_driver.flashiap)
set_variable_ifdef(CONFIG_WDT_MCUX_WWDT         CONFIG_MCUX_COMPONENT_driver.wwdt)
set_variable_ifdef(CONFIG_ADC_MCUX_ADC12        CONFIG_MCUX_COMPONENT_driver.adc12)
set_variable_ifdef(CONFIG_ADC_MCUX_ADC16        CONFIG_MCUX_COMPONENT_driver.adc16)
set_variable_ifdef(CONFIG_CAN_MCUX_FLEXCAN      CONFIG_MCUX_COMPONENT_driver.flexcan)
set_variable_ifdef(CONFIG_CAN_MCUX_FLEXCAN_FD   CONFIG_MCUX_COMPONENT_driver.flexcan)
set_variable_ifdef(CONFIG_COUNTER_NXP_PIT       CONFIG_MCUX_COMPONENT_driver.pit)
set_variable_ifdef(CONFIG_COUNTER_MCUX_RTC      CONFIG_MCUX_COMPONENT_driver.rtc)
set_variable_ifdef(CONFIG_DAC_MCUX_DAC          CONFIG_MCUX_COMPONENT_driver.dac)
set_variable_ifdef(CONFIG_DAC_MCUX_DAC32        CONFIG_MCUX_COMPONENT_driver.dac32)
set_variable_ifdef(CONFIG_DMA_MCUX_EDMA         CONFIG_MCUX_COMPONENT_driver.dmamux)
set_variable_ifdef(CONFIG_DMA_MCUX_EDMA_V3      CONFIG_MCUX_COMPONENT_driver.dmamux)
set_variable_ifdef(CONFIG_DMA_MCUX_EDMA         CONFIG_MCUX_COMPONENT_driver.edma)
set_variable_ifdef(CONFIG_DMA_MCUX_EDMA_V3      CONFIG_MCUX_COMPONENT_driver.dma3)
set_variable_ifdef(CONFIG_DMA_MCUX_EDMA_V4      CONFIG_MCUX_COMPONENT_driver.edma4)
set_variable_ifdef(CONFIG_ENTROPY_MCUX_RNGA     CONFIG_MCUX_COMPONENT_driver.rnga)
set_variable_ifdef(CONFIG_ENTROPY_MCUX_TRNG     CONFIG_MCUX_COMPONENT_driver.trng)
set_variable_ifdef(CONFIG_ENTROPY_MCUX_CAAM     CONFIG_MCUX_COMPONENT_driver.caam)
set_variable_ifdef(CONFIG_ETH_NXP_ENET          CONFIG_MCUX_COMPONENT_driver.enet)
set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC      CONFIG_MCUX_COMPONENT_driver.netc)
set_variable_ifdef(CONFIG_HAS_MCUX_SMC          CONFIG_MCUX_COMPONENT_driver.smc)
set_variable_ifdef(CONFIG_I2C_MCUX              CONFIG_MCUX_COMPONENT_driver.i2c)
set_variable_ifdef(CONFIG_I2C_NXP_II2C          CONFIG_MCUX_COMPONENT_driver.ii2c)
set_variable_ifdef(CONFIG_I3C_MCUX              CONFIG_MCUX_COMPONENT_driver.i3c)
set_variable_ifdef(CONFIG_SENSOR_MCUX_ACMP      CONFIG_MCUX_COMPONENT_driver.acmp)
set_variable_ifdef(CONFIG_COMPARATOR_MCUX_ACMP  CONFIG_MCUX_COMPONENT_driver.acmp)
set_variable_ifdef(CONFIG_PWM_MCUX_FTM          CONFIG_MCUX_COMPONENT_driver.ftm)
set_variable_ifdef(CONFIG_PWM_MCUX_TPM          CONFIG_MCUX_COMPONENT_driver.tpm)
set_variable_ifdef(CONFIG_COUNTER_MCUX_TPM      CONFIG_MCUX_COMPONENT_driver.tpm)
set_variable_ifdef(CONFIG_PWM_MCUX_PWT          CONFIG_MCUX_COMPONENT_driver.pwt)
set_variable_ifdef(CONFIG_COUNTER_MCUX_QTMR     CONFIG_MCUX_COMPONENT_driver.qtmr_1)
set_variable_ifdef(CONFIG_PWM_MCUX_QTMR         CONFIG_MCUX_COMPONENT_driver.qtmr_1)
set_variable_ifdef(CONFIG_SPI_MCUX_DSPI         CONFIG_MCUX_COMPONENT_driver.dspi)
set_variable_ifdef(CONFIG_SPI_MCUX_ECSPI        CONFIG_MCUX_COMPONENT_driver.ecspi)
set_variable_ifdef(CONFIG_MCUX_FLEXIO           CONFIG_MCUX_COMPONENT_driver.flexio)
set_variable_ifdef(CONFIG_SPI_MCUX_FLEXIO       CONFIG_MCUX_COMPONENT_driver.flexio_spi)
set_variable_ifdef(CONFIG_UART_MCUX             CONFIG_MCUX_COMPONENT_driver.uart)
set_variable_ifdef(CONFIG_UART_MCUX_LPSCI       CONFIG_MCUX_COMPONENT_driver.lpsci)
set_variable_ifdef(CONFIG_WDT_MCUX_WDOG         CONFIG_MCUX_COMPONENT_driver.wdog)
set_variable_ifdef(CONFIG_WDT_MCUX_WDOG32       CONFIG_MCUX_COMPONENT_driver.wdog32)
set_variable_ifdef(CONFIG_COUNTER_MCUX_GPT      CONFIG_MCUX_COMPONENT_driver.gpt)
set_variable_ifdef(CONFIG_MCUX_GPT_TIMER        CONFIG_MCUX_COMPONENT_driver.gpt)
set_variable_ifdef(CONFIG_DISPLAY_MCUX_ELCDIF   CONFIG_MCUX_COMPONENT_driver.elcdif)
set_variable_ifdef(CONFIG_MCUX_PXP              CONFIG_MCUX_COMPONENT_driver.pxp)
set_variable_ifdef(CONFIG_LV_USE_GPU_NXP_PXP    CONFIG_MCUX_COMPONENT_driver.pxp)
set_variable_ifdef(CONFIG_GPIO_MCUX_RGPIO       CONFIG_MCUX_COMPONENT_driver.rgpio)
set_variable_ifdef(CONFIG_I2S_MCUX_SAI          CONFIG_MCUX_COMPONENT_driver.sai)
set_variable_ifdef(CONFIG_DAI_NXP_SAI           CONFIG_MCUX_COMPONENT_driver.sai)
set_variable_ifdef(CONFIG_MEMC_MCUX_FLEXSPI     CONFIG_MCUX_COMPONENT_driver.flexspi)
set_variable_ifdef(CONFIG_PWM_MCUX              CONFIG_MCUX_COMPONENT_driver.pwm)
set_variable_ifdef(CONFIG_VIDEO_MCUX_CSI        CONFIG_MCUX_COMPONENT_driver.csi)
set_variable_ifdef(CONFIG_WDT_MCUX_IMX_WDOG     CONFIG_MCUX_COMPONENT_driver.wdog01)
set_variable_ifdef(CONFIG_WDT_MCUX_RTWDOG       CONFIG_MCUX_COMPONENT_driver.rtwdog)
set_variable_ifdef(CONFIG_HAS_MCUX_RDC          CONFIG_MCUX_COMPONENT_driver.rdc)
set_variable_ifdef(CONFIG_UART_MCUX_IUART       CONFIG_MCUX_COMPONENT_driver.iuart)
set_variable_ifdef(CONFIG_ADC_MCUX_12B1MSPS_SAR CONFIG_MCUX_COMPONENT_driver.adc_12b1msps_sar)
set_variable_ifdef(CONFIG_HWINFO_MCUX_SRC       CONFIG_MCUX_COMPONENT_driver.src)
set_variable_ifdef(CONFIG_HWINFO_MCUX_SIM       CONFIG_MCUX_COMPONENT_driver.sim)
set_variable_ifdef(CONFIG_HWINFO_MCUX_RCM       CONFIG_MCUX_COMPONENT_driver.rcm)
set_variable_ifdef(CONFIG_IPM_MCUX              CONFIG_MCUX_COMPONENT_driver.mailbox)
set_variable_ifdef(CONFIG_MBOX_NXP_MAILBOX      CONFIG_MCUX_COMPONENT_driver.mailbox)
set_variable_ifdef(CONFIG_COUNTER_MCUX_SNVS     CONFIG_MCUX_COMPONENT_driver.snvs_hp)
set_variable_ifdef(CONFIG_MCUX_LPTMR_TIMER      CONFIG_MCUX_COMPONENT_driver.lptmr)
set_variable_ifdef(CONFIG_COUNTER_MCUX_LPTMR    CONFIG_MCUX_COMPONENT_driver.lptmr)
set_variable_ifdef(CONFIG_IMX_USDHC	            CONFIG_MCUX_COMPONENT_driver.usdhc)
set_variable_ifdef(CONFIG_MIPI_DSI_MCUX         CONFIG_MCUX_COMPONENT_driver.mipi_dsi_split)
set_variable_ifdef(CONFIG_MIPI_DSI_MCUX_2L      CONFIG_MCUX_COMPONENT_driver.mipi_dsi)
set_variable_ifdef(CONFIG_MCUX_SDIF             CONFIG_MCUX_COMPONENT_driver.sdif)
set_variable_ifdef(CONFIG_ADC_MCUX_ETC          CONFIG_MCUX_COMPONENT_driver.adc_etc)
set_variable_ifdef(CONFIG_MCUX_XBARA            CONFIG_MCUX_COMPONENT_driver.xbara)
set_variable_ifdef(CONFIG_QDEC_MCUX             CONFIG_MCUX_COMPONENT_driver.enc)
set_variable_ifdef(CONFIG_CRYPTO_MCUX_DCP       CONFIG_MCUX_COMPONENT_driver.dcp)
set_variable_ifdef(CONFIG_DMA_MCUX_SMARTDMA     CONFIG_MCUX_COMPONENT_driver.smartdma)
set_variable_ifdef(CONFIG_DAC_MCUX_LPDAC        CONFIG_MCUX_COMPONENT_driver.dac_1)
set_variable_ifdef(CONFIG_NXP_IRQSTEER          CONFIG_MCUX_COMPONENT_driver.irqsteer)
set_variable_ifdef(CONFIG_AUDIO_DMIC_MCUX       CONFIG_MCUX_COMPONENT_driver.dmic)
set_variable_ifdef(CONFIG_DMA_NXP_SDMA          CONFIG_MCUX_COMPONENT_driver.sdma)
set_variable_ifdef(CONFIG_ADC_MCUX_GAU          CONFIG_MCUX_COMPONENT_driver.cns_adc)
set_variable_ifdef(CONFIG_DAC_MCUX_GAU          CONFIG_MCUX_COMPONENT_driver.cns_dac)
set_variable_ifdef(CONFIG_DAI_NXP_ESAI          CONFIG_MCUX_COMPONENT_driver.esai)
set_variable_ifdef(CONFIG_MCUX_LPCMP            CONFIG_MCUX_COMPONENT_driver.lpcmp)
set_variable_ifdef(CONFIG_NXP_RF_IMU            CONFIG_MCUX_COMPONENT_driver.imu)
set_variable_ifdef(CONFIG_TRDC_MCUX_TRDC        CONFIG_MCUX_COMPONENT_driver.trdc)
set_variable_ifdef(CONFIG_S3MU_MCUX_S3MU        CONFIG_MCUX_COMPONENT_driver.s3mu)
set_variable_ifdef(CONFIG_PINCTRL_NXP_PORT      CONFIG_MCUX_COMPONENT_driver.port)
set_variable_ifdef(CONFIG_DMA_NXP_EDMA          CONFIG_MCUX_COMPONENT_driver.edma_soc_rev2)
set_variable_ifdef(CONFIG_COUNTER_MCUX_SNVS_SRTC    CONFIG_MCUX_COMPONENT_driver.snvs_lp)
set_variable_ifdef(CONFIG_DISPLAY_MCUX_DCNANO_LCDIF CONFIG_MCUX_COMPONENT_driver.lcdif)
set_variable_ifdef(CONFIG_MIPI_DBI_NXP_DCNANO_LCDIF CONFIG_MCUX_COMPONENT_driver.lcdif)
set_variable_ifdef(CONFIG_MIPI_DBI_NXP_FLEXIO_LCDIF CONFIG_MCUX_COMPONENT_driver.flexio_mculcd)
set_variable_ifdef(CONFIG_VIDEO_MCUX_MIPI_CSI2RX    CONFIG_MCUX_COMPONENT_driver.mipi_csi2rx)
set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC          CONFIG_MCUX_COMPONENT_driver.netc)
set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC          CONFIG_MCUX_COMPONENT_driver.netc_switch)

set_variable_ifdef(CONFIG_SOC_SERIES_IMXRT10XX    CONFIG_MCUX_COMPONENT_driver.ocotp)
set_variable_ifdef(CONFIG_SOC_SERIES_IMXRT11XX    CONFIG_MCUX_COMPONENT_driver.ocotp)
set_variable_ifdef(CONFIG_SOC_FAMILY_KINETIS      CONFIG_MCUX_COMPONENT_driver.port)
set_variable_ifdef(CONFIG_SOC_SERIES_MCXW         CONFIG_MCUX_COMPONENT_driver.ccm32k)
set_variable_ifdef(CONFIG_SOC_SERIES_IMXRT5XX     CONFIG_MCUX_COMPONENT_driver.iap3)

if(CONFIG_SOC_MIMXRT1189)
  set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC CONFIG_MCUX_COMPONENT_driver.netc_rt1180)
  set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC CONFIG_MCUX_COMPONENT_driver.msgintr)
elseif(CONFIG_SOC_MIMX9596)
  set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC CONFIG_MCUX_COMPONENT_driver.netc_imx95)
  set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC CONFIG_MCUX_COMPONENT_driver.msgintr)
  set_variable_ifdef(CONFIG_ETH_NXP_IMX_NETC CONFIG_MCUX_COMPONENT_driver.irqsteer)
endif()

if(CONFIG_SOC_SERIES_MCXN OR CONFIG_SOC_SERIES_MCXA)
  set(CONFIG_MCUX_COMPONENT_driver.mcx_spc ON)
endif()

if(CONFIG_BT_NXP AND CONFIG_SOC_SERIES_MCXW OR CONFIG_IEEE802154_MCXW)
  set(CONFIG_MCUX_COMPONENT_driver.spc ON)
endif()

if(((${MCUX_DEVICE} MATCHES "MIMXRT1[0-9][0-9][0-9]") AND (NOT (CONFIG_SOC_MIMXRT1166_CM4 OR CONFIG_SOC_MIMXRT1176_CM4 OR CONFIG_SOC_MIMXRT1189_CM33))) OR
    ((${MCUX_DEVICE} MATCHES "MIMX9596") AND CONFIG_SOC_MIMX9596_M7))
  set_variable_ifdef(CONFIG_HAS_MCUX_CACHE		CONFIG_MCUX_COMPONENT_driver.cache_armv7_m7)
elseif((${MCUX_DEVICE} MATCHES "MIMXRT(5|6)") OR (${MCUX_DEVICE} MATCHES "RW61") OR (${MCUX_DEVICE} MATCHES "MCXN.4."))
  set_variable_ifdef(CONFIG_HAS_MCUX_CACHE		CONFIG_MCUX_COMPONENT_driver.cache_cache64)
elseif((${MCUX_DEVICE} MATCHES "MK(28|66)") OR (${MCUX_DEVICE} MATCHES "MKE(14|16|18)") OR (CONFIG_SOC_MIMXRT1166_CM4) OR (CONFIG_SOC_MIMXRT1176_CM4))
  set_variable_ifdef(CONFIG_HAS_MCUX_CACHE		CONFIG_MCUX_COMPONENT_driver.cache_lmem)
endif()
  set_variable_ifdef(CONFIG_HAS_MCUX_XCACHE		CONFIG_MCUX_COMPONENT_driver.cache_xcache)

if((${MCUX_DEVICE} MATCHES "MIMX9596") OR (${MCUX_DEVICE} MATCHES "MIMX8UD7") OR (${MCUX_DEVICE} MATCHES "MIMXRT118"))
  set_variable_ifdef(CONFIG_IPM_IMX	CONFIG_MCUX_COMPONENT_driver.mu1)
  set_variable_ifdef(CONFIG_MBOX_NXP_IMX_MU	CONFIG_MCUX_COMPONENT_driver.mu1)
else()
  set_variable_ifdef(CONFIG_IPM_IMX	CONFIG_MCUX_COMPONENT_driver.mu)
  set_variable_ifdef(CONFIG_MBOX_NXP_IMX_MU	CONFIG_MCUX_COMPONENT_driver.mu)
endif()

if(CONFIG_SOC_FAMILY_KINETIS OR CONFIG_SOC_SERIES_MCXC)
  set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.flash)
endif()

if(CONFIG_SOC_MK82F25615 OR CONFIG_SOC_MK64F12 OR CONFIG_SOC_MK66F18 OR
    CONFIG_SOC_MKE14F16 OR CONFIG_SOC_MKE16F16 OR CONFIG_SOC_MKE18F16 OR
    CONFIG_SOC_MK22F12)
  set(CONFIG_MCUX_COMPONENT_driver.sysmpu ON)
endif()

if(CONFIG_SOC_SERIES_MCXW)
  set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.flash_k4)
endif()

if(CONFIG_SOC_SERIES_LPC51U68 OR CONFIG_SOC_SERIES_LPC54XXX)
  set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.iap)
endif()

if(CONFIG_SOC_SERIES_LPC51U68 OR CONFIG_SOC_SERIES_LPC54XXX)
  set_variable_ifdef(CONFIG_ENTROPY_MCUX_RNG CONFIG_MCUX_COMPONENT_driver.rng)
endif()

if(CONFIG_SOC_SERIES_LPC55XXX)
  set_variable_ifdef(CONFIG_ENTROPY_MCUX_RNG CONFIG_MCUX_COMPONENT_driver.rng_1)
  if(CONFIG_SOC_LPC55S36)
    set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.romapi_flash)
  else()
    set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.iap1)
  endif()
endif()

if(CONFIG_SOC_SERIES_LPC51U68 OR CONFIG_SOC_SERIES_LPC54XXX OR CONFIG_SOC_SERIES_LPC55XXX)
  set(CONFIG_MCUX_COMPONENT_driver.lpc_iocon ON)
endif()

if(CONFIG_SOC_LPC55S36)
  set_variable_ifdef(CONFIG_ADC_MCUX_LPADC CONFIG_MCUX_COMPONENT_driver.vref_1)
  set_variable_ifdef(CONFIG_DAC_MCUX_LPDAC CONFIG_MCUX_COMPONENT_driver.vref_1)
endif()

if(CONFIG_SOC_SERIES_IMXRT5XX OR CONFIG_SOC_SERIES_IMXRT6XX)
  set(CONFIG_MCUX_COMPONENT_driver.lpc_iopctl ON)
endif()

if(CONFIG_SOC_SERIES_RW6XX)
  set(CONFIG_MCUX_COMPONENT_driver.ocotp_rw61x ON)
endif()

if(CONFIG_SOC_SERIES_IMXRT10XX)
  set_variable_ifdef(CONFIG_PM_MCUX_GPC CONFIG_MCUX_COMPONENT_driver.gpc_1)
  set_variable_ifdef(CONFIG_PM_MCUX_DCDC CONFIG_MCUX_COMPONENT_driver.dcdc_1)
  set_variable_ifdef(CONFIG_PM_MCUX_PMU CONFIG_MCUX_COMPONENT_driver.pmu)
endif()

if(CONFIG_SOC_SERIES_IMXRT11XX)
  set(CONFIG_MCUX_COMPONENT_driver.romapi ON)
  set(CONFIG_MCUX_COMPONENT_driver.anadig_pmu ON)
  set(CONFIG_MCUX_COMPONENT_driver.pgmc ON)
  set(CONFIG_MCUX_COMPONENT_driver.dcdc_2 ON)
  set(CONFIG_MCUX_COMPONENT_driver.anatop_ai ON)
  set(CONFIG_MCUX_COMPONENT_driver.gpc_xxx_ctrl ON)
  set_variable_ifdef(CONFIG_VIDEO_MCUX_MIPI_CSI2RX CONFIG_MCUX_COMPONENT_driver.mipi_csi2rx_soc)
endif()

if(CONFIG_SOC_SERIES_IMXRT118X)
  set(CONFIG_MCUX_COMPONENT_driver.ele_base_api ON)
  set(CONFIG_MCUX_COMPONENT_driver.anadig_pmu_1 ON)
  set_variable_ifdef(CONFIG_HWINFO_MCUX_SRC_V2 CONFIG_MCUX_COMPONENT_driver.src_3)
  set_variable_ifdef(CONFIG_WDT_MCUX_RTWDOG	CONFIG_MCUX_COMPONENT_driver.src_3)
endif()

if(CONFIG_SOC_SERIES_MCXA)
  set(CONFIG_MCUX_COMPONENT_driver.romapi ON)
endif()

if(CONFIG_SOC_SERIES_MCXN)
  set_variable_ifdef(CONFIG_SOC_FLASH_MCUX CONFIG_MCUX_COMPONENT_driver.romapi_flashiap)
endif()

set_variable_ifdef(CONFIG_SOC_SERIES_MCXW CONFIG_MCUX_COMPONENT_driver.elemu)

# Load all drivers
mcux_load_all_cmakelists_in_directory(${SdkRootDirPath}/drivers)
