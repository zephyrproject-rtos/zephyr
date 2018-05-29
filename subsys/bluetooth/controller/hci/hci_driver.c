/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr.h>
#include <soc.h>
#include <init.h>
#include <device.h>
#include <clock_control.h>
#include <atomic.h>

#include <misc/util.h>
#include <misc/stack.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#ifdef CONFIG_CLOCK_CONTROL_NRF5
#include <drivers/clock_control/nrf5_clock_control.h>
#endif

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#include "util/util.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll_sw/pdu.h"
#include "ll_sw/ctrl.h"
#include "ll.h"
#include "hci_internal.h"

#include "hal/debug.h"

#define NODE_RX(_node) CONTAINER_OF(_node, struct radio_pdu_node_rx, \
				    hdr.onion.node)

static K_SEM_DEFINE(sem_prio_recv, 0, UINT_MAX);
static K_FIFO_DEFINE(recv_fifo);

struct k_thread prio_recv_thread_data;
static BT_STACK_NOINIT(prio_recv_thread_stack,
		       CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE);
struct k_thread recv_thread_data;
static BT_STACK_NOINIT(recv_thread_stack, CONFIG_BT_RX_STACK_SIZE);

#if defined(CONFIG_INIT_STACKS)
static u32_t prio_ts;
static u32_t rx_ts;
#endif

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static struct k_poll_signal hbuf_signal =
		K_POLL_SIGNAL_INITIALIZER(hbuf_signal);
static sys_slist_t hbuf_pend;
static s32_t hbuf_count;
#endif

static void prio_recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		void *node_rx;
		u8_t num_cmplt;
		u16_t handle;

		while ((num_cmplt = ll_rx_get(&node_rx, &handle))) {
#if defined(CONFIG_BT_CONN)
			struct net_buf *buf;

			buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
			hci_num_cmplt_encode(buf, handle, num_cmplt);
			BT_DBG("Num Complete: 0x%04x:%u", handle, num_cmplt);
			bt_recv_prio(buf);
			k_yield();
#endif
		}

		if (node_rx) {

			ll_rx_dequeue();

			BT_DBG("RX node enqueue");
			k_fifo_put(&recv_fifo, node_rx);

			continue;
		}

		BT_DBG("sem take...");
		k_sem_take(&sem_prio_recv, K_FOREVER);
		BT_DBG("sem taken");

#if defined(CONFIG_INIT_STACKS)
		if (k_uptime_get_32() - prio_ts > K_SECONDS(5)) {
			STACK_ANALYZE("prio recv thread stack",
				      prio_recv_thread_stack);
			prio_ts = k_uptime_get_32();
		}
#endif
	}
}

static inline struct net_buf *encode_node(struct radio_pdu_node_rx *node_rx,
					  s8_t class)
{
	struct net_buf *buf = NULL;

	/* Check if we need to generate an HCI event or ACL data */
	switch (class) {
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	case HCI_CLASS_EVT_CONNECTION:
		if (class == HCI_CLASS_EVT_DISCARDABLE) {
			buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
		} else {
			buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
		}
		if (buf) {
			hci_evt_encode(node_rx, buf);
		}
		break;
#if defined(CONFIG_BT_CONN)
	case HCI_CLASS_ACL_DATA:
		/* generate ACL data */
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		hci_acl_encode(node_rx, buf);
		break;
#endif
	default:
		LL_ASSERT(0);
		break;
	}

#if defined(CONFIG_BT_LL_SW)
	radio_rx_fc_set(node_rx->hdr.handle, 0);
#endif /* CONFIG_BT_LL_SW */

	node_rx->hdr.onion.next = 0;
	ll_rx_mem_release((void **)&node_rx);

	return buf;
}

static inline struct net_buf *process_node(struct radio_pdu_node_rx *node_rx)
{
	s8_t class = hci_get_class(node_rx);
	struct net_buf *buf = NULL;

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (hbuf_count != -1) {
		bool pend = !sys_slist_is_empty(&hbuf_pend);

		/* controller to host flow control enabled */
		switch (class) {
		case HCI_CLASS_EVT_DISCARDABLE:
		case HCI_CLASS_EVT_REQUIRED:
			break;
		case HCI_CLASS_EVT_CONNECTION:
			/* for conn-related events, only pend is relevant */
			hbuf_count = 1;
			/* fallthrough */
		case HCI_CLASS_ACL_DATA:
			if (pend || !hbuf_count) {
				sys_slist_append(&hbuf_pend,
						 &node_rx->hdr.onion.node);
				BT_DBG("FC: Queuing item: %d", class);
				return NULL;
			}
			break;
		default:
			LL_ASSERT(0);
			break;
		}
	}
#endif

	/* process regular node from radio */
	buf = encode_node(node_rx, class);

	return buf;
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static inline struct net_buf *process_hbuf(struct radio_pdu_node_rx *n)
{
	/* shadow total count in case of preemption */
	struct radio_pdu_node_rx *node_rx = NULL;
	s32_t hbuf_total = hci_hbuf_total;
	struct net_buf *buf = NULL;
	sys_snode_t *node = NULL;
	s8_t class;
	int reset;

	reset = atomic_test_and_clear_bit(&hci_state_mask, HCI_STATE_BIT_RESET);
	if (reset) {
		/* flush queue, no need to free, the LL has already done it */
		sys_slist_init(&hbuf_pend);
	}

	if (hbuf_total <= 0) {
		hbuf_count = -1;
		return NULL;
	}

	/* available host buffers */
	hbuf_count = hbuf_total - (hci_hbuf_sent - hci_hbuf_acked);

	/* host acked ACL packets, try to dequeue from hbuf */
	node = sys_slist_peek_head(&hbuf_pend);
	if (!node) {
		return NULL;
	}

	/* Return early if this iteration already has a node to process */
	node_rx = NODE_RX(node);
	class = hci_get_class(node_rx);
	if (n) {
		if (class == HCI_CLASS_EVT_CONNECTION ||
		    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
			/* node to process later, schedule an iteration */
			BT_DBG("FC: signalling");
			k_poll_signal(&hbuf_signal, 0x0);
		}
		return NULL;
	}

	switch (class) {
	case HCI_CLASS_EVT_CONNECTION:
		BT_DBG("FC: dequeueing event");
		(void) sys_slist_get(&hbuf_pend);
		break;
	case HCI_CLASS_ACL_DATA:
		if (hbuf_count) {
			BT_DBG("FC: dequeueing ACL data");
			(void) sys_slist_get(&hbuf_pend);
		} else {
			/* no buffers, HCI will signal */
			node = NULL;
		}
		break;
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	default:
		LL_ASSERT(0);
		break;
	}

	if (node) {
		buf = encode_node(node_rx, class);
		/* Update host buffers after encoding */
		hbuf_count = hbuf_total - (hci_hbuf_sent - hci_hbuf_acked);
		/* next node */
		node = sys_slist_peek_head(&hbuf_pend);
		if (node) {
			node_rx = NODE_RX(node);
			class = hci_get_class(node_rx);

			if (class == HCI_CLASS_EVT_CONNECTION ||
			    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
				/* more to process, schedule an
				 * iteration
				 */
				BT_DBG("FC: signalling");
				k_poll_signal(&hbuf_signal, 0x0);
			}
		}
	}

	return buf;
}
#endif

static void recv_thread(void *p1, void *p2, void *p3)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* @todo: check if the events structure really needs to be static */
	static struct k_poll_event events[2] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SIGNAL,
						K_POLL_MODE_NOTIFY_ONLY,
						&hbuf_signal, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&recv_fifo, 0),
	};
#endif

	while (1) {
		struct radio_pdu_node_rx *node_rx = NULL;
		struct net_buf *buf = NULL;

		BT_DBG("blocking");
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
		int err;

		err = k_poll(events, 2, K_FOREVER);
		LL_ASSERT(err == 0);
		if (events[0].state == K_POLL_STATE_SIGNALED) {
			events[0].signal->signaled = 0;
		} else if (events[1].state ==
			   K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			node_rx = k_fifo_get(events[1].fifo, 0);
		}

		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;

		/* process host buffers first if any */
		buf = process_hbuf(node_rx);

#else
		node_rx = k_fifo_get(&recv_fifo, K_FOREVER);
#endif
		BT_DBG("unblocked");

		if (node_rx && !buf) {
			/* process regular node from radio */
			buf = process_node(node_rx);
		}

		if (buf) {
			if (buf->len) {
				BT_DBG("Packet in: type:%u len:%u",
					bt_buf_get_type(buf), buf->len);
				bt_recv(buf);
			} else {
				net_buf_unref(buf);
			}
		}

		k_yield();

#if defined(CONFIG_INIT_STACKS)
		if (k_uptime_get_32() - rx_ts > K_SECONDS(5)) {
			STACK_ANALYZE("recv thread stack", recv_thread_stack);
			rx_ts = k_uptime_get_32();
		}
#endif
	}
}

static int cmd_handle(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *evt;

	evt = hci_cmd_handle(buf);
	if (evt) {
		hdr = (void *)evt->data;
		BT_DBG("Replying with event of %u bytes", evt->len);
		if (unlikely(!bt_hci_evt_is_prio(hdr->evt))) {
			bt_recv(evt);
		} else {
			bt_recv_prio(evt);
		}
	}

	return 0;
}

#if defined(CONFIG_BT_CONN)
static int acl_handle(struct net_buf *buf)
{
	struct net_buf *evt;
	int err;

	err = hci_acl_handle(buf, &evt);
	if (evt) {
		BT_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);
	}

	return err;
}
#endif /* CONFIG_BT_CONN */

static int hci_driver_send(struct net_buf *buf)
{
	u8_t type;
	int err;

	BT_DBG("enter");

	if (!buf->len) {
		BT_ERR("Empty HCI packet");
		return -EINVAL;
	}

	type = bt_buf_get_type(buf);
	switch (type) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_OUT:
		err = acl_handle(buf);
		break;
#endif /* CONFIG_BT_CONN */
	case BT_BUF_CMD:
		err = cmd_handle(buf);
		break;
	default:
		BT_ERR("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (!err) {
		net_buf_unref(buf);
	}

	BT_DBG("exit: %d", err);

	return err;
}

static int hci_driver_open(void)
{
	u32_t err;

	DEBUG_INIT();

	err = ll_init(&sem_prio_recv);
	if (err) {
		BT_ERR("LL initialization failed: %u", err);
		return err;
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hci_init(&hbuf_signal);
#else
	hci_init(NULL);
#endif

	k_thread_create(&prio_recv_thread_data, prio_recv_thread_stack,
			K_THREAD_STACK_SIZEOF(prio_recv_thread_stack),
			prio_recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_CTLR_RX_PRIO), 0, K_NO_WAIT);

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_THREAD_STACK_SIZEOF(recv_thread_stack),
			recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);

	BT_DBG("Success.");

	return 0;
}

static const struct bt_hci_driver drv = {
	.name	= "Controller",
	.bus	= BT_HCI_DRIVER_BUS_VIRTUAL,
	.open	= hci_driver_open,
	.send	= hci_driver_send,
};

static int _hci_driver_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(_hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
