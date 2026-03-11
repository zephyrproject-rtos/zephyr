/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_STM32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_STM32_H_

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
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	int datawidth;
#ifdef CONFIG_SPI_STM32_INTERRUPT
	irq_config_func_t irq_config;
#ifdef CONFIG_SOC_SERIES_STM32H7X
	uint32_t irq_line;
#endif /* CONFIG_SOC_SERIES_STM32H7X */
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	int midi_clocks;
	int mssi_clocks;
	uint32_t fifo_max_transfer_size;
	uint8_t fifo_size;
#endif
	bool fifo_enabled: 1;
	bool ioswp: 1;
	bool soft_nss: 1;
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_subghz)
	bool use_subghzspi_nss: 1;
#endif
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
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif /* CONFIG_SPI_RTIO */
	struct spi_context ctx;
	uint32_t tx_len;
	uint32_t rx_len;
	uint8_t fifo_threshold;
#ifdef CONFIG_SPI_STM32_DMA
	struct k_sem status_sem;
	volatile uint32_t status_flags;
	struct stream dma_rx;
	struct stream dma_tx;
#endif /* CONFIG_SPI_STM32_DMA */
	bool pm_policy_state_on;
};

#ifdef CONFIG_SPI_STM32_DMA
static inline uint32_t ll_dma_get_reg_addr(SPI_TypeDef *spi, uint32_t location)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (location == SPI_STM32_DMA_TX) {
		return LL_SPI_DMA_GetTxRegAddr(spi);
	}
	return LL_SPI_DMA_GetRxRegAddr(spi);
#else
	ARG_UNUSED(location);
	return (uint32_t)LL_SPI_DMA_GetRegAddr(spi);
#endif /* st_stm32h7_spi */
}

/* checks that DMA Tx packet is fully transmitted over the SPI */
static inline uint32_t ll_spi_dma_busy(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (LL_SPI_GetTransferSize(spi) == 0) {
		return LL_SPI_IsActiveFlag_TXC(spi) == 0;
	} else {
		return LL_SPI_IsActiveFlag_EOT(spi) == 0;
	}
#else
	/* the SPI Tx empty and busy flags are needed */
	return (!LL_SPI_IsActiveFlag_TXE(spi) ||
		LL_SPI_IsActiveFlag_BSY(spi));
#endif /* LL_SPI_SR_TXC */
}
#endif /* st_stm32h7_spi */

static inline uint32_t ll_tx_is_not_full(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_TXP(spi);
#else
	return LL_SPI_IsActiveFlag_TXE(spi);
#endif /* st_stm32h7_spi */
}

static inline uint32_t ll_rx_is_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_RXP(spi);
#else
	return LL_SPI_IsActiveFlag_RXNE(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_enable_int_tx_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_EnableIT_TXP(spi);
#else
	LL_SPI_EnableIT_TXE(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_enable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_EnableIT_RXP(spi);
#else
	LL_SPI_EnableIT_RXNE(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_enable_int_errors(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_EnableIT_UDR(spi);
	LL_SPI_EnableIT_OVR(spi);
	LL_SPI_EnableIT_CRCERR(spi);
	LL_SPI_EnableIT_FRE(spi);
	LL_SPI_EnableIT_MODF(spi);
#else
	LL_SPI_EnableIT_ERR(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_disable_int_tx_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_TXP(spi);
#else
	LL_SPI_DisableIT_TXE(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_disable_int_rx_not_empty(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_RXP(spi);
#else
	LL_SPI_DisableIT_RXNE(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_disable_int_errors(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_UDR(spi);
	LL_SPI_DisableIT_OVR(spi);
	LL_SPI_DisableIT_CRCERR(spi);
	LL_SPI_DisableIT_FRE(spi);
	LL_SPI_DisableIT_MODF(spi);
#else
	LL_SPI_DisableIT_ERR(spi);
#endif /* st_stm32h7_spi */
}

static inline bool ll_are_int_disabled(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return (spi->IER == 0U);
#else
	return !LL_SPI_IsEnabledIT_ERR(spi) &&
	       !LL_SPI_IsEnabledIT_RXNE(spi) &&
	       !LL_SPI_IsEnabledIT_TXE(spi);
#endif
}

static inline uint32_t ll_spi_is_busy(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (LL_SPI_GetTransferSize(spi) == 0) {
		return LL_SPI_IsActiveFlag_TXC(spi) == 0;
	} else {
		return LL_SPI_IsActiveFlag_EOT(spi) == 0;
	}
#else
	return LL_SPI_IsActiveFlag_BSY(spi);
#endif /* st_stm32h7_spi */
}

static inline void ll_disable_spi(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)
	/* Flush RX buffer */
	while (ll_rx_is_not_empty(spi)) {
		(void) LL_SPI_ReceiveData8(spi);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo) */

	LL_SPI_Disable(spi);

	while (LL_SPI_IsEnabled(spi)) {
		/* NOP */
	}
}

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_STM32_H_ */
