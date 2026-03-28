/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_SMARTBOND_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_SMARTBOND_H_

/**
 * @brief Vendror-specific DMA peripheral triggering sources.
 *
 * A valid triggering source should be provided when DMA
 * is configured for peripheral to peripheral or memory to peripheral
 * transactions.
 */
enum dma_smartbond_trig_mux {
	DMA_SMARTBOND_TRIG_MUX_SPI   = 0x0,
	DMA_SMARTBOND_TRIG_MUX_SPI2  = 0x1,
	DMA_SMARTBOND_TRIG_MUX_UART  = 0x2,
	DMA_SMARTBOND_TRIG_MUX_UART2 = 0x3,
	DMA_SMARTBOND_TRIG_MUX_I2C   = 0x4,
	DMA_SMARTBOND_TRIG_MUX_I2C2  = 0x5,
	DMA_SMARTBOND_TRIG_MUX_USB   = 0x6,
	DMA_SMARTBOND_TRIG_MUX_UART3 = 0x7,
	DMA_SMARTBOND_TRIG_MUX_PCM   = 0x8,
	DMA_SMARTBOND_TRIG_MUX_SRC   = 0x9,
	DMA_SMARTBOND_TRIG_MUX_GPADC = 0xC,
	DMA_SMARTBOND_TRIG_MUX_SDADC = 0xD,
	DMA_SMARTBOND_TRIG_MUX_NONE  = 0xF
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_SMARTBOND_H_ */
