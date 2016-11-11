/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#include <bluetooth/bluetooth.h>
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#ifdef CONFIG_CLOCK_CONTROL_NRF5
#include <drivers/clock_control/nrf5_clock_control.h>
#endif

#include "util/defines.h"
#include "util/work.h"
#include "hal/rand.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll/ticker.h"
#include "ll/ctrl_internal.h"
#include "hci_internal.h"

#include "hal/debug.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define HCI_CMD		0x01
#define HCI_ACL		0x02
#define HCI_SCO		0x03
#define HCI_EVT		0x04

static uint8_t ALIGNED(4) _rand_context[3 + 4 + 1];
static uint8_t ALIGNED(4) _ticker_nodes[RADIO_TICKER_NODES][TICKER_NODE_T_SIZE];
static uint8_t ALIGNED(4) _ticker_users[RADIO_TICKER_USERS][TICKER_USER_T_SIZE];
static uint8_t ALIGNED(4) _ticker_user_ops[RADIO_TICKER_USER_OPS]
						[TICKER_USER_OP_T_SIZE];
static uint8_t ALIGNED(4) _radio[LL_MEM_TOTAL];

static struct k_sem sem_recv;
static BT_STACK_NOINIT(recv_thread_stack,
		       CONFIG_BLUETOOTH_CONTROLLER_RX_STACK_SIZE);

void radio_active_callback(uint8_t active)
{
}

void radio_event_callback(void)
{
	k_sem_give(&sem_recv);
}

static void radio_nrf5_isr(void *arg)
{
	radio_isr();
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

	work_run(RTC0_IRQn);
}

static void rng_nrf5_isr(void *arg)
{
	rng_isr();
}

static void swi4_nrf5_isr(void *arg)
{
	work_run(NRF5_IRQ_SWI4_IRQn);
}

static void swi5_nrf5_isr(void *arg)
{
	work_run(NRF5_IRQ_SWI5_IRQn);
}

static void recv_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct radio_pdu_node_rx *node_rx;
		struct pdu_data *pdu_data;
		struct net_buf *buf;
		uint8_t num_cmplt;
		uint16_t handle;

		while ((num_cmplt = radio_rx_get(&node_rx, &handle))) {

			buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS);
			if (buf) {
				hci_num_cmplt_encode(buf, handle, num_cmplt);
				BT_DBG("Num Complete: 0x%04x:%u", handle,
								  num_cmplt);
				bt_recv(buf);
			} else {
				BT_ERR("Cannot allocate Num Complete");
			}

			k_yield();
		}

		if (node_rx) {

			pdu_data = (void *)node_rx->pdu_data;
			/* Check if we need to generate an HCI event or ACL
			 * data
			 */
			if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU ||
			    pdu_data->ll_id == PDU_DATA_LLID_CTRL) {
				/* generate a (non-priority) HCI event */
				buf = bt_buf_get_evt(0);
				if (buf) {
					hci_evt_encode(node_rx, buf);
				} else {
					BT_ERR("Cannot allocate RX event");
				}
			} else {
				/* generate ACL data */
				buf = bt_buf_get_acl();
				if (buf) {
					hci_acl_encode(node_rx, buf);
				} else {
					BT_ERR("Cannot allocate RX ACL");
				}
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

			radio_rx_dequeue();
			radio_rx_fc_set(node_rx->hdr.handle, 0);
			node_rx->hdr.onion.next = 0;
			radio_rx_mem_release(&node_rx);

			k_yield();
		} else {
			k_sem_take(&sem_recv, K_FOREVER);
		}

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
	evt = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE);
	if (!evt) {
		BT_ERR("No available event buffers");
		return -ENOMEM;
	}
	err = hci_cmd_handle(buf, evt);
	if (!err && evt->len) {
		BT_DBG("Replying with event of %u bytes", evt->len);
		bt_recv(evt);
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

	clk_k32 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	_ticker_users[RADIO_TICKER_USER_ID_WORKER][0] =
	    RADIO_TICKER_USER_WORKER_OPS;
	_ticker_users[RADIO_TICKER_USER_ID_JOB][0] =
		RADIO_TICKER_USER_JOB_OPS;
	_ticker_users[RADIO_TICKER_USER_ID_APP][0] =
		RADIO_TICKER_USER_APP_OPS;

	ticker_init(RADIO_TICKER_INSTANCE_ID_RADIO, RADIO_TICKER_NODES,
		   &_ticker_nodes[0]
		   , RADIO_TICKER_USERS, &_ticker_users[0]
		   , RADIO_TICKER_USER_OPS, &_ticker_user_ops[0]
	    );

	rand_init(_rand_context, sizeof(_rand_context));

	clk_m16 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clk_m16) {
		return -ENODEV;
	}

	err = radio_init(clk_m16,
			 CLOCK_CONTROL_NRF5_K32SRC_ACCURACY,
			 RADIO_CONNECTION_CONTEXT_MAX,
			 RADIO_PACKET_COUNT_RX_MAX,
			 RADIO_PACKET_COUNT_TX_MAX,
			 RADIO_LL_LENGTH_OCTETS_RX_MAX, &_radio[0],
			 sizeof(_radio));
	if (err) {
		BT_ERR("Required RAM size: %d, supplied: %u.", err,
		       sizeof(_radio));
		return -ENOMEM;
	}

	IRQ_CONNECT(NRF5_IRQ_RADIO_IRQn, 0, radio_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_RTC0_IRQn, 0, rtc0_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_RNG_IRQn, 1, rng_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI4_IRQn, 0, swi4_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI5_IRQn, 1, swi5_nrf5_isr, 0, 0);
	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_RNG_IRQn);
	irq_enable(NRF5_IRQ_SWI4_IRQn);
	irq_enable(NRF5_IRQ_SWI5_IRQn);

	k_sem_init(&sem_recv, 0, UINT_MAX);
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
