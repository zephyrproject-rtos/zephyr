/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTL8752H DMA Devicetree Bindings
 *
 * Handshake (slot) IDs for the RTL8752H GDMA controller. Values match the
 * GDMA_Handshake_* defines in the HAL (rtl876x_gdma.h). Use these in the slot
 * cell together with the "config" cell macros from <dt-bindings/dma/bee-dma.h>.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL8752H_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL8752H_DMA_H_

#include "bee-dma.h"

/* Handshake (slot) IDs, second cell of a "dmas" entry. */
#define BEE_DMA_HANDSHAKE_UART1_TX  0
#define BEE_DMA_HANDSHAKE_UART1_RX  1
#define BEE_DMA_HANDSHAKE_ENH_TIM0  2
#define BEE_DMA_HANDSHAKE_ENH_TIM1  3
#define BEE_DMA_HANDSHAKE_SPI0_TX   4
#define BEE_DMA_HANDSHAKE_SPI0_RX   5
#define BEE_DMA_HANDSHAKE_SPI1_TX   6
#define BEE_DMA_HANDSHAKE_SPI1_RX   7
#define BEE_DMA_HANDSHAKE_I2C0_TX   8
#define BEE_DMA_HANDSHAKE_I2C0_RX   9
#define BEE_DMA_HANDSHAKE_I2C1_TX   10
#define BEE_DMA_HANDSHAKE_I2C1_RX   11
#define BEE_DMA_HANDSHAKE_ADC       12
#define BEE_DMA_HANDSHAKE_AES_TX    13
#define BEE_DMA_HANDSHAKE_AES_RX    14
#define BEE_DMA_HANDSHAKE_UART0_TX  15
#define BEE_DMA_HANDSHAKE_I2S0_TX   16
#define BEE_DMA_HANDSHAKE_I2S0_RX   17
#define BEE_DMA_HANDSHAKE_UART2_TX  18
#define BEE_DMA_HANDSHAKE_UART2_RX  19
#define BEE_DMA_HANDSHAKE_SPIC0_TX  20
#define BEE_DMA_HANDSHAKE_SPIC0_RX  21
#define BEE_DMA_HANDSHAKE_I8080_RX  22
#define BEE_DMA_HANDSHAKE_UART0_RX  23
#define BEE_DMA_HANDSHAKE_TIM0      24
#define BEE_DMA_HANDSHAKE_TIM1      25
#define BEE_DMA_HANDSHAKE_TIM2      26
#define BEE_DMA_HANDSHAKE_IR_TX     27
#define BEE_DMA_HANDSHAKE_IR_RX     28
#define BEE_DMA_HANDSHAKE_TIM3      29
#define BEE_DMA_HANDSHAKE_TIM4      30
#define BEE_DMA_HANDSHAKE_TIM5      31
#define BEE_DMA_HANDSHAKE_SPIC1_TX  32
#define BEE_DMA_HANDSHAKE_SPIC1_RX  33
#define BEE_DMA_HANDSHAKE_SHA256_TX 35

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL8752H_DMA_H_ */
