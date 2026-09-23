/**
 * @file
 * @brief BAP Broadcast Sink I2S/SAI PCM transport (hardware layer).
 *
 * This module implements the hardware-specific SAI/I2S data path used by
 * hw_codec.c on boards whose codec is a control-only device (for example the
 * WM8962 on NXP i.MX RT boards). The codec driver on such boards only
 * implements the control API, so the PCM samples are transported to the codec
 * over the SoC SAI/I2S peripheral using the Zephyr I2S driver API
 * (i2s_configure / i2s_write / i2s_trigger).
 *
 * Responsibility split:
 *  - hw_codec.c owns the codec control (audio_codec_configure /
 *    start_output / stop_output / volume).
 *  - this module owns the SAI/I2S peripheral configuration and the DMA memory
 *    slab (a hardware/DMA resource).
 *  - i2s_play.c owns the software playback policy: the elastic PCM ring buffer
 *    and the playback thread that keeps the SAI DMA fed. This module hands the
 *    configured device, the DMA slab, the per-block size and the frame
 *    duration to i2s_play via i2s_play_setup().
 *
 * This file is only compiled when the board provides an i2s-codec-tx
 * devicetree alias (see CMakeLists.txt).
 *
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/toolchain.h>

#include "hw_codec_i2s.h"
#include "i2s_play.h"

LOG_MODULE_REGISTER(codec_i2s, CONFIG_LOG_DEFAULT_LEVEL);

#define I2S_TX_NODE DT_ALIAS(i2s_codec_tx)

#define I2S_TX_TIMEOUT_MS (2000U)

/* Fallback default in case the Kconfig option is not selected. */
#ifndef CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT
#define CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT 12
#endif

/* Depth of the elastic PCM ring between producer and playback thread. */
#define PCM_RING_DEPTH CONFIG_BAP_BROADCAST_SINK_PCM_BUFFER_PLAY_COUNT

/*
 * One I2S transfer block holds the interleaved PCM samples produced by the LC3
 * decoder for a single LC3 frame. The block size in bytes is computed at run
 * time from the actual stream parameters:
 *
 *   samples_per_frame = samplerate * frame_dur_us / 1000000
 *   block_size        = samples_per_frame * (word_size / 8) * channels
 *
 * The size is driven by the LC3 frame duration (per the BT LE Audio spec this
 * is either 7500 us or 10000 us), NOT by the ISO interval. ISO interval jitter
 * is absorbed by the PCM ring and the multi-block DMA slab.
 */
#define CODEC_PCM_MAX_CHANNELS 2U
#define CODEC_PCM_MAX_SAMPLERATE 48000U
#define CODEC_PCM_MAX_FRAME_DUR_US 10000U
#define CODEC_PCM_BYTES_PER_SAMPLE (CODEC_PCM_WIDTH_BITS / 8U)

/*
 * Worst-case bytes for a single block, used to statically size the DMA slab.
 * Worst case is 48 kHz, 10 ms, stereo:
 * 48000 * 10000 / 1000000 = 480 samples * 2 bytes * 2 channels = 1920 bytes.
 */
#define I2S_TX_MAX_BLOCK_SIZE\
	(((CODEC_PCM_MAX_SAMPLERATE * CODEC_PCM_MAX_FRAME_DUR_US) / 1000000U) *\
	 CODEC_PCM_BYTES_PER_SAMPLE * CODEC_PCM_MAX_CHANNELS)

/*
 * Total slab blocks: the ring can hold up to PCM_RING_DEPTH filled blocks that
 * have not yet been handed to the SAI, plus up to CONFIG_I2S_TX_BLOCK_COUNT
 * blocks in flight inside the SAI DMA queue, plus a couple of scratch/silence
 * blocks owned by the playback thread. Size the slab for the sum so the
 * producer and consumer never contend for the same pool.
 */
#define I2S_TX_SLAB_BLOCKS (PCM_RING_DEPTH + CONFIG_I2S_TX_BLOCK_COUNT + 2U)

#if defined(CONFIG_NOCACHE_MEMORY)
#define __NOCACHE __attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE __attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else
#define __NOCACHE
#endif

/* DMA memory slab shared by the PCM and silence blocks (hardware resource). */
static __NOCACHE __aligned(4) uint8_t tx_slab_buffer[I2S_TX_SLAB_BLOCKS * I2S_TX_MAX_BLOCK_SIZE];
static struct k_mem_slab tx_mem_slab;

static const struct device *i2s_tx_dev;

/* Set once the DMA slab has been initialised, to detect a reconfiguration. */
static bool tx_slab_initialised;

int hw_codec_i2s_init(void)
{
	i2s_tx_dev = DEVICE_DT_GET(I2S_TX_NODE);
	if (!device_is_ready(i2s_tx_dev)) {
		LOG_ERR("I2S tx device not ready");
		return -EIO;
	}

	return 0;
}

int hw_codec_i2s_cfg(uint32_t samplerate, uint8_t channels, uint32_t frame_dur_us)
{
	int ret;
	uint32_t samples_per_frame;
	uint32_t block_size;

	if ((channels == 0U || channels > CODEC_PCM_MAX_CHANNELS) ||
	    (samplerate == 0U || samplerate > CODEC_PCM_MAX_SAMPLERATE) ||
	    (frame_dur_us == 0U)) {
		LOG_ERR("Unsupported Parameter");
		return -EINVAL;
	}

	/* Clamp to the worst case the DMA slab is sized for (10 ms). */
	if (frame_dur_us > CODEC_PCM_MAX_FRAME_DUR_US) {
		LOG_WRN("Frame duration %u us exceeds max, clamping to %u us", frame_dur_us,
			CODEC_PCM_MAX_FRAME_DUR_US);
		frame_dur_us = CODEC_PCM_MAX_FRAME_DUR_US;
	}

	/*
	 * Number of PCM samples per channel in one LC3 frame. Microsecond math
	 * keeps the 7.5 ms case exact (48 kHz * 7500 / 1000000 = 360 samples).
	 */
	samples_per_frame = (uint32_t)(((uint64_t)samplerate * frame_dur_us) / 1000000U);

	/* samples_per_frame * bytes_per_sample * channels */
	block_size = samples_per_frame * CODEC_PCM_BYTES_PER_SAMPLE * channels;

	if (block_size == 0U || block_size > I2S_TX_MAX_BLOCK_SIZE) {
		LOG_ERR("Computed block size %u out of range (max %u)", block_size,
			I2S_TX_MAX_BLOCK_SIZE);
		return -EINVAL;
	}

	/*
	 * Reconfiguring the slab would reset its free list while the SAI DMA
	 * and the playback thread may still hold blocks carved out of the old
	 * partition, leaking those blocks and letting two owners write the same
	 * memory. Require a close (which stops playback and drains the ring)
	 * before the stream can be configured again.
	 */
	if (tx_slab_initialised && k_mem_slab_num_used_get(&tx_mem_slab) != 0U) {
		LOG_ERR("I2S tx reconfigure with %u blocks still in use",
			k_mem_slab_num_used_get(&tx_mem_slab));
		return -EBUSY;
	}

	/*
	 * Initialise the DMA slab before i2s_configure(): the configuration
	 * stores the mem_slab pointer in the driver and the driver may allocate
	 * from it as soon as the stream is configured, so the slab must already
	 * describe a valid partition at that point.
	 */
	k_mem_slab_init(&tx_mem_slab, tx_slab_buffer, block_size, I2S_TX_SLAB_BLOCKS);
	tx_slab_initialised = true;

	struct i2s_config i2s_cfg = {
		.word_size = CODEC_PCM_WIDTH_BITS,
		.channels = channels,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		/*
		 * Clock roles are deliberately the mirror image of the codec
		 * configuration in hw_codec.c: the SAI and the codec are peers
		 * on the same I2S bus, so exactly one of them must drive BCK
		 * and WS. CONFIG_USE_CODEC_CLOCK selects the codec as the clock
		 * source, which makes the SAI the target here.
		 */
#ifdef CONFIG_USE_CODEC_CLOCK
		.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
#else
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
#endif
		.frame_clk_freq = samplerate,
		.block_size = block_size,
		.mem_slab = &tx_mem_slab,
		.timeout = I2S_TX_TIMEOUT_MS,
	};

	ret = i2s_configure(i2s_tx_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure I2S tx (err %d)", ret);
		return ret;
	}

	/* Hand the hardware handles to the pure-software playback layer. */
	i2s_play_setup(i2s_tx_dev, &tx_mem_slab, block_size, frame_dur_us);

	LOG_INF("I2S tx: rate=%u channels=%u block_size=%u", samplerate, channels, block_size);

	return 0;
}

uint32_t hw_codec_i2s_write(const uint8_t *data, uint32_t len)
{
	return i2s_play_write(data, len);
}

int hw_codec_i2s_close(void)
{
	/*
	 * i2s_play_stop() joins the playback thread, which drops the transmit
	 * stream and returns every block it owned to the slab, so the stream is
	 * already stopped and the slab fully reclaimed when this returns.
	 */
	i2s_play_stop();

	return 0;
}
