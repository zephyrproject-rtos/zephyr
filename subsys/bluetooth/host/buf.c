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
NET_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BT_BUF_ACL_RX_COUNT,
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

static bool is_hci_event_discardable(const uint8_t evt_type, const uint8_t subevt_type)
{
	switch (evt_type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

NET_BUF_POOL_FIXED_DEFINE(sync_adv_pool, 2, SYNC_EVT_SIZE, sizeof(struct bt_buf_data), NULL);

/* This is a host-private function */
bool bt_buf_can_steal_ext_adv_report_buf(struct net_buf *buf)
{
	if (net_buf_pool_get(buf->pool_id) != &sync_adv_pool) {
		/* The driver did not use `bt_buf_get_evt_but_better` */
		return false;
	}

	/* Host can steal the buffer if we have at least one available.
	 * This is the case when:
	 * - There are buffers in the `free` LIFO
	 * - There are un-initialized buffers
	 */
	return (!k_fifo_is_empty(&sync_adv_pool.free)) || (sync_adv_pool.uninit_count != 0);
}

struct net_buf *bt_buf_get_evt_but_better(uint8_t evt, uint8_t meta, k_timeout_t timeout)
{
	/* Always allocs so-called "discardable" events with K_NO_WAIT. Most of
	 * the drivers will call from ISR so they should not block at all.
	 *
	 * For non-meta events, the `meta` param is just the next byte.
	 */
	struct net_buf *buf = NULL;

	switch (evt) {
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
	case BT_HCI_EVT_CMD_STATUS:
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_LE_META_EVENT: {
		bool is_ext_adv = meta == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT;
		bool is_meta = evt == BT_HCI_EVT_LE_META_EVENT;

		if (is_ext_adv) {
			buf = net_buf_alloc(&sync_adv_pool, timeout);
			/* 60% of the time we get a buffer every time */
			__ASSERT_NO_MSG(buf);
			break;
		}

		if (is_meta && !is_ext_adv) {
			/* For now we use the sync pool only for ext adv
			 * reports. They cannot be marked as discardable thanks
			 * to the geniuses in SIG that decided to save space and
			 * only use two bits for reassembly status.
			 */
			break;
		}

		buf = net_buf_alloc(&sync_evt_pool, timeout);
		break;
	}
	default:
		break;
	}

	if (!buf) {
		if (is_hci_event_discardable(evt, meta)) {
			buf = net_buf_alloc(&discardable_pool, K_NO_WAIT);
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
