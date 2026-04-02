/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_MSPI_STM32_H_
#define ZEPHYR_DRIVERS_MSPI_MSPI_STM32_H_

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "mspi_nor.h"

#define MSPI_STM32_FIFO_THRESHOLD 4U
#define MSPI_MAX_DEVICE          1

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

typedef void (*irq_config_func_t)(void);

enum mspi_stm32_access_mode {
	MSPI_ACCESS_ASYNC = 1,
	MSPI_ACCESS_SYNC = 2,
	MSPI_ACCESS_DMA = 3
};

struct mspi_stm32_context {
	struct mspi_xfer xfer;
	int packets_left;
	struct k_sem lock;
};

struct mspi_stm32_conf {
	bool dma_specified;
	size_t pclk_len;
	irq_config_func_t irq_config;
	struct mspi_cfg mspicfg;
	const struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pcfg;
};

struct stm32_stream {
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
};

union mspi_stm32_handle {
#ifdef CONFIG_MSPI_STM32_OSPI
	OSPI_HandleTypeDef ospi;
#endif
#ifdef CONFIG_MSPI_STM32_QSPI
	QSPI_HandleTypeDef qspi;
#endif
#ifdef CONFIG_MSPI_STM32_XSPI
	XSPI_HandleTypeDef xspi;
#endif
};

/* mspi data includes the controller specific config variable */
struct mspi_stm32_data {
	union mspi_stm32_handle hmspi;
	uint32_t memmap_base_addr;
	struct mspi_stm32_context ctx;
	const struct mspi_dev_id *dev_id;
	struct k_mutex lock;
	struct k_sem sync;
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	struct stm32_stream dma_tx;
	struct stm32_stream dma_rx;
	struct stm32_stream dma;
#if defined(CONFIG_MSPI_STM32_OSPI) || defined(CONFIG_MSPI_STM32_QSPI)
	DMA_HandleTypeDef hdma;
#endif
#ifdef CONFIG_MSPI_STM32_XSPI
	DMA_HandleTypeDef hdma_tx;
	DMA_HandleTypeDef hdma_rx;
#endif
};

extern const uint32_t mspi_stm32_table_priority[];
extern const uint32_t mspi_stm32_table_direction[];
extern const uint32_t mspi_stm32_table_src_size[];
extern const uint32_t mspi_stm32_table_dest_size[];

#endif /* ZEPHYR_DRIVERS_MSPI_MSPI_STM32_H_ */
