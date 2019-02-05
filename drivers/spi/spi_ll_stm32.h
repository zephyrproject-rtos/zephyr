/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_

#include "spi_context.h"

#ifdef CONFIG_SPI_STM32_DMA
#include <dma.h>
#endif

typedef void (*irq_config_func_t)(struct device *port);

struct spi_stm32_config {
	struct stm32_pclken pclken;
	SPI_TypeDef *spi;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	irq_config_func_t irq_config;
#endif
#ifdef CONFIG_SPI_STM32_DMA
	u32_t stream[2];
	const char *dmadev;
#endif
};

struct spi_stm32_data {
	struct spi_context ctx;
	bool armed;
#ifdef CONFIG_SPI_STM32_DMA
	struct device *d;
	struct dma_config dma_conf[2];
	struct dma_block_config b[2];
	u32_t nDMA;
#endif
};

#endif  /* ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_ */
