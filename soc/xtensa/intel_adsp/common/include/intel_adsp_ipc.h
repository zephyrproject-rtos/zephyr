/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H
#define ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H

#include <intel_adsp_ipc_devtree.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

struct intel_adsp_ipc_config {
	volatile struct intel_adsp_ipc *regs;
};

/**
 * @brief Intel ADSP IPC Message Handler Callback.
 *
 * This function, once registered via intel_adsp_ipc_set_message_handler(),
 * is invoked in interrupt context to service messages sent from the
 * foreign/connected IPC context. The message contents of the TDR and
 * TDD registers are provided in the data/ext_data argument.
 *
 * The function should return true if processing of the message is
 * complete and return notification to the other side (via the TDA
 * register) is desired immediately. Returning false means that no
 * return "DONE" interrupt will occur until intel_adsp_ipc_complete() is
 * called on this device at some point in the future.
 *
 * @note Further messages on the link will not be transmitted or
 * received while an in-progress message remains incomplete!
 *
 * @param dev IPC device.
 * @param arg Registered argument from intel_adsp_ipc_set_message_handler().
 * @param data Message data from other side (low bits of TDR register).
 * @param ext_dat Extended message data (TDD register).
 * @return true if the message is completely handled.
 */
typedef bool (*intel_adsp_ipc_handler_t)(const struct device *dev, void *arg,
	uint32_t data, uint32_t ext_data);

/**
 * @brief Intel ADSP IPC Message Complete Callback.
 *
 * This function, once registered via intel_adsp_ipc_set_done_handler(), is
 * invoked in interrupt context when a "DONE" return interrupt is
 * received from the other side of the connection (indicating that a
 * previously sent message is finished processing).
 *
 * @note On Intel ADSP hardware the DONE interrupt is transmitted
 * synchronously with the interrupt being cleared on the remote
 * device. It is not possible to delay processing. This callback
 * will still occur, but protocols which rely on notification of
 * asynchronous command processing will need modification.
 *
 * @param dev IPC device.
 * @param arg Registered argument from intel_adsp_ipc_set_done_handler().
 */
typedef void (*intel_adsp_ipc_done_t)(const struct device *dev, void *arg);

struct intel_adsp_ipc_data {
	struct k_sem sem;
	struct k_spinlock lock;
	intel_adsp_ipc_handler_t handle_message;
	void *handler_arg;
	intel_adsp_ipc_done_t done_notify;
	void *done_arg;
};

void z_intel_adsp_ipc_isr(const void *devarg);

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

/** @brief Initialize Intel ADSP IPC device.
 *
 * Initialize the device. Must be called before any API calls or
 * interrupts can be serviced.
 *
 * @param dev IPC device.
 * @return Zero on success, negative codes for error.
 */
int intel_adsp_ipc_init(const struct device *dev);

/** @brief Complete an in-progress message.
 *
 * Notify the other side that the current in-progress message is
 * complete. This is a noop if no message is in progress.
 *
 * @note Further messages on the link will not be transmitted or
 * received while an in-progress message remains incomplete!
 *
 * @param dev IPC device.
 */
void intel_adsp_ipc_complete(const struct device *dev);

/** @brief Message-in-progress predicate.
 *
 * Returns false if a message has been received but not yet completed
 * via intel_adsp_ipc_complete(), true otherwise.
 *
 * @param dev IPC device.
 * @return True if no message is in progress.
 */
bool intel_adsp_ipc_is_complete(const struct device *dev);

/** @brief Send an IPC message.
 *
 * Sends a message to the other side of an IPC link. The data and
 * ext_data parameters are passed using the IDR/IDD registers.
 * Returns true if the message was sent, false if a current message is
 * in progress (in the sense of intel_adsp_ipc_is_complete()).
 *
 * @param dev IPC device.
 * @param data 30 bits value to transmit with the message (IDR register).
 * @param ext_data Extended value to transmit with the message (IDD register).
 * @return message successfully transmitted.
 */
bool intel_adsp_ipc_send_message(const struct device *dev,
	uint32_t data, uint32_t ext_data);

/** @brief Send an IPC message, block until completion.
 *
 * As for intel_adsp_ipc_send_message(), but blocks the current thread until
 * the completion of the message or the expiration of the provided
 * timeout.
 *
 * @param dev IPC device
 * @param data 30 bits value to transmit with the message (IDR register)
 * @param ext_data Extended value to transmit with the message (IDD register)
 * @param timeout Maximum time to wait, or K_FOREVER, or K_NO_WAIT
 * @return message successfully transmitted
 */
bool intel_adsp_ipc_send_message_sync(const struct device *dev,
	uint32_t data, uint32_t ext_data, k_timeout_t timeout);

#endif /* ZEPHYR_INCLUDE_INTEL_ADSP_IPC_H */
