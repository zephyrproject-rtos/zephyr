/** @file
 *  @brief Bluetooth SPP Protocol handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_

/**
 * @brief Serial Port Profile (SPP)
 * @defgroup bt_spp Serial Port Profile (SPP)
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

struct bt_spp;

/** @brief SPP profile application callback structure */
struct bt_spp_ops {
	/** SPP connection established callback
	 *
	 *  If this callback is provided, it will be called when the Serial Port
	 *  Profile (SPP) connection has been fully established and is ready for
	 *  data transfer operations.
	 *
	 *  At this point:
	 *  - The RFCOMM channel is active
	 *  - The SPP object is fully initialized
	 *  - Data transmission via bt_spp_send() can begin
	 *
	 *  @param conn    ACL connection for the SPP link
	 *  @param spp     Valid SPP connection object
	 */
	void (*connected)(struct bt_conn *conn, struct bt_spp *spp);

	/** SPP connection terminated callback
	 *
	 *  If this callback is provided, it will be called when the SPP connection
	 *  has been terminated. This includes both graceful disconnections and any
	 *  error conditions that caused the link to break.
	 *
	 *  Important notes:
	 *  - The SPP object will be freed immediately after this callback returns
	 *  - After this callback, the SPP object becomes invalid
	 *  - All references to this SPP object should be cleared
	 *
	 *  @param spp     SPP connection object (will be freed after callback)
	 */
	void (*disconnected)(struct bt_spp *spp);

	/** SPP data received callback
	 *
	 *  If this callback is provided, it will be called when data has been
	 *  received over the SPP connection.
	 *
	 *  Important considerations:
	 *  - The data buffer is only valid during the callback execution
	 *  - If the data needs to be retained, it must be copied by the application
	 *  - The implementation should process data quickly to avoid flow control issues
	 *
	 *  @param spp     SPP connection object that received the data
	 *  @param buf     Pointer to the received data buffer
	 */
	void (*recv)(struct bt_spp *spp, struct net_buf *buf);
};

struct bt_spp_client {
	/** @internal SPP SDP discover params */
	struct bt_sdp_discover_params sdp_discover;
};

/** @brief SPP server registration context
 *
 *  This structure represents a server-side SPP service instance that
 *  can be registered with the Bluetooth stack. It contains the SDP record
 *  used for service discovery, the RFCOMM server object used to accept
 *  incoming connections, an application-provided accept() callback and a
 *  list node used to link registered servers.
 */
struct bt_spp_server {
	/** Pointer to the SDP record exposed for this server.
	 *
	 *  If NULL when registering, the registration will fail. The record must
	 *  remain valid for the lifetime of the registration.
	 */
	struct bt_sdp_record *sdp_record;

	/** RFCOMM server instance.
	 *
	 *  Populated/used by the stack to accept incoming RFCOMM connections
	 *  for this SPP service. Do not modify directly from application code.
	 */
	struct bt_rfcomm_server rfcomm_server;

	/** Application accept callback.
	 *
	 *  Called by the stack when an incoming RFCOMM connection for this
	 *  server is pending. The callback should fill the provided spp
	 *  pointer with a pre-allocated bt_spp instance (or return an error)
	 *  to accept/reject the connection.
	 *
	 *  @param conn    ACL connection for the incoming request
	 *  @param server  Pointer to this bt_spp_server instance
	 *  @param spp     Out parameter to receive the accepted bt_spp instance
	 *
	 *  @return 0 on success (spp filled), negative error to reject
	 */
	int (*accept)(struct bt_conn *conn, struct bt_spp_server *server, struct bt_spp **spp);

	/** Node to link this server into internal server lists. */
	sys_snode_t node;
};

struct bt_spp {
	/** @internal RFCOMM Data Link Connection (DLC) */
	struct bt_rfcomm_dlc rfcomm_dlc;

	/** @internal ACL connection reference */
	struct bt_conn *acl_conn;

	/** @internal Connection state (atomic) */
	atomic_t state;

	/** Application callbacks
	 *
	 *  Pointer to the bt_spp_ops table supplied by the application. The
	 *  stack calls these callbacks to notify about connection, disconnection
	 *  and received data events.
	 */
	const struct bt_spp_ops *ops;

	/** @internal Role-specific context
	 *
	 *  Union containing context specific to the role of this SPP instance:
	 *   - client: SDP discovery parameters and UUID used when initiating a
	 *             connection to a remote SPP service.
	 *   - server: server registration and accept callback context for
	 *             accepted incoming connections.
	 *
	 *  Access the appropriate member only when the instance role is known.
	 */
	union {
		struct bt_spp_client client;
		struct bt_spp_server server;
	};
};

/**
 * @brief Register an SPP server service
 *
 * This function registers a Serial Port Profile (SPP) server service with the
 * Bluetooth stack. The registered service will be discoverable via SDP and
 * will accept incoming RFCOMM connections on the specified channel.
 *
 *
 * @note The UUID must remain valid for the lifetime of the service registration.
 *       Typically this should be one of the standard UUIDs defined in the specification.
 *
 * @return 0 on successful service registration
 * @return -EINVAL if:
 *         - channel is out of valid range (handled internally)
 * @return -ENOMEM if:
 *         - Unable to allocate SPP service context
 *         - Maximum service limit reached (CONFIG_BT_SPP_MAX_SERVER_NUM)
 */
int bt_spp_server_register(struct bt_spp_server *server);

/**
 * @brief Allocate an SPP SDP record for a given RFCOMM channel
 *
 * Allocate and initialize an available, compile-time-generated SPP SDP record
 * slot and associate it with the provided RFCOMM channel. The stack uses
 * compile-time attribute arrays that reference a runtime channel map; this
 * function stores the runtime channel value into the corresponding slot so
 * the SDP record exposes the correct RFCOMM channel.
 *
 * Thread-safety: the implementation is responsible for serializing access to
 * the record pool.
 *
 * @param channel RFCOMM channel number to publish in the SDP record.
 *                Valid range: SPP_CHANNEL_MIN .. SPP_CHANNEL_MAX
 *
 * @return pointer to a bt_sdp_record on success
 * @return NULL if no free SDP record slot is available or if the channel is invalid
 */
struct bt_sdp_record *bt_spp_alloc_record(uint8_t channel);

/**
 * @brief Initiate the SPP connection establishment procedure
 *
 * Initiate the Serial Port Profile (SPP) connection establishment procedure
 * on the ACL connection specified by the parameter `conn` using the specific
 * UUID to identify the SPP service.
 *
 * This function performs the following operations:
 * 1. Checks for an existing SPP connection on the ACL link
 * 2. Reference the provided UUID for SDP discovery
 * 3. Initiates SDP discovery to find the RFCOMM channel, then connects to it
 * 4. Takes a reference to the ACL connection on success
 *
 * The SPP object returned by this function represents the ongoing connection
 * establishment. The actual connection setup will complete asynchronously
 * through the Bluetooth stack. When the connection is fully established,
 * the registered callback `connected` will be triggered to notify the
 * application.
 *
 * @note The returned SPP object is only valid for connection management
 *       until the connection establishment completes. After the `connected`
 *       callback is triggered, the SPP object becomes fully valid for
 *       data transfer operations.
 *
 * @param conn ACL connection object on which to establish SPP
 * @param spp Pointer to a pointer that will receive the SPP connection object
 * @param uuid UUID of the SPP service to connect to
 *
 * @return Pointer to the SPP connection object on success
 * @return 0 in case of success or negative value in case of error.
 */

int bt_spp_connect(struct bt_conn *conn, struct bt_spp *spp, const struct bt_uuid *uuid);

/**
 * @brief Send data over an established SPP connection
 *
 * This function transmits data over an established Serial Port Profile (SPP)
 * connection. The transmission is asynchronous and may be subject to flow control
 * limitations of the underlying RFCOMM layer.
 *
 * @note The function must only be called after the connection has been fully
 *       established (i.e., after the `connected` callback has been triggered).
 *       Calling this function on a partially established connection will result
 *       in undefined behavior.
 *
 * @param spp   Valid SPP connection object obtained after successful connection
 * @param buf  Pointer to the data buffer to send
 *
 * @return 0 on successful queuing of the data for transmission
 * @return -EINVAL if:
 *         - spp is NULL
 *         - The underlying ACL connection is missing
 *         - data is NULL (implied by implementation)
 * @return -ENOMEM if unable to allocate RFCOMM protocol data unit (PDU)
 * @return Other negative RFCOMM error codes on transmission failure
 */
int bt_spp_send(struct bt_spp *spp, struct net_buf *buf);

/**
 * @brief Release an established SPP connection
 *
 * Terminate the Serial Port Profile (SPP) connection represented by the
 * parameter `spp`.
 *
 * This function performs the following operations:
 * 1. Validates the SPP connection object
 * 2. Initiates disconnection of the underlying RFCOMM DLC channel
 *
 * When the disconnection procedure completes, the registered callback
 * `disconnected` will be triggered to notify the application. After the
 * `disconnected` callback returns, the SPP object will be freed by the
 * stack and becomes invalid.
 *
 * @warning This function must only be called after the connection has been
 *          fully established (i.e., after the `connected` callback has been
 *          triggered). Calling this function on a partially established
 *          connection will result in undefined behavior.
 *
 * @note After this function is called successfully:
 *       - All interfaces using this SPP object should not be called
 *       - The object pointer becomes invalid after the `disconnected` callback
 *       - Any pending data transfers will be aborted
 *
 * @param spp SPP connection object to disconnect
 *
 * @return 0 on successful disconnection initiation
 * @return -EINVAL if:
 *         - spp is NULL
 *         - The underlying ACL connection is missing
 * @return Other negative error codes from the RFCOMM layer on disconnection failure
 */
int bt_spp_disconnect(struct bt_spp *spp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SPP_H_ */
