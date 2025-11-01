/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/multidomain/zbus_multidomain_ipc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_ipc, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

int zbus_multidomain_ipc_backend_ack(void *config, uint32_t msg_id)
{
	int ret;
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;
	struct zbus_proxy_agent_msg ack_msg;

	ret = zbus_create_proxy_agent_ack_msg(&ack_msg, msg_id);
	if (ret < 0) {
		LOG_ERR("Failed to create ACK message: %d", ret);
		return ret;
	}

	if (!ipc_config) {
		LOG_ERR("Invalid parameters to send ACK");
		return -EINVAL;
	}

	ret = ipc_service_send(&ipc_config->ipc_ept, (void *)&ack_msg, sizeof(ack_msg));
	LOG_DBG("ipc_service_send returned %d", ret);
	if (ret < 0) {
		LOG_ERR("Failed to send ACK message: %d", ret);
		return ret;
	}

	LOG_DBG("Sent ACK for message %d via IPC device %s", msg_id, ipc_config->dev->name);

	return 0;
}

static void zbus_multidomain_ipc_backend_ack_work_handler(struct k_work *work)
{
	int ret;
	struct zbus_multidomain_ipc_config *ipc_config =
		CONTAINER_OF(work, struct zbus_multidomain_ipc_config, ack_work);

	if (!ipc_config) {
		LOG_ERR("Invalid IPC config in ACK work handler");
		return;
	}

	ret = zbus_multidomain_ipc_backend_ack(ipc_config, ipc_config->ack_msg_id);
	if (ret < 0) {
		LOG_ERR("Failed to send ACK for message %d: %d", ipc_config->ack_msg_id, ret);
	}
}

void zbus_multidomain_ipc_bound_cb(void *config)
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	k_sem_give(&ipc_config->ept_bound_sem);
	LOG_DBG("IPC endpoint %s bound", ipc_config->ept_cfg->name);
}

void zbus_multidomain_ipc_error_cb(const char *error_msg, void *config)
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	LOG_ERR("IPC error: %s on endpoint %s", error_msg, ipc_config->ept_cfg->name);
}

void zbus_multidomain_ipc_recv_cb(const void *data, size_t len, void *config)
{
	int ret;
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;
	const struct zbus_proxy_agent_msg *msg = (const struct zbus_proxy_agent_msg *)data;

	if (!data || !len) {
		LOG_ERR("Received empty data on IPC endpoint");
		return;
	}

	if (len != sizeof(struct zbus_proxy_agent_msg)) {
		LOG_ERR("Invalid message size: expected %zu, got %zu",
			sizeof(struct zbus_proxy_agent_msg), len);
		return;
	}

	/* Verify CRC32 */
	if (verify_proxy_agent_msg_crc(msg) != 0) {
		LOG_ERR("Received message with invalid CRC, dropping");
		LOG_HEXDUMP_DBG((const uint8_t *)msg, sizeof(*msg), "Invalid message:");
		LOG_DBG("Received CRC32: 0x%08X, Expected CRC32: 0x%08X", msg->crc32,
			crc32_ieee((const uint8_t *)msg, sizeof(*msg) - sizeof(msg->crc32)));
		return;
	}

	if (msg->type == ZBUS_PROXY_AGENT_MSG_TYPE_ACK) {
		if (!ipc_config->ack_cb) {
			LOG_ERR("ACK callback not set, dropping ACK");
			return;
		}

		ret = ipc_config->ack_cb(msg->id, ipc_config->ack_cb_user_data);
		if (ret < 0) {
			LOG_ERR("Failed to process received ACK: %d", ret);
		}

		return;
	} else if (msg->type == ZBUS_PROXY_AGENT_MSG_TYPE_MSG) {
		/* Schedule work to send ACK to avoid blocking the receive callback */
		ipc_config->ack_msg_id = msg->id;
		if (!ipc_config->recv_cb) {
			LOG_ERR("No receive callback set for IPC endpoint %s",
				ipc_config->ept_cfg->name);
			return;
		}

		ret = ipc_config->recv_cb(msg);
		if (ret < 0) {
			LOG_ERR("Failed to process received message on IPC endpoint %s: %d",
				ipc_config->ept_cfg->name, ret);
		}

		ret = k_work_submit(&ipc_config->ack_work);
		if (ret < 0) {
			LOG_ERR("Failed to submit ACK work: %d", ret);
			return;
		}
	} else {
		LOG_WRN("Unknown message type: %d", msg->type);
	}
}

int zbus_multidomain_ipc_backend_set_recv_cb(void *config,
					     int (*recv_cb)(const struct zbus_proxy_agent_msg *msg))
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	if (!ipc_config || !recv_cb) {
		LOG_ERR("Invalid parameters to set receive callback");
		return -EINVAL;
	}

	ipc_config->recv_cb = recv_cb;
	LOG_DBG("Set receive callback for IPC endpoint %s", ipc_config->ept_cfg->name);
	return 0;
}

int zbus_multidomain_ipc_backend_set_ack_cb(void *config,
					    int (*ack_cb)(uint32_t msg_id, void *user_data),
					    void *user_data)
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	if (!ipc_config || !ack_cb) {
		LOG_ERR("Invalid parameters to set ACK callback");
		return -EINVAL;
	}

	ipc_config->ack_cb = ack_cb;
	ipc_config->ack_cb_user_data = user_data;
	LOG_DBG("Set ACK callback for IPC endpoint %s", ipc_config->ept_cfg->name);
	return 0;
}

int zbus_multidomain_ipc_backend_init(void *config)
{
	int ret;
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	if (!config) {
		LOG_ERR("Invalid IPC backend configuration");
		return -EINVAL;
	}

	if (!ipc_config->dev) {
		LOG_ERR("IPC device is NULL");
		return -ENODEV;
	}
	if (!ipc_config->ept_cfg) {
		LOG_ERR("IPC device or endpoint configuration is NULL");
		return -EINVAL;
	}

	ret = k_sem_init(&ipc_config->ept_bound_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize IPC endpoint bound semaphore: %d", ret);
		return ret;
	}

	k_work_init(&ipc_config->ack_work, zbus_multidomain_ipc_backend_ack_work_handler);

	LOG_DBG("Initialized IPC endpoint bound semaphore for %s", ipc_config->ept_cfg->name);

	if (!device_is_ready(ipc_config->dev)) {
		LOG_ERR("IPC device is not ready");
		return -ENODEV;
	}

	/** Set up IPC endpoint configuration */
	ipc_config->ept_cfg->cb.received = zbus_multidomain_ipc_recv_cb;
	ipc_config->ept_cfg->cb.error = zbus_multidomain_ipc_error_cb;
	ipc_config->ept_cfg->cb.bound = zbus_multidomain_ipc_bound_cb;
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
	LOG_DBG("ZBUS Multidomain IPC initialized for device %s with endpoint %s",
		ipc_config->dev->name, ipc_config->ept_cfg->name);

	return 0;
}

int zbus_multidomain_ipc_backend_send(void *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	if (!ipc_config) {
		LOG_ERR("Invalid IPC backend configuration for send");
		return -EINVAL;
	}

	if (!msg || msg->message_size == 0) {
		LOG_ERR("Invalid message to send on IPC endpoint %s", ipc_config->ept_cfg->name);
		return -EINVAL;
	}

	ret = ipc_service_send(&ipc_config->ipc_ept, (void *)msg, sizeof(*msg));
	if (ret < 0) {
		LOG_ERR("Failed to send message on IPC endpoint %s: %d", ipc_config->ept_cfg->name,
			ret);
		return ret;
	}

	LOG_DBG("Sent message of size %d on IPC endpoint %s", ret, ipc_config->ept_cfg->name);

	return 0;
}

/* Define the IPC backend API */
const struct zbus_proxy_agent_api zbus_multidomain_ipc_api = {
	.backend_init = zbus_multidomain_ipc_backend_init,
	.backend_send = zbus_multidomain_ipc_backend_send,
	.backend_set_recv_cb = zbus_multidomain_ipc_backend_set_recv_cb,
	.backend_set_ack_cb = zbus_multidomain_ipc_backend_set_ack_cb,
};
