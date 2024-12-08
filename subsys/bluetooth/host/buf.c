/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/l2cap.h>

#include "buf_view.h"
#include "hci_core.h"
#include "conn_internal.h"
#include "iso_internal.h"

#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_buf, CONFIG_BT_LOG_LEVEL);

/* Events have a length field of 1 byte. This size fits all events.
 *
 * It's true that we don't put all kinds of events there (yet). However, the
 * command complete event has an arbitrary payload, depending on opcode.
 */
#define SYNC_EVT_SIZE (BT_BUF_RESERVE + BT_HCI_EVT_HDR_SIZE + 255)

/* Pool for RX HCI buffers that are always freed by `bt_recv`
 * before it returns.
 *
 * A singleton buffer shall be sufficient for correct operation.
 * The buffer count may be increased as an optimization to allow
 * the HCI transport to fill buffers in parallel with `bt_recv`
 * consuming them.
 */
NET_BUF_POOL_FIXED_DEFINE(sync_evt_pool, 1, SYNC_EVT_SIZE, sizeof(struct bt_buf_data), NULL);

NET_BUF_POOL_FIXED_DEFINE(discardable_pool, CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT,
			  BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE),
			  sizeof(struct bt_buf_data), NULL);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
NET_BUF_POOL_DEFINE(acl_in_pool, BT_BUF_ACL_RX_COUNT,
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_RX_SIZE),
		    sizeof(struct acl_data), bt_hci_host_num_completed_packets);

NET_BUF_POOL_FIXED_DEFINE(evt_pool, CONFIG_BT_BUF_EVT_RX_COUNT,
			  BT_BUF_EVT_RX_SIZE, sizeof(struct bt_buf_data),
			  NULL);
#else
NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, BT_BUF_RX_COUNT,
			  BT_BUF_RX_SIZE, sizeof(struct acl_data),
			  NULL);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN ||
		 type == BT_BUF_ISO_IN, "Invalid buffer type requested");

	if (IS_ENABLED(CONFIG_BT_ISO_RX) && type == BT_BUF_ISO_IN) {
		return bt_iso_get_rx(timeout);
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&evt_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable,
			       k_timeout_t timeout)
{
	struct net_buf *buf;

	switch (evt) {
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_ISO)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
#endif /* CONFIG_BT_CONN || CONFIG_BT_ISO */
	case BT_HCI_EVT_CMD_STATUS:
	case BT_HCI_EVT_CMD_COMPLETE:
		buf = net_buf_alloc(&sync_evt_pool, timeout);
		break;
	default:
		if (discardable) {
			buf = net_buf_alloc(&discardable_pool, timeout);
		} else {
			return bt_buf_get_rx(BT_BUF_EVT, timeout);
		}
	}

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_EVT);
	}

	return buf;
}

#ifdef ZTEST_UNITTEST
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
struct net_buf_pool *bt_buf_get_evt_pool(void)
{
	return &evt_pool;
}

struct net_buf_pool *bt_buf_get_acl_in_pool(void)
{
	return &acl_in_pool;
}
#else
struct net_buf_pool *bt_buf_get_hci_rx_pool(void)
{
	return &hci_rx_pool;
}
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

#if defined(CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT)
struct net_buf_pool *bt_buf_get_discardable_pool(void)
{
	return &discardable_pool;
}
#endif /* CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT */

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_ISO)
struct net_buf_pool *bt_buf_get_num_complete_pool(void)
{
	return &sync_evt_pool;
}
#endif /* CONFIG_BT_CONN || CONFIG_BT_ISO */
#endif /* ZTEST_UNITTEST */

struct net_buf *bt_buf_make_view(struct net_buf *view,
				 struct net_buf *parent,
				 size_t len,
				 struct bt_buf_view_meta *meta)
{
	__ASSERT_NO_MSG(len);
	__ASSERT_NO_MSG(view);
	/* The whole point of this API is to allow prepending data. If the
	 * headroom is 0, that will not happen.
	 */
	__ASSERT_NO_MSG(net_buf_headroom(parent) > 0);

	__ASSERT_NO_MSG(!bt_buf_has_view(parent));

	LOG_DBG("make-view %p viewsize %zu meta %p", view, len, meta);

	net_buf_simple_clone(&parent->b, &view->b);
	view->size = net_buf_headroom(parent) + len;
	view->len = len;
	view->flags = NET_BUF_EXTERNAL_DATA;

	/* we have a view, eat `len`'s worth of data from the parent */
	(void)net_buf_pull(parent, len);

	meta->backup.data = parent->data;
	parent->data = NULL;

	meta->backup.size = parent->size;
	parent->size = 0;

	/* The ref to `parent` is moved in by passing `parent` as argument. */
	/* save backup & "clip" the buffer so the next `make_view` will fail */
	meta->parent = parent;
	parent = NULL;

	return view;
}

void bt_buf_destroy_view(struct net_buf *view, struct bt_buf_view_meta *meta)
{
	LOG_DBG("destroy-view %p meta %p", view, meta);
	__ASSERT_NO_MSG(meta->parent);

	/* "unclip" the parent buf */
	meta->parent->data = meta->backup.data;
	meta->parent->size = meta->backup.size;

	net_buf_unref(meta->parent);

	memset(meta, 0, sizeof(*meta));
	net_buf_destroy(view);
}

bool bt_buf_has_view(const struct net_buf *parent)
{
	/* This is enforced by `make_view`. see comment there. */
	return parent->size == 0 && parent->data == NULL;
}
