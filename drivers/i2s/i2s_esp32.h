/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/dma.h>

#include "esp_types.h"
#include "esp_err.h"
#include "soc/i2s_periph.h"
#include "soc/rtc_periph.h"
#include "soc/soc_caps.h"
#include "hal/i2s_types.h"
#include "hal/i2s_hal.h"
#include "esp_intr_alloc.h"

struct queue_item {
	void *mem_block;
	size_t size;
};

/* Minimal ring buffer implementation */
struct ring_buf {
	struct queue_item *buf;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

/* Device constant configuration parameters */
struct i2s_esp32_cfg {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
	const int i2s_num;
};

struct stream {
	int32_t state;
	struct k_sem sem;

	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t fifo_threshold;

	struct i2s_config cfg;
	struct ring_buf mem_block_queue;
	void *mem_block;
	bool last_block;
	bool master;
	int (*stream_start)(struct stream *, const struct device *dev);
	void (*stream_disable)(struct stream *, const struct device *dev);
	void (*queue_drop)(struct stream *);
};

/* Device run time data */
struct i2s_esp32_data {
	struct stream rx;
	struct stream tx;

	i2s_hal_context_t hal_ctx;    /*!< I2S hal context*/
	i2s_hal_config_t hal_cfg; /*!< I2S hal configurations*/
	i2s_hal_clock_cfg_t clk_cfg;
};

#ifdef __cplusplus
}
#endif
