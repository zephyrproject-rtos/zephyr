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

#include <misc/util.h>
#include <misc/stack.h>
#include <misc/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include <bluetooth/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#ifdef CONFIG_CLOCK_CONTROL_NRF5
#include <drivers/clock_control/nrf5_clock_control.h>
#endif

#include "util/util.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll_sw/pdu.h"
#include "ll_sw/ctrl.h"
#include "ll.h"
#include "hci_internal.h"

#include "hal/debug.h"

static K_SEM_DEFINE(sem_prio_recv, 0, UINT_MAX);
static K_FIFO_DEFINE(recv_fifo);

static BT_STACK_NOINIT(prio_recv_thread_stack,
		       CONFIG_BLUETOOTH_CONTROLLER_RX_PRIO_STACK_SIZE);
static BT_STACK_NOINIT(recv_thread_stack, CONFIG_BLUETOOTH_RX_STACK_SIZE);

#if defined(CONFIG_INIT_STACKS)
static uint32_t prio_ts;
static uint32_t rx_ts;
#endif

static void prio_recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct radio_pdu_node_rx *node_rx;
		struct net_buf *buf;
		uint8_t num_cmplt;
		uint16_t handle;

		while ((num_cmplt = radio_rx_get(&node_rx, &handle))) {

			buf = bt_buf_get_rx(K_FOREVER);
			bt_buf_set_type(buf, BT_BUF_EVT);
			hci_num_cmplt_encode(buf, handle, num_cmplt);
			BT_DBG("Num Complete: 0x%04x:%u", handle, num_cmplt);
			bt_recv_prio(buf);

			k_yield();
		}

		if (node_rx) {

			radio_rx_dequeue();

			BT_DBG("RX node enqueue");
			k_fifo_put(&recv_fifo, node_rx);

			continue;
		}

		BT_DBG("sem take...");
		k_sem_take(&sem_prio_recv, K_FOREVER);
		BT_DBG("sem taken");

#if defined(CONFIG_INIT_STACKS)
		if (k_uptime_get_32() - prio_ts > K_SECONDS(5)) {
			stack_analyze("prio recv thread stack",
				      prio_recv_thread_stack,
				      sizeof(prio_recv_thread_stack));
			prio_ts = k_uptime_get_32();
		}
#endif
	}
}

static void recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_data;
		struct net_buf *buf;

		BT_DBG("RX node get");
		node_rx = k_fifo_get(&recv_fifo, K_FOREVER);
		BT_DBG("RX node dequeued");

		pdu_data = (void *)node_rx->pdu_data;
		/* Check if we need to generate an HCI event or ACL
		 * data
		 */
		if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU ||
		    pdu_data->ll_id == PDU_DATA_LLID_CTRL) {
			/* generate a (non-priority) HCI event */
			if (hci_evt_is_discardable(node_rx)) {
				buf = bt_buf_get_rx(K_NO_WAIT);
			} else {
				buf = bt_buf_get_rx(K_FOREVER);
			}

			if (buf) {
				bt_buf_set_type(buf, BT_BUF_EVT);
				hci_evt_encode(node_rx, buf);
			}
		} else {
			/* generate ACL data */
			buf = bt_buf_get_rx(K_FOREVER);
			bt_buf_set_type(buf, BT_BUF_ACL_IN);
			hci_acl_encode(node_rx, buf);
		}

		radio_rx_fc_set(node_rx->hdr.handle, 0);
		node_rx->hdr.onion.next = 0;
		radio_rx_mem_release(&node_rx);

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
			stack_analyze("recv thread stack", recv_thread_stack,
				      sizeof(recv_thread_stack));
			rx_ts = k_uptime_get_32();
		}
#endif
	}
}

static int cmd_handle(struct net_buf *buf)
{
	struct net_buf *evt;

	evt = hci_cmd_handle(buf);
	if (!evt) {
		return -EINVAL;
	}

	BT_DBG("Replying with event of %u bytes", evt->len);
	bt_recv_prio(evt);

	return 0;
}

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
	case BT_BUF_ACL_OUT:
		err = hci_acl_handle(buf);
		break;
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
		BT_ERR("LL initialization failed: %u", err);
		return err;
	}

	k_thread_spawn(prio_recv_thread_stack, sizeof(prio_recv_thread_stack),
		       prio_recv_thread, NULL, NULL, NULL, K_PRIO_COOP(6), 0,
		       K_NO_WAIT);

	k_thread_spawn(recv_thread_stack, sizeof(recv_thread_stack),
		       recv_thread, NULL, NULL, NULL, K_PRIO_COOP(7), 0,
		       K_NO_WAIT);

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
