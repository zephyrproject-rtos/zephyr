/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_dispatcher.h>
#include <nrfs_backend_ipc_service.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(NRFS_BACKEND, CONFIG_NRFS_BACKEND_LOG_LEVEL);

#define MAX_PACKET_DATA_SIZE (CONFIG_NRFS_MAX_BACKEND_PACKET_SIZE)

K_MSGQ_DEFINE(ipc_transmit_msgq, sizeof(ipc_data_packet_t), CONFIG_NRFS_BACKEND_TX_MSG_QUEUE_SIZE,
	      4);

static struct k_work backend_send_work;

static ipc_data_packet_t rx_data;
static ipc_data_packet_t tx_data;

static void ipc_sysctrl_ept_bound(void *priv);
static void ipc_sysctrl_ept_recv(const void *data, size_t size, void *priv);

static K_EVENT_DEFINE(ipc_connected_event);

#define IPC_INIT_DONE_EVENT (0x01)

struct ipc_channel_config_t {
	const struct device *ipc_instance;
	struct ipc_ept_cfg *endpoint_config;
	struct ipc_ept ipc_ept;
	atomic_t status;
	bool enabled;
};

static struct ipc_ept_cfg ipc_sysctrl_ept_cfg = {
	.name = "ipc_to_sysctrl",
	.cb = {
		.bound    = ipc_sysctrl_ept_bound,
		.received = ipc_sysctrl_ept_recv,
	},
};

static struct ipc_channel_config_t ipc_cpusys_channel_config = {
	.ipc_instance	 = DEVICE_DT_GET(DT_NODELABEL(ipc_to_cpusys)),
	.endpoint_config = &ipc_sysctrl_ept_cfg,
	.status		 = ATOMIC_INIT(not_connected),
	.enabled	 = true
};

/**
 * @brief nrfs backend error handler
 *
 * @param error_id The id of an error to handle.
 * @param error additional error code if needed, if not needed use 0.
 * @param fatal true if fatal error and needs special handling
 */
__weak void nrfs_backend_error_handler(enum nrfs_backend_error error_id, int error, bool fatal)
{
	switch (error_id) {
	case NRFS_ERROR_EPT_RECEIVE_DATA_TOO_LONG:
		LOG_ERR("Received data is too long. Config error.");
		break;

	case NRFS_ERROR_NO_DATA_RECEIVED:
		LOG_ERR("No data in received message!");
		break;

	case NRFS_ERROR_IPC_OPEN_INSTANCE:
		LOG_ERR("IPC open instance failure with error: %d", error);
		break;

	case NRFS_ERROR_IPC_REGISTER_ENDPOINT:
		LOG_ERR("IPC register endpoint failure with error: %d", error);
		break;

	default:
		LOG_ERR("Undefined error id: %d, error cause: %d", error_id, error);
		break;
	}

	if (fatal) {
		nrfs_backend_fatal_error_handler(error_id);
	}
}

static void ipc_sysctrl_ept_bound(void *priv)
{
	LOG_INF("Bound to sysctrl.");
	k_event_post(&ipc_connected_event, IPC_INIT_DONE_EVENT);
	atomic_set(&ipc_cpusys_channel_config.status, connected);
}

static void ipc_sysctrl_ept_recv(const void *data, size_t size, void *priv)
{
	__ASSERT(size <= MAX_PACKET_DATA_SIZE, "Received data is too long. Config error.");
	if (size <= MAX_PACKET_DATA_SIZE) {
		rx_data.channel_id = IPC_CPUSYS_CHANNEL_ID;
		rx_data.size	   = size;
		if (data) {
			memcpy(rx_data.data, (uint8_t *)data, size);
			nrfs_dispatcher_notify(&rx_data.data, rx_data.size);
		} else {
			nrfs_backend_error_handler(NRFS_ERROR_NO_DATA_RECEIVED, 0, false);
		}
	} else {
		nrfs_backend_error_handler(NRFS_ERROR_EPT_RECEIVE_DATA_TOO_LONG, 0, true);
	}
}

static void nrfs_backend_send_work(struct k_work *item)
{
	LOG_DBG("Sending data from workqueue");
	static ipc_data_packet_t data_to_send;

	while (k_msgq_get(&ipc_transmit_msgq, &data_to_send, K_NO_WAIT) == 0) {
		ipc_service_send(&ipc_cpusys_channel_config.ipc_ept, &tx_data.data, tx_data.size);
	}
}

/**
 *  @brief Initialize ipc channel
 *
 *  @param ipc_channel_config pointer to ipc channel config structure
 *
 *  @return -EINVAL when instance configuration is invalid.
 *  @return -EIO when no backend is registered.
 *  @return -EALREADY when the instance is already opened (or being opened).
 *  @return -EBUSY when the instance is busy.
 *  @return 0 on suscess
 */
static int ipc_channel_init(void)
{
	int ret = 0;

	k_work_init(&backend_send_work, nrfs_backend_send_work);

	struct ipc_channel_config_t *ipc_channel_config = &ipc_cpusys_channel_config;

	ret = ipc_service_open_instance(ipc_channel_config->ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_OPEN_INSTANCE, ret, false);
		return ret;
	}

	LOG_INF("ipc_service_open_instance() done.");

	ret = ipc_service_register_endpoint(ipc_channel_config->ipc_instance,
					    &ipc_channel_config->ipc_ept,
					    ipc_channel_config->endpoint_config);
	if (ret < 0) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_REGISTER_ENDPOINT, ret, false);
		return ret;
	}

	LOG_INF("ipc_service_register_endpoint() done.");

	return ret;
}

nrfs_err_t nrfs_backend_send(void *message, size_t size)
{
	return nrfs_backend_send_ex(message, size, K_NO_WAIT, false);
}

nrfs_err_t nrfs_backend_send_ex(void *message, size_t size, k_timeout_t timeout, bool high_prio)
{
	if (atomic_get(&ipc_cpusys_channel_config.status) != connected) {
		LOG_WRN("Backend not yet connected to sysctrl");
		return NRFS_ERR_INVALID_STATE;
	}

	if (size < MAX_PACKET_DATA_SIZE) {
		tx_data.channel_id = IPC_CPUSYS_CHANNEL_ID;
		tx_data.size	   = size;
		memcpy(tx_data.data, (uint8_t *)message, size);

		int err = k_msgq_put(&ipc_transmit_msgq, &tx_data, timeout);

		if (err) {
			return NRFS_ERR_IPC;
		}

		err = k_work_submit(&backend_send_work);

		return err >= 0 ? 0 : NRFS_ERR_IPC;
	}

	LOG_ERR("Trying to send %d bytes where max is %d.", size, MAX_PACKET_DATA_SIZE);
	return NRFS_ERR_IPC;
}

bool nrfs_backend_connected(void)
{
	return atomic_get(&ipc_cpusys_channel_config.status) == connected;
}

int nrfs_backend_wait_for_connection(k_timeout_t timeout)
{
	uint32_t events;

	if (nrfs_backend_connected()) {
		return 0;
	}

	events = k_event_wait(&ipc_connected_event, IPC_INIT_DONE_EVENT, false, timeout);

	return (events == IPC_INIT_DONE_EVENT ? 0 : (-EAGAIN));
}

__weak void nrfs_backend_fatal_error_handler(enum nrfs_backend_error error_id)
{
	LOG_ERR("Fatal error: %d rebooting...", error_id);
	sys_reboot(SYS_REBOOT_WARM);
}

SYS_INIT(ipc_channel_init, POST_KERNEL, CONFIG_NRFS_BACKEND_IPC_SERVICE_LITE_INIT_PRIO);
