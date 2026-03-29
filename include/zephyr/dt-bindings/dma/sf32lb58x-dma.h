/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief SF32LB58X Direct Memory Access Controller device tree DMA bindings.
  */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_DMA_SF32LB58X_DMA_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_DMA_SF32LB58X_DMA_H_

#include "sf32lb-dma-config.h"

/**
 * @name DMA request ID
 * @{
 */

/** @brief DMA request ID for MPI1. */
#define SF32LB58X_DMA_REQ_MPI1              0U
/** @brief DMA request ID for MPI2. */
#define SF32LB58X_DMA_REQ_MPI2              1U
/** @brief DMA request ID for I2C4. */
#define SF32LB58X_DMA_REQ_I2C4              3U
/** @brief DMA request ID for USART1_TX. */
#define SF32LB58X_DMA_REQ_USART1_TX         4U
/** @brief DMA request ID for USART1_RX. */
#define SF32LB58X_DMA_REQ_USART1_RX         5U
/** @brief DMA request ID for USART2_TX. */
#define SF32LB58X_DMA_REQ_USART2_TX         6U
/** @brief DMA request ID for USART2_RX. */
#define SF32LB58X_DMA_REQ_USART2_RX         7U
/** @brief DMA request ID for GPTIM1_UPDATE. */
#define SF32LB58X_DMA_REQ_GPTIM1_UPDATE     8U
/** @brief DMA request ID for GPTIM1_TRIGGER. */
#define SF32LB58X_DMA_REQ_GPTIM1_TRIGGER    9U
/** @brief DMA request ID for GPTIM1_CC1. */
#define SF32LB58X_DMA_REQ_GPTIM1_CC1        10U
/** @brief DMA request ID for GPTIM1_CC2. */
#define SF32LB58X_DMA_REQ_GPTIM1_CC2        11U
/** @brief DMA request ID for GPTIM1_CC3. */
#define SF32LB58X_DMA_REQ_GPTIM1_CC3        12U
/** @brief DMA request ID for GPTIM1_CC4. */
#define SF32LB58X_DMA_REQ_GPTIM1_CC4        13U
/** @brief DMA request ID for BTIM1. */
#define SF32LB58X_DMA_REQ_BTIM1             14U
/** @brief DMA request ID for BTIM2. */
#define SF32LB58X_DMA_REQ_BTIM2             15U
/** @brief DMA request ID for ATIM1_UPDATE. */
#define SF32LB58X_DMA_REQ_ATIM1_UPDATE      16U
/** @brief DMA request ID for ATIM1_TRIGGER. */
#define SF32LB58X_DMA_REQ_ATIM1_TRIGGER     17U
/** @brief DMA request ID for ATIM1_CC1. */
#define SF32LB58X_DMA_REQ_ATIM1_CC1         18U
/** @brief DMA request ID for ATIM1_CC2. */
#define SF32LB58X_DMA_REQ_ATIM1_CC2         19U
/** @brief DMA request ID for ATIM1_CC3. */
#define SF32LB58X_DMA_REQ_ATIM1_CC3         20U
/** @brief DMA request ID for ATIM1_CC4. */
#define SF32LB58X_DMA_REQ_ATIM1_CC4         21U
/** @brief DMA request ID for I2C1. */
#define SF32LB58X_DMA_REQ_I2C1              22U
/** @brief DMA request ID for I2C2. */
#define SF32LB58X_DMA_REQ_I2C2              23U
/** @brief DMA request ID for I2C3. */
#define SF32LB58X_DMA_REQ_I2C3              24U
/** @brief DMA request ID for ATIM1_COM. */
#define SF32LB58X_DMA_REQ_ATIM1_COM         25U
/** @brief DMA request ID for USART3_TX. */
#define SF32LB58X_DMA_REQ_USART3_TX         26U
/** @brief DMA request ID for USART3_RX. */
#define SF32LB58X_DMA_REQ_USART3_RX         27U
/** @brief DMA request ID for SPI1_TX. */
#define SF32LB58X_DMA_REQ_SPI1_TX           28U
/** @brief DMA request ID for SPI1_RX. */
#define SF32LB58X_DMA_REQ_SPI1_RX           29U
/** @brief DMA request ID for SPI2_TX. */
#define SF32LB58X_DMA_REQ_SPI2_TX           30U
/** @brief DMA request ID for SPI2_RX. */
#define SF32LB58X_DMA_REQ_SPI2_RX           31U
/** @brief DMA request ID for I2S1_TX. */
#define SF32LB58X_DMA_REQ_I2S1_TX           32U
/** @brief DMA request ID for I2S1_RX. */
#define SF32LB58X_DMA_REQ_I2S1_RX           33U
/** @brief DMA request ID for PDM1_L. */
#define SF32LB58X_DMA_REQ_PDM1_L            36U
/** @brief DMA request ID for PDM1_R. */
#define SF32LB58X_DMA_REQ_PDM1_R            37U
/** @brief DMA request ID for GPADC. */
#define SF32LB58X_DMA_REQ_GPADC             38U
/** @brief DMA request ID for AUDADC_CH0. */
#define SF32LB58X_DMA_REQ_AUDADC_CH0        39U
/** @brief DMA request ID for AUDADC_CH1. */
#define SF32LB58X_DMA_REQ_AUDADC_CH1        40U
/** @brief DMA request ID for AUDAC_CH0. */
#define SF32LB58X_DMA_REQ_AUDAC_CH0         41U
/** @brief DMA request ID for AUDAC_CH1. */
#define SF32LB58X_DMA_REQ_AUDAC_CH1         42U
/** @brief DMA request ID for GPTIM2_UPDATE. */
#define SF32LB58X_DMA_REQ_GPTIM2_UPDATE     43U
/** @brief DMA request ID for GPTIM2_TRIGGER. */
#define SF32LB58X_DMA_REQ_GPTIM2_TRIGGER    44U
/** @brief DMA request ID for GPTIM2_CC1. */
#define SF32LB58X_DMA_REQ_GPTIM2_CC1        45U
/** @brief DMA request ID for AUDPRC_TX_OUT_CH1. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_OUT_CH1 46U
/** @brief DMA request ID for AUDPRC_TX_OUT_CH0. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_OUT_CH0 47U
/** @brief DMA request ID for AUDPRC_TX_CH3. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_CH3     48U
/** @brief DMA request ID for AUDPRC_TX_CH2. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_CH2     49U
/** @brief DMA request ID for AUDPRC_TX_CH1. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_CH1     50U
/** @brief DMA request ID for AUDPRC_TX_CH0. */
#define SF32LB58X_DMA_REQ_AUDPRC_TX_CH0     51U
/** @brief DMA request ID for AUDPRC_RX_CH1. */
#define SF32LB58X_DMA_REQ_AUDPRC_RX_CH1     52U
/** @brief DMA request ID for AUDPRC_RX_CH0. */
#define SF32LB58X_DMA_REQ_AUDPRC_RX_CH0     53U
/** @brief DMA request ID for GPTIM2_CC2. */
#define SF32LB58X_DMA_REQ_GPTIM2_CC2        54U
/** @brief DMA request ID for GPTIM2_CC3. */
#define SF32LB58X_DMA_REQ_GPTIM2_CC3        55U
/** @brief DMA request ID for GPTIM2_CC4. */
#define SF32LB58X_DMA_REQ_GPTIM2_CC4        56U
/** @brief DMA request ID for SDMMC1. */
#define SF32LB58X_DMA_REQ_SDMMC1            57U

/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_DMA_SF32LB58X_DMA_H_ */
