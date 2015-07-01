/** @file
 *  @brief Bluetooth buffer management.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_BUF_H
#define __BT_BUF_H

#include <stddef.h>
#include <stdint.h>

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

/** Type of data contained in a buffer */
enum bt_buf_type {
	BT_CMD,			/** HCI command */
	BT_EVT,			/** HCI event */
	BT_ACL_OUT,		/** Outgoing ACL data */
	BT_ACL_IN,		/** Incoming ACL data */
	BT_DUMMY = BT_CMD,	/** Only used for waking up fibers */
};

/** HCI command specific information */
struct bt_buf_hci_data {
	/** Used by bt_hci_cmd_send_sync. Initially contains the waiting
	 *  semaphore, as the semaphore is given back contains the bt_buf
	 *  for the return parameters.
	 */
	void *sync;

	/** The command OpCode that the buffer contains */
	uint16_t opcode;
};

/** ACL data buffer specific information */
struct bt_buf_acl_data {
	uint16_t handle;
};

struct bt_buf {
	/** FIFO uses first 4 bytes itself, reserve space */
	int __unused;

	union {
		struct bt_buf_hci_data	hci;
		struct bt_buf_acl_data	acl;
	};

	/** Pointer to the start of data in the buffer. */
	uint8_t *data;

	/** Length of the data behind the data pointer. */
	uint8_t len;

	uint8_t ref:5,   /** Reference count */
	        type:3;  /** Type of data contained in the buffer */

	/** The full available buffer. */
	uint8_t buf[BT_BUF_MAX_DATA];
};

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
#define bt_buf_tail(buf) ((buf)->data + (buf)->len)

/** @brief Initialize buffer handling.
 *
 *  Initialize the buffers with specified amount of incoming and outgoing
 *  ACL buffers. The HCI command and event buffers will be allocated from
 *  whatever is left over.
 *
 *  @param acl_in Number of incoming ACL data buffers.
 *  @param acl_out Number of outgoing ACL data buffers.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_buf_init(int acl_in, int acl_out);

#endif /* __BT_BUF_H */
