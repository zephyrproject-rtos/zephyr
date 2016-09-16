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

#include <nanokernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/stack.h>
#include <misc/byteorder.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/driver.h>

#include "util/defines.h"
#include "util/work.h"
#include "hal/clock.h"
#include "hal/rand.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll/ticker.h"
#include "ll/ctrl_internal.h"
#include "hci_internal.h"

#include "hal/debug.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_DRIVER)
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

static struct nano_sem nano_sem_recv;
static BT_STACK_NOINIT(recv_fiber_stack,
		       CONFIG_BLUETOOTH_CONTROLLER_RX_STACK_SIZE);

void radio_active_callback(uint8_t active)
{
}

void radio_event_callback(void)
{
	nano_isr_sem_give(&nano_sem_recv);
}

static void power_clock_nrf5_isr(void *arg)
{
	power_clock_isr();
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
	work_run(NRF52_IRQ_SWI4_EGU4_IRQn);
}

static void swi5_nrf5_isr(void *arg)
{
	work_run(NRF52_IRQ_SWI5_EGU5_IRQn);
}

static struct net_buf *evt_create(uint8_t *remaining, uint8_t **in)
{
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;

	/* TODO: check available length */
	memcpy(&hdr, *in, sizeof(hdr));
	*in += sizeof(hdr);

	*remaining = hdr.len;

	buf = bt_buf_get_evt(0);
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available event buffers!");
	}

	BT_DBG("len %u", hdr.len);

	return buf;
}

static struct net_buf *acl_create(uint8_t *remaining, uint8_t **in)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	/* TODO: check available length */
	memcpy(&hdr, *in, sizeof(hdr));
	*in += sizeof(hdr);

	buf = bt_buf_get_acl();
	if (buf) {
		memcpy(net_buf_add(buf, sizeof(hdr)), &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available ACL buffers!");
	}

	*remaining = sys_le16_to_cpu(hdr.len);

	BT_DBG("len %u", *remaining);

	return buf;
}

static int evt_acl_create(uint8_t remaining, uint8_t *in)
{
	struct net_buf *buf;
	uint8_t type;

	type = *in++;

	switch (type) {
	case HCI_EVT:
		buf = evt_create(&remaining, &in);
		break;
	case HCI_ACL:
		buf = acl_create(&remaining, &in);
		break;
	default:
		BT_ERR("Unknown HCI type %u", type);
		return -EINVAL;
	}

	BT_DBG("remaining %u bytes", remaining);

	if (buf && remaining > net_buf_tailroom(buf)) {
		BT_ERR("Not enough space in buffer");
		net_buf_unref(buf);
		buf = NULL;
	}

	if (buf) {
		memcpy(net_buf_tail(buf), in, remaining);
		buf->len += remaining;

		BT_DBG("bt_recv");

		bt_recv(buf);
	}

	return 0;
}

static void recv_fiber(int unused0, int unused1)
{
	while (1) {
		uint16_t handle;
		uint8_t num_cmplt;
		struct radio_pdu_node_rx *radio_pdu_node_rx;

		while ((num_cmplt =
			radio_rx_get(&radio_pdu_node_rx, &handle))) {
			uint8_t len;
			uint8_t *buf;
			int retval;

			hcic_encode_num_cmplt(handle, num_cmplt, &len, &buf);
			BT_ASSERT(len);

			retval = evt_acl_create(len, buf);
			BT_ASSERT(!retval);

			fiber_yield();
		}

		if (radio_pdu_node_rx) {
			uint8_t len;
			uint8_t *buf;
			int retval;

			hcic_encode((uint8_t *)radio_pdu_node_rx, &len, &buf);

			/* Not all radio_rx_get are translated to HCI!,
			 * hence just dequeue.
			 */
			if (len) {
				retval = evt_acl_create(len, buf);
				BT_ASSERT(!retval);
			}

			radio_rx_dequeue();
			radio_rx_fc_set(radio_pdu_node_rx->hdr.handle, 0);
			radio_pdu_node_rx->hdr.onion.next = 0;
			radio_rx_mem_release(&radio_pdu_node_rx);

			fiber_yield();
		} else {
			nano_fiber_sem_take(&nano_sem_recv,
					    TICKS_UNLIMITED);
		}

		stack_analyze("recv fiber stack",
			      recv_fiber_stack,
			      sizeof(recv_fiber_stack));
	}
}

static int hci_driver_send(struct net_buf *buf)
{
	uint8_t type;
	uint8_t remaining;
	uint8_t *in;

	BT_DBG("enter");

	remaining = 0;

	type = bt_buf_get_type(buf);
	switch (type) {
	case BT_BUF_ACL_OUT:
		hcic_handle(HCI_ACL, &remaining, &in);
		break;
	case BT_BUF_CMD:
		hcic_handle(HCI_CMD, &remaining, &in);
		break;
	default:
		BT_ERR("Unknown HCI type %u", type);
		return -EINVAL;
	}

	if (remaining || !buf->len) {
		BT_ERR("Empty or Len greater than expected");
		return -EINVAL;
	}

	if (buf->len) {
		while (buf->len - 1) {
			hcic_handle(net_buf_pull_u8(buf), &remaining, &in);
		}

		if (remaining) {
			BT_ERR("Len greater than expected");
			return -EINVAL;
		}

		hcic_handle(net_buf_pull_u8(buf), &remaining, &in);

		BT_DBG("hcic_handle returned %u bytes", remaining);

		if (remaining) {
			int retval;

			retval = evt_acl_create(remaining, in);
			if (retval) {
				return retval;
			}
		}
	}

	net_buf_unref(buf);

	BT_DBG("exit");

	return 0;
}

static int hci_driver_open(void)
{
	uint32_t retval;

	clock_k32src_start(1);

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

	retval = radio_init(7,	/* 20ppm = 7 ... 250ppm = 1, 500ppm = 0 */
			    RADIO_CONNECTION_CONTEXT_MAX,
			    RADIO_PACKET_COUNT_RX_MAX,
			    RADIO_PACKET_COUNT_TX_MAX,
			    RADIO_LL_LENGTH_OCTETS_RX_MAX, &_radio[0],
			    sizeof(_radio)
	    );
	if (retval) {
		BT_ERR("Required RAM size: %d, supplied: %u.", retval,
		       sizeof(_radio));
		return -ENOMEM;
	}

	IRQ_CONNECT(NRF52_IRQ_POWER_CLOCK_IRQn, 2, power_clock_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF52_IRQ_RADIO_IRQn, 0, radio_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF52_IRQ_RTC0_IRQn, 0, rtc0_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF52_IRQ_RNG_IRQn, 2, rng_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF52_IRQ_SWI4_EGU4_IRQn, 0, swi4_nrf5_isr, 0, 0);
	IRQ_CONNECT(NRF52_IRQ_SWI5_EGU5_IRQn, 2, swi5_nrf5_isr, 0, 0);
	irq_enable(NRF52_IRQ_POWER_CLOCK_IRQn);
	irq_enable(NRF52_IRQ_RADIO_IRQn);
	irq_enable(NRF52_IRQ_RTC0_IRQn);
	irq_enable(NRF52_IRQ_RNG_IRQn);
	irq_enable(NRF52_IRQ_SWI4_EGU4_IRQn);
	irq_enable(NRF52_IRQ_SWI5_EGU5_IRQn);

	nano_sem_init(&nano_sem_recv);
	fiber_start(recv_fiber_stack, sizeof(recv_fiber_stack),
		    (nano_fiber_entry_t)recv_fiber, 0, 0, 7, 0);

	BT_DBG("Success.");

	return 0;
}

static struct bt_driver drv = {
	.name	= "Controller",
	.bus	= BT_DRIVER_BUS_VIRTUAL,
	.open	= hci_driver_open,
	.send	= hci_driver_send,
};

static int _hci_driver_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_driver_register(&drv);

	return 0;
}

SYS_INIT(_hci_driver_init, NANOKERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
