/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 *	Andrew Davis <afd@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mbox_ipm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_mbox, CONFIG_IPM_LOG_LEVEL);

struct ipm_mbox_data {
	ipm_callback_t callback;
	void *user_data;
};

struct ipm_mbox_config {
	struct mbox_dt_spec mbox_tx;
	struct mbox_dt_spec mbox_rx;
};

static void ipm_mbox_callback(const struct device *mboxdev, mbox_channel_id_t channel_id,
			      void *user_data, struct mbox_msg *data)
{
	const struct device *ipmdev = user_data;
	struct ipm_mbox_data *ipm_mbox_data = ipmdev->data;

	ipm_mbox_data->callback(ipmdev, ipm_mbox_data->user_data, channel_id, (void *)data->data);
}

static int ipm_mbox_send(const struct device *ipmdev, int wait, uint32_t id,
			 const void *data, int size)
{
	const struct ipm_mbox_config *config = ipmdev->config;

	struct mbox_msg message = {
		.data = data,
		.size = size,
	};

	return mbox_send_dt(&config->mbox_tx, &message);
}

static void ipm_mbox_register_callback(const struct device *ipmdev,
				       ipm_callback_t cb,
				       void *user_data)
{
	struct ipm_mbox_data *data = ipmdev->data;

	data->callback = cb;
	data->user_data = user_data;
}

static int ipm_mbox_get_max_data_size(const struct device *ipmdev)
{
	const struct ipm_mbox_config *config = ipmdev->config;

	return mbox_mtu_get_dt(&config->mbox_tx);
}

static uint32_t ipm_mbox_get_max_id(const struct device *ipmdev)
{
	const struct ipm_mbox_config *config = ipmdev->config;

	return mbox_max_channels_get_dt(&config->mbox_tx);
}

static int ipm_mbox_set_enable(const struct device *ipmdev, int enable)
{
	const struct ipm_mbox_config *config = ipmdev->config;

	mbox_set_enabled_dt(&config->mbox_rx, enable);

	return 0;
}

static int ipm_mbox_init(const struct device *ipmdev)
{
	const struct ipm_mbox_config *config = ipmdev->config;

	mbox_register_callback_dt(&config->mbox_rx, ipm_mbox_callback, (void *)ipmdev);

	return 0;
}

static const struct ipm_driver_api ipm_mbox_funcs = {
	.send = ipm_mbox_send,
	.register_callback = ipm_mbox_register_callback,
	.max_data_size_get = ipm_mbox_get_max_data_size,
	.max_id_val_get = ipm_mbox_get_max_id,
	.set_enabled = ipm_mbox_set_enable,
};

#define IPM_MBOX_DEV_DEFINE(n)						\
	static struct ipm_mbox_data ipm_mbox_data_##n;			\
	static const struct ipm_mbox_config ipm_mbox_config_##n = {	\
		.mbox_tx = MBOX_DT_SPEC_INST_GET(n, tx),		\
		.mbox_rx = MBOX_DT_SPEC_INST_GET(n, rx),		\
	};								\
	DEVICE_DT_INST_DEFINE(n,					\
			      &ipm_mbox_init,				\
			      NULL,					\
			      &ipm_mbox_data_##n,			\
			      &ipm_mbox_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &ipm_mbox_funcs);

DT_INST_FOREACH_STATUS_OKAY(IPM_MBOX_DEV_DEFINE)
