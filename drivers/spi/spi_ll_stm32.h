/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_

#include "spi_context.h"

typedef void (*irq_config_func_t)(const struct device *port);

struct spi_stm32_config {
	struct stm32_pclken pclken;
	SPI_TypeDef *spi;
	const struct soc_gpio_pinctrl *pinctrl_list;
	size_t pinctrl_list_size;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	irq_config_func_t irq_config;
#endif
};

#ifdef CONFIG_SPI_STM32_DMA

#define SPI_STM32_DMA_ERROR_FLAG	0x01
#define SPI_STM32_DMA_RX_DONE_FLAG	0x02
#define SPI_STM32_DMA_TX_DONE_FLAG	0x04
#define SPI_STM32_DMA_DONE_FLAG	\
	(SPI_STM32_DMA_RX_DONE_FLAG | SPI_STM32_DMA_TX_DONE_FLAG)

struct stream {
	const char *dma_name;
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma or mux */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
	int fifo_threshold;
};
#endif

struct spi_stm32_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_STM32_DMA
	struct k_sem status_sem;
	volatile uint32_t status_flags;
	struct stream dma_rx;
	struct stream dma_tx;
#endif
};

static inline uint32_t ll_func_tx_is_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	return LL_SPI_IsActiveFlag_TXP(spi);
#else
	return LL_SPI_IsActiveFlag_TXE(spi);
#endif
}

static inline uint32_t ll_func_rx_is_not_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	return LL_SPI_IsActiveFlag_RXP(spi);
#else
	return LL_SPI_IsActiveFlag_RXNE(spi);
#endif
}

static inline void ll_func_enable_int_tx_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_EnableIT_TXP(spi);
#else
	LL_SPI_EnableIT_TXE(spi);
#endif
}

static inline void ll_func_enable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_EnableIT_RXP(spi);
#else
	LL_SPI_EnableIT_RXNE(spi);
#endif
}

static inline void ll_func_enable_int_errors(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_EnableIT_UDR(spi);
	LL_SPI_EnableIT_OVR(spi);
	LL_SPI_EnableIT_CRCERR(spi);
	LL_SPI_EnableIT_FRE(spi);
	LL_SPI_EnableIT_MODF(spi);
#else
	LL_SPI_EnableIT_ERR(spi);
#endif
}

static inline void ll_func_disable_int_tx_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_DisableIT_TXP(spi);
#else
	LL_SPI_DisableIT_TXE(spi);
#endif
}

static inline void ll_func_disable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_DisableIT_RXP(spi);
#else
	LL_SPI_DisableIT_RXNE(spi);
#endif
}

static inline void ll_func_disable_int_errors(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_DisableIT_UDR(spi);
	LL_SPI_DisableIT_OVR(spi);
	LL_SPI_DisableIT_CRCERR(spi);
	LL_SPI_DisableIT_FRE(spi);
	LL_SPI_DisableIT_MODF(spi);
#else
	LL_SPI_DisableIT_ERR(spi);
#endif
}

static inline uint32_t ll_func_spi_is_busy(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	return (!LL_SPI_IsActiveFlag_MODF(spi) &&
		!LL_SPI_IsActiveFlag_TXC(spi));
#else
	return LL_SPI_IsActiveFlag_BSY(spi);
#endif
}

/* Header is compiled first, this switch avoid the compiler to lookup for
 * non-existing LL FIFO functions for SoC without SPI FIFO
 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
static inline void ll_func_set_fifo_threshold_8bit(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_01DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#endif
}

static inline void ll_func_set_fifo_threshold_16bit(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_02DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_HALF);
#endif
}
#endif

static inline void ll_func_disable_spi(SPI_TypeDef *spi)
{
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
    defined(CONFIG_SOC_SERIES_STM32H7X)
	if (LL_SPI_IsActiveMasterTransfer(spi)) {
		LL_SPI_SuspendMasterTransfer(spi);
		while (LL_SPI_IsActiveMasterTransfer(spi)) {
			/* NOP */
		}
	}

	LL_SPI_Disable(spi);
	while (LL_SPI_IsEnabled(spi)) {
		/* NOP */
	}

	/* Flush RX buffer */
	while (LL_SPI_IsActiveFlag_RXP(spi)) {
		(void)LL_SPI_ReceiveData8(spi);
	}
	LL_SPI_ClearFlag_SUSP(spi);
#else
	LL_SPI_Disable(spi);
#endif
}

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_ */
