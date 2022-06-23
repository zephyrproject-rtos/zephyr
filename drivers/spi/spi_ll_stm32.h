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
/* is configured in dts with an optional clock */
#if STM32_DT_INST_DEV_OPT_CLOCK_SUPPORT
#define STM32_SPI_OPT_CLOCK_SUPPORT 1
#else
#define STM32_SPI_OPT_CLOCK_SUPPORT 0
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
#endif
};

#ifdef CONFIG_SPI_STM32_DMA
static inline uint32_t ll_func_dma_get_reg_addr(SPI_TypeDef *spi, uint32_t location)
{
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (location == SPI_STM32_DMA_TX) {
		/* use direct register location until the LL_SPI_DMA_GetTxRegAddr exists */
		return (uint32_t)&(spi->TXDR);
	}
	/* use direct register location until the LL_SPI_DMA_GetRxRegAddr exists */
	return (uint32_t)&(spi->RXDR);
#else
	ARG_UNUSED(location);
	return (uint32_t)LL_SPI_DMA_GetRegAddr(spi);
#endif /* CONFIG_SOC_SERIES_STM32H7X */
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
#endif
}
#endif /* CONFIG_SPI_STM32_DMA */

static inline uint32_t ll_func_tx_is_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_TXP(spi);
#else
	return LL_SPI_IsActiveFlag_TXE(spi);
#endif
}

static inline uint32_t ll_func_rx_is_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_RXP(spi);
#else
	return LL_SPI_IsActiveFlag_RXNE(spi);
#endif
}

static inline void ll_func_enable_int_tx_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_EnableIT_TXP(spi);
#else
	LL_SPI_EnableIT_TXE(spi);
#endif
}

static inline void ll_func_enable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_EnableIT_RXP(spi);
#else
	LL_SPI_EnableIT_RXNE(spi);
#endif
}

static inline void ll_func_enable_int_errors(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
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
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_TXP(spi);
#else
	LL_SPI_DisableIT_TXE(spi);
#endif
}

static inline void ll_func_disable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_RXP(spi);
#else
	LL_SPI_DisableIT_RXNE(spi);
#endif
}

static inline void ll_func_disable_int_errors(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
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
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_EOT(spi);
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
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_01DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#endif
}

static inline void ll_func_set_fifo_threshold_16bit(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_02DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_HALF);
#endif
}
#endif

static inline void ll_func_disable_spi(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
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
