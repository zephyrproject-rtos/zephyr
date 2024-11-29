/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_BACKEND_IPC_SERVICE_H
#define NRFS_BACKEND_IPC_SERVICE_H

#include <stdint.h>
#include <nrfs_common.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __packed ipc_data_packet {
	uint16_t channel_id;
	uint16_t size;
	uint8_t data[CONFIG_NRFS_MAX_BACKEND_PACKET_SIZE];
};

enum ipc_channel_status {
	NOT_CONNECTED = 0,
	CONNECTED     = 1
};

enum nrfs_backend_error {
	NRFS_ERROR_EPT_RECEIVE_QUEUE_ERROR = 0,
	NRFS_ERROR_EPT_RECEIVE_DATA_TOO_LONG,
	NRFS_ERROR_IPC_CHANNEL_INIT,
	NRFS_ERROR_SEND_DATA,
	NRFS_ERROR_NO_DATA_RECEIVED,
	NRFS_ERROR_IPC_OPEN_INSTANCE,
	NRFS_ERROR_IPC_REGISTER_ENDPOINT,
	NRFS_ERROR_BACKEND_NOT_CONNECTED,
	NRFS_ERROR_COUNT
};

#define IPC_CPUSYS_CHANNEL_ID 0x5C

/**
 * @brief function to check if backend is connected to sysctrl
 *
 * @return true Backend connected.
 * @return false Backend not connected.
 */
bool nrfs_backend_connected(void);

/**
 * @brief this function will block until connection or timeout expires
 *
 * @param timeout
 *
 * @return 0 Connection done.
 * @return -EAGAIN Waiting period timed out.
 */
int nrfs_backend_wait_for_connection(k_timeout_t timeout);

/**
 * @brief Extended function for sending a message over the chosen transport backend.
 *
 * This function is used by services to send requests to the System Controller.
 *
 * @param[in] message   Pointer to the message payload.
 * @param[in] size      Message payload size.
 * @param[in] timeout   Non-negative waiting period to add the message,
 *                      or one of the special values K_NO_WAIT and K_FOREVER.
 * @param[in] high_prio True if message should be sent with higher priority.
 *
 * @retval NRFS_SUCCESS Message sent successfully.
 * @retval NRFS_ERR_IPC Backend returned error during message sending.
 */
nrfs_err_t nrfs_backend_send_ex(void *message, size_t size, k_timeout_t timeout, bool high_prio);

/**
 * @brief Fatal error handler for unrecoverable errors
 *
 * This is weak function so it can be overridden if needed.
 * Error is considered fatal when there is no option to send message to sysctrl
 * even after retry. Communication with sysctrl is crucial for system to work properly.
 *
 * @param error_id parameter to identify error.
 */
void nrfs_backend_fatal_error_handler(enum nrfs_backend_error error_id);

typedef void (*nrfs_backend_bound_info_cb_t)(void);
struct nrfs_backend_bound_info_subs {
	sys_snode_t node;
	nrfs_backend_bound_info_cb_t cb;
};
/**
 * @brief Register callback function to notify when nrfs is connected.
 * There can be multiple callbacks registered.
 *
 * @param subs Subcription instance.
 * @param cb Callback
 */
void nrfs_backend_register_bound_subscribe(struct nrfs_backend_bound_info_subs *subs,
				    nrfs_backend_bound_info_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_BACKEND_IPC_SERVICE_H */
