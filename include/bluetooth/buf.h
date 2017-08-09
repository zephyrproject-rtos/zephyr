/** @file
 *  @brief Bluetooth data buffer API
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_BUF_H
#define __BT_BUF_H

/**
 * @brief Data buffers
 * @defgroup bt_buf Data buffers
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/types.h>
#include <net/buf.h>
#include <bluetooth/hci.h>

/** Possible types of buffers passed around the Bluetooth stack */
enum bt_buf_type {
	/** HCI command */
	BT_BUF_CMD,
	/** HCI event */
	BT_BUF_EVT,
	/** Outgoing ACL data */
	BT_BUF_ACL_OUT,
	/** Incoming ACL data */
	BT_BUF_ACL_IN,
};

/** Minimum amount of user data size for buffers passed to the stack. */
#define BT_BUF_USER_DATA_MIN 4

/** Data size neeed for HCI RX buffers */
#define BT_BUF_RX_SIZE (CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_LEN)

/** Allocate a buffer for incoming data
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_recv_prio().
 *
 *  @param type    Type of buffer. Only BT_BUF_EVT and BT_BUF_ACL_IN are
 *                 allowed.
 *  @param timeout Timeout in milliseconds, or one of the special values
 *                 K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_rx(enum bt_buf_type type, s32_t timeout);

/** Allocate a buffer for an HCI Command Complete/Status Event
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_recv_prio().
 *
 *  @param timeout Timeout in milliseconds, or one of the special values
 *                 K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_cmd_complete(s32_t timeout);

/** Set the buffer type
 *
 *  @param buf   Bluetooth buffer
 *  @param type  The BT_* type to set the buffer to
 */
static inline void bt_buf_set_type(struct net_buf *buf, enum bt_buf_type type)
{
	*(u8_t *)net_buf_user_data(buf) = type;
}

/** Get the buffer type
 *
 *  @param buf   Bluetooth buffer
 *
 *  @return The BT_* type to of the buffer
 */
static inline enum bt_buf_type bt_buf_get_type(struct net_buf *buf)
{
	return *(u8_t *)net_buf_user_data(buf);
}

/**
 * @}
 */

#endif /* __BT_BUF_H */
