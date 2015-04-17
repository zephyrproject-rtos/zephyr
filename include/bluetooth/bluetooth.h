/* bluetooth.h - HCI core Bluetooth definitions */

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
#ifndef __BT_BLUETOOTH_H
#define __BT_BLUETOOTH_H

#include <misc/printk.h>

/* Bluetooth subsystem logging helpers */

#define BT_DBG(fmt, ...) printk("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) printk("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) printk("bt: " fmt,  ##__VA_ARGS__)

/* Data buffer API - used for all data to/from HCI */

/* The biggest foreseeable buffer size requirement right now comes from
 * the Bluetooth 4.2 SMP MTU which is 65. This then become 65 + 4 (L2CAP
 * header) + 4 (ACL header) + 1 (H4 header) = 74. This also covers the
 * biggest HCI commands and events which are a bit under the 70 byte
 * mark.
 */
#define BT_BUF_MAX_DATA 74

/* Type of data contained in this buffer */
enum bt_buf_type {
	BT_CMD,
	BT_EVT,
	BT_ACL,
};

struct bt_buf {
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;

	/* HCI command specific info */
	struct nano_sem *sync;
	uint16_t opcode;

	/* Type of data contained in the buffer */
	uint8_t type;

	/* Buffer data variables */
	uint8_t len;
	uint8_t *data;
	uint8_t buf[BT_BUF_MAX_DATA];
};

/* Get buffer from the available buffers pool */
struct bt_buf *bt_buf_get(void);

/* Same as bt_buf_get, but also reserve headroom for potential headers */
struct bt_buf *bt_buf_get_reserve(size_t reserve_head);

/* Place buffer back into the available buffers pool */
void bt_buf_put(struct bt_buf *buf);

/* Prepare data to be added at the end of the buffer */
uint8_t *bt_buf_add(struct bt_buf *buf, size_t len);

/* Push data to the beginning of the buffer */
uint8_t *bt_buf_push(struct bt_buf *buf, size_t len);

/* Remove data from the beginning of the buffer */
uint8_t *bt_buf_pull(struct bt_buf *buf, size_t len);

/* Returns how much free space there is at the end of the buffer */
size_t bt_buf_tailroom(struct bt_buf *buf);

/* Returns how much free space there is at the beginning of the buffer */
size_t bt_buf_headroom(struct bt_buf *buf);

/* Return pointer to the end of the data in the buffer */
#define bt_buf_tail(buf) ((buf)->data + (buf)->len)

/* HCI control APIs */

/* Reset the state of the controller (i.e. perform full HCI init */
int bt_hci_reset(void);

/* Initialize Bluetooth. Must be the called before anything else. */
int bt_init(void);

/* HCI driver API */

/* Receive data from the controller/HCI driver */
void bt_recv(struct bt_buf *buf);

struct bt_driver {
	/* How much headroom is needed for HCI transport headers */
	size_t head_reserve;

	/* Open the HCI transport */
	int (*open) (void);

	/* Send data to HCI */
	int (*send) (struct bt_buf *buf);
};

/* Register a new HCI driver to the Bluetooth stack */
int bt_driver_register(struct bt_driver *drv);

/* Unregister a previously registered HCI driver */
void bt_driver_unregister(struct bt_driver *drv);

/* Advertising testing API */
int bt_start_advertising(uint8_t type, const char *name, uint8_t name_len);

#endif /* __BT_BLUETOOTH_H */
