/** @file
 *  @brief Bluetooth data buffer API
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BT_BUF_H
#define __BT_BUF_H

/**
 * @brief Data buffers
 * @defgroup bt_buf Data buffers
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
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
#define BT_BUF_RX_SIZE (CONFIG_BLUETOOTH_HCI_RECV_RESERVE + \
			sizeof(struct bt_hci_acl_hdr) + \
			CONFIG_BLUETOOTH_RX_BUF_LEN)

/** Allocate a buffer for incoming data
 *
 *  This will not set the buffer type so bt_buf_set_type() needs to be called
 *  before bt_recv().
 *
 *  @param timeout Timeout in milliseconds, or one of the special values
 *                 K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_rx(int32_t timeout);

/** Allocate a buffer for an HCI event
 *
 *  This will set the BT_BUF_EVT buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called.
 *
 *  @param opcode HCI event opcode or 0 if not known
 *  @param timeout Timeout in milliseconds, or one of the special values
 *                 K_NO_WAIT and K_FOREVER.
 *  @return A new buffer with the BT_BUF_EVT type.
 */
struct net_buf *bt_buf_get_evt(uint8_t opcode, int32_t timeout);

/** Allocate a buffer for incoming ACL data
 *
 *  This will set the BT_BUF_ACL_IN buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called.
 *
 *  @param timeout Timeout in milliseconds, or one of the special values
 *                 K_NO_WAIT and K_FOREVER.
 *  @return A new buffer with the BT_BUF_ACL_IN type.
 */
struct net_buf *bt_buf_get_acl(int32_t timeout);

/** Set the buffer type
 *
 *  @param buf   Bluetooth buffer
 *  @param type  The BT_* type to set the buffer to
 */
static inline void bt_buf_set_type(struct net_buf *buf, enum bt_buf_type type)
{
	*(uint8_t *)net_buf_user_data(buf) = type;
}

/** Get the buffer type
 *
 *  @param buf   Bluetooth buffer
 *
 *  @return The BT_* type to of the buffer
 */
static inline enum bt_buf_type bt_buf_get_type(struct net_buf *buf)
{
	return *(uint8_t *)net_buf_user_data(buf);
}

/**
 * @}
 */

#endif /* __BT_BUF_H */
