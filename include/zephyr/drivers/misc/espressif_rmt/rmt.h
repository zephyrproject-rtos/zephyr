/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
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

/* Undefined Tx/Rx DMA channel */
#define ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED (0xFF)

struct espressif_rmt_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_channel;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct espressif_rmt_data {
	rmt_hal_context_t hal;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_H_ */
