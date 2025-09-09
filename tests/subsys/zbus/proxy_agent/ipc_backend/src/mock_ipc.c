/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>
#include "mock_ipc.h"

#define DT_DRV_COMPAT fake_ipc

DEFINE_FAKE_VALUE_FUNC4(int, fake_ipc_send, const struct device *, void *, const void *, size_t);
DEFINE_FAKE_VALUE_FUNC3(int, fake_ipc_register_endpoint, const struct device *, void **,
			const struct ipc_ept_cfg *);

DEFINE_FAKE_VALUE_FUNC1(int, fake_ipc_open_instance, const struct device *);
DEFINE_FAKE_VALUE_FUNC1(int, fake_ipc_close_instance, const struct device *);
DEFINE_FAKE_VALUE_FUNC2(int, fake_ipc_deregister_endpoint, const struct device *, void *);

/* Global stored config for callback testing */
static const struct ipc_ept_cfg *stored_ept_cfg;

/* Flag to track if bound callback was triggered */
static volatile bool bound_callback_triggered;

struct fake_ipc_data {
	const struct ipc_ept_cfg *stored_ept_cfg;
};

int fake_ipc_send_with_copy(const struct device *instance, void *token, const void *data,
			    size_t len)
{
	/* Copy data to a static buffer to avoid issues with test data scope
	 * and lifetime.
	 * Would not be needed in a real backend implementation as data is copied
	 * during the sending between cores.
	 */
	static uint8_t ipc_data_copy[512];

	CHECKIF(len > sizeof(ipc_data_copy)) {
		return -ENOMEM;
	}
	CHECKIF(data == NULL) {
		return -EINVAL;
	}

	memcpy(ipc_data_copy, data, len);
	return fake_ipc_send(instance, token, ipc_data_copy, len);
}

int fake_ipc_register_endpoint_with_storage(const struct device *instance, void **token,
					    const struct ipc_ept_cfg *cfg)
{
	struct fake_ipc_data *data = instance->data;

	data->stored_ept_cfg = cfg;
	stored_ept_cfg = cfg; /* Also update global for callback testing */

	return fake_ipc_register_endpoint(instance, token, cfg);
}

void trigger_bound_callback(void)
{
	bound_callback_triggered = true;
	if (stored_ept_cfg && stored_ept_cfg->cb.bound) {
		stored_ept_cfg->cb.bound(stored_ept_cfg->priv);
	}
}

bool was_bound_callback_triggered(void)
{
	return bound_callback_triggered;
}

void reset_bound_callback_flag(void)
{
	bound_callback_triggered = false;
}

void trigger_unbound_callback(void)
{
	if (stored_ept_cfg && stored_ept_cfg->cb.unbound) {
		stored_ept_cfg->cb.unbound(stored_ept_cfg->priv);
	}
}

void trigger_received_callback(const void *data, size_t len)
{
	if (stored_ept_cfg && stored_ept_cfg->cb.received) {
		stored_ept_cfg->cb.received(data, len, stored_ept_cfg->priv);
	}
}

void trigger_error_callback(const char *error_msg)
{
	if (stored_ept_cfg && stored_ept_cfg->cb.error) {
		stored_ept_cfg->cb.error(error_msg, stored_ept_cfg->priv);
	}
}

void clear_stored_ept_cfg(void)
{
	stored_ept_cfg = NULL;
}

const struct ipc_service_backend fake_backend_ops = {
	.open_instance = fake_ipc_open_instance,
	.close_instance = fake_ipc_close_instance,
	.send = fake_ipc_send_with_copy,
	.register_endpoint = fake_ipc_register_endpoint_with_storage,
	.deregister_endpoint = fake_ipc_deregister_endpoint,
};

#define DEFINE_FAKE_IPC_DEVICE(i)                                                                  \
	static struct fake_ipc_data fake_ipc_data_##i;                                             \
												   \
	DEVICE_DT_INST_DEFINE(i, NULL, NULL, &fake_ipc_data_##i, NULL, POST_KERNEL,                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_FAKE_IPC_DEVICE)
