/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 transport abstraction API.
 */

#ifndef ZEPHYR_INCLUDE_FIDO2_FIDO2_TRANSPORT_H_
#define ZEPHYR_INCLUDE_FIDO2_FIDO2_TRANSPORT_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/fido2/fido2_types.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief FIDO2 transport API
 * @ingroup fido2
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct fido2_transport;

/**
 * @brief Callback invoked when a transport receives data from the host.
 *
 * Executes in the transport's internal work queue thread, not the FIDO2
 * core thread. Must not block. Implementations should enqueue work and
 * return immediately. Not callable from userspace.
 *
 * @param transport Transport instance that received the data.
 * @param cid       CTAPHID channel ID associated with the received message.
 * @param buf       Received data payload.
 * @param len       Length of the received data in bytes.
 * @param user_data Opaque context provided during callback registration.
 */
typedef void (*fido2_transport_recv_cb_t)(const struct fido2_transport *transport, uint32_t cid,
					  const uint8_t *buf, size_t len, void *user_data);

/** @brief Transport driver API callbacks. */
struct fido2_transport_api {
	/**
	 * @brief Initialize the transport.
	 *
	 * @retval 0 If successful.
	 * @retval -ENODEV If hardware is not available.
	 */
	int (*init)(void);

	/**
	 * @brief Send response data to the host.
	 *
	 * @param cid  CTAPHID channel ID to send the response on.
	 * @param data Response payload bytes.
	 * @param len  Length of data in bytes.
	 *
	 * @retval 0 If successful.
	 * @retval -EIO On transport error.
	 */
	int (*send)(uint32_t cid, const uint8_t *data, size_t len);

	/**
	 * @brief Send a CTAPHID keepalive notification to the host.
	 *
	 * For transports that encapsulate CTAPHID framing (e.g. USB HID), this
	 * allows long-running operations to periodically notify the host that the
	 * authenticator is still processing or waiting for user interaction.
	 *
	 * @param cid    CTAPHID channel ID.
	 * @param status Keepalive status byte (CTAPHID keepalive payload).
	 *
	 * @retval 0 If successful.
	 * @retval -EIO On transport error.
	 */
	int (*send_keepalive)(uint32_t cid, uint8_t status);

	/**
	 * @brief Register the receive callback for this transport.
	 *
	 * @param cb        Callback to invoke on receive.
	 * @param user_data Opaque context forwarded to the callback.
	 */
	void (*register_recv_cb)(fido2_transport_recv_cb_t cb, void *user_data);

	/**
	 * @brief Shut down the transport.
	 */
	void (*shutdown)(void);
};

/** @brief A registered FIDO2 transport. */
struct fido2_transport {
	/** Transport name. */
	const char *name;
	/** Transport driver API callbacks. */
	const struct fido2_transport_api *api;
};

/**
 * @brief Register a FIDO2 transport.
 *
 * @param _name  C identifier for this transport instance.
 * @param _label Human-readable name string (e.g. "usb_ctaphid").
 * @param _api   Pointer to the transport's @ref fido2_transport_api.
 */
#define FIDO2_TRANSPORT_DEFINE(_name, _label, _api)                                                \
	STRUCT_SECTION_ITERABLE(fido2_transport, _name) = {                                        \
		.name = _label,                                                                    \
		.api = _api,                                                                       \
	}

/**
 * @brief Initialize all registered transports.
 *
 * @retval 0 If successful.
 * @retval -errno If any transport fails to initialize.
 */
int fido2_transport_init_all(void);

/**
 * @brief Register the receive callback with all transports.
 *
 * @param cb        Callback to invoke when any transport receives data.
 * @param user_data Opaque context forwarded to the callback.
 */
void fido2_transport_register_recv_cb(fido2_transport_recv_cb_t cb, void *user_data);

/**
 * @brief Shut down all registered transports.
 */
void fido2_transport_shutdown_all(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_FIDO2_FIDO2_TRANSPORT_H_ */
