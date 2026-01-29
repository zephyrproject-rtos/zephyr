/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************
 * @file     drv/dev_tag.h
 * @brief    Header File for DEV TAG Driver
 * @version  V1.0
 * @date     31. March 2020
 * @model    common
 ******************************************************************************/

#ifndef _ZEPHYR_XUANTIE_XIAOHUI_SOC_DRV_DEV_TAG_H_
#define _ZEPHYR_XUANTIE_XIAOHUI_SOC_DRV_DEV_TAG_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <drv/list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DEV_BLANK_TAG = 0U,
	DEV_DW_UART_TAG,
	DEV_DW_DMA_TAG,
	DEV_DW_GPIO_TAG,
	DEV_DW_IIC_TAG,
	DEV_DW_QSPI_TAG,
	DEV_DW_SDMMC_TAG,
	DEV_DW_SDHCI_TAG,
	DEV_DW_SPI_TAG,
	DEV_DW_TIMER_TAG,
	DEV_DW_WDT_TAG,
	DEV_WJ_ADC_TAG,
	DEV_WJ_AES_TAG,
	DEV_WJ_CODEC_TAG,
	DEV_WJ_CRC_TAG,
	DEV_WJ_DMA_TAG,
	DEV_WJ_EFLASH_TAG,
	DEV_WJ_EFUSE_TAG,
	DEV_WJ_ETB_TAG,
	DEV_WJ_FFT_TAG,
	DEV_WJ_I2S_TAG,
	DEV_WJ_MBOX_TAG,
	DEV_WJ_PADREG_TAG,
	DEV_WJ_PDM_TAG,
	DEV_WJ_PINMUX_TAG,
	DEV_WJ_PMU_TAG,
	DEV_WJ_PWM_TAG,
	DEV_WJ_RNG_TAG,
	DEV_WJ_ROM_TAG,
	DEV_WJ_RSA_TAG,
	DEV_WJ_RTC_TAG,
	DEV_WJ_SASC_TAG,
	DEV_WJ_SHA_TAG,
	DEV_WJ_SPDIF_TAG,
	DEV_WJ_SPIDF_TAG,
	DEV_WJ_TDM_TAG,
	DEV_WJ_TIPC_TAG,
	DEV_WJ_USB_TAG,
	DEV_WJ_USI_TAG,
	DEV_WJ_VAD_TAG,
	DEV_CD_QSPI_TAG,
	DEV_DCD_ISO7816_TAG,
	DEV_OSR_RNG_TAG,
	DEV_QX_RTC_TAG,
	DEV_RCHBAND_CODEC_TAG,
	DEV_CMSDK_UART_TAG,
	DEV_RAMBUS_150B_PKA_TAG,
	DEV_RAMBUS_150B_TRNG_TAG,
	DEV_RAMBUS_120SI_TAG,
	DEV_RAMBUS_120SII_TAG,
	DEV_RAMBUS_120SIII_TAG,
	DEV_WJ_AVFS_TAG,
	DEV_WJ_BMU_TAG,
} csi_dev_tag_t;

#ifdef __cplusplus
}
#endif

#endif /* _ZEPHYR_XUANTIE_XIAOHUI_SOC_DRV_DEV_TAG_H_ */
