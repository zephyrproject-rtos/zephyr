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

/* Temporary define to ease porting */
#define bt_buf net_buf

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
struct bt_buf *bt_buf_get(enum bt_buf_type type, size_t reserve_head);

/** @brief Decrements the reference count of a buffer.
 *
 *  Decrements the reference count of a buffer and puts it back into the
 *  pool if the count reaches zero.
 *
 *  @param buf Buffer.
 */
void bt_buf_put(struct bt_buf *buf);

/** Increment the reference count of a buffer.
 *
 *  Increment the reference count of a buffer.
 *
 *  @param buf Buffer.
 */
struct bt_buf *bt_buf_hold(struct bt_buf *buf);

/** @brief Duplicate buffer
 *
 *  Duplicate given buffer including any data and headers currently stored.
 *
 *  @param buf Buffer.
 *
 *  @return Duplicated buffer or NULL if out of buffers.
 */
struct bt_buf *bt_buf_clone(struct bt_buf *buf);

/** @brief Prepare data to be added at the end of the buffer
 *
 *  Increments the data length of a buffer to account for more data
 *  at the end.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to increment the length with.
 *
 *  @return The original tail of the buffer.
 */
void *bt_buf_add(struct bt_buf *buf, size_t len);

/** @brief Add 16-bit value at the end of the buffer
 *
 *  Adds 16-bit value in little endian format at the end of buffer.
 *  Increments the data length of a buffer to account for more data
 *  at the end.
 *
 *  @param buf Buffer to update.
 *  @param value 16-bit value to be added.
 *
 *  @return void
 */
void bt_buf_add_le16(struct bt_buf *buf, uint16_t value);

/** @brief Push data to the beginning of the buffer.
 *
 *  Modifies the data pointer and buffer length to account for more data
 *  in the beginning of the buffer.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to add to the beginning.
 *
 *  @return The new beginning of the buffer data.
 */
void *bt_buf_push(struct bt_buf *buf, size_t len);

/** @brief Remove data from the beginning of the buffer.
 *
 *  Removes data from the beginnig of the buffer by modifying the data
 *  pointer and buffer length.
 *
 *  @param buf Buffer to update.
 *  @param len Number of bytes to remove.
 *
 *  @return New beginning of the buffer data.
 */
void *bt_buf_pull(struct bt_buf *buf, size_t len);

/** @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 *  Same idea as with bt_buf_pull(), but a helper for operating on
 *  16-bit little endian data.
 *
 *  @param buf Buffer.
 *
 *  @return 16-bit value converted from little endian to host endian.
 */
uint16_t bt_buf_pull_le16(struct bt_buf *buf);

/** @brief Check buffer tailroom.
 *
 *  Check how much free space there is at the end of the buffer.
 *
 *  @return Number of bytes available at the end of the buffer.
 */
size_t bt_buf_tailroom(struct bt_buf *buf);

/** @brief Check buffer headroom.
 *
 *  Check how much free space there is in the beginning of the buffer.
 *
 *  @return Number of bytes available in the beginning of the buffer.
 */
size_t bt_buf_headroom(struct bt_buf *buf);

/** @def bt_buf_tail
 *  @brief Get the tail pointer for a buffer.
 *
 *  Get a pointer to the end of the data in a buffer.
 *
 *  @param buf Buffer.
 *
 *  @return Tail pointer for the buffer.
 */
void *bt_buf_tail(struct bt_buf *buf);

/** @brief Initialize buffer handling.
 *
 *  Initialize the HCI and ACL buffers.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_buf_init(void);

#endif /* __BT_BUF_H */
