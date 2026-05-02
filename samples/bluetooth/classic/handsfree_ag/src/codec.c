/* codec.c - CODEC implementation for Bluetooth Hands-Free Profile */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/audio/codec.h>
#include <zephyr/toolchain.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "codec.h"

#if DT_HAS_ALIAS(i2s_codec_tx) && DT_HAS_ALIAS(i2s_codec_rx)
#define I2S_CODEC_TX DT_ALIAS(i2s_codec_tx)
#define I2S_CODEC_RX DT_ALIAS(i2s_codec_rx)
#define I2C_CODEC    DT_NODELABEL(audio_codec)

#define MAX_SAMPLE_FREQ      16000
#define MAX_SAMPLE_BIT_WIDTH 16
#define MAX_CHANNELS         2

#define SAMPLES_PER_BLOCK ((MAX_SAMPLE_FREQ * CONFIG_AUDIO_TRANSFER_INTERVAL / 1000) * MAX_CHANNELS)

#define BLOCK_SIZE     (MAX_SAMPLE_BIT_WIDTH * SAMPLES_PER_BLOCK / 8)
#define RX_BLOCK_COUNT (CONFIG_CODEC_BUFFERS)
#define TX_BLOCK_COUNT (CONFIG_CODEC_BUFFERS)
#define TIMEOUT        (2 * 1000 / CONFIG_AUDIO_TRANSFER_INTERVAL)

static const struct device *i2s_codec_tx = DEVICE_DT_GET(I2S_CODEC_TX);
static const struct device *i2s_codec_rx = DEVICE_DT_GET(I2S_CODEC_RX);
static const struct device *i2c_codec = DEVICE_DT_GET(I2C_CODEC);

K_MEM_SLAB_DEFINE_IN_SECT_STATIC(tx_mem_slab, __nocache, BLOCK_SIZE, TX_BLOCK_COUNT, 4);
K_MEM_SLAB_DEFINE_IN_SECT_STATIC(rx_mem_slab, __nocache, BLOCK_SIZE, RX_BLOCK_COUNT, 4);

static int configure_codec_streams(const struct device *i2s_codec_rx,
				   const struct device *i2s_codec_tx, struct i2s_config *config)
{
	int err;

	config->mem_slab = &rx_mem_slab;
	err = i2s_configure(i2s_codec_rx, I2S_DIR_RX, config);
	if (err < 0) {
		printk("Failed to configure CODEC RX stream: %d\n", err);
		return err;
	}

	config->mem_slab = &tx_mem_slab;
	err = i2s_configure(i2s_codec_tx, I2S_DIR_TX, config);
	if (err < 0) {
		printk("Failed to configure CODEC TX stream: %d\n", err);
		return err;
	}

	return 0;
}

static struct k_sem codec_rx_thread_notify;
static codec_rx_cb_t codec_rx_cb;

#define CODEC_RX_FLAG_STOPPED  0
#define CODEC_RX_FLAG_STARTED  1
#define CODEC_RX_FLAG_STOPPING 2

static atomic_t codec_rx_flag[1];

static void codec_rx_task(void *p1, void *p2, void *p3)
{
	int err;
	void *mem_block;
	uint32_t block_size;

	while (true) {
		err = k_sem_take(&codec_rx_thread_notify, K_FOREVER);
		if (err < 0) {
			continue;
		}

		if (atomic_test_and_clear_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPING)) {
			err = i2s_trigger(i2s_codec_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
			if (err < 0) {
				printk("Failed to stop CODEC RX %d\n", err);
			}
			atomic_clear_bit(codec_rx_flag, CODEC_RX_FLAG_STARTED);
			atomic_set_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPED);
		}

		if (!atomic_test_bit(codec_rx_flag, CODEC_RX_FLAG_STARTED)) {
			continue;
		} else {
			k_sem_give(&codec_rx_thread_notify);
		}

		err = i2s_read(i2s_codec_rx, &mem_block, &block_size);
		if (err < 0) {
			continue;
		}

		if (codec_rx_cb != NULL) {
			codec_rx_cb(mem_block, block_size);
		}
		k_mem_slab_free(&rx_mem_slab, (void *)mem_block);
	}
}

static K_KERNEL_STACK_MEMBER(codec_rx_thread_stack, CONFIG_CODEC_RX_THREAD_STACK_SIZE);

int codec_init(uint8_t air_mode)
{
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;
	int err;
	uint8_t word_size;
	uint8_t channels;
	uint32_t sample_rate;

	static bool is_initiated;
	static struct k_thread codec_rx_thread;
	static k_tid_t codec_rx_thread_id;

	if (is_initiated) {
		return 0;
	}

	if (!device_is_ready(i2s_codec_rx)) {
		printk("%s is not ready\n", i2s_codec_rx->name);
		return -EINVAL;
	}

	if (i2s_codec_rx != i2s_codec_tx && !device_is_ready(i2s_codec_tx)) {
		printk("%s is not ready\n", i2s_codec_tx->name);
		return -EINVAL;
	}

	if (!device_is_ready(i2c_codec)) {
		printk("%s is not ready\n", i2c_codec->name);
		return -EINVAL;
	}

	switch (air_mode) {
	case BT_HCI_CODING_FORMAT_CVSD:
		word_size = 16;
		channels = 2;
		sample_rate = 8000;
		break;
	case BT_HCI_CODING_FORMAT_TRANSPARENT:
		word_size = 16;
		channels = 2;
		sample_rate = 16000;
		break;
	case BT_HCI_CODING_FORMAT_ULAW_LOG:
	case BT_HCI_CODING_FORMAT_ALAW_LOG:
	case BT_HCI_CODING_FORMAT_LINEAR_PCM:
	case BT_HCI_CODING_FORMAT_MSBC:
	case BT_HCI_CODING_FORMAT_LC3:
	case BT_HCI_CODING_FORMAT_G729A:
	case BT_HCI_CODING_FORMAT_VS:
	default:
		printk("Unsupported air mode: %d\n", air_mode);
		return -ENOTSUP;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = word_size;
	audio_cfg.dai_cfg.i2s.channels = channels;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = &rx_mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	err = audio_codec_configure(i2c_codec, &audio_cfg);
	if (err != 0) {
		printk("Failed to configure audio codec: %d\n", err);
		return err;
	}

	config.word_size = word_size;
	config.channels = channels;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	config.frame_clk_freq = sample_rate;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;

	err = configure_codec_streams(i2s_codec_rx, i2s_codec_tx, &config);
	if (err < 0) {
		return err;
	}

	if (codec_rx_thread_id == NULL) {
		k_sem_init(&codec_rx_thread_notify, 0, 1);
		codec_rx_thread_id = k_thread_create(&codec_rx_thread, codec_rx_thread_stack,
						     K_KERNEL_STACK_SIZEOF(codec_rx_thread_stack),
						     codec_rx_task, NULL, NULL, NULL,
						     K_PRIO_COOP(CONFIG_CODEC_RX_THREAD_PRIO), 0,
						     K_NO_WAIT);
		k_thread_name_set(codec_rx_thread_id, "HFP CODEC RX");
	}

	atomic_set_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPED);
	is_initiated = true;

	return 0;
}

int codec_rx_start(codec_rx_cb_t cb)
{
	int err;

	if (!atomic_test_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPED)) {
		printk("CODEC RX is not idle\n");
		return -EBUSY;
	}

	codec_rx_cb = cb;

	err = i2s_trigger(i2s_codec_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	if (err < 0) {
		printk("Failed to trigger start on RX: %d\n", err);
		return err;
	}

	atomic_clear_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPED);
	atomic_set_bit(codec_rx_flag, CODEC_RX_FLAG_STARTED);
	k_sem_give(&codec_rx_thread_notify);

	return 0;
}

int codec_rx_stop(void)
{
	if (!atomic_test_bit(codec_rx_flag, CODEC_RX_FLAG_STARTED)) {
		return 0;
	}

	atomic_set_bit(codec_rx_flag, CODEC_RX_FLAG_STOPPING);
	k_sem_give(&codec_rx_thread_notify);

	return 0;
}

int codec_tx(const uint8_t *data, uint32_t len)
{
	int err;
	void *mem_block;

	if (len * 2 != BLOCK_SIZE) {
		printk("Invalid data len %u != %u\n", len, BLOCK_SIZE / 2);
		return -EINVAL;
	}

	err = k_mem_slab_alloc(&tx_mem_slab, &mem_block, K_NO_WAIT);
	if (err < 0) {
		printk("Failed to allocate TX block: %d\n", err);
		return err;
	}

	for (uint32_t i = 0; i < len; i += 2) {
		uint32_t *dst;
		uint16_t *src;

		dst = (uint32_t *)&((uint8_t *)mem_block)[i * 2];
		src = (uint16_t *)&data[i];

		*dst = (*src) | ((*src) << 16);
	}

	err = i2s_write(i2s_codec_tx, mem_block, BLOCK_SIZE);
	if (err < 0) {
		k_mem_slab_free(&tx_mem_slab, mem_block);
		printk("Failed to write block: %d\n", err);
		return err;
	}

	/* Try to trigger TX start each time, because TX may have stopped if there is no more
	 * data in pending.
	 * Ignore the error.
	 */
	(void)i2s_trigger(i2s_codec_tx, I2S_DIR_TX, I2S_TRIGGER_START);

	return 0;
}

#else

int codec_init(uint8_t air_mode)
{
	printk("Codec is unsupported\n");
	return 0;
}

int codec_tx(const uint8_t *data, uint32_t len)
{
	return 0;
}

int codec_rx_start(codec_rx_cb_t cb)
{
	return 0;
}

int codec_rx_stop(void)
{
	return 0;
}

#endif /* DT_HAS_ALIAS(i2s_codec_tx) && DT_HAS_ALIAS(i2s_codec_rx) */
