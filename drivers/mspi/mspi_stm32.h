/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_nor.h"

#ifndef ZEPHYR_DRIVERS_MSPI_STM32_H_
#define ZEPHYR_DRIVERS_MSPI_STM32_H_
/* Macro to check if any xspi device has a domain clock or more */
#define MSPI_STM32_DOMAIN_CLOCK_INST_SUPPORT(inst) DT_CLOCKS_HAS_IDX(DT_INST_PARENT(inst), 1) ||
#define MSPI_STM32_INST_DEV_DOMAIN_CLOCK_SUPPORT                                                   \
	(DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_DOMAIN_CLOCK_INST_SUPPORT) 0)

/* This symbol takes the value 1 if device instance has a domain clock in its dts */
#if MSPI_STM32_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define MSPI_STM32_DOMAIN_CLOCK_SUPPORT 1
#else
#define MSPI_STM32_DOMAIN_CLOCK_SUPPORT 0
#endif

#define MSPI_STM32_FIFO_THRESHOLD 4U
#define MSPI_MAX_FREQ      250000000
#define MSPI_MAX_DEVICE    2

#if defined(CONFIG_SOC_SERIES_STM32U5X)
/* Valid range is [1, 256] */
#define MSPI_STM32_CLOCK_PRESCALER_MIN  1U
#define MSPI_STM32_CLOCK_PRESCALER_MAX  256U
#define MSPI_STM32_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / (prescaler))
#else
/* Valid range is [0, 255] */
#define MSPI_STM32_CLOCK_PRESCALER_MIN  0U
#define MSPI_STM32_CLOCK_PRESCALER_MAX  255U
#define MSPI_STM32_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / ((prescaler) + 1U))
#endif

#define MSPI_STM32_WRITE_REG_MAX_TIME          40U
#define MSPI_STM32_MAX_FREQ                    48000000


typedef void (*irq_config_func_t)(void);

#ifndef DMA_LOW_PRIORITY_LOW_WEIGHT
#define DMA_LOW_PRIORITY_LOW_WEIGHT    DMA_PRIORITY_LOW
#endif

#ifndef DMA_LOW_PRIORITY_MID_WEIGHT
#define DMA_LOW_PRIORITY_MID_WEIGHT    DMA_PRIORITY_MEDIUM
#endif

#ifndef DMA_LOW_PRIORITY_HIGH_WEIGHT
#define DMA_LOW_PRIORITY_HIGH_WEIGHT   DMA_PRIORITY_HIGH
#endif

#ifndef DMA_HIGH_PRIORITY
#define DMA_HIGH_PRIORITY              DMA_PRIORITY_VERY_HIGH
#endif

enum mspi_access_mode {
	MSPI_ACCESS_ASYNC = 1,
	MSPI_ACCESS_SYNC = 2,
	MSPI_ACCESS_DMA = 3
};

struct mspi_context {
	struct mspi_xfer xfer;
	int packets_left;
	struct k_sem lock;
};

struct mspi_stm32_conf {
	bool use_dma;
	size_t pclk_len;
	irq_config_func_t irq_config;
	struct mspi_cfg mspicfg;
	const struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pcfg;
};

/* Lookup table to set dma priority from the DTS */
static const uint32_t table_priority[] = {
	DMA_LOW_PRIORITY_LOW_WEIGHT,
	DMA_LOW_PRIORITY_MID_WEIGHT,
	DMA_LOW_PRIORITY_HIGH_WEIGHT,
	DMA_HIGH_PRIORITY,
};

/* Lookup table to set dma channel direction from the DTS */
static const uint32_t table_direction[] = {
	DMA_MEMORY_TO_MEMORY,
	DMA_MEMORY_TO_PERIPH,
	DMA_PERIPH_TO_MEMORY,
};

static const uint32_t table_src_size[] = {
	#ifndef LL_DMA_SRC_DATAWIDTH_BYTE
	#define LL_DMA_SRC_DATAWIDTH_BYTE    LL_DMA_MDATAALIGN_BYTE
	#endif
	#ifndef LL_DMA_SRC_DATAWIDTH_HALFWORD
	#define LL_DMA_SRC_DATAWIDTH_HALFWORD    LL_DMA_MDATAALIGN_HALFWORD
	#endif
	#ifndef LL_DMA_SRC_DATAWIDTH_WORD
	#define LL_DMA_SRC_DATAWIDTH_WORD    LL_DMA_MDATAALIGN_WORD
	#endif
};

static const uint32_t table_dest_size[] = {
	#ifndef LL_DMA_DEST_DATAWIDTH_BYTE
	#define LL_DMA_DEST_DATAWIDTH_BYTE    LL_DMA_PDATAALIGN_BYTE
	#endif
	#ifndef LL_DMA_DEST_DATAWIDTH_HALFWORD
	#define LL_DMA_DEST_DATAWIDTH_HALFWORD    LL_DMA_PDATAALIGN_HALFWORD
	#endif
	#ifndef LL_DMA_DEST_DATAWIDTH_WORD
	#define LL_DMA_DEST_DATAWIDTH_WORD    LL_DMA_PDATAALIGN_WORD
	#endif
};

struct stream {
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
};

union hmspi_handle {
#ifdef CONFIG_MSPI_STM32_XSPI
	XSPI_HandleTypeDef xspi;
#endif
#ifdef CONFIG_MSPI_STM32_OSPI
	OSPI_HandleTypeDef ospi;
#endif
#ifdef CONFIG_MSPI_STM32_QSPI
	QSPI_HandleTypeDef qspi;
#endif
};

/* mspi data includes the controller specific config variable */
struct mspi_stm32_data {
	union hmspi_handle hmspi;
	uint32_t memmap_base_addr;
	struct mspi_context ctx;
	struct mspi_dev_id *dev_id;
	struct k_mutex lock;
	struct k_sem sync;
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	struct stream dma_tx;
	struct stream dma_rx;
	struct stream dma;
};

#endif /* ZEPHYR_DRIVERS_MSPI_STM32_H_ */
