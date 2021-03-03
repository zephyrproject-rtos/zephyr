/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>

#include "util/mem.h"
#include "util/memq.h"

#include "lll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_common
#include "common/log.h"
#include "hal/debug.h"

/**
 * @brief Common entry point for LLL event prepare invocations from ULL.
 *
 * This function will resolve the event priority and invoke the LLL
 * lll_prepare_resolve, which decides if event should be programmed in
 * the radio via the prepare callback function, or queued in the prepare
 * pipeline.
 *
 * @param is_abort_cb   Callback for checking if event is aborted
 * @param abort_cb      Callback for aborting event
 * @param prepare_cb    Callback for event prepare
 * @param event_prio    Priority of event [-128..127]
 * @param prepare_param Prepare data
 *
 * @return 0: Prepare was successfully completed
 *	   1: TICKER_STATUS_FAILURE: Preemption ticker stop error
 *	   2: TICKER_STATUS_BUSY: Preemption ticker stop error
 *	   -EINPROGRESS: Event already in progress and prepare was queued
 */
int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int8_t event_prio,
		struct lll_prepare_param *prepare_param)
{
	/* TODO: Calculate priority */

	return lll_prepare_resolve(is_abort_cb, abort_cb, prepare_cb,
				   prepare_param, 0, 0);
}

void lll_resume(void *param)
{
	struct lll_event *next;
	int ret;

	next = param;
	ret = lll_prepare_resolve(next->is_abort_cb, next->abort_cb,
				  next->prepare_cb, &next->prepare_param,
				  next->is_resume, 1);
	LL_ASSERT(!ret || ret == -EINPROGRESS);
}
