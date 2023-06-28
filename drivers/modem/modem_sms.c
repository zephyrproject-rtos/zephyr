/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/modem/sms.h>

#include "modem_sms.h"
#include "modem_context.h"

static sys_slist_t sms_recv_cbs = SYS_SLIST_STATIC_INIT(&sms_recv_cbs);

void notify_sms_recv(const struct device *dev, struct sms_in *sms, int csms_ref, int csms_idx,
			 int csms_tot)
{
	struct sms_recv_cb *cb, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sms_recv_cbs, cb, next, node) {
		if (cb->recv) {
			cb->recv(dev, sms, csms_ref, csms_idx, csms_tot);
		}
	}
}

static struct modem_context *modem_context_from_modem_dev(const struct device *dev)
{
	struct modem_context *mctx;

	for (int i = 0; i < CONFIG_MODEM_CONTEXT_MAX_NUM; i++) {
		mctx = modem_context_from_id(i);
		if (mctx && mctx->dev == dev) {
			return mctx;
		}
	}
	return 0;
}

int sms_msg_send(const struct device *dev, const struct sms_out *sms)
{
	struct modem_context *mctx;

	mctx = modem_context_from_modem_dev(dev);

	if (!mctx) {
		return -ENODEV;
	}

	if (!mctx->send_sms) {
		return -ENOSYS;
	}

	return mctx->send_sms(sms);
}

int sms_msg_recv(const struct device *dev, struct sms_in *sms)
{
	struct modem_context *mctx;

	mctx = modem_context_from_modem_dev(dev);

	if (!mctx) {
		return -ENODEV;
	}

	if (!mctx->recv_sms) {
		return -ENOSYS;
	}

	return mctx->recv_sms(sms);
}

#if defined(CONFIG_MODEM_SMS_CALLBACK)

int sms_recv_cb_en(const struct device *dev, bool enable)
{
	struct modem_context *mctx;

	mctx = modem_context_from_modem_dev(dev);

	if (!mctx) {
		return -ENODEV;
	}

	if (!mctx->recv_sms_cb_en) {
		return -ENOSYS;
	}

	return mctx->recv_sms_cb_en(enable);
}

int sms_recv_cb_register(struct sms_recv_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	sys_slist_append(&sms_recv_cbs, &cb->node);

	return 0;
}

int sms_recv_cb_unregister(struct sms_recv_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&sms_recv_cbs, &cb->node)) {
		return -EALREADY;
	}

	return 0;
}

#endif /* CONFIG_MODEM_SMS_CALLBACK */
