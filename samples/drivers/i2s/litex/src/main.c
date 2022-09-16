/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <stdlib.h>
#include <string.h>
#define AUDIO_SAMPLE_FREQ (44100)
#define AUDIO_SAMPLES_PER_CH_PER_FRAME (128)
#define AUDIO_NUM_CHANNELS (2)
#define AUDIO_SAMPLES_PER_FRAME                                                \
	(AUDIO_SAMPLES_PER_CH_PER_FRAME * AUDIO_NUM_CHANNELS)
#define AUDIO_SAMPLE_BYTES (3)
#define AUDIO_SAMPLE_BIT_WIDTH (24)

#define AUDIO_FRAME_BUF_BYTES (AUDIO_SAMPLES_PER_FRAME * AUDIO_SAMPLE_BYTES)

#define I2S_PLAY_BUF_COUNT (500)

static const struct device *host_i2s_rx_dev;
static const struct device *host_i2s_tx_dev;
static struct k_mem_slab i2s_rx_mem_slab;
static struct k_mem_slab i2s_tx_mem_slab;
static char rx_buffers[AUDIO_FRAME_BUF_BYTES * I2S_PLAY_BUF_COUNT];
static char tx_buffer[AUDIO_FRAME_BUF_BYTES * I2S_PLAY_BUF_COUNT];
static struct i2s_config i2s_rx_cfg;
static struct i2s_config i2s_tx_cfg;
static int ret;

static void init(void)
{
	/*configure rx device*/
	host_i2s_rx_dev = device_get_binding("i2s_rx");
	if (!host_i2s_rx_dev) {
		printk("unable to find i2s_rx device\n");
		exit(-1);
	}
	k_mem_slab_init(&i2s_rx_mem_slab, rx_buffers, AUDIO_FRAME_BUF_BYTES,
			I2S_PLAY_BUF_COUNT);

	/* configure i2s for audio playback */
	i2s_rx_cfg.word_size = AUDIO_SAMPLE_BIT_WIDTH;
	i2s_rx_cfg.channels = AUDIO_NUM_CHANNELS;
	i2s_rx_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_rx_cfg.options = I2S_OPT_FRAME_CLK_SLAVE;
	i2s_rx_cfg.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	i2s_rx_cfg.block_size = AUDIO_FRAME_BUF_BYTES;
	i2s_rx_cfg.mem_slab = &i2s_rx_mem_slab;
	i2s_rx_cfg.timeout = -1;
	ret = i2s_configure(host_i2s_rx_dev, I2S_DIR_RX, &i2s_rx_cfg);

	if (ret != 0) {
		printk("i2s_configure failed with %d error", ret);
		exit(-1);
	}

	/*configure tx device*/
	host_i2s_tx_dev = device_get_binding("i2s_tx");
	if (!host_i2s_tx_dev) {
		printk("unable to find i2s_tx device\n");
		exit(-1);
	}
	k_mem_slab_init(&i2s_tx_mem_slab, tx_buffer, AUDIO_FRAME_BUF_BYTES,
			I2S_PLAY_BUF_COUNT);

	/* configure i2s for audio playback */
	i2s_tx_cfg.word_size = AUDIO_SAMPLE_BIT_WIDTH;
	i2s_tx_cfg.channels = AUDIO_NUM_CHANNELS;
	i2s_tx_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_tx_cfg.options = I2S_OPT_FRAME_CLK_SLAVE;
	i2s_tx_cfg.frame_clk_freq = AUDIO_SAMPLE_FREQ;
	i2s_tx_cfg.block_size = AUDIO_FRAME_BUF_BYTES;
	i2s_tx_cfg.mem_slab = &i2s_tx_mem_slab;
	i2s_tx_cfg.timeout = -1;
	ret = i2s_configure(host_i2s_tx_dev, I2S_DIR_TX, &i2s_tx_cfg);
	if (ret != 0) {
		printk("i2s_configure failed with %d error\n", ret);
		exit(-1);
	}
}

void main(void)
{
	init();

	/* start i2s rx driver */
	ret = i2s_trigger(host_i2s_rx_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("i2s_trigger failed with %d error\n", ret);
		exit(-1);
	}

	/* start i2s tx driver */
	ret = i2s_trigger(host_i2s_tx_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("i2s_trigger failed with %d error\n", ret);
		exit(-1);
	}

	/* receive data */
	void *rx_mem_block, *tx_mem_block;
	size_t size;

	while (true) {
		k_mem_slab_alloc(&i2s_tx_mem_slab, &tx_mem_block, K_NO_WAIT);
		i2s_read(host_i2s_rx_dev, &rx_mem_block, &size);
		memcpy(tx_mem_block, rx_mem_block, size);
		i2s_write(host_i2s_tx_dev, tx_mem_block, size);
		k_mem_slab_free(&i2s_rx_mem_slab, &rx_mem_block);
	}
}
