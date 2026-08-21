/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AMP audio test (remote HiFi4 DSP side).
 *
 * Waits for the ARM core to command an audio run over the MU/mbox, then
 * configures the audio codec and the I2S RX/TX streams and performs a fixed
 * number of I2S transactions doing simultaneous playback (TX) and capture
 * (RX). After the bounded run it drains the streams and reports the number of
 * completed transactions back to the ARM core, or an error code if anything
 * fails along the way.
 *
 * The DSP also emits an initial "alive" beacon so the ARM side can confirm the
 * DSP started before it issues the audio command.
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/devicetree.h>

#include "testipc.h"

#define printk(...)

#define I2S_RX_NODE DT_ALIAS(i2s_rx)
#define I2S_TX_NODE DT_ALIAS(i2s_tx)

#define SAMPLE_FREQUENCY   48000
#define SAMPLE_BIT_WIDTH   16
#define BYTES_PER_SAMPLE   sizeof(int16_t)
#define NUMBER_OF_CHANNELS 2

#define SAMPLES_PER_BLOCK  ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS     4
#define TIMEOUT            1000

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 4)

K_MEM_SLAB_DEFINE_IN_SECT_STATIC(mem_slab, __nocache, BLOCK_SIZE, BLOCK_COUNT, 4);

static const struct device *const i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);
static const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);
static const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

static bool configure_streams(const struct device *i2s_dev_rx,
			      const struct device *i2s_dev_tx,
			      const struct i2s_config *config)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_configure(i2s_dev_rx, I2S_DIR_BOTH, config);
		if (ret == 0) {
			return true;
		}
		/* -ENOSYS means the RX and TX streams need separate config. */
		if (ret != -ENOSYS) {
			printk("[DSP] Failed to configure streams: %d\n", ret);
			return false;
		}
	}

	ret = i2s_configure(i2s_dev_rx, I2S_DIR_RX, config);
	if (ret < 0) {
		printk("[DSP] Failed to configure RX stream: %d\n", ret);
		return false;
	}

	ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, config);
	if (ret < 0) {
		printk("[DSP] Failed to configure TX stream: %d\n", ret);
		return false;
	}

	return true;
}

static bool prepare_transfer(const struct device *i2s_dev_tx)
{
	int ret;

	for (int i = 0; i < INITIAL_BLOCKS; ++i) {
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
		if (ret < 0) {
			printk("[DSP] Failed to allocate TX block %d: %d\n", i, ret);
			return false;
		}

		memset(mem_block, 0, BLOCK_SIZE);

		ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("[DSP] Failed to write block %d: %d\n", i, ret);
			return false;
		}
	}

	return true;
}

static bool trigger_command(const struct device *i2s_dev_rx,
			    const struct device *i2s_dev_tx,
			    enum i2s_trigger_cmd cmd)
{
	int ret;

	if (i2s_dev_rx == i2s_dev_tx) {
		ret = i2s_trigger(i2s_dev_rx, I2S_DIR_BOTH, cmd);
		if (ret == 0) {
			return true;
		}
		if (ret != -ENOSYS) {
			printk("[DSP] Failed to trigger command %d: %d\n", cmd, ret);
			return false;
		}
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, cmd);
	if (ret < 0) {
		printk("[DSP] Failed to trigger command %d on RX: %d\n", cmd, ret);
		return false;
	}

	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, cmd);
	if (ret < 0) {
		printk("[DSP] Failed to trigger command %d on TX: %d\n", cmd, ret);
		return false;
	}

	return true;
}


int init_audio(void)
{
	int ret;
	uint32_t msg;

	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;

	/* Check for the state of audio drivers */
	if (!device_is_ready(codec_dev)) {
		printk("[DSP] %s is not ready\n", codec_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(i2s_dev_rx)) {
		printk("[DSP] %s is not ready\n", i2s_dev_rx->name);
		return -ENODEV;
	}

	if (i2s_dev_rx != i2s_dev_tx && !device_is_ready(i2s_dev_tx)) {
		printk("[DSP] %s is not ready\n", i2s_dev_tx->name);
		return -ENODEV;
	}

	/* Proceed with audio playback */
	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels = NUMBER_OF_CHANNELS;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_CONTROLLER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	ret = audio_codec_configure(codec_dev, &audio_cfg);
	if (ret < 0) {
		printk("[DSP] Codec failed to configure: %d\n", ret);
		return ret;
	}

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	if (!configure_streams(i2s_dev_rx, i2s_dev_tx, &config)) {
		printk("[DSP] Failed to configure streams\n");
		return -EIO;
	}

	return 0;
}

int dispatch_audio_start(void)
{
	printk("[DSP] Dispatching audio...\n");
	int ret;

	if (!prepare_transfer(i2s_dev_tx)) {
		return -EIO;
	}

	if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_START)) {
		return -EIO;
	}

	printk("[DSP] Streams started.\n");

	uint32_t completed = 0;

	for (; completed < AMP_AUDIO_ITERATIONS; completed++) {
		void *mem_block;
		uint32_t block_size;
		int ret;

		/* Capture a block. */
		ret = i2s_read(i2s_dev_rx, &mem_block, &block_size);
		if (ret < 0) {
			printk("[DSP] Failed to read data: %d\n", ret);
			return ret;
		}

		/* Play it back. */
		ret = i2s_write(i2s_dev_tx, mem_block, block_size);
		if (ret < 0) {
			printk("[DSP] Failed to write data: %d\n", ret);
			return ret;
		}
	}

	/* Stop cleanly by draining the TX queue. */
	if (!trigger_command(i2s_dev_rx, i2s_dev_tx, I2S_TRIGGER_DRAIN)) {
		return -EIO;
	}

	printk("[DSP] Streams stopped after %u transactions.\n", completed);

	ret = testipc_send(testipc_msg_make(AMP_OP_AUDIO_DONE, completed));
	if (ret < 0) {
		printk("[DSP] Failed to send audio done (%d)\n", ret);
		return ret;
	}
	return (int)completed;
}

static int dispatch_echo(uint32_t msg)
{
	int32_t payload = testipc_msg_get_payload(msg);

	return testipc_send(testipc_msg_make(AMP_OP_ECHO_RESP, payload));
}

int main(void)
{
	int ret;
	uint32_t msg = 0;

	printk("[DSP] Hello World! %s\n", CONFIG_BOARD_TARGET);

	ret = testipc_init();
	if (ret < 0) {
		return ret;
	}

	ret = init_audio();
	if (ret < 0) {
		testipc_report_error(ret);
		return ret;
	}

	ret = testipc_send(testipc_msg_make(AMP_OP_ALIVE, 0));
	if (ret < 0) {
		testipc_report_error(ret);
		return ret;
	}

	k_msleep(200);

	while (true) {
		ret = testipc_recv(&msg);
		if (ret < 0) {
			testipc_report_error(ret);
			return ret;
		}

		uint8_t op = testipc_msg_get_op(msg);

		switch (op) {
		case AMP_OP_ECHO_REQ:
			printk("[DSP] Got command: echo\n");
			ret = dispatch_echo(msg);
			break;

		case AMP_OP_AUDIO_START:
			printk("[DSP] Got command: audio\n");
			ret = dispatch_audio_start();
			printk("[DSP] Finished command: audio\n");
			break;

		default:
			printk("[DSP] Whaaat? %x %x\n", op, msg);
			ret = -EINVAL;
			break;
		}

		if (ret < 0) {
			printk("[DSP] Subfunction failed (%d)\n", ret);
			testipc_report_error(ret);
			return ret;
		}
	}

	return 0;
}
