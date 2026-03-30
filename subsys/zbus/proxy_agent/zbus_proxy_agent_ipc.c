/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_proxy_agent_ipc, CONFIG_ZBUS_PROXY_AGENT_IPC_LOG_LEVEL);

static void zbus_proxy_agent_ipc_bound_cb(void *config)
{
	struct zbus_proxy_agent_ipc_data *ipc_data = (struct zbus_proxy_agent_ipc_data *)config;

	k_sem_give(&ipc_data->ept_bound_sem);
	LOG_DBG("IPC endpoint %s bound", ipc_data->ept_cfg.name);
}

static void zbus_proxy_agent_ipc_error_cb(const char *error_msg, void *config)
{
	__maybe_unused struct zbus_proxy_agent_ipc_data *ipc_data =
		(struct zbus_proxy_agent_ipc_data *)config;

	LOG_ERR("IPC error: %s on endpoint %s", error_msg, ipc_data->ept_cfg.name);
}

static void zbus_proxy_agent_ipc_recv_callback(const void *data, size_t len, void *config)
{
	struct zbus_proxy_agent_ipc_data *ipc_data;
	const struct zbus_proxy_msg *msg;
	int ret;

	CHECKIF(data == NULL) {
		LOG_ERR("Received invalid data");
		return;
	}
	CHECKIF(len == 0 || len != sizeof(struct zbus_proxy_msg)) {
		LOG_ERR("Received data of invalid length: %zu", len);
		return;
	}

	ipc_data = (struct zbus_proxy_agent_ipc_data *)config;
	msg = (const struct zbus_proxy_msg *)data;

	if (ipc_data->recv_cb) {
		ret = ipc_data->recv_cb(ipc_data->recv_cb_config_ptr, msg);
		if (ret < 0) {
			LOG_WRN("Proxy agent receive callback failed for channel '%s': %d",
				msg->channel_name, ret);
		}
	}
}

static int zbus_proxy_agent_ipc_backend_init(const struct zbus_proxy_agent *agent)
{
	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in IPC backend init");
	_ZBUS_ASSERT(agent->backend_config != NULL,
		     "Backend configuration is NULL in backend_init");

	int ret;
	const struct zbus_proxy_agent_ipc_config *ipc_config;
	struct zbus_proxy_agent_ipc_data *ipc_data;

	ipc_config = (const struct zbus_proxy_agent_ipc_config *)agent->backend_config;
	ipc_data = ipc_config->data;

	CHECKIF(ipc_config->dev == NULL) {
		LOG_ERR("IPC device is NULL");
		return -ENODEV;
	}
	CHECKIF(ipc_data == NULL) {
		LOG_ERR("IPC runtime data is NULL");
		return -EINVAL;
	}
	CHECKIF(ipc_data->initialized) {
		LOG_ERR("IPC backend for endpoint %s already initialized", ipc_config->ept_name);
		return -EALREADY;
	}

	LOG_DBG("Initializing IPC backend for proxy agent");
	LOG_DBG("IPC config pointer: %p, device: %p", (void *)ipc_config, (void *)ipc_config->dev);

	ret = k_sem_init(&ipc_data->ept_bound_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize IPC endpoint bound semaphore: %d", ret);
		return ret;
	}

	if (!device_is_ready(ipc_config->dev)) {
		LOG_ERR("IPC device is not ready");
		return -ENODEV;
	}

	ipc_data->ept_cfg.name = ipc_config->ept_name;
	ipc_data->ept_cfg.cb.received = zbus_proxy_agent_ipc_recv_callback;
	ipc_data->ept_cfg.cb.error = zbus_proxy_agent_ipc_error_cb;
	ipc_data->ept_cfg.cb.bound = zbus_proxy_agent_ipc_bound_cb;
	ipc_data->ept_cfg.priv = ipc_data;

	ret = ipc_service_open_instance(ipc_config->dev);
	if (ret < 0) {
		LOG_ERR("Failed to open IPC instance %s: %d", ipc_config->dev->name, ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_config->dev, &ipc_data->ipc_ept,
					    &ipc_data->ept_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register IPC endpoint %s: %d", ipc_data->ept_cfg.name, ret);
		ipc_service_close_instance(ipc_config->dev);

		return ret;
	}

	ret = k_sem_take(&ipc_data->ept_bound_sem,
			 K_MSEC(CONFIG_ZBUS_PROXY_AGENT_IPC_BIND_TIMEOUT_MS));
	if (ret < 0) {
		LOG_ERR("IPC endpoint %s failed to bind within %d ms: %d", ipc_data->ept_cfg.name,
			CONFIG_ZBUS_PROXY_AGENT_IPC_BIND_TIMEOUT_MS, ret);
		ipc_service_deregister_endpoint(&ipc_data->ipc_ept);
		ipc_service_close_instance(ipc_config->dev);

		return ret;
	}
	LOG_DBG("ZBUS Proxy agent IPC initialized for device %s with endpoint %s",
		ipc_config->dev->name, ipc_data->ept_cfg.name);
	ipc_data->initialized = 1U;

	return 0;
}

static int zbus_proxy_agent_ipc_backend_send(const struct zbus_proxy_agent *agent,
					     struct zbus_proxy_msg *msg)
{
	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in IPC backend send");
	_ZBUS_ASSERT(msg != NULL, "Message is NULL in IPC backend send");

	int ret;
	const struct zbus_proxy_agent_ipc_config *ipc_config;
	struct zbus_proxy_agent_ipc_data *ipc_data;

	ipc_config = (const struct zbus_proxy_agent_ipc_config *)agent->backend_config;
	ipc_data = ipc_config->data;

	LOG_DBG("Sending message via IPC backend for proxy agent");

	ret = ipc_service_send(&ipc_data->ipc_ept, msg, sizeof(struct zbus_proxy_msg));
	if (ret != sizeof(struct zbus_proxy_msg)) {
		LOG_ERR("Failed to send message via IPC endpoint %s: %d", ipc_data->ept_cfg.name,
			ret);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int zbus_proxy_agent_ipc_backend_set_recv_cb(const struct zbus_proxy_agent *agent,
						    zbus_proxy_agent_recv_cb_t recv_cb)
{
	_ZBUS_ASSERT(recv_cb != NULL, "Receive callback is NULL in set_recv_cb for IPC backend");
	_ZBUS_ASSERT(agent != NULL, "Proxy agent config is NULL in set_recv_cb for IPC backend");

	const struct zbus_proxy_agent_ipc_config *ipc_config;
	struct zbus_proxy_agent_ipc_data *ipc_data;

	ipc_config = (const struct zbus_proxy_agent_ipc_config *)agent->backend_config;
	ipc_data = ipc_config->data;

	LOG_DBG("Setting receive callback for IPC backend of proxy agent");

	ipc_data->recv_cb = recv_cb;
	ipc_data->recv_cb_config_ptr = agent;

	return 0;
}

const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api = {
	.backend_init = zbus_proxy_agent_ipc_backend_init,
	.backend_send = zbus_proxy_agent_ipc_backend_send,
	.backend_set_recv_cb = zbus_proxy_agent_ipc_backend_set_recv_cb,
};

ZBUS_PROXY_AGENT_BACKEND_DEFINE(zbus_proxy_agent_ipc_backend_desc, ZBUS_PROXY_AGENT_BACKEND_IPC,
				&zbus_proxy_agent_ipc_backend_api);
