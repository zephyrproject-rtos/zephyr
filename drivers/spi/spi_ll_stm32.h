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

static inline u32_t ll_func_tx_is_empty(SPI_TypeDef *spi)
{
	return LL_SPI_IsActiveFlag_TXE(spi);
}

static inline u32_t ll_func_rx_is_not_empty(SPI_TypeDef *spi)
{
	return LL_SPI_IsActiveFlag_RXNE(spi);
}

static inline void ll_func_enable_int_tx_empty(SPI_TypeDef *spi)
{
	LL_SPI_EnableIT_TXE(spi);
}

static inline void ll_func_enable_int_rx_not_empty(SPI_TypeDef *spi)
{
	LL_SPI_EnableIT_RXNE(spi);
}

static inline void ll_func_enable_int_errors(SPI_TypeDef *spi)
{
	LL_SPI_EnableIT_ERR(spi);
}

static inline void ll_func_disable_int_tx_empty(SPI_TypeDef *spi)
{
	LL_SPI_DisableIT_TXE(spi);
}

static inline void ll_func_disable_int_rx_not_empty(SPI_TypeDef *spi)
{
	LL_SPI_DisableIT_RXNE(spi);
}

static inline void ll_func_disable_int_errors(SPI_TypeDef *spi)
{
	LL_SPI_DisableIT_ERR(spi);
}

static inline u32_t ll_func_spi_is_busy(SPI_TypeDef *spi)
{
	return LL_SPI_IsActiveFlag_BSY(spi);
}

/* Header is compiled first, this switch avoid the compiler to lookup for
 * non-existing LL FIFO functions for SoC without SPI FIFO
 */
#ifdef CONFIG_SPI_STM32_HAS_FIFO
static inline void ll_func_set_fifo_threshold_8bit(SPI_TypeDef *spi)
{
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
}
#endif

static inline void ll_func_disable_spi(SPI_TypeDef *spi)
{
	LL_SPI_Disable(spi);
}

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_ */
