/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Simple backend that adds an offset (defined into the DT) to whatever it is
 * passed in input as IPC message.
 */

#include <zephyr/ipc/ipc_msg_service_backend.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#define DT_DRV_COMPAT ipc_msg_service_backend

struct backend_data_t {
	bool endpoint_registered;
	const struct ipc_msg_ept_cfg *cfg;
};

struct backend_config_t {
	unsigned int offset;
};

static int send(const struct device *instance, void *token, uint16_t msg_type, const void *msg_data)
{
	const struct backend_config_t *dev_config;
	struct backend_data_t *dev_data;
	const struct ipc_msg_type_cmd *msg;
	struct ipc_msg_type_cmd cb_msg;
	int ret;

	if (msg_type != IPC_MSG_TYPE_CMD) {
		return -ENOTSUP;
	}

	dev_config = instance->config;
	dev_data = instance->data;
	msg = (const struct ipc_msg_type_cmd *)msg_data;

	cb_msg.cmd = msg->cmd + dev_config->offset;

	ret = dev_data->cfg->cb.event(IPC_MSG_EVT_REMOTE_DONE, NULL, dev_data->cfg->priv);
	ARG_UNUSED(ret);

	ret = dev_data->cfg->cb.received(msg_type, &cb_msg, dev_data->cfg->priv);
	ARG_UNUSED(ret);

	return 0;
}

static int query(const struct device *instance, void *token, uint16_t query_type,
		 const void *query_data, void *query_response)
{
	struct backend_data_t *dev_data = instance->data;
	int ret;

	ARG_UNUSED(query_data);
	ARG_UNUSED(query_response);

	if (query_type == IPC_MSG_QUERY_IS_READY) {
		ret = dev_data->endpoint_registered ? 0 : -ENOENT;
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static int register_ept(const struct device *instance, void **token,
			const struct ipc_msg_ept_cfg *cfg)
{
	struct backend_data_t *data = instance->data;

	data->cfg = cfg;
	data->endpoint_registered = true;

	return 0;
}

static int deregister_ept(const struct device *instance, void *token)
{
	struct backend_data_t *data = instance->data;

	data->cfg = NULL;
	data->endpoint_registered = false;

	return 0;
}

const static struct ipc_msg_service_backend backend_ops = {
	.query = query,
	.send = send,
	.register_endpoint = register_ept,
	.deregister_endpoint = deregister_ept,
};

#define DEFINE_BACKEND_DEVICE(i)                                                                   \
	static struct backend_config_t backend_config_##i = {                                      \
		.offset = DT_INST_PROP(i, offset),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct backend_data_t backend_data_##i;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, NULL, NULL, &backend_data_##i, &backend_config_##i, POST_KERNEL,  \
			      CONFIG_IPC_MSG_SERVICE_REG_BACKEND_PRIORITY, &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
