/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 transport abstraction API.
 */

#ifndef ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_TRANSPORT_H_
#define ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_TRANSPORT_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/authentication/fido2/fido2_types.h>
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

/** @brief Transport progress status codes. */
enum fido2_wire_status {
	/** The authenticator is waiting for a user presence gesture. */
	FIDO2_WIRE_STATUS_UP_NEEDED,
	/** A long-running operation has started (e.g. crypto, storage). */
	FIDO2_WIRE_STATUS_PROCESSING,
	/** The operation is complete. */
	FIDO2_WIRE_STATUS_DONE
};

/**
 * @brief Callback invoked when a transport receives data from the host.
 *
 * Executes in the transport's internal work queue thread, not the FIDO2
 * core thread. Must not block. Implementations should enqueue work and
 * return immediately. Not callable from userspace.
 *
 * @param transport Transport instance that received the data.
 * @param cid       Transport channel ID associated with the received message.
 * @param buf       Received data payload.
 * @param len       Length of the received data in bytes.
 */
typedef void (*fido2_transport_recv_cb_t)(const struct fido2_transport *transport, uint32_t cid,
					  const uint8_t *buf, size_t len);

/**
 * @brief Callback invoked when a transport receives a cancel command.
 */
typedef void (*fido2_transport_cancel_cb_t)(void);

/** @brief Transport driver API callbacks. */
struct fido2_transport_api {
	/**
	 * @brief Initialize the transport.
	 *
	 * Called once during subsystem startup. The transport must store
	 * the callbacks and invoke them on every received CBOR message
	 * or cancel command.
	 *
	 * @param cb        Callback to invoke when a CBOR message is received.
	 * @param cancel_cb Callback to invoke when the cancel command is received
	 *
	 * @retval 0 If successful.
	 * @retval -ENODEV If hardware is not available.
	 */
	int (*init)(fido2_transport_recv_cb_t cb, fido2_transport_cancel_cb_t cancel_cb);

	/**
	 * @brief Send response data to the host.
	 *
	 * @param cid  Transport channel ID to send the response on.
	 * @param data Response payload bytes.
	 * @param len  Length of data in bytes.
	 *
	 * @retval 0 If successful.
	 * @retval -EIO On transport error.
	 */
	int (*send)(uint32_t cid, const uint8_t *data, size_t len);

	/**
	 * @brief Notify the transport of an operation progress change.
	 *
	 * Called by the FIDO2 core to signal state transitions during
	 * long-running operations. Each transport translates this into the
	 * appropriate wire-protocol signaling.
	 *
	 * @param cid    Channel/connection identifier.
	 * @param status Progress status indicating what the core is doing.
	 */
	void (*notify)(uint32_t cid, enum fido2_wire_status status);

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
 * @param _label Human-readable name string (e.g. "USB_HID").
 * @param _api   Pointer to the transport's @ref fido2_transport_api.
 */
#define FIDO2_TRANSPORT_DEFINE(_name, _label, _api)                                                \
	STRUCT_SECTION_ITERABLE(fido2_transport, _name) = {                                        \
		.name = _label,                                                                    \
		.api = _api,                                                                       \
	}

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief Initialize all registered transports.
 *
 * @param cb        Callback to invoke when any transport receives data.
 * @param cancel_cb Callback to invoke when the cancel command is received.
 *
 * @retval 0 If successful.
 * @retval -errno If any transport fails to initialize.
 */
int fido2_transport_init_all(fido2_transport_recv_cb_t cb, fido2_transport_cancel_cb_t cancel_cb);

/**
 * @brief Shut down all registered transports.
 */
void fido2_transport_shutdown_all(void);

/**
 * @endcond
 */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_FIDO2_TRANSPORT_H_ */
