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
#include <drivers/clock_control.h>
#include <sys/atomic.h>

#include <sys/util.h>
#include <debug/stack.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#ifdef CONFIG_CLOCK_CONTROL_NRF
#include <drivers/clock_control/nrf_clock_control.h>
#endif

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_hci_driver
#include "common/log.h"

#include "util/util.h"
#include "util/memq.h"

#include "hal/ccm.h"

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/radio.h"
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "ll_sw/pdu.h"
#include "ll_sw/lll.h"
#include "ll.h"

#include "hci_internal.h"

#include "hal/debug.h"

static K_SEM_DEFINE(sem_prio_recv, 0, UINT_MAX);
static K_FIFO_DEFINE(recv_fifo);

struct k_thread prio_recv_thread_data;
static K_KERNEL_STACK_DEFINE(prio_recv_thread_stack,
			     CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE);
struct k_thread recv_thread_data;
static K_KERNEL_STACK_DEFINE(recv_thread_stack, CONFIG_BT_RX_STACK_SIZE);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static struct k_poll_signal hbuf_signal =
		K_POLL_SIGNAL_INITIALIZER(hbuf_signal);
static sys_slist_t hbuf_pend;
static int32_t hbuf_count;
#endif

static struct net_buf *process_prio_evt(struct node_rx_pdu *node_rx,
					uint8_t *evt_flags)
{
#if defined(CONFIG_BT_CONN)
	if (node_rx->hdr.user_meta == HCI_CLASS_EVT_CONNECTION) {
		uint16_t handle;
		struct pdu_data *pdu_data = (void *)node_rx->pdu;

		handle = node_rx->hdr.handle;
		if (node_rx->hdr.type == NODE_RX_TYPE_TERMINATE) {
			struct net_buf *buf;

			buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false,
					     K_FOREVER);
			hci_disconn_complete_encode(pdu_data, handle, buf);
			hci_disconn_complete_process(handle);
			*evt_flags = BT_HCI_EVT_FLAG_RECV_PRIO | BT_HCI_EVT_FLAG_RECV;
			return buf;
		}
	}
#endif /* CONFIG_BT_CONN */

	*evt_flags = BT_HCI_EVT_FLAG_RECV;
	return NULL;
}

/**
 * @brief Handover from Controller thread to Host thread
 * @details Execution context: Controller thread
 *   Pull from memq_ll_rx and push up to Host thread recv_thread() via recv_fifo
 * @param p1  Unused. Required to conform with Zephyr thread protoype
 * @param p2  Unused. Required to conform with Zephyr thread protoype
 * @param p3  Unused. Required to conform with Zephyr thread protoype
 */
static void prio_recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct node_rx_pdu *node_rx;
		struct net_buf *buf;
		uint8_t num_cmplt;
		uint16_t handle;

		/* While there are completed rx nodes */
		while ((num_cmplt = ll_rx_get((void *)&node_rx, &handle))) {
#if defined(CONFIG_BT_CONN)

			buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS,
					     false, K_FOREVER);
			hci_num_cmplt_encode(buf, handle, num_cmplt);
			BT_DBG("Num Complete: 0x%04x:%u", handle, num_cmplt);
			bt_recv_prio(buf);
			k_yield();
#endif
		}

		if (node_rx) {
			uint8_t evt_flags;

			/* Until now we've only peeked, now we really do
			 * the handover
			 */
			ll_rx_dequeue();

			/* Find out and store the class for this node */
			node_rx->hdr.user_meta = hci_get_class(node_rx);

			buf = process_prio_evt(node_rx, &evt_flags);
			if (buf) {
				BT_DBG("Priority event");
				if (!(evt_flags & BT_HCI_EVT_FLAG_RECV)) {
					node_rx->hdr.next = NULL;
					ll_rx_mem_release((void **)&node_rx);
				}

				bt_recv_prio(buf);
				/* bt_recv_prio would not release normal evt
				 * buf.
				 */
				if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
					net_buf_unref(buf);
				}
			}

			if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
				/* Send the rx node up to Host thread,
				 * recv_thread()
				 */
				BT_DBG("RX node enqueue");
				k_fifo_put(&recv_fifo, node_rx);
			}

			/* There may still be completed nodes, continue
			 * pushing all those up to Host before waiting
			 * for ULL mayfly
			 */
			continue;

		}

		BT_DBG("sem take...");
		/* Wait until ULL mayfly has something to give us.
		 * Blocking-take of the semaphore; we take it once ULL mayfly
		 * has let it go in ll_rx_sched().
		 */
		k_sem_take(&sem_prio_recv, K_FOREVER);
		/* Now, ULL mayfly has something to give to us */
		BT_DBG("sem taken");
	}
}

static inline struct net_buf *encode_node(struct node_rx_pdu *node_rx,
					  int8_t class)
{
	struct net_buf *buf = NULL;

	/* Check if we need to generate an HCI event or ACL data */
	switch (class) {
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	case HCI_CLASS_EVT_CONNECTION:
	case HCI_CLASS_EVT_LLCP:
		if (class == HCI_CLASS_EVT_DISCARDABLE) {
			buf = bt_buf_get_evt(BT_HCI_EVT_UNKNOWN, true,
					     K_NO_WAIT);
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

	node_rx->hdr.next = NULL;
	ll_rx_mem_release((void **)&node_rx);

	return buf;
}

static inline struct net_buf *process_node(struct node_rx_pdu *node_rx)
{
	uint8_t class = node_rx->hdr.user_meta;
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
		case HCI_CLASS_EVT_LLCP:
			/* for conn-related events, only pend is relevant */
			hbuf_count = 1;
			__fallthrough;
		case HCI_CLASS_ACL_DATA:
			if (pend || !hbuf_count) {
				sys_slist_append(&hbuf_pend, (void *)node_rx);
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
static inline struct net_buf *process_hbuf(struct node_rx_pdu *n)
{
	/* shadow total count in case of preemption */
	struct node_rx_pdu *node_rx = NULL;
	int32_t hbuf_total = hci_hbuf_total;
	struct net_buf *buf = NULL;
	uint8_t class;
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
	node_rx = (void *)sys_slist_peek_head(&hbuf_pend);
	if (!node_rx) {
		return NULL;
	}

	/* Return early if this iteration already has a node to process */
	class = node_rx->hdr.user_meta;
	if (n) {
		if (class == HCI_CLASS_EVT_CONNECTION ||
		    class == HCI_CLASS_EVT_LLCP ||
		    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
			/* node to process later, schedule an iteration */
			BT_DBG("FC: signalling");
			k_poll_signal_raise(&hbuf_signal, 0x0);
		}
		return NULL;
	}

	switch (class) {
	case HCI_CLASS_EVT_CONNECTION:
	case HCI_CLASS_EVT_LLCP:
		BT_DBG("FC: dequeueing event");
		(void) sys_slist_get(&hbuf_pend);
		break;
	case HCI_CLASS_ACL_DATA:
		if (hbuf_count) {
			BT_DBG("FC: dequeueing ACL data");
			(void) sys_slist_get(&hbuf_pend);
		} else {
			/* no buffers, HCI will signal */
			node_rx = NULL;
		}
		break;
	case HCI_CLASS_EVT_DISCARDABLE:
	case HCI_CLASS_EVT_REQUIRED:
	default:
		LL_ASSERT(0);
		break;
	}

	if (node_rx) {
		buf = encode_node(node_rx, class);
		/* Update host buffers after encoding */
		hbuf_count = hbuf_total - (hci_hbuf_sent - hci_hbuf_acked);
		/* next node */
		node_rx = (void *)sys_slist_peek_head(&hbuf_pend);
		if (node_rx) {
			class = node_rx->hdr.user_meta;

			if (class == HCI_CLASS_EVT_CONNECTION ||
			    class == HCI_CLASS_EVT_LLCP ||
			    (class == HCI_CLASS_ACL_DATA && hbuf_count)) {
				/* more to process, schedule an
				 * iteration
				 */
				BT_DBG("FC: signalling");
				k_poll_signal_raise(&hbuf_signal, 0x0);
			}
		}
	}

	return buf;
}
#endif

/**
 * @brief Blockingly pull from Controller thread's recv_fifo
 * @details Execution context: Host thread
 */
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
		struct node_rx_pdu *node_rx = NULL;
		struct net_buf *buf = NULL;

		BT_DBG("blocking");
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
		int err;

		err = k_poll(events, 2, K_FOREVER);
		LL_ASSERT(err == 0);
		if (events[0].state == K_POLL_STATE_SIGNALED) {
			events[0].signal->signaled = 0U;
		} else if (events[1].state ==
			   K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			node_rx = k_fifo_get(events[1].fifo, K_NO_WAIT);
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
	}
}

static int cmd_handle(struct net_buf *buf)
{
	struct node_rx_pdu *node_rx = NULL;
	struct net_buf *evt;

	evt = hci_cmd_handle(buf, (void **) &node_rx);
	if (evt) {
		BT_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);

		if (node_rx) {
			BT_DBG("RX node enqueue");
			node_rx->hdr.user_meta = hci_get_class(node_rx);
			k_fifo_put(&recv_fifo, node_rx);
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
	uint8_t type;
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
	uint32_t err;

	DEBUG_INIT();

	err = ll_init(&sem_prio_recv);
	if (err) {
		BT_ERR("LL initialization failed: %d", err);
		return err;
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hci_init(&hbuf_signal);
#else
	hci_init(NULL);
#endif

	k_thread_create(&prio_recv_thread_data, prio_recv_thread_stack,
			K_KERNEL_STACK_SIZEOF(prio_recv_thread_stack),
			prio_recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&prio_recv_thread_data, "BT RX pri");

	k_thread_create(&recv_thread_data, recv_thread_stack,
			K_KERNEL_STACK_SIZEOF(recv_thread_stack),
			recv_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&recv_thread_data, "BT RX");

	BT_DBG("Success.");

	return 0;
}

static const struct bt_hci_driver drv = {
	.name	= "Controller",
	.bus	= BT_HCI_DRIVER_BUS_VIRTUAL,
	.quirks = BT_QUIRK_NO_AUTO_DLE,
	.open	= hci_driver_open,
	.send	= hci_driver_send,
};

static int hci_driver_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
