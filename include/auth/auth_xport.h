/**
 * Copyright (c) 2021 Golden Bits Software, Inc.
 *
 * @file auth_xport.h
 *
 * @brief
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUTH_XPORT_H_
#define ZEPHYR_INCLUDE_AUTH_XPORT_H_

#if defined(CONFIG_BT_XPORT)
#include <bluetooth/gatt.h>
#endif


/**
 * Transport functions and defines.
 */


/**
 * Handle to lower transport, should be treated as opaque object.
 *
 * @note A typedef is use here because the transport handle is intended to
 *       be an opaque object to the lower transport layers and to the
 *       upper layers calling into the transport.  This use satisfies
 *       the Linux coding standards.
 */
typedef void *auth_xport_hdl_t;

/**
 * The lower transport type.
 */
enum auth_xport_type {
	AUTH_XP_TYPE_NONE = 0,
	AUTH_XP_TYPE_BLUETOOTH,
	AUTH_XP_TYPE_SERIAL,
	AUTH_XP_TYPE_FUTURE /* new transports added here */
};


/**
 * Transport event type.
 */
enum auth_xport_evt_type {
	XP_EVT_NONE = 0,
	XP_EVT_CONNECT,
	XP_EVT_DISCONNECT,
	XP_EVT_RECONNECT,

	/* transport specific events */
	XP_EVT_SERIAL_BAUDCHANGE
};

/**
 * Transport event structure
 */
struct auth_xport_evt {
	enum auth_xport_evt_type event;

	/* transport specific event information */
	void *xport_ctx;
};

/**
 * Callback invoked when sending data asynchronously.
 *
 * @param err       Error code, 0 == success.
 * @param numbytes  Number of bytes sent, can be 0.
 */
typedef void (*send_callback_t)(int err, uint16_t numbytes);


/**
 * Function for sending data directly to the lower layer transport
 * instead of putting data on an output queue. Some lower transport
 * layers have the ability to queue outbound data, no need to double
 * buffer.
 *
 * @param  xport_hdl    Opaque transport handle.
 * @param  data         Data to send.
 * @param  len          Number of bytes to send
 *
 * @return Number of bytes sent, on error negative error value.
 */
typedef int (*send_xport_t)(auth_xport_hdl_t xport_hdl, const uint8_t *data,
			    const size_t len);


/**
 * Initializes the lower transport layer.
 *
 * @param xporthdl      New transport handle is returned here.
 * @param instance      Authentication instance.
 * @param xport_type    Transport type
 * @param xport_params  Transport specific params, passed directly to lower transport.
 *
 * @return 0 on success, else negative error number.
 */
int auth_xport_init(auth_xport_hdl_t *xporthdl,  enum auth_instance_id instance,
		    enum auth_xport_type xport_type, void *xport_params);

/**
 * De-initializes the transport.  The lower layer transport should
 * free any allocated resources.
 *
 * @param xporthdl
 *
 * @return AUTH_SUCCESS or negative error value.
 */
int auth_xport_deinit(const auth_xport_hdl_t xporthdl);

/**
 * Send event to the lower transport.
 *
 * @param xporthdl Transport handle.
 * @param event    Event
 *
 * @return  0 on success, else -1
 */

int auth_xport_event(const auth_xport_hdl_t xporthdl, struct auth_xport_evt *event);
/**
 * Sends packet of data to peer.
 *
 * @param xporthdl  Transport handle
 * @param data      Buffer to send.
 * @param len       Number of bytes to send.
 *
 * @return  Number of bytes sent on success, can be less than requested.
 *          On error, negative error code.
 */
int auth_xport_send(const auth_xport_hdl_t xporthdl, const uint8_t *data, size_t len);


/**
 * Receive data from the lower transport.
 *
 * @param xporthdl  Transport handle
 * @param buff      Buffer to read bytes into.
 * @param buf_len   Size of buffer.
 * @param timeoutMsec   Wait timeout in milliseconds.  If no bytes available, then
 *                      wait timeoutMec milliseconds.  If 0, then will not wait.
 *
 * @return Negative on error or timeout, else number of bytes received.
 */
int auth_xport_recv(const auth_xport_hdl_t xporthdl, uint8_t *buff, uint32_t buf_len, uint32_t timeoutMsec);


/**
 * Peeks at the contents of the receive queue used by the lower transport.  The
 * data returned is not removed from the receive queue.
 *
 * @param xporthdl  Transport handle
 * @param buff      Buffer to read bytes into.
 * @param buf_len   Size of buffer.
 *
 * @return Negative on error or timeout, else number of bytes peeked.
 */
int auth_xport_recv_peek(const auth_xport_hdl_t xporthdl, uint8_t *buff, uint32_t buf_len);

/**
 * Used by lower transport to put bytes reveived into rx queue.
 *
 * @param xporthdl   Transport handle.
 * @param buf        Pointer to bytes to put.
 * @param buflen     Byte len of buffer.
 *
 * @return           Number of bytes added to receive queue.
 */
int auth_xport_put_recv(const auth_xport_hdl_t xporthdl, const uint8_t *buf, size_t buflen);


/**
 * Get the number of bytes queued for sending.
 *
 * @param xporthdl  Transport handle.
 *
 * @return  Number of queued bytes, negative value on error.
 */
int auth_xport_getnum_send_queued_bytes(const auth_xport_hdl_t xporthdl);


/**
 * Get the number of bytes in the receive queue
 *
 * @param xporthdl  Transport handle.
 *
 * @return  Number of queued bytes, negative value on error.
 */
int auth_xport_getnum_recvqueue_bytes(const auth_xport_hdl_t xporthdl);

/**
 * Get the number of bytes in the receive queue, if no byte wait until
 * bytes are received or time out.
 *
 * @param xporthdl  Transport handle.l
 * @param waitmsec  Number of milliseconds to wait.
 *
 * @return  Number of queued bytes, negative value on error.
 */
int auth_xport_getnum_recvqueue_bytes_wait(const auth_xport_hdl_t xporthdl, uint32_t waitmsec);

/**
 * Sets a direct send function to the lower transport layer instead of
 * queuing bytes into an output buffer.  Some lower transports can handle
 * all of the necessary output queuing while others (serial UARTs for example)
 * may not have the ability to queue outbound byes.
 *
 * @param xporthdl   Transport handle.
 * @param send_func  Lower transport send function.
 */
void auth_xport_set_sendfunc(auth_xport_hdl_t xporthdl, send_xport_t send_func);


/**
 * Used by the lower transport to set a context for a given transport handle.  To
 * clear a previously set context, use NULL as context pointer.
 *
 * @param xporthdl   Transport handle.
 * @param context    Context pointer to set.
 *
 */
void auth_xport_set_context(auth_xport_hdl_t xporthdl, void *context);

/**
 * Returns pointer to context.
 *
 * @param xporthdl   Transport handle.
 *
 * @return  Pointer to transport layer context, else NULL
 */
void *auth_xport_get_context(auth_xport_hdl_t xporthdl);

/**
 * Get the application max payload the lower transport can handle in one
 * in one frame.  The common transport functions will break up a larger
 * application packet into multiple frames.
 *
 * @param xporthdl   Transport handle.
 *
 * @return The max payload, or negative error number.
 */
int auth_xport_get_max_payload(const auth_xport_hdl_t xporthdl);


#if defined(CONFIG_BT_XPORT)


/**
 * Bluetooth UUIDs for the Authentication service.
 */
#if defined(CONFIG_ALT_AUTH_BT_UUIDS)

/**
 * If desired, define differnet UUIDs in this header file to be included
 * by the firmware application.
 */
#include "authlib_alt_bt_uuids.h"

#else

#define BT_BASE_AUTH_UUID                 "00000000-a3f6-4491-8b4d-b830f521243b"
#define BT_UUID_AUTH_SVC                  (0x3010)
#define BT_UUID_AUTH_SVC_CLIENT_CHAR      (0x3015)
#define BT_UUID_AUTH_SVC_SERVER_CHAR      (0x3016)

#define AUTH_SERVICE_UUID_BYTES  BT_UUID_128_ENCODE(BT_UUID_AUTH_SVC, 0xa3f6, 0x4491, 0x8b4d, 0xb830f521243b)
#define AUTH_SERVICE_UUID        BT_UUID_INIT_128(AUTH_SERVICE_UUID_BYTES)


#define AUTH_SVC_CLIENT_CHAR_UUID   BT_UUID_INIT_128( \
		BT_UUID_128_ENCODE(BT_UUID_AUTH_SVC_CLIENT_CHAR, 0xa3f6, 0x4491, 0x8b4d, 0xb830f521243b))

#define AUTH_SVC_SERVER_CHAR BT_UUID_INIT_128( \
		BT_UUID_128_ENCODE(BT_UUID_AUTH_SVC_SERVER_CHAR, 0xa3f6, 0x4491, 0x8b4d, 0xb830f521243b))
#endif


/**
 * Bluetooth params.
 */
struct auth_xp_bt_params {
	struct bt_conn *conn;
	bool is_central;

	/* The BT value handle used by the Central to send to the Peripheral.
	 * Not used by the Peripheral. */
	uint16_t server_char_hdl;

	/* Client attribute, used by peripheral to indicate data for client.
	 * Not used by the Central (client) */
	const struct bt_gatt_attr *client_attr;
};

/**
 * Initialize Bluetooth transport
 *
 * @param xport_hdl    Transport handle.
 * @param flags        Reserved for future use, should be set to 0.
 * @param xport_param  Pointer to Bluetooth transport params for use by BT layer.
 *
 * @return 0 on success, else negative error value
 */
int auth_xp_bt_init(const auth_xport_hdl_t xport_hdl, uint32_t flags, void *xport_param);


/**
 * Deinit the lower BT later.
 *
 * @param xport_hdl  The transport handle to de-initialize.
 *
 * @return 0 on success, else negative error number.
 */
int auth_xp_bt_deinit(const auth_xport_hdl_t xport_hdl);

/*
 * Called when the Central (client) writes to a Peripheral (server) characteristic.
 *
 * @param conn   The BT connection.
 * @param attr   GATT attribute to write to.
 * @param buf    Data to write.
 * @param len    Number of bytes to write.
 * @param offset Offset to start writing from.
 * @param flags  BT_GATT_WRITE_* flags
 *
 * @return  Number of bytes written.
 */
ssize_t auth_xp_bt_central_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags);


/**
 * Called by the Central (client) when a Peripheral (server) writes/updates a characteristic.
 * This function is called by the Central BT stack when data is received by the Peripheral (server)
 *
 * @param conn    Bluetooth connection struct.
 * @param params  Gatt subscription params.
 * @param data    Data from peripheral.
 * @param length  Number of bytes reived.
 *
 * @return BT_GATT_ITER_CONTINUE or BT_GATT_ITER_STOP
 */
uint8_t auth_xp_bt_central_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length);


/**
 * Sends Bluetooth event to lower Bluetooth transport.
 *
 * @param xporthdl   Transport handle.
 * @param event      The event.
 *
 * @return AUTH_SUCCESS, else negative error code.
 */
int auth_xp_bt_event(const auth_xport_hdl_t xporthdl, struct auth_xport_evt *event);

/**
 * Gets the maximum payload for the Bluetooth link, which is the MTU less any
 * Bluetooth link overhead.
 *
 * @param xporthdl   Transport handle.
 *
 * @return Max application payload.
 */
int auth_xp_bt_get_max_payload(const auth_xport_hdl_t xporthdl);

#endif  /* CONFIG_BT_XPORT */


#if defined(CONFIG_SERIAL_XPORT)

/**
 * Serial transport param.
 */
struct auth_xp_serial_params {
	const struct device *uart_dev; /* pointer to Uart instance */
};

/**
 * Initialize Serial lower layer transport.
 *
 * @param xport_hdl      Transport handle.
 * @param flags          RFU (Reserved for future use), set to 0.
 * @param xport_params   Serial specific transport parameters.
 *
 * @return 0 on success, else negative value.
 */
int auth_xp_serial_init(const auth_xport_hdl_t xport_hdl, uint32_t flags, void *xport_param);


/**
 * Deinit lower serial transport.
 *
 * @param xport_hdl  Transport handle
 *
 * @return 0 on success, else negative value.
 */
int auth_xp_serial_deinit(const auth_xport_hdl_t xport_hdl);


/**
 * Sends an event to lower serial transport.
 *
 * @param xporthdl   Transport handle.
 * @param event      The event.
 *
 * @return AUTH_SUCCESS, else negative error code.
 */
int auth_xp_serial_event(const auth_xport_hdl_t xporthdl, struct auth_xport_evt *event);


/**
 * Gets the maximum payload for the serial link.
 *
 * @param xporthdl   Transport handle.
 *
 * @return Max application payload.
 */
int auth_xp_serial_get_max_payload(const auth_xport_hdl_t xporthdl);

#endif  /* CONFIG_SERIAL_XPORT */


#endif  /* ZEPHYR_INCLUDE_AUTH_XPORT_H_ */