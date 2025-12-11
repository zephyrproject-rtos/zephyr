/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_BACKEND_INTEL_ADSP_IPC_H
#define ZEPHYR_INCLUDE_IPC_BACKEND_INTEL_ADSP_IPC_H

#include <intel_adsp_ipc_devtree.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>

#include <zephyr/ipc/ipc_service_backend.h>

/** Enum on IPC send length argument to indicate IPC message type. */
enum intel_adsp_send_len {
	/** Normal IPC message. */
	INTEL_ADSP_IPC_SEND_MSG,

	/** Synchronous IPC message. */
	INTEL_ADSP_IPC_SEND_MSG_SYNC,

	/** Emergency IPC message. */
	INTEL_ADSP_IPC_SEND_MSG_EMERGENCY,

	/** Send a DONE message. */
	INTEL_ADSP_IPC_SEND_DONE,

	/** Query backend to see if IPC is complete. */
	INTEL_ADSP_IPC_SEND_IS_COMPLETE,
};

/** Enum on callback return values. */
enum intel_adsp_cb_ret {
	/** Callback return to indicate no issue. Must be 0. */
	INTEL_ADSP_IPC_CB_RET_OKAY = 0,

	/** Callback return to signal needing external completion. */
	INTEL_ADSP_IPC_CB_RET_EXT_COMPLETE,
};

/** Enum on callback length argument to indicate which triggers the callback. */
enum intel_adsp_cb_len {
	/** Callback length to indicate this is an IPC message. */
	INTEL_ADSP_IPC_CB_MSG,

	/** Callback length to indicate this is a DONE message. */
	INTEL_ADSP_IPC_CB_DONE,
};

/** Struct for IPC message descriptor. */
struct intel_adsp_ipc_msg {
	/** Header specific to platform. */
	uint32_t data;

	/** Extension specific to platform. */
	uint32_t ext_data;

	/** Timeout for sending synchronuous message. */
	k_timeout_t timeout;
};

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE

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
typedef bool (*intel_adsp_ipc_handler_t)(const struct device *dev, void *arg, uint32_t data,
					 uint32_t ext_data);

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
 * @return True if IPC completion will be done externally, otherwise false.
 * @note Returning True will cause this API to skip writing IPC registers
 * signalling IPC message completion and those actions should be done by
 * external code manually. Returning false from the handler will perform
 * writing to IPC registers signalling message completion normally by this API.
 */
typedef bool (*intel_adsp_ipc_done_t)(const struct device *dev, void *arg);

#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

#ifdef CONFIG_PM_DEVICE
typedef int (*intel_adsp_ipc_resume_handler_t)(const struct device *dev, void *arg);

typedef int (*intel_adsp_ipc_suspend_handler_t)(const struct device *dev, void *arg);
#endif /* CONFIG_PM_DEVICE */

/**
 * Intel Audio DSP IPC service backend config struct.
 */
struct intel_adsp_ipc_config {
	/** Pointer to hardware register block. */
	volatile struct intel_adsp_ipc *regs;
};

/**
 * Intel Audio DSP IPC service backend data struct.
 */
struct intel_adsp_ipc_data {
	/** Semaphore used to wait for remote acknowledgment of sent message. */
	struct k_sem sem;

	/** General driver lock. */
	struct k_spinlock lock;

	/** Pending TX acknowlegement. */
	bool tx_ack_pending;

	/** Pointer to endpoint configuration. */
	const struct ipc_ept_cfg *ept_cfg;

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	/** Callback for message handler. */
	intel_adsp_ipc_handler_t handle_message;

	/** Argument for message handler callback. */
	void *handler_arg;

	/** Callback for done notification. */
	intel_adsp_ipc_done_t done_notify;

	/** Argument for done notification callback. */
	void *done_arg;
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

#ifdef CONFIG_PM_DEVICE
	/** Pointer to resume handler. */
	intel_adsp_ipc_resume_handler_t resume_fn;

	/** Argument for resume handler. */
	void *resume_fn_args;

	/** Pointer to suspend handler. */
	intel_adsp_ipc_suspend_handler_t suspend_fn;

	/** Argument for suspend handler. */
	void *suspend_fn_args;
#endif /* CONFIG_PM_DEVICE */
};

/**
 * Endpoint private data struct.
 */
struct intel_adsp_ipc_ept_priv_data {
	/** Callback return value (enum intel_adsp_cb_ret). */
	int cb_ret;

	/** Pointer to additional private data. */
	void *priv;
};

#ifdef CONFIG_PM_DEVICE

/**
 * @brief Registers resume callback handler used to resume Device from suspended state.
 *
 * @param dev IPC device.
 * @param fn Callback function.
 * @param arg Value to pass as the "arg" parameter to the function.
 */
void intel_adsp_ipc_set_resume_handler(const struct device *dev, intel_adsp_ipc_resume_handler_t fn,
				       void *arg);

/**
 * @brief Registers suspend callback handler used to suspend active Device.
 *
 * @param dev IPC device.
 * @param fn Callback function.
 * @param arg Value to pass as the "arg" parameter to the function.
 */
void intel_adsp_ipc_set_suspend_handler(const struct device *dev,
					intel_adsp_ipc_suspend_handler_t fn, void *arg);

#endif /* CONFIG_PM_DEVICE */

#endif /* ZEPHYR_INCLUDE_IPC_BACKEND_INTEL_ADSP_IPC_H */
