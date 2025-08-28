/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "buf_view.h"
#include "common/hci_common_internal.h"
#include "conn_internal.h"
#include "hci_core.h"
#include "iso_internal.h"

LOG_MODULE_REGISTER(bt_buf, CONFIG_BT_LOG_LEVEL);

/* Events have a length field of 1 byte. This size fits all events.
 *
 * It's true that we don't put all kinds of events there (yet). However, the
 * command complete event has an arbitrary payload, depending on opcode.
 */
#define SYNC_EVT_SIZE (BT_BUF_RESERVE + BT_HCI_EVT_HDR_SIZE + 255)

static bt_buf_rx_freed_cb_t buf_rx_freed_cb;

static void buf_rx_freed_notify(enum bt_buf_type mask)
{
	k_sched_lock();

	if (buf_rx_freed_cb) {
		buf_rx_freed_cb(mask);
	}

	k_sched_unlock();
}

#if defined(CONFIG_BT_ISO_RX)
static void iso_rx_freed_cb(void)
{
	buf_rx_freed_notify(BT_BUF_ISO_IN);
}
#endif

/* Pool for RX HCI buffers that are always freed by `bt_recv`
 * before it returns.
 *
 * A singleton buffer shall be sufficient for correct operation.
 * The buffer count may be increased as an optimization to allow
 * the HCI transport to fill buffers in parallel with `bt_recv`
 * consuming them.
 */
NET_BUF_POOL_FIXED_DEFINE(sync_evt_pool, 1, SYNC_EVT_SIZE, 0, NULL);

NET_BUF_POOL_FIXED_DEFINE(discardable_pool, CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT,
			  BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE),
			  0, NULL);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static void acl_in_pool_destroy(struct net_buf *buf)
{
	bt_hci_host_num_completed_packets(buf);
	buf_rx_freed_notify(BT_BUF_ACL_IN);
}

static void evt_pool_destroy(struct net_buf *buf)
{
	net_buf_destroy(buf);
	buf_rx_freed_notify(BT_BUF_EVT);
}

NET_BUF_POOL_DEFINE(acl_in_pool, (BT_BUF_ACL_RX_COUNT_EXTRA + BT_BUF_HCI_ACL_RX_COUNT),
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_RX_SIZE), sizeof(struct bt_conn_rx),
		    acl_in_pool_destroy);

NET_BUF_POOL_FIXED_DEFINE(evt_pool, CONFIG_BT_BUF_EVT_RX_COUNT, BT_BUF_EVT_RX_SIZE, 0,
			  evt_pool_destroy);
#else
static void hci_rx_pool_destroy(struct net_buf *buf)
{
	net_buf_destroy(buf);

	/* When ACL Flow Control is disabled, a single pool is used for events and acl data.
	 * Therefore the callback will always notify about both types of buffers, BT_BUF_EVT and
	 * BT_BUF_ACL_IN.
	 */
	buf_rx_freed_notify(BT_BUF_EVT | BT_BUF_ACL_IN);
}

NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, BT_BUF_RX_COUNT, BT_BUF_RX_SIZE,
			  sizeof(struct bt_conn_rx), hci_rx_pool_destroy);
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
		net_buf_add_u8(buf, bt_buf_type_to_h4(type));
	}

	return buf;
}

void bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb_t cb)
{
	k_sched_lock();

	buf_rx_freed_cb = cb;

#if defined(CONFIG_BT_ISO_RX)
	bt_iso_buf_rx_freed_cb_set(cb != NULL ? iso_rx_freed_cb : NULL);
#endif

	k_sched_unlock();
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
			/* Discardable, decided in Host-side HCI Transport driver. */
			buf = net_buf_alloc(&discardable_pool, timeout);
		} else {
			return bt_buf_get_rx(BT_BUF_EVT, timeout);
		}
	}

	if (buf) {
		net_buf_add_u8(buf, BT_HCI_H4_EVT);
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
