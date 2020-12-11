/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _I2S_LITEI2S__H
#define _I2S_LITEI2S__H

#include <device.h>
#include <drivers/i2s.h>
#include <devicetree.h>

/* i2s register offsets*/
#define I2S_EV_STATUS_REG_OFFSET 0x0
#define I2S_EV_PENDING_REG_OFFSET 0x4
#define I2S_EV_ENABLE_REG_OFFSET 0x8
#define I2S_CONTROL_REG_OFFSET 0xc
#define I2S_STATUS_REG_OFFSET 0x10
#define I2S_CONFIG_REG_OFFSET 0x20

/* i2s configuration mask*/
#define I2S_CONF_FORMAT_OFFSET 0
#define I2S_CONF_SAMPLE_WIDTH_OFFSET 2
#define I2S_CONF_LRCK_FREQ_OFFSET 8
#define I2S_CONF_FORMAT_MASK (0x3 << I2S_CONF_FORMAT_OFFSET)
#define I2S_CONF_SAMPLE_WIDTH_MASK (0x3f << I2S_CONF_SAMPLE_WIDTH_OFFSET)
#define I2S_CONF_LRCK_MASK (0xffffff << I2S_CONF_LRCK_FREQ_OFFSET)

/* i2s control register options*/
#define I2S_ENABLE (1 << 0)
#define I2S_FIFO_RESET (1 << 1)
/* i2s event*/
#define I2S_EV_ENABLE (1 << 0)
/* i2s event types*/
#define I2S_EV_READY (1 << 0)
#define I2S_EV_ERROR (1 << 1)
/* i2s rx*/
#define I2S_RX_BASE_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(i2s_rx), control)
#define I2S_RX_EV_STATUS_REG (I2S_RX_BASE_ADDR + I2S_EV_STATUS_REG_OFFSET)
#define I2S_RX_EV_PENDING_REG (I2S_RX_BASE_ADDR + I2S_EV_PENDING_REG_OFFSET)
#define I2S_RX_EV_ENABLE_REG (I2S_RX_BASE_ADDR + I2S_EV_ENABLE_REG_OFFSET)
#define I2S_RX_CONTROL_REG (I2S_RX_BASE_ADDR + I2S_CONTROL_REG_OFFSET)
#define I2S_RX_STATUS_REG (I2S_RX_BASE_ADDR + I2S_STATUS_REG_OFFSET)
#define I2S_RX_CONFIG_REG (I2S_RX_BASE_ADDR + I2S_CONFIG_REG_OFFSET)

#define I2S_RX_STAT_CHANNEL_CONCATENATED_OFFSET 31
#define I2S_RX_STAT_CHANNEL_CONCATENATED_MASK                                  \
	(0x1 << I2S_RX_STAT_CHANNEL_CONCATENATED_OFFSET)

#define I2S_RX_FIFO_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(i2s_rx), fifo)
#define I2S_RX_FIFO_DEPTH DT_PROP(DT_NODELABEL(i2s_rx), fifo_depth)

/* i2s tx*/
#define I2S_TX_BASE_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(i2s_tx), control)
#define I2S_TX_EV_STATUS_REG (I2S_RX_BASE_ADDR + I2S_EV_STATUS_REG_OFFSET)
#define I2S_TX_EV_PENDING_REG (I2S_TX_BASE_ADDR + I2S_EV_PENDING_REG_OFFSET)
#define I2S_TX_EV_ENABLE_REG (I2S_TX_BASE_ADDR + I2S_EV_ENABLE_REG_OFFSET)
#define I2S_TX_CONTROL_REG (I2S_TX_BASE_ADDR + I2S_CONTROL_REG_OFFSET)
#define I2S_TX_STATUS_REG (I2S_TX_BASE_ADDR + I2S_STATUS_REG_OFFSET)
#define I2S_TX_CONFIG_REG (I2S_TX_BASE_ADDR + I2S_CONFIG_REG_OFFSET)

#define I2S_TX_STAT_CHANNEL_CONCATENATED_OFFSET 24
#define I2S_TX_STAT_CHANNEL_CONCATENATED_MASK                                  \
	(0x1 << I2S_TX_STAT_CHANNEL_CONCATENATED_OFFSET)

#define I2S_TX_FIFO_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(i2s_tx), fifo)
#define I2S_TX_FIFO_DEPTH DT_PROP(DT_NODELABEL(i2s_tx), fifo_depth)

enum litex_i2s_fmt {
	LITEX_I2S_STANDARD = 1,
	LITEX_I2S_LEFT_JUSTIFIED = 2,
};

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

struct stream {
	int32_t state;
	struct k_sem sem;
	struct i2s_config cfg;
	struct ring_buf mem_block_queue;
	void *mem_block;
};

/* Device run time data */
struct i2s_litex_data {
	struct stream rx;
	struct stream tx;
};

/* Device const configuration */
struct i2s_litex_cfg {
	uint32_t base;
	uint32_t fifo_base;
	uint16_t fifo_depth;
	void (*irq_config)(const struct device *dev);
};

#endif /* _I2S_LITEI2S__H */
