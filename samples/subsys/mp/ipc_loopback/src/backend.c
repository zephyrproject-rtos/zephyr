/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT ipc_service_backend

LOG_MODULE_REGISTER(loopback_backend, LOG_LEVEL_INF);

struct backend_data_t {
	const struct ipc_ept_cfg *ept_cfgs[2];
	int ept_count;
};

static int loopback_send(const struct device *instance, void *token,
			 const void *p_data, size_t len)
{
	struct backend_data_t *data = instance->data;

	/* Route the message to the other registered endpoint */
	for (int i = 0; i < 2; i++) {
		if (data->ept_cfgs[i] && data->ept_cfgs[i] != token) {
			if (data->ept_cfgs[i]->cb.received) {
				data->ept_cfgs[i]->cb.received(p_data, len, data->ept_cfgs[i]->priv);
			}
			return 0;
		}
	}

	return -ENOTCONN;
}

static int loopback_register_ept(const struct device *instance,
				 void **token,
				 const struct ipc_ept_cfg *cfg)
{
	struct backend_data_t *data = instance->data;

	if (data->ept_count >= 2) {
		return -ENOMEM;
	}

	data->ept_cfgs[data->ept_count] = cfg;
	*token = (void *)cfg;
	data->ept_count++;

	LOG_INF("Registered endpoint %s on loopback backend", cfg->name);

	/* When both sink and src endpoints are registered, trigger the bound callback on both */
	if (data->ept_count == 2) {
		LOG_INF("Both endpoints registered. Binding connection...");
		if (data->ept_cfgs[0]->cb.bound) {
			data->ept_cfgs[0]->cb.bound(data->ept_cfgs[0]->priv);
		}
		if (data->ept_cfgs[1]->cb.bound) {
			data->ept_cfgs[1]->cb.bound(data->ept_cfgs[1]->priv);
		}
	}

	return 0;
}

static int loopback_deregister_ept(const struct device *instance, void *token)
{
	struct backend_data_t *data = instance->data;

	for (int i = 0; i < 2; i++) {
		if (data->ept_cfgs[i] == token) {
			data->ept_cfgs[i] = NULL;
			data->ept_count--;
			return 0;
		}
	}

	return -ENOENT;
}

static const struct ipc_service_backend backend_ops = {
	.send = loopback_send,
	.register_endpoint = loopback_register_ept,
	.deregister_endpoint = loopback_deregister_ept,
};

#define DEFINE_BACKEND_DEVICE(i) \
	static struct backend_data_t backend_data_##i = { \
		.ept_count = 0, \
	}; \
	DEVICE_DT_INST_DEFINE(i, \
			      NULL, \
			      NULL, \
			      &backend_data_##i, \
			      NULL, \
			      POST_KERNEL, \
			      CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY, \
			      &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)