/** @file
 *  @brief Bluetooth L2CAP BR/EDR handling
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_BR_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_BR_H_

/**
 * @brief L2CAP
 * @defgroup bt_l2cap L2CAP
 * @ingroup bluetooth
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ECHO request/response callback structure.
 *
 * This structure is used for tracking the ECHO request/response signaling packets of L2CAP BR.
 * It is registered with the help of the bt_l2cap_br_echo_cb_register() API.
 * It's permissible to register multiple instances of this @ref bt_l2cap_br_echo_cb type, in case
 * different modules of an application are interested in tracking the ECHO request/response
 * signaling packets. If a callback is not of interest for an instance, it may be set to NULL and
 * will as a consequence not be used for that instance.
 */
struct bt_l2cap_br_echo_cb {
	/**
	 * @brief A ECHO request has been received.
	 *
	 * This callback notifies the application of a ECHO request has been received.
	 * The ECHO response should be performed by calling the bt_l2cap_br_echo_rsp() API.
	 *
	 * @param conn The ACL connection object.
	 * @param identifier The identifier of the ECHO request.
	 * @param buf Received ECHO data.
	 */
	void (*req)(struct bt_conn *conn, uint8_t identifier, struct net_buf *buf);

	/**
	 * @brief A ECHO response has been received.
	 *
	 * This callback notifies the application of a ECHO response has been received.
	 *
	 * @param conn The ACL connection object.
	 * @param buf Received ECHO data.
	 */
	void (*rsp)(struct bt_conn *conn, struct net_buf *buf);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

/**
 * @brief Register ECHO callbacks.
 *
 * Register callbacks to monitor the packets of L2CAP BR echo request/response.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EEXIST if @p cb was already registered.
 */
int bt_l2cap_br_echo_cb_register(struct bt_l2cap_br_echo_cb *cb);

/**
 * @brief Unregister ECHO callbacks.
 *
 * Unregister callbacks that are used to monitor the packets of L2CAP BR echo request/response.
 *
 * @param cb Callback struct point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -ENOENT if @p cb was not registered.
 */
int bt_l2cap_br_echo_cb_unregister(struct bt_l2cap_br_echo_cb *cb);

/**
 *  @brief Headroom needed for outgoing L2CAP ECHO REQ PDUs.
 */
#define BT_L2CAP_BR_ECHO_REQ_RESERVE BT_L2CAP_BUF_SIZE(4)

/**
 *  @brief Headroom needed for outgoing L2CAP ECHO RSP PDUs.
 */
#define BT_L2CAP_BR_ECHO_RSP_RESERVE BT_L2CAP_BUF_SIZE(4)

/**
 * @brief Send ECHO data through ECHO request
 *
 * Send ECHO data through ECHO request. The application is required to have reserved
 * @ref BT_L2CAP_BR_ECHO_REQ_RESERVE bytes in the buffer before sending.
 *
 * @param conn The ACL connection object.
 * @param buf Sending ECHO data.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_br_echo_req(struct bt_conn *conn, struct net_buf *buf);

/**
 * @brief Send ECHO data through ECHO response
 *
 * Send ECHO data through ECHO response. The application is required to have reserved
 * @ref BT_L2CAP_BR_ECHO_RSP_RESERVE bytes in the buffer before sending.
 *
 * @param conn The ACL connection object.
 * @param identifier The identifier of the ECHO request.
 * @param buf Sending ECHO data.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_br_echo_rsp(struct bt_conn *conn, uint8_t identifier, struct net_buf *buf);

/**
 * @brief Connectionless data channel callback structure.
 *
 * This structure is used for tracking the connectionless data channel frames.
 * It is registered with the help of the bt_l2cap_br_connless_register() API.
 * It's permissible to register multiple instances of this @ref bt_l2cap_br_connless_cb type,
 * in case different modules of an application are interested in tracking the connectionless
 * data channel frames.
 */
struct bt_l2cap_br_connless_cb {
	/** @brief Register PSM.
	 *
	 *  For BR, possible values:
	 *
	 *  The PSM field is at least two octets in length. All PSM values shall have the least
	 *  significant bit of the most significant octet equal to 0 and the least significant bit
	 *  of all other octets equal to 1.
	 *
	 *  0               Any connectionless data channel frame will be notified.
	 *
	 *  0x0001-0x0eff   Standard, Bluetooth SIG-assigned fixed values.
	 *
	 *  > 0x1000        Dynamically allocated. May be pre-set by the
	 *                  application before server registration (not
	 *                  recommended however), or auto-allocated by the
	 *                  stack if the app gave 0 as the value.
	 */
	uint16_t psm;

	/** Required minimum security level */
	bt_security_t sec_level;

	/**
	 * @brief A connectionless channel data has been received.
	 *
	 * This callback notifies the application of a connectionless channel data has been
	 * received.
	 *
	 * @param conn The ACL connection object.
	 * @param psm Protocol/Service Multiplexer.
	 * @param buf Received connectionless channel data.
	 */
	void (*recv)(struct bt_conn *conn, uint16_t psm, struct net_buf *buf);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

/**
 * @brief Register connectionless data channel callbacks.
 *
 * Register callbacks to monitor and receive connectionless data channel frames.
 * This allows the application to receive data sent to a specific PSM without
 * establishing a connection.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 *           The PSM field must be set to the desired Protocol/Service Multiplexer
 *           value, or 0 to receive all connectionless data frames.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL, has invalid PSM value, has invalid `recv` callback,
 *                 or has invalid security level.
 * @retval -EEXIST If a callback with the same PSM was already registered.
 */
int bt_l2cap_br_connless_register(struct bt_l2cap_br_connless_cb *cb);

/**
 * @brief Unregister connectionless data channel callbacks.
 *
 * Unregister callbacks that are used to monitor and receive connectionless data channel frames.
 *
 * @param cb Registered cb.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -ENOENT if @p cb was not registered.
 */
int bt_l2cap_br_connless_unregister(struct bt_l2cap_br_connless_cb *cb);

/**
 *  @brief L2CAP connectionless SDU header size.
 */
#define BT_L2CAP_CONNLESS_SDU_HDR_SIZE 2

/**
 *  @brief Headroom needed for outgoing L2CAP PDU format within a connectionless data channel.
 */
#define BT_L2CAP_CONNLESS_RESERVE BT_L2CAP_BUF_SIZE(BT_L2CAP_CONNLESS_SDU_HDR_SIZE)

/**
 * @brief Send data over connectionless data channel
 *
 * Send data over a connectionless data channel. This function allows sending data to
 * a specific PSM without establishing the L2CAP channel connection.
 *
 * The application is required to have reserved @ref BT_L2CAP_CONNLESS_RESERVE bytes
 * in the buffer before sending.
 *
 * @param conn Connection object.
 * @param psm Protocol/Service Multiplexer identifying the destination service.
 * @param buf Buffer containing the data to be sent. The application is required to
 *            have reserved enough headroom in the buffer for the L2CAP header.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p conn or @p buf is NULL, @p psm is invalid, the reference counter of @p buf
 *                 != 1, the head room of @p buf is less than @ref BT_L2CAP_CONNLESS_RESERVE, or
 *                 the `buf->user_data_size` is less than the size of `struct closure`.
 * @return -ENOTSUP If the connectionless reception feature is unsupported by remote.
 * @return -ENOTCONN If the L2CAP channel with connectionless channel is not found.
 * @return -EMSGSIZE If the data size exceeds the peer's connectionless channel MTU.
 * @retval -ESHUTDOWN If the L2CAP channel with connectionless channel is NULL in low level.
 */
int bt_l2cap_br_connless_send(struct bt_conn *conn, uint16_t psm, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_L2CAP_BR_H_ */
