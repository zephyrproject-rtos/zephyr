/** @file
 *  @brief Bluetooth SCO logical transport handling.
 */
/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_

/**
 * @file
 * @brief Synchronous Connection-Oriented (SCO)
 * @defgroup bt_sco Synchronous Connection-Oriented (SCO)
 * @since 4.5
 * @version 0.1.0
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Reserved headroom for SCO channel data transmission.
 *
 *  This macro defines the number of bytes that must be reserved at the beginning of outgoing SCO
 *  data buffers for the HCI SCO header.
 *  Applications should account for this headroom when allocating buffers for SCO data transmission.
 */
#define BT_SCO_CHAN_SEND_RESERVE BT_BUF_SCO_SIZE(0)

/** @brief Calculate total SCO SDU buffer size including header.
 *
 *  Helper macro to calculate the total buffer size needed for an SCO Service Data Unit (SDU),
 *  including the required headroom for the HCI SCO header.
 *
 *  @param mtu Maximum Transmission Unit size (payload only).
 *
 *  @return Total buffer size required (MTU + header reserve).
 */
#define BT_SCO_SDU_SIZE(mtu) BT_BUF_SCO_SIZE(mtu)

struct bt_sco_stream;

/**
 * @brief SCO stream callback operations structure.
 *
 * @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 * This structure defines callback functions for handling SCO stream events.
 * These callbacks are invoked when data is received or transmitted over the
 * SCO stream.
 */
struct bt_sco_stream_ops {
	/** @brief Data received callback
	 *
	 *  Called when data is received on the SCO stream.
	 *
	 *  @param stream Pointer to the SCO stream that received data.
	 *  @param flag Status flag for the received data. Refer to BT_HCI_SCO_* for values,
	 *              - @ref BT_HCI_SCO_CORRECTLY_RECEIVED for good data
	 *              - @ref BT_HCI_SCO_POSSIBLY_INVALID for possibly errors
	 *              - @ref BT_HCI_SCO_NO_DATA_RECEIVED for post data
	 *              - @ref BT_HCI_SCO_DATA_PARTIALLY_LOST for partial lost
	 *  @param buf The buffer containing the received data. After the callback returns, the
	 *             buffer is released by the stack and must not be referenced.
	 */
	void (*recv)(struct bt_sco_stream *stream, uint8_t flag, struct net_buf *buf);

	/** @brief Data sent callback
	 *
	 *  Called when data has been successfully sent on the SCO stream.
	 *  This callback can be used to track transmission completion and manage flow control.
	 *
	 *  @param stream Pointer to the SCO stream that sent the data.
	 */
	void (*sent)(struct bt_sco_stream *stream);
};

/**
 * @brief SCO stream structure.
 *
 * @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 * This structure represents an SCO audio/data stream and contains
 * references to the underlying SCO connection and stream operation callbacks.
 */
struct bt_sco_stream {
	/** Pointer to the SCO connection object */
	struct bt_conn *sco;
	/** Pointer to stream operation callbacks */
	const struct bt_sco_stream_ops *ops;
};

/** @brief Register SCO stream callbacks.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Register callback operations for an SCO stream. These callbacks will be invoked for data
 *  reception and transmission events on the stream.
 *
 *  @param stream Pointer to the SCO stream.
 *  @param ops    Pointer to the stream operations structure.
 *                Must point to memory that remains valid.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream or @p ops is NULL.
 */
int bt_sco_stream_cb_register(struct bt_sco_stream *stream, struct bt_sco_stream_ops *ops);

/** @brief Unregister SCO stream callbacks.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Unregister previously registered stream callbacks.
 *
 *  @param stream Pointer to the SCO stream.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream is NULL.
 */
int bt_sco_stream_cb_unregister(struct bt_sco_stream *stream);

/** @brief Connect an SCO stream to an SCO connection.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Associates an SCO stream with an established SCO connection, enabling data transmission and
 *  reception through the stream interface.
 *  When the SCO connection is established, this function should be called to bind the stream to
 *  the SCO connection before any data operations. Such as, when @ref bt_hfp_hf_cb::sco_connected
 *  callback is invoked, the function should be called to bind the stream to the SCO connection.
 *
 *  @param sco    Pointer to an established SCO connection object.
 *  @param stream Pointer to the SCO stream to connect.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p sco or @p stream is NULL.
 *  @retval -EALREADY If @p sco has been connected to @p stream.
 *  @retval -EBUSY If there is any other stream has been connected to @p sco, or @p stream has been
 *                 connected to another SCO connection.
 */
int bt_sco_stream_connect(struct bt_conn *sco, struct bt_sco_stream *stream);

/** @brief Disconnect an SCO stream.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Disconnects an SCO stream from its associated SCO connection.
 *  After disconnection, the stream can no longer be used for data transmission or reception until
 *  reconnected.
 *  When the SCO connection is closed or the stream is no longer needed, this function should be
 *  called to release the stream from the connection and clean up associated resources. Such as,
 *  when @ref bt_hfp_hf_cb::sco_disconnected callback is invoked, the function should be called to
 *  disconnect the stream. Otherwise, the SCO connection will not be released.
 *
 *  @param stream Pointer to the SCO stream to disconnect.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream is NULL.
 *  @retval -ENOTCONN If the stream is not connected.
 */
int bt_sco_stream_disconnect(struct bt_sco_stream *stream);

/** @brief Send data over an SCO stream.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Send audio or data packets over the SCO stream. The buffer should contain
 *  properly formatted SCO data according to the negotiated voice settings.
 *
 *  @note The buffer must have sufficient headroom (at least
 *        @ref BT_SCO_CHAN_SEND_RESERVE bytes) for the HCI SCO header.
 *  @note The buffer's user_data_size must be at least
 *        CONFIG_BT_CONN_TX_USER_DATA_SIZE bytes.
 *  @note The data length must not exceed the SCO MTU.
 *
 *  @param stream Pointer to the connected SCO stream.
 *  @param buf    Network buffer containing the data to send. Ownership of the buffer is
 *                transferred to the stack if no errors returned.
 *
 *  @retval 0 Success. Buffer queued for transmission.
 *  @retval -EINVAL If @p stream or @p buf is NULL, insufficient headroom,
 *                  or buffer user_data_size is too small.
 *  @retval -ENOTCONN If the stream or underlying SCO connection is not connected.
 *  @retval -ENOTSUP If the voice over HCI feature is unsupported by controller.
 *  @retval -EMSGSIZE If the data length exceeds the SCO MTU.
 */
int bt_sco_stream_send(struct bt_sco_stream *stream, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_ */
