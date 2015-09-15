/* buf.c - Bluetooth buffer management */

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

#include <nanokernel.h>
#include <toolchain.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <atomic.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/buf.h>

#include "hci_core.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_BUF)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* Total number of all types of buffers */
#define NUM_BUFS		22
static struct bt_buf		buffers[NUM_BUFS];

/* Available (free) buffers queues */
static struct nano_fifo		avail_hci;
static struct nano_fifo		avail_acl_in;
static struct nano_fifo		avail_acl_out;

static struct nano_fifo *get_avail(enum bt_buf_type type)
{
	switch (type) {
	case BT_CMD:
	case BT_EVT:
		return &avail_hci;
	case BT_ACL_IN:
		return &avail_acl_in;
	case BT_ACL_OUT:
		return &avail_acl_out;
	default:
		return NULL;
	}
}

struct bt_buf *bt_buf_get(enum bt_buf_type type, size_t reserve_head)
{
	struct nano_fifo *avail = get_avail(type);
	struct bt_buf *buf;

	BT_DBG("type %d reserve %u\n", type, reserve_head);

	buf = nano_fifo_get(avail);
	if (!buf) {
		if (sys_execution_context_type_get() == NANO_CTX_ISR) {
			BT_ERR("Failed to get free buffer\n");
			return NULL;
		}

		BT_WARN("Low on buffers. Waiting (type %d)\n", type);
		buf = nano_fifo_get_wait(avail);
	}

	memset(buf, 0, sizeof(*buf));

	buf->ref  = 1;
	buf->type = type;
	buf->data = buf->buf + reserve_head;

	BT_DBG("buf %p type %d reserve %u\n", buf, buf->type, reserve_head);

	return buf;
}

void bt_buf_put(struct bt_buf *buf)
{
	struct bt_hci_cp_host_num_completed_packets *cp;
	struct bt_hci_handle_count *hc;
	struct nano_fifo *avail = get_avail(buf->type);
	uint16_t handle;

	BT_DBG("buf %p ref %u type %d\n", buf, buf->ref, buf->type);

	if (--buf->ref) {
		return;
	}

	handle = buf->acl.handle;
	nano_fifo_put(avail, buf);

	if (avail != &avail_acl_in) {
		return;
	}

	BT_DBG("Reporting completed packet for handle %u\n", handle);

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS,
				sizeof(*cp) + sizeof(*hc));
	if (!buf) {
		BT_ERR("Unable to allocate new HCI command\n");
		return;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->num_handles = sys_cpu_to_le16(1);

	hc = bt_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(handle);
	hc->count  = sys_cpu_to_le16(1);

	bt_hci_cmd_send(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS, buf);
}

struct bt_buf *bt_buf_hold(struct bt_buf *buf)
{
	BT_DBG("buf %p (old) ref %u type %d\n", buf, buf->ref, buf->type);
	buf->ref++;
	return buf;
}

struct bt_buf *bt_buf_clone(struct bt_buf *buf)
{
	struct bt_buf *clone;

	clone = bt_buf_get(buf->type, bt_buf_headroom(buf));
	if (!clone) {
		return NULL;
	}

	/* TODO: Add reference to the original buffer instead of copying it. */
	memcpy(bt_buf_add(clone, buf->len), buf->data, buf->len);

	return clone;
}

void *bt_buf_add(struct bt_buf *buf, size_t len)
{
	uint8_t *tail = bt_buf_tail(buf);

	BT_DBG("buf %p len %u\n", buf, len);

	BT_ASSERT(bt_buf_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void bt_buf_add_le16(struct bt_buf *buf, uint16_t value)
{
	BT_DBG("buf %p value %u\n", buf, value);

	value = sys_cpu_to_le16(value);
	memcpy(bt_buf_add(buf, sizeof(value)), &value, sizeof(value));
}

void *bt_buf_push(struct bt_buf *buf, size_t len)
{
	BT_DBG("buf %p len %u\n", buf, len);

	BT_ASSERT(bt_buf_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void *bt_buf_pull(struct bt_buf *buf, size_t len)
{
	BT_DBG("buf %p len %u\n", buf, len);

	BT_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

uint16_t bt_buf_pull_le16(struct bt_buf *buf)
{
	uint16_t value;

	value = UNALIGNED_GET((uint16_t *)buf->data);
	bt_buf_pull(buf, sizeof(value));

	return sys_le16_to_cpu(value);
}

size_t bt_buf_headroom(struct bt_buf *buf)
{
	return buf->data - buf->buf;
}

size_t bt_buf_tailroom(struct bt_buf *buf)
{
	return BT_BUF_MAX_DATA - bt_buf_headroom(buf) - buf->len;
}

int bt_buf_init(int acl_in, int acl_out)
{
	int i;

	/* Check that we have enough buffers configured */
	if (acl_out + acl_in >= NUM_BUFS - 2) {
		BT_ERR("Too many ACL buffers requested\n");
		return -EINVAL;
	}

	BT_DBG("Available bufs: ACL in: %d, ACL out: %d, cmds/evts: %d\n",
	       acl_in, acl_out, NUM_BUFS - (acl_in + acl_out));

	nano_fifo_init(&avail_acl_in);
	for (i = 0; acl_in > 0; i++, acl_in--) {
		nano_fifo_put(&avail_acl_in, &buffers[i]);
	}

	nano_fifo_init(&avail_acl_out);
	for (; acl_out > 0; i++, acl_out--) {
		nano_fifo_put(&avail_acl_out, &buffers[i]);
	}

	nano_fifo_init(&avail_hci);
	for (; i < NUM_BUFS; i++) {
		nano_fifo_put(&avail_hci, &buffers[i]);
	}

	BT_DBG("%u buffers * %u bytes = %u bytes\n", NUM_BUFS,
	       sizeof(buffers[0]), sizeof(buffers));

	return 0;
}
