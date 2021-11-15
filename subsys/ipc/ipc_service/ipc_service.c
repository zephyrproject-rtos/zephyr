/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipc/ipc_service.h>
#include <ipc/ipc_service_backend.h>

#include <logging/log.h>
#include <zephyr.h>
#include <device.h>

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
