/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Meta-IRQ based mayfly implementation.
 *
 * This provides a HAL abstraction of the mayfly scheduling mechanism using
 * Zephyr meta-IRQ priority threads instead of SWI (Software Interrupts)
 * and NVIC tail-chaining. Each mayfly execution context (LLL, ULL_HIGH,
 * ULL_LOW) is serviced by a dedicated meta-IRQ thread. mayfly_pend() wakes
 * up the corresponding thread which then calls mayfly_run() for that context.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include "util/memq.h"
#include "util/mayfly.h"

#include "ll_sw/lll.h"

#include "hal/debug.h"

#define MAYFLY_CALL_ID_LLL    TICKER_USER_ID_LLL
#define MAYFLY_CALL_ID_WORKER TICKER_USER_ID_ULL_HIGH
#define MAYFLY_CALL_ID_JOB    TICKER_USER_ID_ULL_LOW

/* Number of mayfly contexts: LLL, ULL_HIGH, ULL_LOW */
#define MAYFLY_META_IRQ_CNT 3

static struct k_sem mayfly_sem[MAYFLY_META_IRQ_CNT];
static bool mayfly_enabled[MAYFLY_META_IRQ_CNT];

static K_KERNEL_STACK_ARRAY_DEFINE(mayfly_stack, MAYFLY_META_IRQ_CNT,
				   CONFIG_BT_CTLR_MAYFLY_META_IRQ_STACK_SIZE);
static struct k_thread mayfly_thread[MAYFLY_META_IRQ_CNT];

static void mayfly_meta_irq_thread(void *p1, void *p2, void *p3)
{
	uint8_t callee_id = (uint8_t)(uintptr_t)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&mayfly_sem[callee_id], K_FOREVER);

		if (mayfly_enabled[callee_id]) {
			mayfly_run(callee_id);
		}
	}
}

/*
 * Map callee_id to meta-IRQ thread priority.
 * LLL gets the highest (numerically lowest) meta-IRQ priority,
 * ULL_HIGH gets the next, and ULL_LOW gets the lowest meta-IRQ priority.
 */
static int mayfly_meta_irq_prio(uint8_t callee_id)
{
	switch (callee_id) {
	case MAYFLY_CALL_ID_LLL:
		return K_HIGHEST_THREAD_PRIO;
	case MAYFLY_CALL_ID_WORKER:
		return K_HIGHEST_THREAD_PRIO + 1;
	case MAYFLY_CALL_ID_JOB:
		return K_HIGHEST_THREAD_PRIO + 2;
	default:
		return K_HIGHEST_THREAD_PRIO;
	}
}

static int mayfly_meta_irq_init(void)
{
	for (uint8_t i = 0U; i < MAYFLY_META_IRQ_CNT; i++) {
		k_sem_init(&mayfly_sem[i], 0, 1);
		mayfly_enabled[i] = true;

		k_thread_create(&mayfly_thread[i],
				mayfly_stack[i],
				K_KERNEL_STACK_SIZEOF(mayfly_stack[i]),
				mayfly_meta_irq_thread,
				(void *)(uintptr_t)i, NULL, NULL,
				mayfly_meta_irq_prio(i),
				K_ESSENTIAL,
				K_NO_WAIT);
		k_thread_name_set(&mayfly_thread[i],
				  i == MAYFLY_CALL_ID_LLL ? "mayfly_lll" :
				  i == MAYFLY_CALL_ID_WORKER ? "mayfly_ull_high" :
				  "mayfly_ull_low");
	}

	return 0;
}

SYS_INIT(mayfly_meta_irq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void mayfly_enable_cb(uint8_t caller_id, uint8_t callee_id, uint8_t enable)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_WORKER:
	case MAYFLY_CALL_ID_JOB:
		mayfly_enabled[callee_id] = (enable != 0);
		break;

	default:
		LL_ASSERT_DBG(0);
		break;
	}
}

uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_LLL:
	case MAYFLY_CALL_ID_WORKER:
	case MAYFLY_CALL_ID_JOB:
		return mayfly_enabled[callee_id] ? 1U : 0U;

	default:
		LL_ASSERT_DBG(0);
		break;
	}

	return 0;
}

uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id)
{
	/* In the meta-IRQ implementation each execution context runs in its
	 * own thread at a distinct meta-IRQ priority, so priorities are equal
	 * only when caller and callee are the same context.
	 */
	return (caller_id == callee_id) ? 1U : 0U;
}

void mayfly_pend(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_LLL:
	case MAYFLY_CALL_ID_WORKER:
	case MAYFLY_CALL_ID_JOB:
		k_sem_give(&mayfly_sem[callee_id]);
		break;

	default:
		LL_ASSERT_DBG(0);
		break;
	}
}

uint32_t mayfly_is_running(void)
{
	/* In meta-IRQ mode, mayfly runs in thread context, not ISR context.
	 * Return true if the current thread is one of the mayfly meta-IRQ
	 * threads.
	 */
	k_tid_t current = k_current_get();

	for (uint8_t i = 0U; i < MAYFLY_META_IRQ_CNT; i++) {
		if (current == &mayfly_thread[i]) {
			return 1U;
		}
	}

	return k_is_in_isr();
}
