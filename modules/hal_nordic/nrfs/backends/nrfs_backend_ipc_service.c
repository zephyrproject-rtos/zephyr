/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrfs_backend_ipc_service.h"

#include <internal/nrfs_backend.h>
#include <internal/nrfs_dispatcher.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(NRFS_BACKEND, CONFIG_NRFS_BACKEND_LOG_LEVEL);

#define MAX_PACKET_DATA_SIZE (CONFIG_NRFS_MAX_BACKEND_PACKET_SIZE)

K_MSGQ_DEFINE(ipc_transmit_msgq, sizeof(struct ipc_data_packet),
							CONFIG_NRFS_BACKEND_TX_MSG_QUEUE_SIZE, 4);

static struct k_work backend_send_work;

static void ipc_sysctrl_ept_bound(void *priv);
static void ipc_sysctrl_ept_recv(const void *data, size_t size, void *priv);

static K_EVENT_DEFINE(ipc_connected_event);

#define IPC_INIT_DONE_EVENT (0x01)

struct ipc_channel_config {
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

static struct ipc_channel_config ipc_cpusys_channel_config = {
	.ipc_instance	 = DEVICE_DT_GET(DT_ALIAS(ipc_to_cpusys)),
	.endpoint_config = &ipc_sysctrl_ept_cfg,
	.status		 = ATOMIC_INIT(NOT_CONNECTED),
	.enabled	 = true
};

static sys_slist_t nrfs_backend_info_cb_slist = SYS_SLIST_STATIC_INIT(&nrfs_backend_info_cb_slist);

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
	LOG_DBG("Bound to sysctrl.");
	k_event_post(&ipc_connected_event, IPC_INIT_DONE_EVENT);
	atomic_set(&ipc_cpusys_channel_config.status, CONNECTED);

	if (k_msgq_num_used_get(&ipc_transmit_msgq) > 0) {
		k_work_submit(&backend_send_work);
	}

	struct nrfs_backend_bound_info_subs *subs;

	SYS_SLIST_FOR_EACH_CONTAINER(&nrfs_backend_info_cb_slist, subs, node) {
		subs->cb();
	}
}

static void ipc_sysctrl_ept_recv(const void *data, size_t size, void *priv)
{
	struct ipc_data_packet rx_data;

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
	struct ipc_data_packet data_to_send;

	LOG_DBG("Sending data from workqueue");
	while (k_msgq_get(&ipc_transmit_msgq, &data_to_send, K_NO_WAIT) == 0) {
		ipc_service_send(&ipc_cpusys_channel_config.ipc_ept, &data_to_send.data,
				 data_to_send.size);
	}
}

/**
 *  @brief Initialize ipc channel
 *
 *  @return -EINVAL when instance configuration is invalid.
 *  @return -EIO when no backend is registered.
 *  @return -EALREADY when the instance is already opened (or being opened).
 *  @return -EBUSY when the instance is busy.
 *  @return 0 on success
 */
static int ipc_channel_init(void)
{
	struct ipc_channel_config *ch_cfg;
	int ret = 0;

	k_work_init(&backend_send_work, nrfs_backend_send_work);
	ch_cfg = &ipc_cpusys_channel_config;

	ret = ipc_service_open_instance(ch_cfg->ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_OPEN_INSTANCE, ret, false);
		return ret;
	}

	LOG_DBG("ipc_service_open_instance() done.");

	ret = ipc_service_register_endpoint(ch_cfg->ipc_instance,
					    &ch_cfg->ipc_ept,
					    ch_cfg->endpoint_config);
	if (ret < 0) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_REGISTER_ENDPOINT, ret, false);
		return ret;
	}

	LOG_DBG("ipc_service_register_endpoint() done.");

	return ret;
}

nrfs_err_t nrfs_backend_send(void *message, size_t size)
{
	return nrfs_backend_send_ex(message, size, K_NO_WAIT, false);
}

nrfs_err_t nrfs_backend_send_ex(void *message, size_t size, k_timeout_t timeout, bool high_prio)
{
	if (!k_is_in_isr() && nrfs_backend_connected()) {
		return ipc_service_send(&ipc_cpusys_channel_config.ipc_ept, message, size) ?
			NRFS_SUCCESS : NRFS_ERR_IPC;
	} else if (size <= MAX_PACKET_DATA_SIZE) {
		int err;
		struct ipc_data_packet tx_data;

		tx_data.channel_id = IPC_CPUSYS_CHANNEL_ID;
		tx_data.size	   = size;
		memcpy(tx_data.data, (uint8_t *)message, size);

		err = k_msgq_put(&ipc_transmit_msgq, &tx_data, timeout);
		if (err) {
			return NRFS_ERR_IPC;
		}

		if (nrfs_backend_connected()) {
			err = k_work_submit(&backend_send_work);
		}

		return err >= 0 ? 0 : NRFS_ERR_IPC;
	}

	LOG_ERR("Trying to send %d bytes where max is %d.", size, MAX_PACKET_DATA_SIZE);

	return NRFS_ERR_IPC;
}

bool nrfs_backend_connected(void)
{
	return atomic_get(&ipc_cpusys_channel_config.status) == CONNECTED;
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

void nrfs_backend_register_bound_subscribe(struct nrfs_backend_bound_info_subs *subs,
					   nrfs_backend_bound_info_cb_t cb)
{
	if (cb) {
		subs->cb = cb;
		sys_slist_append(&nrfs_backend_info_cb_slist, &subs->node);
	}
}

__weak void nrfs_backend_fatal_error_handler(enum nrfs_backend_error error_id)
{
	LOG_ERR("Fatal error: %d rebooting...", error_id);
	sys_reboot(SYS_REBOOT_WARM);
}

SYS_INIT(ipc_channel_init, POST_KERNEL, CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO);
