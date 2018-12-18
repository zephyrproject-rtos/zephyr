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
#elif defined(CONFIG_BT_LL_SW_SPLIT)
#include "ll_sw/lll.h"
#define MAYFLY_CALL_ID_LLL    TICKER_USER_ID_LLL
#define MAYFLY_CALL_ID_WORKER TICKER_USER_ID_ULL_HIGH
#define MAYFLY_CALL_ID_JOB    TICKER_USER_ID_ULL_LOW
#else
#error Unknown LL variant.
#endif

void mayfly_enable_cb(u8_t caller_id, u8_t callee_id, u8_t enable)
{
	(void)caller_id;

	LL_ASSERT(callee_id == MAYFLY_CALL_ID_JOB);

	if (enable) {
		irq_enable(SWI5_IRQn);
	} else {
		irq_disable(SWI5_IRQn);
	}
}

u32_t mayfly_is_enabled(u8_t caller_id, u8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
#if defined(CONFIG_BT_LL_SW_SPLIT)
	case MAYFLY_CALL_ID_LLL:
		return irq_is_enabled(SWI4_IRQn);
#endif /* CONFIG_BT_LL_SW_SPLIT */

	case MAYFLY_CALL_ID_WORKER:
		return irq_is_enabled(RTC0_IRQn);

	case MAYFLY_CALL_ID_JOB:
		return irq_is_enabled(SWI5_IRQn);

	default:
		LL_ASSERT(0);
		break;
	}

	return 0;
}

u32_t mayfly_prio_is_equal(u8_t caller_id, u8_t callee_id)
{
	return (caller_id == callee_id) ||
#if defined(CONFIG_BT_LL_SW)
#if (CONFIG_BT_CTLR_WORKER_PRIO == CONFIG_BT_CTLR_JOB_PRIO)
	       ((caller_id == MAYFLY_CALL_ID_WORKER) &&
		(callee_id == MAYFLY_CALL_ID_JOB)) ||
	       ((caller_id == MAYFLY_CALL_ID_JOB) &&
		(callee_id == MAYFLY_CALL_ID_WORKER)) ||
#endif
#elif defined(CONFIG_BT_LL_SW_SPLIT)
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_HIGH_PRIO)
	       ((caller_id == MAYFLY_CALL_ID_LLL) &&
		(callee_id == MAYFLY_CALL_ID_WORKER)) ||
	       ((caller_id == MAYFLY_CALL_ID_WORKER) &&
		(callee_id == MAYFLY_CALL_ID_LLL)) ||
#endif
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	       ((caller_id == MAYFLY_CALL_ID_LLL) &&
		(callee_id == MAYFLY_CALL_ID_JOB)) ||
	       ((caller_id == MAYFLY_CALL_ID_JOB) &&
		(callee_id == MAYFLY_CALL_ID_LLL)) ||
#endif
#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	       ((caller_id == MAYFLY_CALL_ID_WORKER) &&
		(callee_id == MAYFLY_CALL_ID_JOB)) ||
	       ((caller_id == MAYFLY_CALL_ID_JOB) &&
		(callee_id == MAYFLY_CALL_ID_WORKER)) ||
#endif
#endif
	       0;
}

void mayfly_pend(u8_t caller_id, u8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
#if defined(CONFIG_BT_LL_SW_SPLIT)
	case MAYFLY_CALL_ID_LLL:
		NVIC_SetPendingIRQ(SWI4_IRQn);
		break;
#endif /* CONFIG_BT_LL_SW_SPLIT */

	case MAYFLY_CALL_ID_WORKER:
		NVIC_SetPendingIRQ(RTC0_IRQn);
		break;

	case MAYFLY_CALL_ID_JOB:
		NVIC_SetPendingIRQ(SWI5_IRQn);
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
