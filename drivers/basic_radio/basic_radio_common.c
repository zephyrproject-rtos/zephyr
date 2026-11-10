/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common basic_radio code: blocking API on top of the asynchronous backend
 * operations.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/basic_radio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* State shared by a blocking call and its callback, on the caller's stack */
struct sync_ctx {
	struct k_sem done;
	uint8_t *buf;
	uint16_t buf_len;
	struct basic_radio_rx_info *info;
	/* Status of the final event */
	int status;
	/* Bytes copied to buf, negative until a packet is received */
	int rx_len;
};

static void sync_cb(const struct device *dev, const struct basic_radio_evt *evt, void *user_data)
{
	struct sync_ctx *ctx = user_data;

	switch (evt->type) {
	case BASIC_RADIO_EVT_RX_PKT:
		/* Keep the first packet and end the RX operation */
		if (ctx->rx_len < 0) {
			ctx->rx_len = MIN(evt->len, ctx->buf_len);

			if (evt->data != NULL) {
				memcpy(ctx->buf, evt->data, ctx->rx_len);
			}

			if (ctx->info != NULL) {
				*ctx->info = evt->info;
			}

			(void)basic_radio_stop(dev, BASIC_RADIO_STOP_ABORT);
		}
		break;

	case BASIC_RADIO_EVT_TX_DONE:
	case BASIC_RADIO_EVT_RX_DONE:
	case BASIC_RADIO_EVT_TONE_DONE:
		ctx->status = evt->status;
		/* Last access to ctx: the caller returns once it is given */
		k_sem_give(&ctx->done);
		break;

	default:
		break;
	}
}

/* Wait for the final event. Returns true if the operation was aborted on timeout. */
static bool sync_wait(const struct device *dev, struct sync_ctx *ctx, k_timeout_t timeout)
{
	if (k_sem_take(&ctx->done, timeout) == 0) {
		return false;
	}

	(void)basic_radio_stop(dev, BASIC_RADIO_STOP_ABORT);

	/* Every operation ends with a final event */
	(void)k_sem_take(&ctx->done, K_FOREVER);

	return true;
}

int basic_radio_send(const struct device *dev, const uint8_t *data, uint16_t len,
		     const struct basic_radio_tx_params *params, k_timeout_t timeout)
{
	struct sync_ctx ctx = {.rx_len = -1};
	bool timed_out;
	int ret;

	(void)k_sem_init(&ctx.done, 0, 1);

	ret = basic_radio_send_async(dev, data, len, params, sync_cb, &ctx);
	if (ret != 0) {
		return ret;
	}

	timed_out = sync_wait(dev, &ctx, timeout);

	if (timed_out && (ctx.status == -ECANCELED)) {
		return -ETIMEDOUT;
	}

	return ctx.status;
}

int basic_radio_recv(const struct device *dev, uint8_t *buf, uint16_t buf_len,
		     const struct basic_radio_rx_params *params, k_timeout_t timeout,
		     struct basic_radio_rx_info *info)
{
	struct sync_ctx ctx = {
		.buf = buf,
		.buf_len = buf_len,
		.info = info,
		.rx_len = -1,
	};
	bool timed_out;
	int ret;

	(void)k_sem_init(&ctx.done, 0, 1);

	ret = basic_radio_recv_async(dev, params, sync_cb, &ctx);
	if (ret != 0) {
		return ret;
	}

	timed_out = sync_wait(dev, &ctx, timeout);

	/* A packet counts even if the abort that followed it ended the RX */
	if (ctx.rx_len >= 0) {
		return ctx.rx_len;
	}

	if (timed_out && (ctx.status == -ECANCELED)) {
		return -EAGAIN;
	}

	if (ctx.status == 0) {
		return -EIO;
	}

	return ctx.status;
}

/* Slack on top of the tone duration before the blocking call aborts it */
#define TEST_CW_TIMEOUT_MARGIN_MS 100U

int basic_radio_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power,
			uint16_t duration_ms)
{
	const struct basic_radio_tone_params params = {
		.frequency = frequency,
		.duration_us = (uint32_t)duration_ms * USEC_PER_MSEC,
		.tx_power_dbm = tx_power,
	};
	struct sync_ctx ctx = {.rx_len = -1};
	bool timed_out;
	int ret;

	/* A zero duration would mean a tone until stopped */
	if (duration_ms == 0U) {
		return -EINVAL;
	}

	(void)k_sem_init(&ctx.done, 0, 1);

	ret = basic_radio_tone_async(dev, &params, sync_cb, &ctx);
	if (ret != 0) {
		return ret;
	}

	timed_out = sync_wait(dev, &ctx, K_MSEC(duration_ms + TEST_CW_TIMEOUT_MARGIN_MS));

	if (timed_out && (ctx.status == -ECANCELED)) {
		return -ETIMEDOUT;
	}

	return ctx.status;
}
