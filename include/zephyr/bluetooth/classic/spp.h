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

struct bt_spp;

/** @brief SPP profile application callback structure */
struct bt_spp_cb {
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
	 *  @param spp     Valid SPP connection object
	 *  @param channel RFCOMM channel number used for this connection
	 */
	void (*connected)(struct bt_spp *spp, uint8_t channel);

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
	 *  @param channel RFCOMM channel number that was used for this connection
	 */
	void (*disconnected)(struct bt_spp *spp, uint8_t channel);

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
	 *  @param channel RFCOMM channel number for this connection
	 *  @param data    Pointer to the received data buffer
	 *  @param len     Length of the received data in bytes
	 */
	void (*recv)(struct bt_spp *spp, uint8_t channel, const uint8_t *data, uint16_t len);
};

/**
 * @brief Register SPP event callback handlers
 *
 * This function registers application callback handlers for SPP connection events.
 * The callback structure should contain function pointers for handling:
 * - Connection established events
 * - Disconnection events
 * - Data reception events
 *
 * @param cb Pointer to the callback structure containing event handlers
 *
 * @return 0 on successful registration
 * @return -EINVAL if cb is NULL
 */
int bt_spp_register_cb(const struct bt_spp_cb *cb);

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
 * @param channel RFCOMM channel number to register (1-30)
 * @param uuid    UUID of the SPP service to advertise
 *
 * @return 0 on successful service registration
 * @return -EINVAL if:
 *         - uuid is NULL (implied by implementation)
 *         - channel is out of valid range (handled internally)
 * @return -ENOMEM if:
 *         - Unable to allocate SPP service context
 *         - Maximum service limit reached (CONFIG_BT_SPP_MAX_SERVER_NUM)
 */
int bt_spp_register_srv(uint8_t channel, const struct bt_uuid *uuid);

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

int bt_spp_connect(struct bt_conn *conn, struct bt_spp **spp, const struct bt_uuid *uuid);

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
 * @param data  Pointer to the data buffer to send
 * @param len   Length of data to send (in bytes)
 *
 * @return 0 on successful queuing of the data for transmission
 * @return -EINVAL if:
 *         - spp is NULL
 *         - The underlying ACL connection is missing
 *         - data is NULL (implied by implementation)
 * @return -ENOMEM if unable to allocate RFCOMM protocol data unit (PDU)
 * @return Other negative RFCOMM error codes on transmission failure
 */
int bt_spp_send(struct bt_spp *spp, const uint8_t *data, uint16_t len);

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
