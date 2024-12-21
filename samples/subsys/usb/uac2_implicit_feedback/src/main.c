/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

#include <sample_usbd.h>
#include "feedback.h"

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uac2_sample, LOG_LEVEL_INF);

#define HEADPHONES_OUT_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(out_terminal))
#define MICROPHONE_IN_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))

#define SAMPLES_PER_SOF     48
#define SAMPLE_FREQUENCY    (SAMPLES_PER_SOF * 1000)
#define SAMPLE_BIT_WIDTH    16
#define NUMBER_OF_CHANNELS  2
#define BYTES_PER_SAMPLE    DIV_ROUND_UP(SAMPLE_BIT_WIDTH, 8)
#define BYTES_PER_SLOT      (BYTES_PER_SAMPLE * NUMBER_OF_CHANNELS)
#define MIN_BLOCK_SIZE      ((SAMPLES_PER_SOF - 1) * BYTES_PER_SLOT)
#define BLOCK_SIZE          (SAMPLES_PER_SOF * BYTES_PER_SLOT)
#define MAX_BLOCK_SIZE      ((SAMPLES_PER_SOF + 1) * BYTES_PER_SLOT)

/* Absolute minimum is 5 TX buffers (1 actively consumed by I2S, 2nd queued as
 * next buffer, 3rd acquired by USB stack to receive data to, and 2 to handle
 * SOF/I2S offset errors), but add 2 additional buffers to prevent out of memory
 * errors when USB host decides to perform rapid terminal enable/disable cycles.
 */
#define I2S_BLOCKS          7
K_MEM_SLAB_DEFINE_STATIC(i2s_tx_slab, MAX_BLOCK_SIZE, I2S_BLOCKS, 4);
K_MEM_SLAB_DEFINE_STATIC(i2s_rx_slab, MAX_BLOCK_SIZE, I2S_BLOCKS, 4);

struct usb_i2s_ctx {
	const struct device *i2s_dev;
	bool headphones_enabled;
	bool microphone_enabled;
	bool i2s_started;
	bool rx_started;
	bool usb_data_received;
	/* Counter used to determine when to start I2S and then when to start
	 * sending RX packets to host. Overflows are not a problem because this
	 * variable is not necessary after both I2S and RX is started.
	 */
	uint8_t i2s_counter;
	struct feedback_ctx *fb;

	/* Leftover samples from I2S receive buffer, already compacted to mono,
	 * that were not sent to host. The buffer, if not NULL, is allocated
	 * from I2S RX slab.
	 */
	uint8_t *pending_mic_buf;
	uint8_t pending_mic_samples;

	/* Rolling bit buffers for tracking nominal + 1 and nominal - 1 samples
	 * sent. Bits are mutually exclusive, i.e.:
	 *   plus_ones | minus_ones = plus_ones ^ minus_ones
	 *
	 * Used to avoid overcompensation in feedback regulator. LSBs indicate
	 * latest write size.
	 */
	uint8_t plus_ones;
	uint8_t minus_ones;
};

static void uac2_terminal_update_cb(const struct device *dev, uint8_t terminal,
				    bool enabled, bool microframes,
				    void *user_data)
{
	struct usb_i2s_ctx *ctx = user_data;

	/* This sample is for Full-Speed only devices. */
	__ASSERT_NO_MSG(microframes == false);

	if (terminal == HEADPHONES_OUT_TERMINAL_ID) {
		ctx->headphones_enabled = enabled;
	} else if (terminal == MICROPHONE_IN_TERMINAL_ID) {
		ctx->microphone_enabled = enabled;
	}

	if (ctx->i2s_started && !ctx->headphones_enabled &&
	    !ctx->microphone_enabled) {
		i2s_trigger(ctx->i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
		ctx->i2s_started = false;
		ctx->i2s_counter = 0;
		ctx->plus_ones = ctx->minus_ones = 0;
		if (ctx->pending_mic_samples) {
			k_mem_slab_free(&i2s_rx_slab, ctx->pending_mic_buf);
			ctx->pending_mic_buf = NULL;
			ctx->pending_mic_samples = 0;
		}
	}
}

static void *uac2_get_recv_buf(const struct device *dev, uint8_t terminal,
			       uint16_t size, void *user_data)
{
	ARG_UNUSED(dev);
	struct usb_i2s_ctx *ctx = user_data;
	void *buf = NULL;
	int ret;

	if (terminal == HEADPHONES_OUT_TERMINAL_ID) {
		__ASSERT_NO_MSG(size <= MAX_BLOCK_SIZE);

		if (!ctx->headphones_enabled) {
			LOG_ERR("Buffer request on disabled terminal");
			return NULL;
		}

		ret = k_mem_slab_alloc(&i2s_tx_slab, &buf, K_NO_WAIT);
		if (ret != 0) {
			buf = NULL;
		}
	}

	return buf;
}

static void uac2_data_recv_cb(const struct device *dev, uint8_t terminal,
			      void *buf, uint16_t size, void *user_data)
{
	struct usb_i2s_ctx *ctx = user_data;
	int ret;

	ctx->usb_data_received = true;

	if (!ctx->headphones_enabled && !ctx->microphone_enabled) {
		k_mem_slab_free(&i2s_tx_slab, buf);
		return;
	}

	if (!size) {
		/* This code path is expected when host only records microphone
		 * data and is not streaming any audio to the headphones. Simply
		 * transmit as many zero-filled samples were last sent to allow
		 * the feedback regulator to work.
		 *
		 * When host is streaming audio, this can be a transient error.
		 * While the "feedback regulator delay" is likely to differ,
		 * it is still probably best to just zero-fill last sent number
		 * of samples. If we overcompensate as a result, the situation
		 * will stabilize after a while anyway.
		 *
		 * In either case, we have to keep I2S going and the only way
		 * we can control the SOF to I2S offset is by varying the number
		 * of samples sent.
		 */
		if (ctx->plus_ones & 1) {
			size = (SAMPLES_PER_SOF + 1) * BYTES_PER_SLOT;
		} else if (ctx->minus_ones & 1) {
			size = (SAMPLES_PER_SOF - 1) * BYTES_PER_SLOT;
		} else {
			size = SAMPLES_PER_SOF * BYTES_PER_SLOT;
		}
		memset(buf, 0, size);
		sys_cache_data_flush_range(buf, size);
	}

	LOG_DBG("Received %d data to input terminal %d", size, terminal);

	ret = i2s_write(ctx->i2s_dev, buf, size);
	if (ret < 0) {
		ctx->i2s_started = false;
		ctx->i2s_counter = 0;
		ctx->plus_ones = ctx->minus_ones = 0;
		if (ctx->pending_mic_samples) {
			k_mem_slab_free(&i2s_rx_slab, ctx->pending_mic_buf);
			ctx->pending_mic_buf = NULL;
			ctx->pending_mic_samples = 0;
		}

		/* Most likely underrun occurred, prepare I2S restart */
		i2s_trigger(ctx->i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_PREPARE);

		ret = i2s_write(ctx->i2s_dev, buf, size);
		if (ret < 0) {
			/* Drop data block, will try again on next frame */
			k_mem_slab_free(&i2s_tx_slab, buf);
		}
	}

	if (ret == 0) {
		ctx->i2s_counter++;
	}
}

static void uac2_buf_release_cb(const struct device *dev, uint8_t terminal,
				void *buf, void *user_data)
{
	if (terminal == MICROPHONE_IN_TERMINAL_ID) {
		k_mem_slab_free(&i2s_rx_slab, buf);
	}
}

/* Determine next number of samples to send, called at most once every SOF */
static int next_mic_num_samples(struct usb_i2s_ctx *ctx)
{
	int offset = feedback_samples_offset(ctx->fb);

	/* The rolling buffers essentially handle controller dead time, i.e.
	 * the buffers are used to prevent overcompensating on feedback offset.
	 * Remove the oldest entry by shifting the values by one bit.
	 */
	ctx->plus_ones <<= 1;
	ctx->minus_ones <<= 1;

	if ((offset < 0) && (POPCOUNT(ctx->plus_ones) < -offset)) {
		/* I2S buffer starts at least 1 sample before SOF, send nominal
		 * + 1 samples to host in order to shift offset towards 0.
		 */
		ctx->plus_ones |= 1;
		return SAMPLES_PER_SOF + 1;
	}

	if ((offset > 0) && (POPCOUNT(ctx->minus_ones) < offset)) {
		/* I2S buffer starts at least 1 sample after SOF, send nominal
		 * - 1 samples to host in order to shift offset towards 0
		 */
		ctx->minus_ones |= 1;
		return SAMPLES_PER_SOF - 1;
	}

	/* I2S is either spot on, or the offset is expected to correct soon */
	return SAMPLES_PER_SOF;
}

static void process_mic_data(const struct device *dev, struct usb_i2s_ctx *ctx)
{
	size_t num_bytes;
	uint8_t *dst, *src;
	uint8_t *mic_buf;
	void *rx_block;
	int ret;
	int samples_to_send, mic_samples, rx_samples, leftover_samples;

	samples_to_send = next_mic_num_samples(ctx);

	if (ctx->pending_mic_samples >= samples_to_send) {
		/* No need to fetch new I2S samples, this happens shortly after
		 * we have "borrowed" samples from next buffer. This is expected
		 * and means that the streams have synchronized.
		 */
		rx_block = NULL;
		rx_samples = 0;
	} else {
		ret = i2s_read(ctx->i2s_dev, &rx_block, &num_bytes);
		if (ret) {
			/* No data available, I2S will restart soon */
			return;
		}
		sys_cache_data_invd_range(rx_block, num_bytes);

		/* I2S operates on 2 channels (stereo) */
		rx_samples = num_bytes / (BYTES_PER_SAMPLE * 2);
	}

	/* Prepare microphone data to send, use pending samples if any */
	src = rx_block;
	if (ctx->pending_mic_buf) {
		mic_buf = ctx->pending_mic_buf;
		mic_samples = ctx->pending_mic_samples;
		dst = &ctx->pending_mic_buf[mic_samples * BYTES_PER_SAMPLE];
	} else if (rx_samples >= 1) {
		/* First sample is already in place */
		mic_buf = rx_block;
		dst = &mic_buf[BYTES_PER_SAMPLE];
		src += 2 * BYTES_PER_SAMPLE;
		mic_samples = 1;
		rx_samples--;
	} else {
		/* Something went horribly wrong, free the buffer and leave */
		k_mem_slab_free(&i2s_rx_slab, rx_block);
		return;
	}

	/* Copy as many samples as possible, stop if mic buffer is ready */
	while ((mic_samples < samples_to_send) && (rx_samples > 0)) {
		memcpy(dst, src, BYTES_PER_SAMPLE);

		dst += BYTES_PER_SAMPLE;
		src += 2 * BYTES_PER_SAMPLE;

		mic_samples++;
		rx_samples--;
	}

	/* Is mic buffer ready to go? */
	if (mic_samples < samples_to_send) {
		/* No, we have to borrow sample from next buffer. This can only
		 * happen if we fully drained current receive buffer.
		 */
		__ASSERT_NO_MSG(rx_samples == 0);

		if (rx_block != mic_buf) {
			/* RX buffer no longer needed, samples are in mic_buf */
			k_mem_slab_free(&i2s_rx_slab, rx_block);
		}

		ret = i2s_read(ctx->i2s_dev, &rx_block, &num_bytes);
		if (ret) {
			/* No data, I2S will likely restart due to error soon */
			ctx->pending_mic_buf = mic_buf;
			ctx->pending_mic_samples = mic_samples;
			return;
		}
		sys_cache_data_invd_range(rx_block, num_bytes);

		src = rx_block;
		rx_samples = num_bytes / (BYTES_PER_SAMPLE * 2);
	}

	/* Copy remaining sample, under normal conditions (i.e. connected to
	 * non-malicious host) this is guaranteed to fully fill mic_buf.
	 */
	while ((mic_samples < samples_to_send) && (rx_samples > 0)) {
		memcpy(dst, src, BYTES_PER_SAMPLE);

		dst += BYTES_PER_SAMPLE;
		src += 2 * BYTES_PER_SAMPLE;

		mic_samples++;
		rx_samples--;
	}

	/* Are we still short on samples? */
	if (mic_samples < samples_to_send) {
		/* The only possibility for this code to execute is that we were
		 * short on samples and the next block (pointed to by rx_block)
		 * did not contain enough samples to fill the gap.
		 */
		__ASSERT_NO_MSG(rx_block != mic_buf);

		/* Bailing out at this point likely leads to faster recovery.
		 * Note that this should never happen during normal operation.
		 */
		ctx->pending_mic_buf = mic_buf;
		ctx->pending_mic_samples = mic_samples;

		/* RX buffer is no longer needed */
		k_mem_slab_free(&i2s_rx_slab, rx_block);
		return;
	}

	/* Handle any potential leftover, start by sanitizing length */
	leftover_samples = mic_samples - samples_to_send + rx_samples;
	if (leftover_samples > (MAX_BLOCK_SIZE / BYTES_PER_SAMPLE)) {
		size_t dropped_samples =
			leftover_samples - (MAX_BLOCK_SIZE / BYTES_PER_SAMPLE);

		LOG_WRN("Too many leftover samples, dropping %d samples",
			dropped_samples);
		if (rx_samples >= dropped_samples) {
			rx_samples -= dropped_samples;
		} else {
			mic_samples -= (dropped_samples - rx_samples);
			rx_samples = 0;
		}

		leftover_samples = (MAX_BLOCK_SIZE / BYTES_PER_SAMPLE);
	}

	if (leftover_samples == 0) {
		/* No leftover samples */
		if ((rx_block != NULL) && (rx_block != mic_buf)) {
			/* All samples were copied, free source buffer */
			k_mem_slab_free(&i2s_rx_slab, rx_block);
		}
		rx_block = NULL;
	} else if ((mic_samples > samples_to_send) ||
		   ((rx_samples > 0) && (rx_block == mic_buf))) {
		/* Leftover samples have to be copied to new buffer */
		ret = k_mem_slab_alloc(&i2s_rx_slab, &rx_block, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("Out of memory dropping %d samples",
				leftover_samples);
			mic_samples = samples_to_send;
			rx_samples = 0;
			rx_block = NULL;
		}
	}

	/* At this point rx_block is either
	 *   * NULL if there are no leftover samples, OR
	 *   * src buffer if leftover data can be copied from back to front, OR
	 *   * brand new buffer if there is leftover data in mic buffer.
	 */
	ctx->pending_mic_buf = rx_block;
	ctx->pending_mic_samples = 0;

	/* Copy excess samples from pending mic buf, if any */
	if (mic_samples > samples_to_send) {
		size_t bytes;

		/* Samples in mic buffer are already compacted */
		bytes = (mic_samples - samples_to_send) * BYTES_PER_SAMPLE;
		memcpy(ctx->pending_mic_buf, &mic_buf[mic_samples], bytes);

		ctx->pending_mic_samples = mic_samples - samples_to_send;
		dst = &ctx->pending_mic_buf[bytes];
	} else {
		dst = ctx->pending_mic_buf;
	}

	/* Copy excess samples from src buffer, so we don't lose any */
	while (rx_samples > 0) {
		memcpy(dst, src, BYTES_PER_SAMPLE);

		dst += BYTES_PER_SAMPLE;
		src += 2 * BYTES_PER_SAMPLE;

		ctx->pending_mic_samples++;
		rx_samples--;
	}

	/* Finally send the microphone samples to host */
	sys_cache_data_flush_range(mic_buf, mic_samples * BYTES_PER_SAMPLE);
	if (usbd_uac2_send(dev, MICROPHONE_IN_TERMINAL_ID,
			   mic_buf, mic_samples * BYTES_PER_SAMPLE) < 0) {
		k_mem_slab_free(&i2s_rx_slab, mic_buf);
	}
}

static void uac2_sof(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	struct usb_i2s_ctx *ctx = user_data;

	if (ctx->i2s_started) {
		feedback_process(ctx->fb);
	}

	/* If we didn't receive data since last SOF but either terminal is
	 * enabled, then we have to come up with the buffer ourself to keep
	 * I2S going.
	 */
	if (!ctx->usb_data_received &&
	    (ctx->microphone_enabled || ctx->headphones_enabled)) {
		/* No data received since last SOF but we have to keep going */
		void *buf;
		int ret;

		ret = k_mem_slab_alloc(&i2s_tx_slab, &buf, K_NO_WAIT);
		if (ret != 0) {
			buf = NULL;
		}

		if (buf) {
			/* Use size 0 to utilize zero-fill functionality */
			uac2_data_recv_cb(dev, HEADPHONES_OUT_TERMINAL_ID,
					  buf, 0, user_data);
		}
	}
	ctx->usb_data_received = false;

	/* We want to maintain 3 SOFs delay, i.e. samples received from host
	 * during SOF n should be transmitted on I2S during SOF n+3. This
	 * provides enough wiggle room for software scheduling that effectively
	 * eliminates "buffers not provided in time" problem.
	 *
	 * ">= 2" translates into 3 SOFs delay because the timeline is:
	 * USB SOF n
	 *   OUT DATA0 n received from host
	 * USB SOF n+1
	 *   DATA0 n is available to UDC driver (See Universal Serial Bus
	 *   Specification Revision 2.0 5.12.5 Data Prebuffering) and copied
	 *   to I2S buffer before SOF n+2; i2s_counter = 1
	 *   OUT DATA0 n+1 received from host
	 * USB SOF n+2
	 *   DATA0 n+1 is copied; i2s_counter = 2
	 *   OUT DATA0 n+2 received from host
	 * USB SOF n+3
	 *   This function triggers I2S start
	 *   DATA0 n+2 is copied; i2s_counter is no longer relevant
	 *   OUT DATA0 n+3 received from host
	 */
	if (!ctx->i2s_started &&
	    (ctx->headphones_enabled || ctx->microphone_enabled) &&
	    ctx->i2s_counter >= 2) {
		i2s_trigger(ctx->i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
		ctx->i2s_started = true;
		feedback_start(ctx->fb, ctx->i2s_counter);
		ctx->i2s_counter = 0;
	}

	/* Start sending I2S RX data only when there are at least 3 buffers
	 * ready with data. This guarantees that there'll always be a buffer
	 * available from which sample can be borrowed.
	 */
	if (!ctx->rx_started && ctx->i2s_started && ctx->i2s_counter >= 3) {
		ctx->rx_started = true;
	}

	if (ctx->rx_started) {
		process_mic_data(dev, ctx);
	}
}

static struct uac2_ops usb_audio_ops = {
	.sof_cb = uac2_sof,
	.terminal_update_cb = uac2_terminal_update_cb,
	.get_recv_buf = uac2_get_recv_buf,
	.data_recv_cb = uac2_data_recv_cb,
	.buf_release_cb = uac2_buf_release_cb,
};

static struct usb_i2s_ctx main_ctx;

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(uac2_headset));
	struct usbd_context *sample_usbd;
	struct i2s_config config;
	int ret;

	main_ctx.i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s_rxtx));

	if (!device_is_ready(main_ctx.i2s_dev)) {
		printk("%s is not ready\n", main_ctx.i2s_dev->name);
		return 0;
	}

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &i2s_tx_slab;
	config.block_size = MAX_BLOCK_SIZE;
	config.timeout = 0;

	ret = i2s_configure(main_ctx.i2s_dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		printk("Failed to configure TX stream: %d\n", ret);
		return 0;
	}

	config.mem_slab = &i2s_rx_slab;
	ret = i2s_configure(main_ctx.i2s_dev, I2S_DIR_RX, &config);
	if (ret < 0) {
		printk("Failed to configure RX stream: %d\n", ret);
		return 0;
	}

	main_ctx.fb = feedback_init();

	usbd_uac2_set_ops(dev, &usb_audio_ops, &main_ctx);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		return ret;
	}

	return 0;
}
