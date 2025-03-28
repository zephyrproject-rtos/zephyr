# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/drivers
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/common
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/flexcomm
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpflexcomm
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/flexio
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/dmamux
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/osa
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/lists
)

if(CONFIG_CPU_CORTEX_A)
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS/Core_AArch64/Include
    )
else()
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/cache/armv7-m7
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/CMSIS/Core/Include
    )
endif()

function(include_ifdef feature_toggle module)
  if(${${feature_toggle}})
    include(${module})
  endif()
endfunction()

function(include_driver_ifdef feature_toggle directory module)
  if(${${feature_toggle}})
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory}
    )
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/${directory})
    include(${module})
  endif()
endfunction()

message("Load components for ${MCUX_DEVICE}:")

#specific operation to shared drivers
if((DEFINED CONFIG_FLASH_MCUX_FLEXSPI_XIP) AND (DEFINED CONFIG_FLASH))
  zephyr_code_relocate(FILES ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/flexspi/fsl_flexspi.c
    LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
endif()

# Required by all SCFW-based SoCs
if (CONFIG_SOC_MIMX8QM6_ADSP OR CONFIG_SOC_MIMX8QX6_ADSP)
    list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/scfw_api
    )
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/${MCUX_DEVICE_PATH}/scfw_api)
    include(driver_scfw_api)
endif()

include(driver_common)

#Include system_xxx file
#This can be extended to other SoC series if needed
if (DEFINED CONFIG_SOC_RESET_HOOK OR DEFINED CONFIG_SOC_SERIES_IMXRT6XX
    OR DEFINED CONFIG_SOC_SERIES_LPC55XXX OR DEFINED CONFIG_SOC_SERIES_MCXN
    OR DEFINED CONFIG_SOC_MCXW727C)
  if (CONFIG_SOC_MIMXRT1166_CM4)
    include(device_system_MIMXRT1166_cm4)
  elseif (CONFIG_SOC_MIMXRT1166_CM7)
    include(device_system_MIMXRT1166_cm7)
  elseif (CONFIG_SOC_MIMXRT1176_CM4)
    include(device_system_MIMXRT1176_cm4)
  elseif (CONFIG_SOC_MIMXRT1176_CM7)
    include(device_system_MIMXRT1176_cm7)
  elseif (CONFIG_SOC_MIMXRT1189_CM33)
    include(device_system_MIMXRT1189_cm33)
  elseif (CONFIG_SOC_MIMXRT1189_CM7)
    include(device_system_MIMXRT1189_cm7)
  elseif (CONFIG_SOC_LPC55S69_CPU0)
    include(device_system_LPC55S69_cm33_core0)
  elseif (CONFIG_SOC_LPC55S69_CPU1)
    include(device_system_LPC55S69_cm33_core1)
  elseif (CONFIG_SOC_LPC54114_M4)
    include(device_system_LPC54114_cm4)
  elseif (CONFIG_SOC_LPC54114_M0)
    include(device_system_LPC54114_cm0plus)
  elseif (CONFIG_SOC_MCXN947_CPU0)
    include(device_system_MCXN947_cm33_core0)
  elseif (CONFIG_SOC_MCXN947_CPU1)
    include(device_system_MCXN947_cm33_core1)
  elseif (CONFIG_SOC_MCXN236)
    include(device_system_MCXN236)
  elseif (CONFIG_SOC_MIMXRT798S_CM33_CPU0)
    include(device_system_MIMXRT798S_cm33_core0)
  elseif (CONFIG_SOC_MIMXRT798S_CM33_CPU1)
    include(device_system_MIMXRT798S_cm33_core1)
  elseif (CONFIG_SOC_MCXW727C_CPU0)
    include(device_system_MCXW727C_cm33_core0)
  else()
    include(device_system)
  endif()
endif()

zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/common)

#include shared drivers
include_driver_ifdef(CONFIG_ADC_MCUX_LPADC		lpadc		driver_lpadc)
include_driver_ifdef(CONFIG_COUNTER_MCUX_CTIMER		ctimer		driver_ctimer)
include_driver_ifdef(CONFIG_COUNTER_MCUX_LPC_RTC	lpc_rtc		driver_lpc_rtc)
include_driver_ifdef(CONFIG_DMA_MCUX_LPC		lpc_dma		driver_lpc_dma)
include_driver_ifdef(CONFIG_GPIO_MCUX_LPC		lpc_gpio        driver_lpc_gpio)
include_driver_ifdef(CONFIG_NXP_PINT			pint    	driver_pint)
include_driver_ifdef(CONFIG_NXP_PINT			inputmux	driver_inputmux)
include_driver_ifdef(CONFIG_I2C_MCUX_FLEXCOMM		flexcomm	driver_flexcomm)
include_driver_ifdef(CONFIG_I2C_MCUX_FLEXCOMM		flexcomm/i2c	driver_flexcomm_i2c)
include_driver_ifdef(CONFIG_I2S_MCUX_FLEXCOMM		flexcomm	driver_flexcomm)
include_driver_ifdef(CONFIG_I2S_MCUX_FLEXCOMM		flexcomm/i2s	driver_flexcomm_i2s)
include_driver_ifdef(CONFIG_MCUX_OS_TIMER		ostimer		driver_ostimer)
include_driver_ifdef(CONFIG_PWM_MCUX_SCTIMER		sctimer		driver_sctimer)
include_driver_ifdef(CONFIG_PWM_MCUX_CTIMER		ctimer		driver_ctimer)
include_driver_ifdef(CONFIG_SOC_FLASH_LPC		flashiap	driver_flashiap)
include_driver_ifdef(CONFIG_SPI_MCUX_FLEXCOMM		flexcomm	driver_flexcomm)
include_driver_ifdef(CONFIG_SPI_MCUX_FLEXCOMM		flexcomm/spi	driver_flexcomm_spi)
include_driver_ifdef(CONFIG_UART_MCUX_FLEXCOMM		flexcomm	driver_flexcomm)
include_driver_ifdef(CONFIG_UART_MCUX_FLEXCOMM		flexcomm/usart	driver_flexcomm_usart)
include_driver_ifdef(CONFIG_WDT_MCUX_WWDT		wwdt		driver_wwdt)
include_driver_ifdef(CONFIG_ADC_MCUX_ADC12		adc12		driver_adc12)
include_driver_ifdef(CONFIG_ADC_MCUX_ADC16		adc16		driver_adc16)
include_driver_ifdef(CONFIG_CAN_MCUX_FLEXCAN		flexcan		driver_flexcan)
include_driver_ifdef(CONFIG_CAN_MCUX_FLEXCAN_FD		flexcan		driver_flexcan)
include_driver_ifdef(CONFIG_COUNTER_NXP_PIT		pit		driver_pit)
include_driver_ifdef(CONFIG_COUNTER_MCUX_RTC		rtc		driver_rtc)
include_driver_ifdef(CONFIG_DAC_MCUX_DAC		dac		driver_dac)
include_driver_ifdef(CONFIG_DAC_MCUX_DAC32		dac32		driver_dac32)
include_driver_ifdef(CONFIG_DMA_MCUX_EDMA		dmamux		driver_dmamux)
include_driver_ifdef(CONFIG_DMA_MCUX_EDMA_V3		dmamux		driver_dmamux)
include_driver_ifdef(CONFIG_DMA_MCUX_EDMA		edma		driver_edma)
include_driver_ifdef(CONFIG_DMA_MCUX_EDMA_V3		dma3		driver_dma3)
include_driver_ifdef(CONFIG_DMA_MCUX_EDMA_V4		edma4		driver_edma4)
include_driver_ifdef(CONFIG_ENTROPY_MCUX_RNGA		rnga		driver_rnga)
include_driver_ifdef(CONFIG_ENTROPY_MCUX_TRNG		trng		driver_trng)
include_driver_ifdef(CONFIG_ENTROPY_MCUX_CAAM		caam		driver_caam)
include_driver_ifdef(CONFIG_ETH_NXP_ENET		enet		driver_enet)
include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC		netc		driver_netc)
include_driver_ifdef(CONFIG_HAS_MCUX_SMC		smc		driver_smc)
include_driver_ifdef(CONFIG_I2C_MCUX			i2c		driver_i2c)
include_driver_ifdef(CONFIG_I2C_NXP_II2C		ii2c		driver_ii2c)
if (CONFIG_NXP_LP_FLEXCOMM)
include_driver_ifdef(CONFIG_I2C_MCUX_LPI2C		lpflexcomm	driver_lpflexcomm)
include_driver_ifdef(CONFIG_I2C_MCUX_LPI2C		lpflexcomm/lpi2c	driver_lpi2c)
else()
include_driver_ifdef(CONFIG_I2C_MCUX_LPI2C		lpi2c		driver_lpi2c)
endif()
include_driver_ifdef(CONFIG_I3C_MCUX			i3c		driver_i3c)
include_driver_ifdef(CONFIG_COMPARATOR_MCUX_ACMP	acmp		driver_acmp)
include_driver_ifdef(CONFIG_SENSOR_MCUX_ACMP		acmp		driver_acmp)
include_driver_ifdef(CONFIG_PWM_MCUX_FTM		ftm     	driver_ftm)
include_driver_ifdef(CONFIG_PWM_MCUX_TPM		tpm		driver_tpm)
include_driver_ifdef(CONFIG_PWM_MCUX_PWT		pwt		driver_pwt)
include_driver_ifdef(CONFIG_PWM_MCUX_QTMR		qtmr_1		driver_qtmr_1)
include_driver_ifdef(CONFIG_SPI_MCUX_DSPI		dspi		driver_dspi)
include_driver_ifdef(CONFIG_SPI_MCUX_ECSPI		ecspi		driver_ecspi)
if (CONFIG_NXP_LP_FLEXCOMM)
include_driver_ifdef(CONFIG_SPI_MCUX_LPSPI		lpflexcomm	driver_lpflexcomm)
include_driver_ifdef(CONFIG_SPI_MCUX_LPSPI		lpflexcomm/lpspi	driver_lpspi)
else()
include_driver_ifdef(CONFIG_SPI_MCUX_LPSPI		lpspi		driver_lpspi)
endif()
include_driver_ifdef(CONFIG_MCUX_FLEXIO			flexio		driver_flexio)
include_driver_ifdef(CONFIG_SPI_MCUX_FLEXIO		flexio/spi	driver_flexio_spi)
include_driver_ifdef(CONFIG_MIPI_DBI_NXP_FLEXIO_LCDIF	flexio/mculcd	driver_flexio_mculcd)
include_driver_ifdef(CONFIG_UART_MCUX			uart		driver_uart)
include_driver_ifdef(CONFIG_UART_MCUX_LPSCI		lpsci		driver_lpsci)
if (CONFIG_NXP_LP_FLEXCOMM)
include_driver_ifdef(CONFIG_UART_MCUX_LPUART		lpflexcomm	driver_lpflexcomm)
include_driver_ifdef(CONFIG_UART_MCUX_LPUART		lpflexcomm/lpuart	driver_lpuart)
else()
include_driver_ifdef(CONFIG_UART_MCUX_LPUART		lpuart		driver_lpuart)
endif()
include_driver_ifdef(CONFIG_WDT_MCUX_WDOG		wdog		driver_wdog)
include_driver_ifdef(CONFIG_WDT_MCUX_WDOG32		wdog32		driver_wdog32)
include_driver_ifdef(CONFIG_COUNTER_MCUX_GPT		gpt		driver_gpt)
include_driver_ifdef(CONFIG_COUNTER_MCUX_TPM		tpm		driver_tpm)
include_driver_ifdef(CONFIG_MCUX_GPT_TIMER		gpt		driver_gpt)
include_driver_ifdef(CONFIG_COUNTER_MCUX_QTMR		qtmr_1		driver_qtmr_1)
include_driver_ifdef(CONFIG_DISPLAY_MCUX_ELCDIF		elcdif		driver_elcdif)
include_driver_ifdef(CONFIG_DISPLAY_MCUX_DCNANO_LCDIF	lcdif		driver_lcdif)
include_driver_ifdef(CONFIG_MCUX_PXP			pxp		driver_pxp)
include_driver_ifdef(CONFIG_LV_USE_GPU_NXP_PXP		pxp		driver_pxp)
include_driver_ifdef(CONFIG_GPIO_MCUX_IGPIO		igpio		driver_igpio)
include_driver_ifdef(CONFIG_GPIO_MCUX_RGPIO		rgpio		driver_rgpio)
include_driver_ifdef(CONFIG_I2S_MCUX_SAI		sai		driver_sai)
include_driver_ifdef(CONFIG_DAI_NXP_SAI			sai		driver_sai)
include_driver_ifdef(CONFIG_MEMC_MCUX_FLEXSPI		flexspi		driver_flexspi)
include_driver_ifdef(CONFIG_PWM_MCUX			pwm		driver_pwm)
include_driver_ifdef(CONFIG_VIDEO_MCUX_CSI		csi		driver_csi)
include_driver_ifdef(CONFIG_VIDEO_MCUX_MIPI_CSI2RX	mipi_csi2rx     driver_mipi_csi2rx)
include_driver_ifdef(CONFIG_WDT_MCUX_IMX_WDOG		wdog01		driver_wdog01)
include_driver_ifdef(CONFIG_WDT_MCUX_RTWDOG		rtwdog		driver_rtwdog)
include_driver_ifdef(CONFIG_DMA_MCUX_LPC		lpc_dma		driver_lpc_dma)
include_driver_ifdef(CONFIG_MEMC_MCUX_FLEXSPI		flexspi		driver_flexspi)
include_driver_ifdef(CONFIG_HAS_MCUX_RDC		rdc		driver_rdc)
include_driver_ifdef(CONFIG_UART_MCUX_IUART		iuart		driver_iuart)
include_driver_ifdef(CONFIG_ADC_MCUX_12B1MSPS_SAR	adc_12b1msps_sar	driver_adc_12b1msps_sar)
include_driver_ifdef(CONFIG_HWINFO_MCUX_SRC		src		driver_src)
include_driver_ifdef(CONFIG_HWINFO_MCUX_SIM		sim		driver_sim)
include_driver_ifdef(CONFIG_HWINFO_MCUX_RCM		rcm		driver_rcm)
include_driver_ifdef(CONFIG_IPM_MCUX			mailbox		driver_mailbox)
include_driver_ifdef(CONFIG_MBOX_NXP_MAILBOX		mailbox		driver_mailbox)
include_driver_ifdef(CONFIG_COUNTER_MCUX_SNVS		snvs_hp		driver_snvs_hp)
include_driver_ifdef(CONFIG_COUNTER_MCUX_SNVS_SRTC	snvs_lp		driver_snvs_lp)
include_driver_ifdef(CONFIG_COUNTER_MCUX_LPTMR		lptmr		driver_lptmr)
include_driver_ifdef(CONFIG_MCUX_LPTMR_TIMER		lptmr		driver_lptmr)
include_driver_ifdef(CONFIG_IMX_USDHC			usdhc		driver_usdhc)
include_driver_ifdef(CONFIG_MIPI_DSI_MCUX		mipi_dsi_split	driver_mipi_dsi_split)
include_driver_ifdef(CONFIG_MIPI_DSI_MCUX_2L		mipi_dsi	driver_mipi_dsi)
include_driver_ifdef(CONFIG_MCUX_SDIF			sdif		driver_sdif)
include_driver_ifdef(CONFIG_ADC_MCUX_ETC		adc_etc		driver_adc_etc)
include_driver_ifdef(CONFIG_MCUX_XBARA			xbara		driver_xbara)
include_driver_ifdef(CONFIG_MCUX_XBARB			xbarb		driver_xbarb)
include_driver_ifdef(CONFIG_QDEC_MCUX			enc		driver_enc)
include_driver_ifdef(CONFIG_CRYPTO_MCUX_DCP		dcp		driver_dcp)
include_driver_ifdef(CONFIG_DMA_MCUX_SMARTDMA		smartdma	driver_lpc_smartdma)
include_driver_ifdef(CONFIG_DMA_MCUX_SMARTDMA		inputmux	driver_inputmux)
include_driver_ifdef(CONFIG_DAC_MCUX_LPDAC		dac_1		driver_dac_1)
include_driver_ifdef(CONFIG_NXP_IRQSTEER		irqsteer	driver_irqsteer)
include_driver_ifdef(CONFIG_AUDIO_DMIC_MCUX		dmic		driver_dmic)
include_driver_ifdef(CONFIG_DMA_NXP_EDMA		edma_rev2	driver_edma_rev2)
include_driver_ifdef(CONFIG_DMA_NXP_SDMA		sdma		driver_sdma)
include_driver_ifdef(CONFIG_SOC_SERIES_MCXN		mcx_spc		driver_mcx_spc)
include_driver_ifdef(CONFIG_SOC_SERIES_MCXA		mcx_spc		driver_mcx_spc)
include_driver_ifdef(CONFIG_ADC_MCUX_GAU		cns_adc		driver_cns_adc)
include_driver_ifdef(CONFIG_DAC_MCUX_GAU		cns_dac		driver_cns_dac)
include_driver_ifdef(CONFIG_DAI_NXP_ESAI		esai		driver_esai)
include_driver_ifdef(CONFIG_MCUX_LPCMP		        lpcmp		driver_lpcmp)
include_driver_ifdef(CONFIG_NXP_RF_IMU			imu		driver_imu)
include_driver_ifdef(CONFIG_TRDC_MCUX_TRDC              trdc            driver_trdc)
include_driver_ifdef(CONFIG_S3MU_MCUX_S3MU              s3mu            driver_s3mu)
include_driver_ifdef(CONFIG_DAI_NXP_MICFIL		pdm		driver_pdm)
include_driver_ifdef(CONFIG_PINCTRL_NXP_PORT            port            driver_port)
include_driver_ifdef(CONFIG_GLIKEY_MCUX_GLIKEY		glikey		driver_glikey)
if(CONFIG_BT_NXP)
include_driver_ifdef(CONFIG_SOC_SERIES_MCXW		spc		driver_spc)
endif()

if (${MCUX_DEVICE} MATCHES "MIMXRT1189")
  include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC	netc/socs/imxrt1180	driver_netc_soc_imxrt1180)
  include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC	msgintr			driver_msgintr)
elseif (${MCUX_DEVICE} MATCHES "MIMX9596")
  include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC	netc/socs/imx95		driver_netc_soc_imx95)
  include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC	msgintr			driver_msgintr)
  include_driver_ifdef(CONFIG_ETH_NXP_IMX_NETC	irqsteer		driver_irqsteer)
endif()

if (((${MCUX_DEVICE} MATCHES "MIMXRT1[0-9][0-9][0-9]") AND (NOT (CONFIG_SOC_MIMXRT1166_CM4 OR CONFIG_SOC_MIMXRT1176_CM4 OR CONFIG_SOC_MIMXRT1189_CM33))) OR
    ((${MCUX_DEVICE} MATCHES "MIMX9596") AND CONFIG_SOC_MIMX9596_M7))
  include_driver_ifdef(CONFIG_HAS_MCUX_CACHE		cache/armv7-m7	driver_cache_armv7_m7)
elseif((${MCUX_DEVICE} MATCHES "MIMXRT(5|6|7)") OR (${MCUX_DEVICE} MATCHES "RW61") OR (${MCUX_DEVICE} MATCHES "MCXN.4."))
  include_driver_ifdef(CONFIG_HAS_MCUX_CACHE		cache/cache64	driver_cache_cache64)
elseif((${MCUX_DEVICE} MATCHES "MK(28|66)") OR (${MCUX_DEVICE} MATCHES "MKE(14|16|18)") OR (CONFIG_SOC_MIMXRT1166_CM4) OR (CONFIG_SOC_MIMXRT1176_CM4))
  include_driver_ifdef(CONFIG_HAS_MCUX_CACHE		cache/lmem	driver_cache_lmem)
endif()
  include_driver_ifdef(CONFIG_HAS_MCUX_XCACHE		cache/xcache	driver_cache_xcache)

if ((${MCUX_DEVICE} MATCHES "MIMX9596") OR (${MCUX_DEVICE} MATCHES "MIMX8UD7") OR (${MCUX_DEVICE} MATCHES "MIMXRT118"))
  include_driver_ifdef(CONFIG_IPM_IMX			mu1		driver_mu1)
  include_driver_ifdef(CONFIG_MBOX_NXP_IMX_MU		mu1		driver_mu1)
else()
  include_driver_ifdef(CONFIG_IPM_IMX			mu		driver_mu)
  include_driver_ifdef(CONFIG_MBOX_NXP_IMX_MU		mu		driver_mu)
endif()

if(CONFIG_CPU_CORTEX_A)
  include_driver_ifdef(CONFIG_HAS_MCUX_CACHE		cache/armv8-a	driver_cache_armv8a)
endif()

if (${MCUX_DEVICE} MATCHES "MIMXRT11[0-9][0-9]")
   include_driver_ifdef(CONFIG_PM_MCUX_GPC		gpc_3		driver_gpc_3)
   include_ifdef(CONFIG_HWINFO_MCUX_SRC_V2		driver_soc_src)
   include_driver_ifdef(CONFIG_WDT_MCUX_RTWDOG		soc_src		driver_soc_src)
elseif (${MCUX_DEVICE} MATCHES "MIMXRT10[0-9][0-9]")
   include_driver_ifdef(CONFIG_PM_MCUX_GPC		gpc_1		driver_gpc_1)
   include_driver_ifdef(CONFIG_PM_MCUX_DCDC		dcdc_1		driver_dcdc_1)
   include_driver_ifdef(CONFIG_PM_MCUX_PMU		pmu		driver_pmu)
endif()

if((${MCUX_DEVICE} MATCHES "RW61") AND (NOT DEFINED CONFIG_MINIMAL_LIBC))
	# Whenever building for RW61x without minimal LIBC, use optimized memcpy.
	# This will avoid issues with unaligned access to peripheral RAM regions
	# caused by the memcpy implementation in newlib
	zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/utilities/misc_utilities/fsl_memcpy.S)
endif()


if("${CONFIG_SOC_FAMILY}" STREQUAL "nxp_kinetis")

  include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		flash		driver_flash)

  include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/port/driver_port.cmake)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/port)

  if(${MCUX_DEVICE} MATCHES "MK(80|82|64|66|M34|M35|E14F|E16F|E18F|22F12)")
    include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/sysmpu/driver_sysmpu.cmake)
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/sysmpu)
  endif()

endif()

if(CONFIG_SOC_SERIES_MCXC)
    include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		flash		driver_flash)
elseif(CONFIG_SOC_SERIES_MCXA)
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/mcxa_romapi)
elseif(CONFIG_SOC_SERIES_MCXN)
    include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		mcx_romapi	driver_flashiap)
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/mcx_romapi/flash)
elseif(CONFIG_SOC_SERIES_MCXW)
    include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		flash_k4	driver_flash_k4)
endif()

# Temporary change to handle LPC SOC family name change between HWMv1 and HWMv2
if(("${CONFIG_SOC_FAMILY}" STREQUAL "lpc") OR ("${CONFIG_SOC_FAMILY}" STREQUAL "nxp_lpc"))

if ((${MCUX_DEVICE} MATCHES "LPC8[0-9][0-9]") OR (${MCUX_DEVICE} MATCHES "LPC5(1|4)"))
  include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		iap		driver_iap)
  include_driver_ifdef(CONFIG_ENTROPY_MCUX_RNG		iap		driver_rng)
elseif (${MCUX_DEVICE} MATCHES "LPC55")
  if (${MCUX_DEVICE} MATCHES "LPC55S*3")
   if(${CONFIG_SOC_FLASH_MCUX})
      list(APPEND CMAKE_MODULE_PATH
        ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/LPC55S36/drivers
      )
      zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/devices/LPC55S36/drivers/flash)
      include(driver_flashiap)
    endif()
  else()
    include_driver_ifdef(CONFIG_SOC_FLASH_MCUX		iap1		driver_iap1)
  endif()
  include_driver_ifdef(CONFIG_ENTROPY_MCUX_RNG		rng_1		driver_rng_1)
endif()

if (${MCUX_DEVICE} MATCHES "LPC5")
  include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iocon/driver_lpc_iocon.cmake)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iocon)
  if (${MCUX_DEVICE} MATCHES "LPC55S*3" AND (DEFINED CONFIG_ADC_MCUX_LPADC OR DEFINED CONFIG_DAC_MCUX_LPDAC))
    include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/vref_1/driver_vref_1.cmake)
    zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/vref_1)
  endif()
elseif (${MCUX_DEVICE} MATCHES "LPC8")
  include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iocon_lite/driver_lpc_iocon_lite.cmake)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iocon_lite)
endif()

endif()

if(${MCUX_DEVICE} MATCHES "MIMXRT(5|6)")
  include(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iopctl/driver_lpc_iopctl.cmake)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/lpc_iopctl)
endif()

if(CONFIG_SOC_FAMILY_NXP_IMXRT)
  include_driver_ifdef(CONFIG_ETH_NXP_ENET ocotp driver_ocotp)
endif()

if (CONFIG_USB_DEVICE_DRIVER)
  set(CONFIG_USE_component_osa_zephyr true)

  list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux-sdk/components/osa
  )

  list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb
  )
  include(middleware_usb_phy)
  include_ifdef(CONFIG_USB_DC_NXP_EHCI         middleware_usb_device_ehci)
  include_ifdef(CONFIG_USB_DC_NXP_LPCIP3511    middleware_usb_device_ip3511fs)
  include(set_component_osa)

  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/device)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/include)
endif()

if (CONFIG_UDC_DRIVER)
  set(CONFIG_USE_component_osa_zephyr true)

  list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux-sdk/components/osa
  )

  list(APPEND CMAKE_MODULE_PATH
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb
  )
  include_ifdef(CONFIG_DT_HAS_NXP_USBPHY_ENABLED  middleware_usb_phy)
  include_ifdef(CONFIG_UDC_NXP_EHCI               middleware_usb_device_ehci)
  include_ifdef(CONFIG_UDC_NXP_IP3511             middleware_usb_device_ip3511fs)
  include(set_component_osa)

  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/device)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/phy)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/middleware/mcux-sdk-middleware-usb/include)
endif()

if(${MCUX_DEVICE} MATCHES "RW61")
  set(CONFIG_USE_component_osa_zephyr true)
  if(CONFIG_NXP_FW_LOADER)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/components/flash/mflash)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/components/flash/mflash/rdrw612bga)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/drivers/cache/cache64)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/drivers/flexspi)
    include(component_mflash_rdrw610)
  endif()
endif()

if(${CONFIG_USE_component_osa_zephyr})
  zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/components/osa)
endif()

include_ifdef(CONFIG_USE_component_osa_zephyr set_component_osa)

if(CONFIG_WIFI_NXP)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/components/wifi_bt_module)
    include(component_wifi_bt_module_tx_pwr_limits)
endif()

if(CONFIG_NXP_FW_LOADER)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mcux-sdk/components/conn_fwloader)
    include(driver_conn_fwloader)
endif()

if(CONFIG_NXP_RF_IMU)
  if(CONFIG_SOC_SERIES_RW6XX)
      list(APPEND CMAKE_MODULE_PATH
          ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg
          ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/imu
          ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/gdma
      )
    include(component_wireless_imu_adapter)
  elseif(CONFIG_SOC_SERIES_MCXW)
      zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg)
      zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/components/rpmsg/fsl_adapter_rpmsg.c)
      include(component_lists)
      zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
  endif()
endif()

if(${MCUX_DEVICE} MATCHES "MCXW")
  list(APPEND CMAKE_MODULE_PATH
      ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/ccm32k
  )

  include(driver_ccm32k)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/ccm32k)
endif()
