/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/ipc_msg_service.h>
#include <zephyr/ipc/ipc_msg_service_backend.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(ipc_msg_service, CONFIG_IPC_MSG_SERVICE_LOG_LEVEL);

int ipc_msg_service_open_instance(const struct device *instance)
{
	const struct ipc_msg_service_backend *backend;

	if (!instance) {
		LOG_ERR("Invalid instance");
		return -EINVAL;
	}

	backend = (const struct ipc_msg_service_backend *)instance->api;

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

int ipc_msg_service_close_instance(const struct device *instance)
{
	const struct ipc_msg_service_backend *backend;

	if (!instance) {
		LOG_ERR("Invalid instance");
		return -EINVAL;
	}

	backend = (const struct ipc_msg_service_backend *)instance->api;

	if (!backend) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	if (!backend->close_instance) {
		/* maybe not needed on backend */
		return 0;
	}

	return backend->close_instance(instance);
}

int ipc_msg_service_register_endpoint(const struct device *instance, struct ipc_msg_ept *ept,
				      const struct ipc_msg_ept_cfg *cfg)
{
	const struct ipc_msg_service_backend *backend;

	if (!instance || !ept || !cfg) {
		LOG_ERR("Invalid instance, endpoint or configuration");
		return -EINVAL;
	}

	backend = (const struct ipc_msg_service_backend *)instance->api;

	if (!backend || !backend->register_endpoint) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	LOG_DBG("Register endpoint %s", cfg->name ? cfg->name : "");

	ept->instance = instance;

	return backend->register_endpoint(instance, &ept->token, cfg);
}

int ipc_msg_service_deregister_endpoint(struct ipc_msg_ept *ept)
{
	const struct ipc_msg_service_backend *backend;
	int err;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	if (!ept->instance) {
		LOG_ERR("Endpoint not registered\n");
		return -ENOENT;
	}

	backend = ept->instance->api;

	if (!backend || !backend->deregister_endpoint) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	err = backend->deregister_endpoint(ept->instance, ept->token);
	if (err != 0) {
		return err;
	}

	ept->instance = 0;

	return 0;
}

int ipc_msg_service_send(struct ipc_msg_ept *ept, uint16_t msg_type, const void *msg_data)
{
	const struct ipc_msg_service_backend *backend;

	if (!ept) {
		LOG_ERR("Invalid endpoint");
		return -EINVAL;
	}

	if (!ept->instance) {
		LOG_ERR("Endpoint not registered\n");
		return -ENOENT;
	}

	backend = ept->instance->api;

	if (!backend || !backend->send) {
		LOG_ERR("Invalid backend configuration");
		return -EIO;
	}

	return backend->send(ept->instance, ept->token, msg_type, msg_data);
}

int ipc_msg_service_query(struct ipc_msg_ept *ept, uint16_t query_type, const void *query_data,
			  void *query_response)
{
	const struct ipc_msg_service_backend *backend;

	if (!ept) {
		return -EINVAL;
	}

	if (!ept->instance) {
		return -ENOENT;
	}

	backend = ept->instance->api;

	if (!backend || !backend->query) {
		return -EIO;
	}

	return backend->query(ept->instance, ept->token, query_type, query_data, query_response);
}
