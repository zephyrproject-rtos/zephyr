/*
 * Copyright 2026 Basalte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define I2S_NODE_RX DT_ALIAS(i2s_rtio_rx)
#define I2S_NODE_TX DT_ALIAS(i2s_rtio_tx)

#if DT_HAS_ALIAS(i2s_rtio_adc)
#include <zephyr/audio/codec.h>
#define ADC_NODE DT_ALIAS(i2s_rtio_adc)
#endif /* DT_HAS_ALIAS(i2s_rtio_adc) */
#if DT_HAS_ALIAS(i2s_rtio_dac)
#include <zephyr/audio/codec.h>
#define DAC_NODE DT_ALIAS(i2s_rtio_dac)
#endif /* DT_HAS_ALIAS(i2s_rtio_dac) */

/* Corresponds with 1ms of 32 bit samples at 48kHz stereo */
#define I2S_BLOCK_SIZE 384

/* Must stay a power of 2 (RTIO_DEFINE_WITH_MEMPOOL requirement) and evenly divide
 * I2S_BLOCK_SIZE, so a read request allocates exactly I2S_BLOCK_SIZE bytes with no
 * leftover/stale bytes and no change to the AES67 1ms packet cadence. This does slightly increase
 * the chance at memory fragmentation
 */
#define BLOCK_SIZE   64
#define BLOCK_ALLIGN 32
/* Room for 16 concurrent I2S_BLOCK_SIZE reads (I2S_BLOCK_SIZE / BLOCK_SIZE blocks each) */
#define N_BLOCKS     (16 * (I2S_BLOCK_SIZE / BLOCK_SIZE))

#define SAMPLE_BIT_WIDTH 32U
#define SAMPLE_FREQUENCY (48000U)

static I2S_IODEV_DEFINE(my_i2s_rx_iodev, I2S_NODE_RX);
static I2S_IODEV_DEFINE(my_i2s_tx_iodev, I2S_NODE_TX);

/* Better to have 2 contexts, as otherwhise we can have race conditions when loading the sqe's for
 * the tx jitter buffer with rtio_sqe_copy_in, while the rx dma callback is already going */
RTIO_DEFINE_WITH_MEMPOOL(my_rtio_rx_ctx, 16, 16, N_BLOCKS, BLOCK_SIZE, BLOCK_ALLIGN);
RTIO_DEFINE(my_rtio_tx_ctx, 16, 8);

#ifdef ADC_NODE
static void adc_worker(struct k_work *work)
{
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);

	audio_codec_start_output(dev_adc);
}

K_WORK_DELAYABLE_DEFINE(adc_work, adc_worker);
#endif /* ADC_NODE */

#ifdef DAC_NODE
static void dac_worker(struct k_work *work)
{
	const struct device *dev_dac = DEVICE_DT_GET(DAC_NODE);
	struct audio_codec_cfg audio_cfg;
	int ret;

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels = 2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;

	ret = audio_codec_configure(dev_dac, &audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure codec (%d)", ret);
		return;
	}

	k_msleep(1000);
	audio_codec_start_output(dev_dac);
}

K_WORK_DELAYABLE_DEFINE(dac_work, dac_worker);

#endif /* DAC_NODE */

static void tx_write_done(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int res, void *buf)
{
	// NOTE: rtio_ctx here is equal to my_rtio_tx_ctx!

	rtio_release_buffer(&my_rtio_rx_ctx, buf, I2S_BLOCK_SIZE);
}

int main(void)
{
	const struct device *dev_i2s = DEVICE_DT_GET(I2S_NODE_RX);
	struct i2s_config i2s_cfg;
	struct rtio_sqe *sqe_rx_cfg, *sqe_rx_trig;
	struct rtio_sqe *sqes_rx_buf[8];
	// 1 configure, 4 * (tx data loads + callback), 1 trigger
	struct rtio_sqe sqes_tx_buf[10];
	// 1 for write operation, 1 for callback operation to free the buffer after write completed
	struct rtio_sqe sqes_audio[2];
	struct rtio_cqe *cqe;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int ret;

	LOG_INF("Hello world from i2s rtio sample!");

#ifdef ADC_NODE
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);

	if (!device_is_ready(dev_adc)) {
		LOG_ERR("Adc device is not ready");
		return -ENODEV;
	}

	/* Enable ADC after I2S is enabled */
	k_work_schedule(&adc_work, K_MSEC(500));
#endif /* ADC_NODE */

#ifdef DAC_NODE
	const struct device *dev_dac = DEVICE_DT_GET(DAC_NODE);

	if (!device_is_ready(dev_dac)) {
		LOG_ERR("Dac device is not ready");
		return -ENODEV;
	}

	/* Enable DAC after I2S is enabled */
	k_work_schedule(&dac_work, K_MSEC(500));
#endif /* ADC_NODE */

	if (!device_is_ready(dev_i2s)) {
		printf("I2S device not ready\n");
		return -ENODEV;
	}
	/* Configure I2S stream */
	i2s_cfg.word_size = SAMPLE_BIT_WIDTH;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg.block_size = I2S_BLOCK_SIZE;
	i2s_cfg.timeout = 2000;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;

	/* START RECEIVER */

	sqe_rx_cfg = rtio_sqe_acquire(&my_rtio_rx_ctx);
	rtio_sqe_prep_i2s_configure(sqe_rx_cfg, &my_i2s_rx_iodev, RTIO_PRIO_NORM, I2S_DIR_RX,
				    (void *)&i2s_cfg, (void *)"configure rx");

	ARRAY_FOR_EACH(sqes_rx_buf, i) {
		// Will get rescheduled once complete. We cannot use only 1 multishot sqe, as after
		// the jitter buffer is emptied, it will be the only remaining sqe, and will not be
		// able to keep up
		sqes_rx_buf[i] = rtio_sqe_acquire(&my_rtio_rx_ctx);
		rtio_sqe_prep_read_multishot(sqes_rx_buf[i], &my_i2s_rx_iodev, RTIO_PRIO_NORM,
					     NULL);
	}

	sqe_rx_trig = rtio_sqe_acquire(&my_rtio_rx_ctx);
	rtio_sqe_prep_i2s_trigger(sqe_rx_trig, &my_i2s_rx_iodev, RTIO_PRIO_NORM, I2S_DIR_RX,
				  I2S_TRIGGER_START, (void *)"trigger start rx");
	// Wait for configure and trigger to end
	rtio_submit(&my_rtio_rx_ctx, 2);
	for (size_t i = 0; i < 2; i++) {
		cqe = rtio_cqe_consume(&my_rtio_rx_ctx);
		LOG_INF("Result for %s: %d", (char *)cqe->userdata, cqe->result);
		rtio_cqe_release(&my_rtio_rx_ctx, cqe);
	}

	LOG_INF("Started receiver");

	/* START TRANSMITTER */

	rtio_sqe_prep_i2s_configure(&sqes_tx_buf[0], &my_i2s_tx_iodev, RTIO_PRIO_NORM, I2S_DIR_TX,
				    (void *)&i2s_cfg, (void *)"configure tx");

	// Fill tx jitter buffer with buffers from rx
	for (size_t i = 0; i < 4; i++) {
		cqe = rtio_cqe_consume_block(&my_rtio_rx_ctx);
		if (cqe->result < 0) {
			LOG_ERR("Failed to consume rx block (%d)", cqe->result);
		}

		if (cqe->userdata != NULL) {
			LOG_ERR("Received cqe not from initial reads");
			rtio_cqe_release(&my_rtio_rx_ctx, cqe);
			continue;
		}

		ret = rtio_cqe_get_mempool_buffer(&my_rtio_rx_ctx, cqe, &buf, &buf_len);
		if (ret < 0) {
			LOG_ERR("Failed to get mempool buffer (%d)", ret);
			rtio_cqe_release(&my_rtio_rx_ctx, cqe);
			continue;
		}

		if (buf_len != I2S_BLOCK_SIZE) {
			LOG_ERR("Buf len not equal to I2S block size (%d != %d)", buf_len,
				I2S_BLOCK_SIZE);
			goto close;
		}

		rtio_sqe_prep_write(&sqes_tx_buf[2 * i + 1], &my_i2s_tx_iodev, RTIO_PRIO_NORM, buf,
				    I2S_BLOCK_SIZE, NULL);
		rtio_sqe_prep_callback_no_cqe(&sqes_tx_buf[2 * i + 2], tx_write_done, (void *)buf,
					      NULL);

		/* We want the callback sqe to submit right after write sqe completed. No need for a
		 * cqe. */
		sqes_tx_buf[2 * i + 1].flags |= RTIO_SQE_CHAINED | RTIO_SQE_NO_RESPONSE;
	}

	rtio_sqe_prep_i2s_trigger(&sqes_tx_buf[9], &my_i2s_tx_iodev, RTIO_PRIO_NORM, I2S_DIR_TX,
				  I2S_TRIGGER_START, (void *)"trigger start tx");

	rtio_sqe_copy_in(&my_rtio_tx_ctx, sqes_tx_buf, ARRAY_SIZE(sqes_tx_buf));

	// Wait for configure and trigger to succeed
	rtio_submit(&my_rtio_tx_ctx, 2);
	for (size_t i = 0; i < 2; i++) {
		cqe = rtio_cqe_consume(&my_rtio_tx_ctx);
		LOG_INF("Result for %s: %d", (char *)cqe->userdata, cqe->result);
		rtio_cqe_release(&my_rtio_tx_ctx, cqe);
	}

	LOG_INF("Started transmitter");

	/* LOOP */

	while (true) {
		cqe = rtio_cqe_consume_block(&my_rtio_rx_ctx);

		if (cqe->result < 0) {
			LOG_ERR("Loop returned non 0 (%d)", cqe->result);
			break;
		}

		ret = rtio_cqe_get_mempool_buffer(&my_rtio_rx_ctx, cqe, &buf, &buf_len);
		if (ret < 0) {
			LOG_ERR("Failed to get mempool buffer (%d)", ret);
			rtio_cqe_release(&my_rtio_rx_ctx, cqe);
			continue;
		}
		rtio_cqe_release(&my_rtio_rx_ctx, cqe);

		rtio_sqe_prep_write(&sqes_audio[0], &my_i2s_tx_iodev, RTIO_PRIO_NORM, buf,
				    I2S_BLOCK_SIZE, NULL);
		rtio_sqe_prep_callback_no_cqe(&sqes_audio[1], tx_write_done, (void *)buf, NULL);

		/* We want the callback sqe to submit right after write sqe completed. No need for a
		 * cqe. */
		sqes_audio[0].flags |= RTIO_SQE_CHAINED | RTIO_SQE_NO_RESPONSE;

		rtio_sqe_copy_in(&my_rtio_tx_ctx, sqes_audio, ARRAY_SIZE(sqes_audio));
		rtio_submit(&my_rtio_tx_ctx, 0);

		LOG_HEXDUMP_INF_RATELIMIT(buf, I2S_BLOCK_SIZE, "buf");
	}

close:
	sqe_rx_trig = rtio_sqe_acquire(&my_rtio_rx_ctx);
	rtio_sqe_prep_i2s_trigger(sqe_rx_trig, &my_i2s_rx_iodev, RTIO_PRIO_NORM, I2S_DIR_RX,
				  I2S_TRIGGER_DROP, (void *)"trigger drop rx");
	rtio_submit(&my_rtio_rx_ctx, 1);

	sqe_rx_trig = rtio_sqe_acquire(&my_rtio_tx_ctx);
	rtio_sqe_prep_i2s_trigger(sqe_rx_trig, &my_i2s_tx_iodev, RTIO_PRIO_NORM, I2S_DIR_TX,
				  I2S_TRIGGER_DROP, (void *)"trigger drop tx");
	rtio_submit(&my_rtio_tx_ctx, 1);

	return 0;
}
