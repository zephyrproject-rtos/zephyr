/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif RMT public API
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_H_

/**
 * @brief Espressif RMT.
 * @defgroup espressif_rmt Espressif RMT
 * @ingroup misc_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include "hal/rmt_hal.h"

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Undefined Tx/Rx DMA channel */
#define ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED (0xFF)

/**
 * @brief Espressif RMT config
 */
struct espressif_rmt_config {
	/** Reference to the pinctrl */
	const struct pinctrl_dev_config *pcfg;
	/** Reference to the DMA device */
	const struct device *dma_dev;
	/** TX DMA channel, ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED if not defined */
	uint8_t tx_dma_channel;
	/** RX DMA channel, ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED if not defined */
	uint8_t rx_dma_channel;
	/** Reference to the clock device */
	const struct device *clock_dev;
	/** Reference to the clock controller */
	const clock_control_subsys_t clock_subsys;
	/** RMT IRQ source */
	int irq_source;
	/** RMT IRQ priority */
	int irq_priority;
	/** RMT IRQ flags */
	int irq_flags;
};

/**
 * @brief Espressif RMT data
 */
struct espressif_rmt_data {
	/** Reference to HAL */
	rmt_hal_context_t hal;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_H_ */
