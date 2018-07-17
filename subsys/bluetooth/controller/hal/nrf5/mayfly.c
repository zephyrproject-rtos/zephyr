/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>

#include "util/memq.h"
#include "util/mayfly.h"

#define LOG_MODULE_NAME bt_ctlr_nrf5_mayfly
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_LL_SW)
#define MAYFLY_CALL_ID_WORKER MAYFLY_CALL_ID_0
#define MAYFLY_CALL_ID_JOB    MAYFLY_CALL_ID_1
#else
#error Unknown LL variant.
#endif

void mayfly_enable_cb(u8_t caller_id, u8_t callee_id, u8_t enable)
{
	(void)caller_id;

	LL_ASSERT(callee_id == MAYFLY_CALL_ID_JOB);

	if (enable) {
		irq_enable(SWI4_IRQn);
	} else {
		irq_disable(SWI4_IRQn);
	}
}

u32_t mayfly_is_enabled(u8_t caller_id, u8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_WORKER:
		return irq_is_enabled(RTC0_IRQn);

	case MAYFLY_CALL_ID_JOB:
		return irq_is_enabled(SWI4_IRQn);

	default:
		LL_ASSERT(0);
		break;
	}

	return 0;
}

u32_t mayfly_prio_is_equal(u8_t caller_id, u8_t callee_id)
{
#if (RADIO_TICKER_USER_ID_WORKER_PRIO == RADIO_TICKER_USER_ID_JOB_PRIO)
	return (caller_id == callee_id) ||
	       ((caller_id == MAYFLY_CALL_ID_WORKER) &&
		(callee_id == MAYFLY_CALL_ID_JOB)) ||
	       ((caller_id == MAYFLY_CALL_ID_JOB) &&
		(callee_id == MAYFLY_CALL_ID_WORKER));
#else
	/* TODO: check Kconfig set priorities */
	return caller_id == callee_id;
#endif
}

void mayfly_pend(u8_t caller_id, u8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_WORKER:
		NVIC_SetPendingIRQ(RTC0_IRQn);
		break;

	case MAYFLY_CALL_ID_JOB:
		NVIC_SetPendingIRQ(SWI4_IRQn);
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
