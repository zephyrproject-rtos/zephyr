/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rmt

#include "hal/rmt_hal.h"
#include "rmt_private.h"
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <esp_attr.h>

#if SOC_RMT_SUPPORT_DMA && !DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_gdma)
#error "DMA peripheral is not enabled!"
#endif

LOG_MODULE_REGISTER(espressif_rmt, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

static int rmt_init(const struct device *dev)
{
	const struct espressif_rmt_config *config = dev->config;
	int rc;

	/* Configure pins */
	rc = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc) {
		LOG_ERR("Failed to configure RMT pins");
		return rc;
	}

	return 0;
}

/* Retrieve DMA device if defined, else default to NULL */
#define ESPRESSIF_RMT_DT_INST_DMA_CTLR(idx)                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, dmas),                                   \
		(DEVICE_DT_GET(DT_INST_DMAS_CTLR(0))),                                      \
		(NULL))

/* Retrieve DMA channel if defined, else default to ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED */
#define ESPRESSIF_RMT_DT_INST_DMA_CELL(idx, name, cell)                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, dmas),                                   \
		(COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, name),                              \
		(DT_INST_DMAS_CELL_BY_NAME(idx, name, cell)),                               \
		(ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED))),                                    \
		(ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED))

#define ESPRESSIF_RMT_INIT(idx)                                                     \
	PINCTRL_DT_DEFINE(DT_NODELABEL(rmt));                                           \
	static const DRAM_ATTR struct espressif_rmt_config espressif_rmt_cfg_##idx = {  \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(rmt)),                       \
		.dma_dev = ESPRESSIF_RMT_DT_INST_DMA_CTLR(idx),                             \
		.tx_dma_channel = ESPRESSIF_RMT_DT_INST_DMA_CELL(0, tx, channel),           \
		.rx_dma_channel = ESPRESSIF_RMT_DT_INST_DMA_CELL(0, rx, channel),           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                       \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),   \
		.irq_source = DT_INST_IRQ_BY_IDX(idx, 0, irq),                              \
		.irq_priority = DT_INST_IRQ_BY_IDX(idx, 0, priority),                       \
		.irq_flags = DT_INST_IRQ_BY_IDX(idx, 0, flags),                             \
	};                                                                              \
	static struct espressif_rmt_data espressif_rmt_data_##idx = {                   \
		.hal = {                                                                    \
			.regs = (rmt_soc_handle_t)DT_INST_REG_ADDR(idx),                        \
		},                                                                          \
	};                                                                              \
	DEVICE_DT_INST_DEFINE(idx, rmt_init, NULL, &espressif_rmt_data_##idx,           \
		&espressif_rmt_cfg_##idx, PRE_KERNEL_1, CONFIG_ESPRESSIF_RMT_INIT_PRIORITY, \
		NULL);

DT_INST_FOREACH_STATUS_OKAY(ESPRESSIF_RMT_INIT);
