/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

#define TX_TASK_PRIORITY (CONFIG_NRFS_BACKEND_TX_TASK_PRIORITY)
#define TX_TASK_STACK_SIZE (CONFIG_NRFS_BACKEND_TX_TASK_STACK_SIZE)
#define RX_TASK_PRIORITY (CONFIG_NRFS_BACKEND_RX_TASK_PRIORITY)
#define RX_TASK_STACK_SIZE (CONFIG_NRFS_BACKEND_RX_TASK_STACK_SIZE)
#define MAX_PACKET_DATA_SIZE (CONFIG_NRFS_MAX_BACKEND_PACKET_SIZE)
#define MAX_SEND_RETRIES (CONFIG_NRFS_BACKEND_TX_MAX_RETRIES)

#define START_DELAY (0)

K_MSGQ_DEFINE(ipc_receive_msgq, sizeof(ipc_data_packet_t),
		CONFIG_NRFS_BACKEND_RX_MSG_QUEUE_SIZE, 4);
K_MSGQ_DEFINE(ipc_transmit_msgq,
	      sizeof(ipc_data_packet_t),
	      CONFIG_NRFS_BACKEND_TX_MSG_QUEUE_SIZE,
	      4);

static ipc_data_packet_t rx_data;
static ipc_data_packet_t tx_data;

static K_SEM_DEFINE(ipc_init_done_sem, 0, 1);
static K_SEM_DEFINE(ipc_sysctrl_bound_sem, 0, 1);
static K_EVENT_DEFINE(ipc_connected_event);

#define IPC_INIT_DONE_EVENT (0x01)

struct ipc_channel_config_t {
	const struct device *ipc_instance;
	struct ipc_ept_cfg *endpoint_config;
	struct k_sem *bound_sem;
	struct ipc_ept ipc_ept;
	atomic_t status;
	bool enabled;
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
	case NRFS_ERROR_EPT_RECEIVE_QUEUE_ERROR:
		LOG_ERR("Receive queue error: %d", error);
		break;

	case NRFS_ERROR_EPT_RECEIVE_DATA_TOO_LONG:
		LOG_ERR("Received data is too long. Config error.");
		break;

	case NRFS_ERROR_IPC_CHANNEL_INIT:
		LOG_ERR("IPC channel init failed with error %d", error);
		break;

	case NRFS_ERROR_SEND_DATA:
		LOG_ERR("Send message to sysctrl failed with error: %d", error);
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

	case NRFS_ERROR_BACKEND_NOT_CONNECTED:
		LOG_ERR("Backend not connected to sysctrl.");
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
	k_sem_give(&ipc_sysctrl_bound_sem);
}

static void ipc_sysctrl_ept_recv(const void *data, size_t size, void *priv)
{
	__ASSERT(size <= MAX_PACKET_DATA_SIZE, "Received data is too long. Config error.");
	if (size <= MAX_PACKET_DATA_SIZE) {
		rx_data.channel_id = IPC_CPUSYS_CHANNEL_ID;
		rx_data.size	   = size;
		if (data) {
			memcpy(rx_data.data, (uint8_t *)data, size);
			int status = k_msgq_put(&ipc_receive_msgq, &rx_data,
						K_USEC(CONFIG_NRFS_BACKEND_RX_QUEUE_MAX_WAIT_US));

			if (status != 0) {
				nrfs_backend_error_handler(NRFS_ERROR_EPT_RECEIVE_QUEUE_ERROR,
							   status, true);
			}

		} else {
			nrfs_backend_error_handler(NRFS_ERROR_NO_DATA_RECEIVED, 0, false);
		}
	} else {
		nrfs_backend_error_handler(NRFS_ERROR_EPT_RECEIVE_DATA_TOO_LONG, 0, true);
	}
}

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
	.bound_sem	 = &ipc_sysctrl_bound_sem,
	.status		 = ATOMIC_INIT(not_connected),
	.enabled	 = true
};

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
static int ipc_channel_init(struct ipc_channel_config_t *ipc_channel_config)
{
	int ret = 0;

	ret = ipc_service_open_instance(ipc_channel_config->ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_OPEN_INSTANCE, ret, false);
		return ret;
	}

	LOG_DBG("ipc_service_open_instance() done.");

	ret = ipc_service_register_endpoint(ipc_channel_config->ipc_instance,
					    &ipc_channel_config->ipc_ept,
					    ipc_channel_config->endpoint_config);
	if (ret < 0) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_REGISTER_ENDPOINT, ret, false);
		return ret;
	}

	LOG_DBG("ipc_service_register_endpoint() done.");

	ret = k_sem_take(ipc_channel_config->bound_sem, K_FOREVER);

	return ret;
}

/**
 *  @brief Zephyr task to handle backend initialization and sending messages
 *
 *  @param dummy0 1st entry point parameter
 *  @param dummy1 2nd entry point parameter
 *  @param dummy2 3rd entry point parameter
 */
static void backend_tx_task(void *dummy0, void *dummy1, void *dummy2)
{
	int ret;
	bool retry	    = false;
	uint8_t retry_count = 0;
	ipc_data_packet_t data_to_send;

	LOG_DBG("Init backend");

	ret = ipc_channel_init(&ipc_cpusys_channel_config);

	LOG_DBG("Init IPC done with code %d", ret);

	if (ret < 0) {
		nrfs_backend_error_handler(NRFS_ERROR_IPC_CHANNEL_INIT, ret, true);
	} else {
		atomic_set(&ipc_cpusys_channel_config.status, connected);
	}

	k_sem_give(&ipc_init_done_sem);

	LOG_DBG("Backend initialized");

	while (1) {
		k_msgq_get(&ipc_transmit_msgq, &data_to_send, K_FOREVER);
		retry	    = false;
		retry_count = 0;

		do {
			if (retry_count >= MAX_SEND_RETRIES) {
				nrfs_backend_error_handler(NRFS_ERROR_SEND_DATA, retry_count, true);
				break;
			}

			ret = ipc_service_send(&ipc_cpusys_channel_config.ipc_ept,
					       &data_to_send.data, data_to_send.size);

			if (ret == -ENOMEM) {
				/* No space in the buffer. Retry. */
				retry = true;
				retry_count++;
				k_sleep(K_USEC(CONFIG_NRFS_BACKEND_TX_RETRY_DELAY_US));
			} else if (ret < 0) {
				nrfs_backend_error_handler(NRFS_ERROR_SEND_DATA, retry_count, true);
				break;
			}

		} while (retry);
	}
}
K_THREAD_DEFINE(backend_tx_task_id,
		TX_TASK_STACK_SIZE,
		backend_tx_task,
		NULL,
		NULL,
		NULL,
		TX_TASK_PRIORITY,
		0,
		START_DELAY);

/**
 *  @brief Zephyr task to handle incoming backend messages
 *
 *  @param dummy0 1st entry point parameter
 *  @param dummy1 2nd entry point parameter
 *  @param dummy2 3rd entry point parameter
 */
static void backend_rx_task(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	ipc_data_packet_t received_data;

	LOG_DBG("Backend receive task started. Waiting for IPC connection.");

	/* Wait for ipc initialization */
	k_sem_take(&ipc_init_done_sem, K_FOREVER);
	k_event_post(&ipc_connected_event, IPC_INIT_DONE_EVENT);

	LOG_DBG("IPC connected. Backend receive task waiting for data.");

	while (1) {
		k_msgq_get(&ipc_receive_msgq, &received_data, K_FOREVER);
		LOG_DBG("Backend data received receive task waiting for data.");
		nrfs_dispatcher_notify(&received_data.data, received_data.size);
	}
}
K_THREAD_DEFINE(backend_rx_task_id,
		RX_TASK_STACK_SIZE,
		backend_rx_task,
		NULL,
		NULL,
		NULL,
		RX_TASK_PRIORITY,
		0,
		0);

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
		return err == 0 ? 0 : NRFS_ERR_IPC;
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
