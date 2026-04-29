/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 authenticator public API.
 */

#ifndef ZEPHYR_INCLUDE_FIDO2_FIDO2_H_
#define ZEPHYR_INCLUDE_FIDO2_FIDO2_H_

#include <zephyr/fido2/fido2_types.h>

/**
 * @brief FIDO2 authenticator subsystem
 * @defgroup fido2 FIDO2
 * @since 4.4
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Runtime states exposed by the FIDO2 subsystem. */
enum fido2_runtime_state {
	/** FIDO2 is stopped and not handling commands. */
	FIDO2_RUNTIME_STATE_STOPPED = 0,
	/** FIDO2 is running and idle. */
	FIDO2_RUNTIME_STATE_IDLE,
	/** FIDO2 is waiting for user presence confirmation. */
	FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE,
	/** FIDO2 is processing a command after user presence. */
	FIDO2_RUNTIME_STATE_PROCESSING,
};

/**
 * @brief FIDO2 runtime state callback.
 *
 * Called when the FIDO2 runtime state changes.
 *
 * @note This callback is called synchronously. Runtime state transitions are
 *       reported from FIDO2 internal thread/workqueue context.
 * @note The callback must be lightweight and must not block for long periods.
 *
 * @param state     New runtime state.
 * @param user_data Opaque context pointer provided during callback setup.
 */
typedef void (*fido2_state_callback_t)(enum fido2_runtime_state state, void *user_data);

/**
 * @brief Initialize the FIDO2 subsystem.
 *
 * Sets up credential storage, crypto, and registered transports.
 *
 * @retval 0 If successful.
 * @retval -errno On failure.
 */
int fido2_init(void);

/**
 * @brief Start the FIDO2 authenticator.
 *
 * Begins listening for CTAP2 commands on all enabled transports.
 *
 * @retval 0 If successful.
 * @retval -errno On failure.
 */
int fido2_start(void);

/**
 * @brief Stop the FIDO2 authenticator.
 *
 * Stops all transports and the processing thread.
 *
 * @retval 0 If successful.
 * @retval -errno On failure.
 */
int fido2_stop(void);

/**
 * @brief Set or clear a single FIDO2 runtime state callback.
 *
 * Set @p cb to NULL to clear the callback.
 *
 * @note If @p cb is non-NULL, it is called once immediately with the current
 *       state in the caller context.
 * @note This function is ISR-safe.
 *
 * @param cb Callback function, or NULL to disable callbacks.
 * @param user_data Opaque context pointer passed to @p cb.
 *
 * @retval 0 Always succeeds.
 */
int fido2_set_state_callback(fido2_state_callback_t cb, void *user_data);

/**
 * @brief Get the current FIDO2 runtime state.
 *
 * @note This function is ISR-safe.
 *
 * @return Current runtime state.
 */
enum fido2_runtime_state fido2_get_state(void);

/**
 * @brief Get authenticator device info.
 *
 * Populates @p info with supported versions, extensions, AAGUID,
 * and current configuration state.
 *
 * @note The `pin_retries` field is populated from storage via
 *       fido2_storage_pin_retries_get().
 *
 * @param info Pointer to device info structure to populate
 *
 * @retval 0 If successful.
 * @retval -EINVAL If info is NULL.
 */
int fido2_get_info(struct fido2_device_info *info);

/**
 * @brief Perform a factory reset.
 *
 * Wipes all stored credentials, PIN state, and resets the authenticator.
 * Requires user presence confirmation (fido2_up_check()), and must be
 * executed within 10 seconds of power-up per CTAP2 specification.
 * The 10-second power-up window is enforced internally.
 *
 * @retval 0 If successful.
 * @retval -errno On failure.
 */
int fido2_reset(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_FIDO2_FIDO2_H_ */
