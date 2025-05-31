/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dali_lpc11u6x

#include <zephyr/drivers/dali.h> /* api */
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dali_common, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

struct dali_tx_default_cb_ctx {
	struct k_sem done;
	int status;
};

static void dalli_tx_default_cb(const struct device *dev, int error, void *user_data)
{
	struct dali_tx_default_cb_ctx *ctx = user_data;

	ctx->status = error;
	k_sem_give(&ctx->done);
}

static inline int z_impl_dali_send(const struct device *dev, const struct dali_frame *frame,
			    dali_tx_callback_t callback, void *user_data)
{
	const struct dali_driver_api *api = (const struct dali_driver_api *)dev->api;

	if (frame == NULL) {
		return -EINVAL;
	}

	if (callback == NULL) {
		struct dali_tx_default_cb_ctx ctx;
		int err;

		k_sem_init(&ctx.done, 0, 1);

		err = api->send(dev, frame, dalli_tx_default_cb, &ctx);
		if (err != 0) {
			return err;
		}

		k_sem_take(&ctx.done, K_FOREVER);

		return ctx.status;
	}

	return api->send(dev, frame, callback, user_data);
}