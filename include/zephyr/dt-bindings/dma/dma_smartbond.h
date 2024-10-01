/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_SMARTBOND_H_
#define DMA_SMARTBOND_H_

/**
 * @brief Vendror-specific DMA peripheral triggering sources.
 *
 * A valid triggering source should be provided when DMA
 * is configured for peripheral to peripheral or memory to peripheral
 * transactions.
 */
#define DMA_SMARTBOND_TRIG_MUX_SPI		0x0
#define DMA_SMARTBOND_TRIG_MUX_SPI2		0x1
#define DMA_SMARTBOND_TRIG_MUX_UART		0x2
#define DMA_SMARTBOND_TRIG_MUX_UART2		0x3
#define DMA_SMARTBOND_TRIG_MUX_I2C		0x4
#define DMA_SMARTBOND_TRIG_MUX_I2C2		0x5
#define DMA_SMARTBOND_TRIG_MUX_USB		0x6
#define DMA_SMARTBOND_TRIG_MUX_UART3		0x7
#define DMA_SMARTBOND_TRIG_MUX_PCM		0x8
#define DMA_SMARTBOND_TRIG_MUX_SRC		0x9
#define DMA_SMARTBOND_TRIG_MUX_GPADC		0xC
#define DMA_SMARTBOND_TRIG_MUX_SDADC		0xD
#define DMA_SMARTBOND_TRIG_MUX_NONE		0xF

#endif /* DMA_SMARTBOND_H_ */
