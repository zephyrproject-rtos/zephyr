/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTL87X2G DMA Devicetree Bindings
 *
 * Handshake (slot) IDs for the RTL87X2G GDMA controller. Values match the
 * GDMA_Handshake_* defines in the HAL (rtl_gdma_def.h). Use these in the slot
 * cell together with the "config" cell macros from <dt-bindings/dma/bee-dma.h>.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL87X2G_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL87X2G_DMA_H_

#include "bee-dma.h"

/* Handshake (slot) IDs, second cell of a "dmas" entry. */
#define BEE_DMA_HANDSHAKE_UART0_TX     0
#define BEE_DMA_HANDSHAKE_UART0_RX     1
#define BEE_DMA_HANDSHAKE_UART1_TX     2
#define BEE_DMA_HANDSHAKE_UART1_RX     3
#define BEE_DMA_HANDSHAKE_UART2_TX     4
#define BEE_DMA_HANDSHAKE_UART2_RX     5
#define BEE_DMA_HANDSHAKE_UART3_TX     6
#define BEE_DMA_HANDSHAKE_UART3_RX     7
#define BEE_DMA_HANDSHAKE_I2C0_TX      8
#define BEE_DMA_HANDSHAKE_I2C0_RX      9
#define BEE_DMA_HANDSHAKE_I2C1_TX      10
#define BEE_DMA_HANDSHAKE_I2C1_RX      11
#define BEE_DMA_HANDSHAKE_I2C2_TX      12
#define BEE_DMA_HANDSHAKE_I2C2_RX      13
#define BEE_DMA_HANDSHAKE_I2C3_TX      14
#define BEE_DMA_HANDSHAKE_I2C3_RX      15
#define BEE_DMA_HANDSHAKE_SPI0_TX      16
#define BEE_DMA_HANDSHAKE_SPI0_RX      17
#define BEE_DMA_HANDSHAKE_SPI1_TX      18
#define BEE_DMA_HANDSHAKE_SPI1_RX      19
#define BEE_DMA_HANDSHAKE_SPI_SLAVE_TX 20
#define BEE_DMA_HANDSHAKE_SPI_SLAVE_RX 21
#define BEE_DMA_HANDSHAKE_ENH_TIM0     22
#define BEE_DMA_HANDSHAKE_ENH_TIM1     23
#define BEE_DMA_HANDSHAKE_ENH_TIM2     24
#define BEE_DMA_HANDSHAKE_ENH_TIM3     25
#define BEE_DMA_HANDSHAKE_ADC_RX       26
#define BEE_DMA_HANDSHAKE_IDU_RX       27
#define BEE_DMA_HANDSHAKE_IDU_TX       28
#define BEE_DMA_HANDSHAKE_TIM5         29
#define BEE_DMA_HANDSHAKE_TIM6         30
#define BEE_DMA_HANDSHAKE_TIM7         31
#define BEE_DMA_HANDSHAKE_CAN_BUS_RX   32
#define BEE_DMA_HANDSHAKE_SPIC0_TX     33
#define BEE_DMA_HANDSHAKE_SPIC0_RX     34
#define BEE_DMA_HANDSHAKE_SPIC1_TX     35
#define BEE_DMA_HANDSHAKE_SPIC1_RX     36
#define BEE_DMA_HANDSHAKE_SPIC2_TX     37
#define BEE_DMA_HANDSHAKE_SPIC2_RX     38
#define BEE_DMA_HANDSHAKE_IR_TX        39
#define BEE_DMA_HANDSHAKE_IR_RX        40
#define BEE_DMA_HANDSHAKE_I2S0_TDM0_TX 41
#define BEE_DMA_HANDSHAKE_I2S0_TDM0_RX 42
#define BEE_DMA_HANDSHAKE_I2S1_TDM0_TX 43
#define BEE_DMA_HANDSHAKE_I2S1_TDM0_RX 44
#define BEE_DMA_HANDSHAKE_SHA256_TX    45
#define BEE_DMA_HANDSHAKE_AES_TX       47
#define BEE_DMA_HANDSHAKE_AES_RX       48
#define BEE_DMA_HANDSHAKE_PUB_KEY_TX   49
#define BEE_DMA_HANDSHAKE_PUB_KEY_RX   50
#define BEE_DMA_HANDSHAKE_24BIT_ADC_RX 51
#define BEE_DMA_HANDSHAKE_UART4_TX     52
#define BEE_DMA_HANDSHAKE_UART4_RX     53
#define BEE_DMA_HANDSHAKE_UART5_TX     54
#define BEE_DMA_HANDSHAKE_UART5_RX     55

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RTL87X2G_DMA_H_ */
