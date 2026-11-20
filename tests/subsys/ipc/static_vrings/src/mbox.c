/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>

/* Descriptor tests do not deliver notifications to another processor. */
static int send(const struct device *dev, mbox_channel_id_t channel, const struct mbox_msg *msg)
{
	return 0;
}

static int register_callback(const struct device *dev, mbox_channel_id_t channel,
			     mbox_callback_t cb, void *user_data)
{
	return 0;
}

static int set_enabled(const struct device *dev, mbox_channel_id_t channel, bool enabled)
{
	return 0;
}

static DEVICE_API(mbox, test_mbox_api) = {
	.send = send,
	.register_callback = register_callback,
	.set_enabled = set_enabled,
};

DEVICE_DT_DEFINE(DT_NODELABEL(test_mbox), NULL, NULL, NULL, NULL, PRE_KERNEL_1, 0, &test_mbox_api);
