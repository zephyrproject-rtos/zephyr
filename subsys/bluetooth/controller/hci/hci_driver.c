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

#include <arch/arm/cortex_m/cmsis.h>

#include "util/config.h"
#include "util/mayfly.h"
#include "util/util.h"
#include "util/mem.h"
#include "hal/rand.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/cntr.h"
#include "hal/cpu.h"
#include "ticker/ticker.h"
#include "ll/pdu.h"
#include "ll/ctrl.h"
#include "ll/ctrl_internal.h"
#include "hci_internal.h"

#include "hal/debug.h"

#define HCI_CMD		0x01
#define HCI_ACL		0x02
#define HCI_SCO		0x03
#define HCI_EVT		0x04

static uint8_t MALIGN(4) _rand_context[3 + 4 + 1];
static uint8_t MALIGN(4) _ticker_nodes[RADIO_TICKER_NODES][TICKER_NODE_T_SIZE];
static uint8_t MALIGN(4) _ticker_users[MAYFLY_CALLER_COUNT]
						[TICKER_USER_T_SIZE];
static uint8_t MALIGN(4) _ticker_user_ops[RADIO_TICKER_USER_OPS]
						[TICKER_USER_OP_T_SIZE];
static uint8_t MALIGN(4) _radio[LL_MEM_TOTAL];

static K_SEM_DEFINE(sem_prio_recv, 0, UINT_MAX);
static K_FIFO_DEFINE(recv_fifo);

static BT_STACK_NOINIT(prio_recv_thread_stack,
		       CONFIG_BLUETOOTH_CONTROLLER_RX_PRIO_STACK_SIZE);
static BT_STACK_NOINIT(recv_thread_stack, CONFIG_BLUETOOTH_RX_STACK_SIZE);

K_MUTEX_DEFINE(mutex_rand);

int bt_rand(void *buf, size_t len)
{
	while (len) {
		k_mutex_lock(&mutex_rand, K_FOREVER);
		len = rand_get(len, buf);
		k_mutex_unlock(&mutex_rand);
		if (len) {
			cpu_sleep();
		}
	}

	return 0;
}

void mayfly_enable(uint8_t caller_id, uint8_t callee_id, uint8_t enable)
{
	(void)caller_id;

	LL_ASSERT(callee_id == MAYFLY_CALL_ID_1);

	if (enable) {
		irq_enable(SWI4_IRQn);
	} else {
		irq_disable(SWI4_IRQn);
	}
}

uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	if (callee_id == MAYFLY_CALL_ID_0) {
		return irq_is_enabled(RTC0_IRQn);
	} else if (callee_id == MAYFLY_CALL_ID_1) {
		return irq_is_enabled(SWI4_IRQn);
	}

	LL_ASSERT(0);

	return 0;
}

uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id)
{
	return (caller_id == callee_id) ||
	       ((caller_id == MAYFLY_CALL_ID_0) &&
		(callee_id == MAYFLY_CALL_ID_1)) ||
	       ((caller_id == MAYFLY_CALL_ID_1) &&
		(callee_id == MAYFLY_CALL_ID_0));
}

void mayfly_pend(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_0:
		NVIC_SetPendingIRQ(RTC0_IRQn);
		break;

	case MAYFLY_CALL_ID_1:
		NVIC_SetPendingIRQ(SWI4_IRQn);
		break;

	case MAYFLY_CALL_ID_PROGRAM:
	default:
		LL_ASSERT(0);
		break;
	}
}

void radio_active_callback(uint8_t active)
{
}

void radio_event_callback(void)
{
	k_sem_give(&sem_prio_recv);
}

static void radio_nrf5_isr(void *arg)
{
	isr_radio(arg);
}

static void rtc0_nrf5_isr(void *arg)
{
	uint32_t compare0, compare1;

	/* store interested events */
	compare0 = NRF_RTC0->EVENTS_COMPARE[0];
	compare1 = NRF_RTC0->EVENTS_COMPARE[1];

	/* On compare0 run ticker worker instance0 */
	if (compare0) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		ticker_trigger(0);
	}

	/* On compare1 run ticker worker instance1 */
	if (compare1) {
		NRF_RTC0->EVENTS_COMPARE[1] = 0;

		ticker_trigger(1);
	}

	mayfly_run(MAYFLY_CALL_ID_0);
}

static void rng_nrf5_isr(void *arg)
{
	isr_rand(arg);
}

static void swi4_nrf5_isr(void *arg)
{
	mayfly_run(MAYFLY_CALL_ID_1);
}

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

		stack_analyze("prio recv thread stack", prio_recv_thread_stack,
			      sizeof(prio_recv_thread_stack));
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

		stack_analyze("recv thread stack", recv_thread_stack,
			      sizeof(recv_thread_stack));
	}
}

static int cmd_handle(struct net_buf *buf)
{
	struct net_buf *evt;
	int err;

	/* Preallocate the response event so that there is no need for
	 * memory checking in hci_cmd_handle().
	 * this might actually be CMD_COMPLETE or CMD_STATUS, but the
	 * actual point is to retrieve the event from the priority
	 * queue
	 */
	evt = bt_buf_get_rx(K_FOREVER);
	bt_buf_set_type(evt, BT_BUF_EVT);
	err = hci_cmd_handle(buf, evt);
	if (!err && evt->len) {
		BT_DBG("Replying with event of %u bytes", evt->len);
		bt_recv_prio(evt);
	} else {
		net_buf_unref(evt);
	}

	return err;
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
	struct device *clk_k32;
	struct device *clk_m16;
	uint32_t err;

	DEBUG_INIT();

	/* TODO: bind and use RNG driver */
	rand_init(_rand_context, sizeof(_rand_context));

	clk_k32 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	/* TODO: bind and use counter driver */
	cntr_init();

	mayfly_init();

	_ticker_users[MAYFLY_CALL_ID_0][0] = RADIO_TICKER_USER_WORKER_OPS;
	_ticker_users[MAYFLY_CALL_ID_1][0] = RADIO_TICKER_USER_JOB_OPS;
	_ticker_users[MAYFLY_CALL_ID_2][0] = 0;
	_ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = RADIO_TICKER_USER_APP_OPS;

	ticker_init(RADIO_TICKER_INSTANCE_ID_RADIO, RADIO_TICKER_NODES,
		    &_ticker_nodes[0], MAYFLY_CALLER_COUNT, &_ticker_users[0],
		    RADIO_TICKER_USER_OPS, &_ticker_user_ops[0]);

	clk_m16 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clk_m16) {
		return -ENODEV;
	}

	err = radio_init(clk_m16, CLOCK_CONTROL_NRF5_K32SRC_ACCURACY,
			 RADIO_CONNECTION_CONTEXT_MAX,
			 RADIO_PACKET_COUNT_RX_MAX,
			 RADIO_PACKET_COUNT_TX_MAX,
			 RADIO_LL_LENGTH_OCTETS_RX_MAX,
			 RADIO_PACKET_TX_DATA_SIZE, &_radio[0], sizeof(_radio));
	if (err) {
		BT_ERR("Required RAM size: %d, supplied: %u.", err,
		       sizeof(_radio));
		return -ENOMEM;
	}

	IRQ_CONNECT(NRF5_IRQ_RADIO_IRQn, 0, radio_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_RTC0_IRQn, 0, rtc0_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_RNG_IRQn, 1, rng_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI4_IRQn, 0, swi4_nrf5_isr, 0, 0);
	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_RNG_IRQn);
	irq_enable(NRF5_IRQ_SWI4_IRQn);

	k_thread_spawn(prio_recv_thread_stack, sizeof(prio_recv_thread_stack),
		       prio_recv_thread, NULL, NULL, NULL, K_PRIO_COOP(6), 0,
		       K_NO_WAIT);

	k_thread_spawn(recv_thread_stack, sizeof(recv_thread_stack),
		       recv_thread, NULL, NULL, NULL, K_PRIO_COOP(7), 0,
		       K_NO_WAIT);

	BT_DBG("Success.");

	return 0;
}

static struct bt_hci_driver drv = {
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
