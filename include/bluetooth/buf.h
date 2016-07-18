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

/** Data size neeed for HCI event buffers */
#define BT_BUF_EVT_SIZE (CONFIG_BLUETOOTH_HCI_RECV_RESERVE + \
			 sizeof(struct bt_hci_evt_hdr) + \
			 CONFIG_BLUETOOTH_MAX_EVT_LEN)

/** Data size needed for incoming ACL buffers */
#define BT_BUF_ACL_IN_SIZE (CONFIG_BLUETOOTH_HCI_RECV_RESERVE + \
			    sizeof(struct bt_hci_acl_hdr) + \
			    4 /* L2CAP header size */ + \
			    CONFIG_BLUETOOTH_L2CAP_IN_MTU)

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
