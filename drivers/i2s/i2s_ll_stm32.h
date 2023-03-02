/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_I2S_H_
#define _STM32_I2S_H_

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
struct i2s_stm32_cfg {
	SPI_TypeDef *i2s;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
	bool master_clk_sel;
};

struct stream {
	int32_t state;
	struct k_sem sem;

	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
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
struct i2s_stm32_data {
	struct stream rx;
	struct stream tx;
};

#endif	/* _STM32_I2S_H_ */
