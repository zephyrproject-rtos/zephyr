/* buf.h - Bluetooth buffer management */

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

/* The biggest foreseeable buffer size requirement right now comes from
 * the Bluetooth 4.2 SMP MTU which is 65. This then become 65 + 4 (L2CAP
 * header) + 4 (ACL header) + 1 (H4 header) = 74. This also covers the
 * biggest HCI commands and events which are a bit under the 70 byte
 * mark.
 */
#define BT_BUF_MAX_DATA 74

/* Type of data contained in this buffer */
enum bt_buf_type {
	BT_CMD,			/* HCI command */
	BT_EVT,			/* HCI event */
	BT_ACL_OUT,		/* Outgoing ACL data */
	BT_ACL_IN,		/* Incoming ACL data */
	BT_DUMMY = BT_CMD,	/* Only used for waking up fibers */
};

/* HCI command specific info */
struct bt_buf_hci_data {
	/* Used by bt_hci_cmd_send_sync. Initially contains the waiting
	 * semaphore, as the semaphore is given back contains the bt_buf
	 * for the return parameters.
	 */
	void *sync;

	/* The command OpCode that the buffer contains */
	uint16_t opcode;
};

struct bt_buf_acl_data {
	uint16_t handle;
};

struct bt_buf {
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;

	union {
		struct bt_buf_hci_data	hci;
		struct bt_buf_acl_data	acl;
	};

	/* Reference count */
	uint8_t ref;

	/* Type of data contained in the buffer */
	uint8_t type;

	/* Buffer data variables */
	uint8_t len;
	uint8_t *data;
	uint8_t buf[BT_BUF_MAX_DATA];
};

/* Get buffer from the available buffers pool with specified type and
 * reserved headroom.
 */
struct bt_buf *bt_buf_get(enum bt_buf_type type, size_t reserve_head);

/* Place buffer back into the available buffers pool */
void bt_buf_put(struct bt_buf *buf);

/* Increment the reference count of a buffer */
struct bt_buf *bt_buf_hold(struct bt_buf *buf);

/* Prepare data to be added at the end of the buffer */
void *bt_buf_add(struct bt_buf *buf, size_t len);

/* Push data to the beginning of the buffer */
void *bt_buf_push(struct bt_buf *buf, size_t len);

/* Remove data from the beginning of the buffer */
void *bt_buf_pull(struct bt_buf *buf, size_t len);

/* Remove and convert 16 bits from the beginning of the buffer */
uint16_t bt_buf_pull_le16(struct bt_buf *buf);

/* Returns how much free space there is at the end of the buffer */
size_t bt_buf_tailroom(struct bt_buf *buf);

/* Returns how much free space there is at the beginning of the buffer */
size_t bt_buf_headroom(struct bt_buf *buf);

/* Return pointer to the end of the data in the buffer */
#define bt_buf_tail(buf) ((buf)->data + (buf)->len)

/* Initialize the buffers with specified amount of incoming and outgoing
 * ACL buffers. The HCI command and event buffers will be allocated from
 * whatever is left over.
 */
int bt_buf_init(int acl_in, int acl_out);

#endif /* __BT_BUF_H */
