/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>
#include "mock_ipc.h"

#define DT_DRV_COMPAT fake_ipc

DEFINE_FAKE_VALUE_FUNC1(int, fake_ipc_open_instance, const struct device *);
DEFINE_FAKE_VALUE_FUNC1(int, fake_ipc_close_instance, const struct device *);
DEFINE_FAKE_VALUE_FUNC4(int, fake_ipc_send, const struct device *, void *, const void *, size_t);
DEFINE_FAKE_VALUE_FUNC3(int, fake_ipc_register_endpoint, const struct device *, void **,
			const struct ipc_ept_cfg *);
DEFINE_FAKE_VALUE_FUNC2(int, fake_ipc_deregister_endpoint, const struct device *, void *);

/* Global stored config for callback testing */
static const struct ipc_ept_cfg *stored_ept_cfg;
static uint8_t stored_message_data[256];
static size_t stored_message_size;

int custom_fake_ipc_register_endpoint(const struct device *instance, void **token,
				      const struct ipc_ept_cfg *cfg)
{
	ARG_UNUSED(instance);
	ARG_UNUSED(token);
	stored_ept_cfg = cfg;

	return fake_ipc_register_endpoint_fake.return_val;
}

int custom_fake_ipc_send(const struct device *instance, void *token, const void *data, size_t len)
{
	ARG_UNUSED(instance);
	ARG_UNUSED(token);
	len = MIN(len, sizeof(stored_message_data));
	memcpy(stored_message_data, data, len);
	stored_message_size = len;

	if (fake_ipc_send_fake.return_val == 0) {
		return len;
	}
	return fake_ipc_send_fake.return_val;

}

void trigger_bound_callback(void)
{
	if (stored_ept_cfg && stored_ept_cfg->cb.bound) {
		stored_ept_cfg->cb.bound(stored_ept_cfg->priv);
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

void get_stored_message(uint8_t **data, size_t *len)
{
	*data = stored_message_data;
	*len = stored_message_size;
}

void clear_stored_ept_cfg(void)
{
	stored_ept_cfg = NULL;
}

void clear_stored_message(void)
{
	memset(stored_message_data, 0, sizeof(stored_message_data));
	stored_message_size = 0;
}

void init_mock_ipc(void)
{
	RESET_FAKE(fake_ipc_open_instance);
	RESET_FAKE(fake_ipc_close_instance);
	RESET_FAKE(fake_ipc_send);
	RESET_FAKE(fake_ipc_register_endpoint);
	RESET_FAKE(fake_ipc_deregister_endpoint);

	clear_stored_ept_cfg();
	clear_stored_message();
	fake_ipc_register_endpoint_fake.custom_fake = custom_fake_ipc_register_endpoint;
	fake_ipc_send_fake.custom_fake = custom_fake_ipc_send;
}

void deinit_mock_ipc(void)
{
	clear_stored_ept_cfg();
	clear_stored_message();
	fake_ipc_register_endpoint_fake.custom_fake = NULL;
	fake_ipc_send_fake.custom_fake = NULL;
}

const struct ipc_service_backend fake_backend_ops = {
	.open_instance = fake_ipc_open_instance,
	.close_instance = fake_ipc_close_instance,
	.send = fake_ipc_send,
	.register_endpoint = fake_ipc_register_endpoint,
	.deregister_endpoint = fake_ipc_deregister_endpoint,
};

#define DEFINE_FAKE_IPC_DEVICE(i)                                                                  \
	DEVICE_DT_INST_DEFINE(i, NULL, NULL, NULL, NULL, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_FAKE_IPC_DEVICE)
