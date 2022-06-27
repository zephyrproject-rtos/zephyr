/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/ipc_service_backend.h>

#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(ipc_service, CONFIG_IPC_SERVICE_LOG_LEVEL);

int ipc_service_open_instance(const struct device *instance)
{
	const struct ipc_service_backend *backend;

	if (!instance) {
		LOG_ERR("Invalid instance");
		return -EINVAL;
	}

	backend = (const struct ipc_service_backend *) instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->open_instance) {
		/* maybe not needed on backend */
		return 0;
	}

	return backend->open_instance(instance);
}

int ipc_service_register_endpoint(const struct device *instance,
				  struct ipc_ept *ept,
				  const struct ipc_ept_cfg *cfg)
{
	const struct ipc_service_backend *backend;

	if (!instance || !ept || !cfg) {
		LOG_ERR("Invalid instance, endpoint or configuration");
		return -EINVAL;
	}

	backend = (const struct ipc_service_backend *) instance->api;

	if (!backend || !backend->register_endpoint) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	LOG_DBG("Register endpoint %s", cfg->name ? cfg->name : "");

	ept->instance = instance;

	return backend->register_endpoint(instance, &ept->token, cfg);
}

int ipc_service_send(struct ipc_ept *ept, const void *data, size_t len)
{
	const struct ipc_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend || !backend->send) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	return backend->send(ept->instance, ept->token, data, len);
}

int ipc_service_get_tx_buffer_size(struct ipc_ept *ept)
{
	const struct ipc_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->get_tx_buffer_size) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->get_tx_buffer_size(ept->instance, ept->token);
}

int ipc_service_get_tx_buffer(struct ipc_ept *ept, void **data, uint32_t *len, k_timeout_t wait)
{
	const struct ipc_service_backend *backend;

	if (!ept || !data || !len) {
		LOG_ERR("Invalid endpoint, data or len pointer");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->send_nocopy || !backend->get_tx_buffer) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->get_tx_buffer(ept->instance, ept->token, data, len, wait);
}

int ipc_service_drop_tx_buffer(struct ipc_ept *ept, const void *data)
{
	const struct ipc_service_backend *backend;

	if (!ept || !data) {
		LOG_ERR("Invalid endpoint or data pointer");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->drop_tx_buffer) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->drop_tx_buffer(ept->instance, ept->token, data);
}

int ipc_service_send_nocopy(struct ipc_ept *ept, const void *data, size_t len)
{
	const struct ipc_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->get_tx_buffer || !backend->send_nocopy) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->send_nocopy(ept->instance, ept->token, data, len);
}

int ipc_service_hold_rx_buffer(struct ipc_ept *ept, void *data)
{
	const struct ipc_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	/* We also need the release function */
	if (!backend->release_rx_buffer || !backend->hold_rx_buffer) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->hold_rx_buffer(ept->instance, ept->token, data);
}
int ipc_service_release_rx_buffer(struct ipc_ept *ept, void *data)
{
	const struct ipc_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	backend = ept->instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	/* We also need the hold function */
	if (!backend->hold_rx_buffer || !backend->release_rx_buffer) {
		LOG_ERR("No-copy feature not available");
		return -EIO;
	}

	return backend->release_rx_buffer(ept->instance, ept->token, data);
}
