/* buf.c - Bluetooth buffer management */

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

/* Available (free) buffers queues */
static struct nano_fifo		avail_hci;
static NET_BUF_POOL(hci_pool, 8, BT_BUF_MAX_DATA, &avail_hci, NULL,
		    sizeof(struct bt_hci_data));

#if defined(CONFIG_BLUETOOTH_CONN)
static void report_completed_packet(struct net_buf *buf)
{

	struct bt_acl_data *acl = net_buf_user_data(buf);
	struct bt_hci_cp_host_num_completed_packets *cp;
	struct bt_hci_handle_count *hc;
	uint16_t handle;

	handle = acl->handle;

	BT_DBG("Reporting completed packet for handle %u\n", handle);

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS,
				sizeof(*cp) + sizeof(*hc));
	if (!buf) {
		BT_ERR("Unable to allocate new HCI command\n");
		return;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->num_handles = sys_cpu_to_le16(1);

	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(handle);
	hc->count  = sys_cpu_to_le16(1);

	bt_hci_cmd_send(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS, buf);
}

static struct nano_fifo		avail_acl_in;
static struct nano_fifo		avail_acl_out;
static NET_BUF_POOL(acl_in_pool, BT_BUF_ACL_IN_MAX, BT_BUF_MAX_DATA,
		    &avail_acl_in, report_completed_packet,
		    sizeof(struct bt_acl_data));
static NET_BUF_POOL(acl_out_pool, BT_BUF_ACL_OUT_MAX, BT_BUF_MAX_DATA,
		    &avail_acl_out, NULL, sizeof(struct bt_acl_data));
#endif /* CONFIG_BLUETOOTH_CONN */

struct bt_buf *bt_buf_get(enum bt_buf_type type, size_t reserve_head)
{
	struct net_buf *buf;

	switch (type) {
	case BT_CMD:
	case BT_EVT:
		buf = net_buf_get(&avail_hci, reserve_head);
		break;
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_ACL_IN:
		buf = net_buf_get(&avail_acl_in, reserve_head);
		break;
	case BT_ACL_OUT:
		buf = net_buf_get(&avail_acl_out, reserve_head);
		break;
#endif /* CONFIG_BLUETOOTH_CONN */
	default:
		return NULL;
	}

	if (buf) {
		uint8_t *buf_type = net_buf_user_data(buf);
		*buf_type = type;
	}

	return buf;
}

void bt_buf_put(struct bt_buf *buf)
{
	net_buf_unref(buf);
}

struct bt_buf *bt_buf_hold(struct bt_buf *buf)
{
	return net_buf_ref(buf);
}

struct bt_buf *bt_buf_clone(struct bt_buf *buf)
{
	return net_buf_clone(buf);
}

void *bt_buf_add(struct bt_buf *buf, size_t len)
{
	return net_buf_add(buf, len);
}

void bt_buf_add_le16(struct bt_buf *buf, uint16_t value)
{
	net_buf_add_le16(buf, value);
}

void *bt_buf_push(struct bt_buf *buf, size_t len)
{
	return net_buf_push(buf, len);
}

void *bt_buf_pull(struct bt_buf *buf, size_t len)
{
	return net_buf_pull(buf, len);
}

uint16_t bt_buf_pull_le16(struct bt_buf *buf)
{
	return net_buf_pull_le16(buf);
}

size_t bt_buf_headroom(struct bt_buf *buf)
{
	return net_buf_headroom(buf);
}

size_t bt_buf_tailroom(struct bt_buf *buf)
{
	return net_buf_tailroom(buf);
}

void *bt_buf_tail(struct bt_buf *buf)
{
	return net_buf_tail(buf);
}

int bt_buf_init(void)
{
#if defined(CONFIG_BLUETOOTH_CONN)
	net_buf_pool_init(acl_in_pool);
	net_buf_pool_init(acl_out_pool);
#endif /* CONFIG_BLUETOOTH_CONN */

	net_buf_pool_init(hci_pool);

	return 0;
}
