/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipc/ipc_service.h>
#include <ipc/ipc_service_backend.h>

#include <logging/log.h>
#include <zephyr.h>
#include <device.h>

#define LOG_MODULE_NAME ipc_service
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_IPC_SERVICE_LOG_LEVEL);

const static struct ipc_service_backend *backend;

int ipc_service_register_backend(const struct ipc_service_backend *bkd)
{
	if (backend) {
		return -EALREADY;
	}

	if (!bkd || !bkd->register_endpoint || !bkd->send) {
		return -EINVAL;
	}

	backend = bkd;
	LOG_DBG("Registered: %s", backend->name ? backend->name : "");

	return 0;
}

int ipc_service_register_endpoint(struct ipc_ept **ept, const struct ipc_ept_cfg *cfg)
{
	LOG_DBG("Register endpoint %s", cfg->name ? cfg->name : "");
	if (!backend || !backend->register_endpoint) {
		LOG_ERR("Backend not registered");
		return -EIO;
	}

	if (!ept || !cfg) {
		LOG_ERR("Invalid endpoint or configuration");
		return -EINVAL;
	}

	return backend->register_endpoint(ept, cfg);
}

int ipc_service_send(struct ipc_ept *ept, const void *data, size_t len)
{
	if (!backend || !backend->send) {
		LOG_ERR("Backend not registered");
		return -EIO;
	}

	return backend->send(ept, data, len);
}
