/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_I2S_AMEBA_H_
#define ZEPHYR_DRIVERS_I2S_I2S_AMEBA_H_

#include <zephyr/drivers/dma.h>

/* Device constant configuration parameters */
struct i2s_ameba_cfg {
	AUDIO_SPORT_TypeDef *i2s;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	bool MultiIO;
	uint8_t index;
	uint8_t pll_tune;
	uint32_t mclk_multiple;
	uint32_t mclk_fixed_max;
	uint32_t clock_mode;
	int irq;
};

struct i2s_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t src_addr_increment;
	uint8_t dst_addr_increment;
	int fifo_threshold;
	struct dma_block_config blk_cfg;
	uint8_t *buffer;
	size_t buffer_length;
	size_t offset;
	volatile size_t counter;
	int32_t timeout;
	struct k_work_delayable timeout_work;
	bool enabled;
};

struct i2s_ameba_stream {
	volatile int32_t state;
	struct i2s_config cfg;
	uint8_t free_tx_dma_blocks;
	bool last_block;
	struct k_msgq in_queue;
	struct k_msgq out_queue;
};

#define BUFFER_REORDER_8_24  BIT(2) /* pay attention to the paddings */
#define BUFFER_REORDER_CH(n) n

/* For <IS_REORDER_CH = 1 mode>, block size should be 384, 512, 1024 or above. */
#define IS_REORDER_CH(reorder_mode) (reorder_mode & BIT(1))

/* For <IS_REORDER_CH_8_24> mode, upper layer shall give u32-padded blocks, and word_size = 24. */
/* The useful data bytes take the 2/3 of block_size for <IS_REORDER_CH_8_24> mode. */
#define IS_REORDER_CH_8_24(reorder_mode) (reorder_mode & BIT(1)) && (reorder_mode & BIT(2))

#define IS_REORDER_NULL(reorder_mode) !(reorder_mode & BIT(2)) && !(reorder_mode & BIT(1))

/* Device run time data */
struct i2s_ameba_data {
	uint8_t tdmmode;
	uint8_t fifo_num;
	uint8_t reorder_mode;
	struct i2s_ameba_stream tx;
	struct i2s_ameba_stream rx;
	void *tx_out_msgs[1];
	void *rx_in_msgs[1];
	void *tx_in_msgs[CONFIG_I2S_AMEBA_TX_BLOCK_COUNT];
	void *rx_out_msgs[CONFIG_I2S_AMEBA_RX_BLOCK_COUNT];
	struct i2s_dma_stream dma_rx;
	struct i2s_dma_stream dma_tx;
#if defined(CONFIG_I2S_AMEBA_CHANNEL_EXT) && CONFIG_I2S_AMEBA_CHANNEL_EXT
	uint8_t dma_sw_irq;
	struct i2s_dma_stream dma_rx_ext;
	struct i2s_dma_stream dma_tx_ext;
#endif
};
#endif /* _ameba_I2S_H_ */
