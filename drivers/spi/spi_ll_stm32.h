/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_

#include "spi_context.h"

typedef void (*irq_config_func_t)(const struct device *port);

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_SPI_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_SPI_DOMAIN_CLOCK_SUPPORT 0
#endif

struct spi_stm32_config {
	SPI_TypeDef *spi;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	irq_config_func_t irq_config;
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
	bool use_subghzspi_nss;
#endif
	size_t pclk_len;
	const struct stm32_pclken *pclken;
};

#ifdef CONFIG_SPI_STM32_DMA

#define SPI_STM32_DMA_ERROR_FLAG	0x01
#define SPI_STM32_DMA_RX_DONE_FLAG	0x02
#define SPI_STM32_DMA_TX_DONE_FLAG	0x04
#define SPI_STM32_DMA_DONE_FLAG	\
	(SPI_STM32_DMA_RX_DONE_FLAG | SPI_STM32_DMA_TX_DONE_FLAG)

#define SPI_STM32_DMA_TX	0x01
#define SPI_STM32_DMA_RX	0x02

struct stream {
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
#endif /* CONFIG_SPI_STM32_DMA */
};

#ifdef CONFIG_SPI_STM32_DMA
static inline uint32_t ll_func_dma_get_reg_addr(SPI_TypeDef *spi, uint32_t location)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (location == SPI_STM32_DMA_TX) {
		/* use direct register location until the LL_SPI_DMA_GetTxRegAddr exists */
		return (uint32_t)&(spi->TXDR);
	}
	/* use direct register location until the LL_SPI_DMA_GetRxRegAddr exists */
	return (uint32_t)&(spi->RXDR);
#else
	ARG_UNUSED(location);
	return (uint32_t)LL_SPI_DMA_GetRegAddr(spi);
#endif /* st_stm32h7_spi */
}

/* checks that DMA Tx packet is fully transmitted over the SPI */
static inline uint32_t ll_func_spi_dma_busy(SPI_TypeDef *spi)
{
#ifdef LL_SPI_SR_TXC
	return LL_SPI_IsActiveFlag_TXC(spi);
#else
	/* the SPI Tx empty and busy flags are needed */
	return (LL_SPI_IsActiveFlag_TXE(spi) &&
		!LL_SPI_IsActiveFlag_BSY(spi));
#endif /* LL_SPI_SR_TXC */
}
#endif /* CONFIG_SPI_STM32_DMA */

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_LL_STM32_H_ */
