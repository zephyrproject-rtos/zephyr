/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_

#include "spi_context.h"

typedef void (*irq_config_func_t)(struct device *port);

struct spi_stm32_config {
	struct stm32_pclken pclken;
	SPI_TypeDef *spi;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	irq_config_func_t irq_config;
#endif
};

struct spi_stm32_data {
	struct spi_context ctx;
};

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_ */
