/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H
#define ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H

#include <intel_adsp_ipc_devtree.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>

#include <zephyr/ipc/backends/intel_adsp_host_ipc.h>

#if defined(CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE) || defined(__DOXYGEN__)

/**
 * @brief Register message callback handler.
 *
 * This function registers a handler function for received messages.
 *
 * @param dev IPC device.
 * @param fn Callback function.
 * @param arg Value to pass as the "arg" parameter to the function.
 */
void intel_adsp_ipc_set_message_handler(const struct device *dev,
	intel_adsp_ipc_handler_t fn, void *arg);

/**
 * @brief Register done callback handler.
 *
 * This function registers a handler function for message completion
 * notifications.
 *
 * @param dev IPC device.
 * @param fn Callback function.
 * @param arg Value to pass as the "arg" parameter to the function.
 */
void intel_adsp_ipc_set_done_handler(const struct device *dev,
	intel_adsp_ipc_done_t fn, void *arg);

/**
 * @brief Complete an in-progress message.
 *
 * Notify the other side that the current in-progress message is
 * complete. This is a noop if no message is in progress.
 *
 * @note Further messages on the link will not be transmitted or
 *       received while an in-progress message remains incomplete!
 *
 * @param dev IPC device.
 */
void intel_adsp_ipc_complete(const struct device *dev);

/**
 * @brief Message-in-progress predicate.
 *
 * Returns false if a message has been received but not yet completed
 * via intel_adsp_ipc_complete(), true otherwise.
 *
 * @param dev IPC device.
 *
 * @return True if no message is in progress.
 */
bool intel_adsp_ipc_is_complete(const struct device *dev);

/** @brief Send an IPC message.
 *
 * Sends a message to the other side of an IPC link. The data and
 * ext_data parameters are passed using the IDR/IDD registers.
 * Returns 0 if the message was sent, negative error values:
 * -EBUSY if there is already IPC message processed (intel_adsp_ipc_is_complete returns false).
 * -ESHUTDOWN if IPC device will not send the message as it undergoes power
 * transition.
 *
 * @param dev IPC device.
 * @param data 30 bits value to transmit with the message (IDR register).
 * @param ext_data Extended value to transmit with the message (IDD register).
 * @return message successfully transmitted.
 */
int intel_adsp_ipc_send_message(const struct device *dev,
	uint32_t data, uint32_t ext_data);

/** @brief Send an IPC message, block until completion.
 *
 * As for intel_adsp_ipc_send_message(), but blocks the current thread until
 * the completion of the message or the expiration of the provided
 * timeout. Returns immediately if IPC device is during power transition.
 *
 * @param dev IPC device
 * @param data 30 bits value to transmit with the message (IDR register)
 * @param ext_data Extended value to transmit with the message (IDD register)
 * @param timeout Maximum time to wait, or K_FOREVER, or K_NO_WAIT
 * @return returns 0 if message successfully transmited, otherwise error code.
 */
int intel_adsp_ipc_send_message_sync(const struct device *dev,
	uint32_t data, uint32_t ext_data, k_timeout_t timeout);


/** @brief Send an emergency IPC message.
 *
 * Sends a message to the other side of an IPC link. The data and ext_data parameters are passed
 * using the IDR/IDD registers. Waits in a loop until it is possible to send a message.
 *
 * @param dev IPC device.
 * @param data 30 bits value to transmit with the message (IDR register).
 * @param ext_data Extended value to transmit with the message (IDD register).
 */
void intel_adsp_ipc_send_message_emergency(const struct device *dev, uint32_t data,
					   uint32_t ext_data);

#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

#endif /* ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H */
