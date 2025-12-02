/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_H_
#define ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_H_

#include <zephyr/kernel.h>

#include <openthread/instance.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The common callback type for receiving IPv4 (translated by NAT64) and IPv6 datagrams.
 *
 * This callback is called when a datagram is received.
 *
 * @param message The message to receive.
 * @param context The context to pass to the callback.
 */
typedef void (*openthread_receive_cb)(struct otMessage *message, void *context);

/** OpenThread state change callback  */

/**
 * @brief OpenThread state change callback structure
 *
 * Used to register a callback in the callback list. As many
 * callbacks as needed can be added as long as each of them
 * are unique pointers of struct openthread_state_changed_callback.
 *
 * @note You may destroy the object only after it is unregistered from the callback list.
 */
struct openthread_state_changed_callback {
	/**
	 * @brief Callback for notifying configuration or state changes.
	 *
	 * @param otCallback OpenThread callback to register.
	 * See https://openthread.io/reference/group/api-instance#otstatechangedcallback for
	 * details.
	 */
	otStateChangedCallback otCallback;

	/** User data if required */
	void *user_data;

	/**
	 * Internally used field for list handling
	 *  - user must not directly modify
	 */
	sys_snode_t node;
};

/**
 * @brief Register callbacks that will be called when a certain configuration
 * or state changes occur within OpenThread.
 *
 * @param cb Callback struct to register.
 */
int openthread_state_changed_callback_register(struct openthread_state_changed_callback *cb);

/**
 * @brief Unregister OpenThread configuration or state changed callbacks.
 *
 * @param cb Callback struct to unregister.
 */
int openthread_state_changed_callback_unregister(struct openthread_state_changed_callback *cb);

/**
 * @brief Get OpenThread thread identification.
 */
k_tid_t openthread_thread_id_get(void);

/**
 * @brief Get pointer to default OpenThread instance.
 *
 * @retval !NULL On success.
 * @retval NULL  On failure.
 */
struct otInstance *openthread_get_default_instance(void);

/**
 * @brief Initialize the OpenThread module.
 *
 * This function:
 * - Initializes the OpenThread module.
 * - Creates an OpenThread single instance.
 * - Starts the shell.
 * - Enables the UART and NCP HDLC for coprocessor purposes.
 * - Initializes the NAT64 translator.
 * - Creates a work queue for the OpenThread module.
 *
 * @note This function is automatically called by Zephyr's networking layer.
 * If you want to initialize the OpenThread independently, call this function
 * in your application init code.
 *
 * @retval 0 On success.
 * @retval -EIO On failure.
 */
int openthread_init(void);

/**
 * @brief Run the OpenThread network.
 *
 * @details Prepares the OpenThread network and enables it.
 * Depends on active settings: it uses the stored network configuration,
 * starts the joining procedure or uses the default network configuration.
 * Additionally, when the device is MTD, it sets the SED mode to properly
 * attach the network.
 */
int openthread_run(void);

/**
 * @brief Disable the OpenThread network.
 */
int openthread_stop(void);

/**
 * @brief Set the additional callback for receiving packets.
 *
 * @details This callback is called once a packet is received and can be
 * used to inject packets into the Zephyr networking stack.
 * Setting this callback is optional.
 *
 * @param cb Callback to set.
 * @param context Context to pass to the callback.
 */
void openthread_set_receive_cb(openthread_receive_cb cb, void *context);

/**
 * @brief Lock internal mutex before accessing OpenThread API.
 *
 * @details OpenThread API is not thread-safe. Therefore, before accessing any
 * API function, you need to lock the internal mutex, to prevent the
 * OpenThread thread from pre-empting the API call.
 */
void openthread_mutex_lock(void);

/**
 * @brief Try to lock internal mutex before accessing OpenThread API.
 *
 * @details This function behaves like openthread_mutex_lock(), provided that
 * the internal mutex is unlocked. Otherwise, it returns a negative value without
 * waiting.
 */
int openthread_mutex_try_lock(void);

/**
 * @brief Unlock internal mutex after accessing OpenThread API.
 */
void openthread_mutex_unlock(void);

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
/**
 * @brief Notify OpenThread task about Border Router pending work.
 */
void openthread_notify_border_router_work(void);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_H_ */
