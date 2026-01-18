/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>

#include <string.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent_ipc, CONFIG_ZBUS_PROXY_AGENT_IPC_LOG_LEVEL);

static void zbus_proxy_agent_ipc_bound_cb(void *config)
{
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;

	k_sem_give(&ipc_config->ept_bound_sem);
	LOG_DBG("IPC endpoint %s bound", ipc_config->ept_cfg->name);
}

static void zbus_proxy_agent_ipc_error_cb(const char *error_msg, void *config)
{
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;

	LOG_ERR("IPC error: %s on endpoint %s", error_msg, ipc_config->ept_cfg->name);
}

static void zbus_proxy_agent_ipc_recv_callback(const void *data, size_t len, void *config)
{
	size_t payload_len;
	uint32_t received_crc;
	uint32_t calculated_crc;
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;

	CHECKIF(data == NULL || len < sizeof(uint32_t)) {
		LOG_ERR("Received invalid data");
		return;
	}

	payload_len = len - sizeof(uint32_t);
	memcpy(&received_crc, (const uint8_t *)data + payload_len, sizeof(received_crc));

	calculated_crc = crc32_ieee((const uint8_t *)data, payload_len);
	if (received_crc != calculated_crc) {
		LOG_ERR("CRC mismatch: received 0x%08X, calculated 0x%08X", received_crc,
			calculated_crc);
		return;
	}

	LOG_DBG("Received verified message of %zu bytes", payload_len);

	if (ipc_config->recv_cb) {
		int ret = ipc_config->recv_cb((const uint8_t *)data, payload_len,
					      ipc_config->recv_cb_user_data);
		if (ret < 0) {
			LOG_ERR("Receive callback failed: %d", ret);
		}
	} else {
		LOG_WRN("No receive callback configured");
	}
}

static int zbus_proxy_agent_ipc_backend_init(void *config)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;

	CHECKIF(config == NULL) {
		LOG_ERR("Invalid IPC backend configuration");
		return -EINVAL;
	}
	CHECKIF(ipc_config->dev == NULL) {
		LOG_ERR("IPC device is NULL");
		return -ENODEV;
	}
	CHECKIF(ipc_config->ept_cfg == NULL) {
		LOG_ERR("IPC device or endpoint configuration is NULL");
		return -EINVAL;
	}

	ret = k_sem_init(&ipc_config->ept_bound_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize IPC endpoint bound semaphore: %d", ret);
		return ret;
	}

	LOG_DBG("Initialized IPC endpoint bound semaphore for %s", ipc_config->ept_cfg->name);

	if (!device_is_ready(ipc_config->dev)) {
		LOG_ERR("IPC device is not ready");
		return -ENODEV;
	}

	/** Set up IPC endpoint configuration */
	ipc_config->ept_cfg->cb.received = zbus_proxy_agent_ipc_recv_callback;
	ipc_config->ept_cfg->cb.error = zbus_proxy_agent_ipc_error_cb;
	ipc_config->ept_cfg->cb.bound = zbus_proxy_agent_ipc_bound_cb;
	ipc_config->ept_cfg->priv = ipc_config;

	ret = ipc_service_open_instance(ipc_config->dev);
	if (ret < 0) {
		LOG_ERR("Failed to open IPC instance %s: %d", ipc_config->dev->name, ret);
		return ret;
	}
	ret = ipc_service_register_endpoint(ipc_config->dev, &ipc_config->ipc_ept,
					    ipc_config->ept_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register IPC endpoint %s: %d", ipc_config->ept_cfg->name, ret);
		return ret;
	}
	ret = k_sem_take(&ipc_config->ept_bound_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to wait for IPC endpoint %s to be bound: %d",
			ipc_config->ept_cfg->name, ret);
		return ret;
	}
	LOG_DBG("ZBUS Proxy agent IPC initialized for device %s with endpoint %s",
		ipc_config->dev->name, ipc_config->ept_cfg->name);

	return 0;
}

static int zbus_proxy_agent_ipc_backend_send(void *config, uint8_t *data, size_t length)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;
	struct zbus_proxy_agent_ipc_msg transport_msg;
	size_t total_size;
	uint32_t crc;

	CHECKIF(ipc_config == NULL) {
		LOG_ERR("Null IPC backend configuration");
		return -EINVAL;
	}
	CHECKIF(data == NULL || length == 0) {
		LOG_ERR("Invalid parameters for IPC backend send");
		return -EINVAL;
	}

	total_size = length + sizeof(uint32_t);

	if (length > sizeof(transport_msg.payload)) {
		LOG_ERR("Message too large: %zu bytes, max %zu bytes", length,
			sizeof(transport_msg.payload));
		return -EMSGSIZE;
	}

	memcpy(transport_msg.payload, data, length);

	/* Append CRC32 */
	crc = crc32_ieee(data, length);
	memcpy(transport_msg.payload + length, &crc, sizeof(crc));

	ret = ipc_service_send(&ipc_config->ipc_ept, &transport_msg, total_size);
	if (ret < 0) {
		LOG_ERR("Failed to send message via IPC: %d", ret);
		return ret;
	}

	LOG_DBG("Sent message of %zu bytes (+ %zu CRC) via IPC", length, sizeof(uint32_t));
	return 0;
}

static int zbus_proxy_agent_ipc_backend_set_recv_cb(void *config,
						    int (*recv_cb)(const uint8_t *data,
								   size_t length, void *user_data),
						    void *user_data)
{
	struct zbus_proxy_agent_ipc_config *ipc_config =
		(struct zbus_proxy_agent_ipc_config *)config;

	CHECKIF(ipc_config == NULL) {
		LOG_ERR("Null IPC backend configuration");
		return -EINVAL;
	}
	CHECKIF(recv_cb == NULL) {
		LOG_ERR("Invalid parameters for setting IPC backend receive callback");
		return -EINVAL;
	}

	ipc_config->recv_cb = recv_cb;
	ipc_config->recv_cb_user_data = user_data;
	LOG_DBG("Set receive callback for IPC endpoint %s", ipc_config->ept_cfg->name);
	return 0;
}

/* Define the IPC backend API */
const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api = {
	.backend_init = zbus_proxy_agent_ipc_backend_init,
	.backend_send = zbus_proxy_agent_ipc_backend_send,
	.backend_set_recv_cb = zbus_proxy_agent_ipc_backend_set_recv_cb,
};
