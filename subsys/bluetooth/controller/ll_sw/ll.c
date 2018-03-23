/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>

#include <soc.h>
#include <device.h>
#include <clock_control.h>
#ifdef CONFIG_CLOCK_CONTROL_NRF5
#include <drivers/clock_control/nrf5_clock_control.h>
#endif
#include <bluetooth/hci.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#include "hal/cpu.h"
#include "hal/cntr.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/debug.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ctrl.h"
#include "ctrl_internal.h"
#include "ll.h"
#include "ll_filter.h"

/* Global singletons */

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
#define FLASH_TICKER_NODES        1 /* No. of tickers reserved for flashing */
#define FLASH_TICKER_USER_APP_OPS 1 /* No. of additional ticker operations */
#else
#define FLASH_TICKER_NODES        0
#define FLASH_TICKER_USER_APP_OPS 0
#endif

#define TICKER_NODES              (RADIO_TICKER_NODES + FLASH_TICKER_NODES)
#define TICKER_USER_APP_OPS       (RADIO_TICKER_USER_APP_OPS + \
				   FLASH_TICKER_USER_APP_OPS)
#define TICKER_USER_OPS           (RADIO_TICKER_USER_OPS + \
				   FLASH_TICKER_USER_APP_OPS)

/* memory for ticker nodes/instances */
static u8_t MALIGN(4) _ticker_nodes[TICKER_NODES][TICKER_NODE_T_SIZE];

/* memory for users/contexts operating on ticker module */
static u8_t MALIGN(4) _ticker_users[MAYFLY_CALLER_COUNT][TICKER_USER_T_SIZE];

/* memory for user/context simultaneous API operations */
static u8_t MALIGN(4) _ticker_user_ops[TICKER_USER_OPS][TICKER_USER_OP_T_SIZE];

/* memory for Bluetooth Controller (buffers, queues etc.) */
static u8_t MALIGN(4) _radio[LL_MEM_TOTAL];

static struct k_sem *sem_recv;

void radio_active_callback(u8_t active)
{
}

void radio_event_callback(void)
{
	k_sem_give(sem_recv);
}

ISR_DIRECT_DECLARE(radio_nrf5_isr)
{
	isr_radio();

	ISR_DIRECT_PM();
	return 1;
}

static void rtc0_nrf5_isr(void *arg)
{
	u32_t compare0, compare1;

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

static void swi4_nrf5_isr(void *arg)
{
	mayfly_run(MAYFLY_CALL_ID_1);
}

int ll_init(struct k_sem *sem_rx)
{
	struct device *clk_k32;
	struct device *clk_m16;
	struct device *entropy;
	u32_t err;

	sem_recv = sem_rx;

	clk_k32 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	entropy = device_get_binding(CONFIG_ENTROPY_NAME);
	if (!entropy) {
		return -ENODEV;
	}

	/* TODO: bind and use counter driver */
	cntr_init();

	mayfly_init();

	_ticker_users[MAYFLY_CALL_ID_0][0] = RADIO_TICKER_USER_WORKER_OPS;
	_ticker_users[MAYFLY_CALL_ID_1][0] = RADIO_TICKER_USER_JOB_OPS;
	_ticker_users[MAYFLY_CALL_ID_2][0] = 0;
	_ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = TICKER_USER_APP_OPS;

	err = ticker_init(RADIO_TICKER_INSTANCE_ID_RADIO,
			  TICKER_NODES, &_ticker_nodes[0],
			  MAYFLY_CALLER_COUNT, &_ticker_users[0],
			  TICKER_USER_OPS, &_ticker_user_ops[0],
			  hal_ticker_instance0_caller_id_get,
			  hal_ticker_instance0_sched,
			  hal_ticker_instance0_trigger_set);
	LL_ASSERT(!err);

	clk_m16 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clk_m16) {
		return -ENODEV;
	}

	err = radio_init(clk_m16, CLOCK_CONTROL_NRF5_K32SRC_ACCURACY, entropy,
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

	ll_filter_reset(true);

	IRQ_DIRECT_CONNECT(NRF5_IRQ_RADIO_IRQn, CONFIG_BT_CTLR_WORKER_PRIO,
			   radio_nrf5_isr, 0);
	IRQ_CONNECT(NRF5_IRQ_RTC0_IRQn, CONFIG_BT_CTLR_WORKER_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI4_IRQn, CONFIG_BT_CTLR_JOB_PRIO, swi4_nrf5_isr,
		    NULL, 0);

	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_SWI4_IRQn);

	return 0;
}

void ll_timeslice_ticker_id_get(u8_t * const instance_index, u8_t * const user_id)
{
	*user_id = (TICKER_NODES - FLASH_TICKER_NODES); /* The last index in the total tickers */
	*instance_index = RADIO_TICKER_INSTANCE_ID_RADIO;
}
