/* main.c - Application main entry point */

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

#include <stdint.h>
#include <stddef.h>
#include <misc/printk.h>

#include <net/buf.h>

struct bt_data {
	void *hci_sync;

	union {
		uint16_t hci_opcode;
		uint16_t acl_handle;
	};

	uint8_t type;
};

static int destroy_called;

static struct nano_fifo bufs_fifo;

static void buf_destroy(struct net_buf *buf)
{
	destroy_called++;

	if (buf->free != &bufs_fifo) {
		printk("Invalid free pointer in buffer!\n");
	}
}

static NET_BUF_POOL(bufs_pool, 22, 74, &bufs_fifo, buf_destroy,
		    sizeof(struct bt_data));

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	struct net_buf *bufs[ARRAY_SIZE(bufs_pool)];
	int i;

	printk("sizeof(struct net_buf) = %u\n", sizeof(struct net_buf));
	printk("sizeof(bufs_pool)      = %u\n", sizeof(bufs_pool));

	net_buf_pool_init(bufs_pool);

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		struct net_buf *buf;

		buf = net_buf_get(&bufs_fifo, 0);
		if (!buf) {
			printk("Failed to get buffer!\n");
			return;
		}

		bufs[i] = buf;
	}

	for (i = 0; i < ARRAY_SIZE(bufs_pool); i++) {
		net_buf_unref(bufs[i]);
	}

	if (destroy_called != ARRAY_SIZE(bufs_pool)) {
		printk("Incorrect destroy callback count: %d\n",
		       destroy_called);
		return;
	}

	printk("Buffer tests passed\n");
}
