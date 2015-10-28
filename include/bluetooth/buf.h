/** @file
 *  @brief Bluetooth buffer management.
 */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <stddef.h>
#include <stdint.h>
#include <net/buf.h>

/** @def BT_BUF_MAX_DATA
 *  @brief Maximum amount of data that can fit in a buffer.
 *
 *  The biggest foreseeable buffer size requirement right now comes from
 *  the Bluetooth 4.2 SMP MTU which is 65. This then become 65 + 4 (L2CAP
 *  header) + 4 (ACL header) + 1 (H4 header) = 74. This also covers the
 *  biggest HCI commands and events which are a bit under the 70 byte
 *  mark.
 */
#define BT_BUF_MAX_DATA 74

#define BT_BUF_ACL_IN_MAX  7
#define BT_BUF_ACL_OUT_MAX 7

enum bt_buf_type {
	BT_CMD,			/** HCI command */
	BT_EVT,			/** HCI event */
	BT_ACL_OUT,		/** Outgoing ACL data */
	BT_ACL_IN,		/** Incoming ACL data */
	BT_DUMMY = BT_CMD,	/** Only used for waking up fibers */
};

struct bt_hci_data {
	/** Type of data contained in a buffer (bt_buf_type) */
	uint8_t type;

	/** The command OpCode that the buffer contains */
	uint16_t opcode;

	/** Used by bt_hci_cmd_send_sync. Initially contains the waiting
	 *  semaphore, as the semaphore is given back contains the bt_buf
	 *  for the return parameters.
	 */
	void *sync;

};
struct bt_acl_data {
	/** Type of data contained in a buffer (bt_buf_type) */
	uint8_t type;

	/** ACL connection handle */
	uint16_t handle;
};

#define bt_hci(buf) ((struct bt_hci_data *)net_buf_user_data(buf))
#define bt_acl(buf) ((struct bt_acl_data *)net_buf_user_data(buf))
#define bt_type(buf) (*((uint8_t *)net_buf_user_data(buf)))

/** @brief Get a new buffer from the pool.
 *
 *  Get buffer from the available buffers pool with specified type and
 *  reserved headroom.
 *
 *  @param type Buffer type.
 *  @param reserve_head How much headroom to reserve.
 *
 *  @return New buffer or NULL if out of buffers.
 *
 *  @warning If there are no available buffers and the function is
 *  called from a task or fiber the call will block until a buffer
 *  becomes available in the pool.
 */
struct net_buf *bt_buf_get(enum bt_buf_type type, size_t reserve_head);

/** @brief Initialize buffer handling.
 *
 *  Initialize the HCI and ACL buffers.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_buf_init(void);

#endif /* __BT_BUF_H */
