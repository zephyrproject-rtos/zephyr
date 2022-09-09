/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Simple backend that adds an offset (defined into the DT) to whatever it is
 * passed in input as IPC message.
 */

#include <zephyr/ipc/ipc_service_backend.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT		ipc_service_backend

struct backend_data_t {
	const struct ipc_ept_cfg *cfg;
};

struct backend_config_t {
	unsigned int offset;
};

static int send(const struct device *instance, void *token,
		const void *p_data, size_t len)
{
	const struct backend_config_t *config;
	struct backend_data_t *data;
	uint8_t *msg;

	config = instance->config;
	data = instance->data;
	msg = (uint8_t *) p_data;

	*msg += config->offset;

	data->cfg->cb.received(msg, sizeof(*msg), data->cfg->priv);

	return 0;
}

static int register_ept(const struct device *instance,
			void **token,
			const struct ipc_ept_cfg *cfg)
{
	struct backend_data_t *data = instance->data;

	data->cfg = cfg;

	return 0;
}

const static struct ipc_service_backend backend_ops = {
	.send = send,
	.register_endpoint = register_ept,
};

static int backend_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Nothing to do */

	return 0;
}

#define DEFINE_BACKEND_DEVICE(i)					\
	static struct backend_config_t backend_config_##i = {		\
		.offset = DT_INST_PROP(i, offset),			\
	};								\
									\
	static struct backend_data_t backend_data_##i;			\
									\
	DEVICE_DT_INST_DEFINE(i,					\
			 &backend_init,					\
			 NULL,						\
			 &backend_data_##i,				\
			 &backend_config_##i,				\
			 POST_KERNEL,					\
			 CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,	\
			 &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
