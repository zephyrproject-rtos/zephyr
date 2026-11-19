/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/mem.h"
#include "util/memq.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_scan.h"
#include "lll_filter.h"

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
	int err;

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	int prio = event_prio;
	struct lll_hdr *hdr = prepare_param->param;

	/* Establish priority based on:
	 * 1. Event priority passed to function
	 * 2. Force flag => priority = -127
	 * 3. Score (events terminated- and too late)
	 * 4. Latency (skipped- and programmed latency)
	 * 5. Critical priority is immutable (-128)
	 */
	if (prio > -128) {
		if (prepare_param->force) {
			prio = -127;
		} else {
			prio = MAX(-127, prio - hdr->score - hdr->latency);
		}
	}

	prepare_param->prio = prio;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

	prepare_param->defer = 0U;

	err = lll_prepare_resolve(is_abort_cb, abort_cb, prepare_cb, prepare_param, 0U, 0U);

	return err;
}

void lll_resume(void *param)
{
	struct lll_event *next;
	int err;

	next = param;
	err = lll_prepare_resolve(next->is_abort_cb, next->abort_cb, next->prepare_cb,
				  &next->prepare_param, next->is_resume, 1U);
	LL_ASSERT_ERR(!err || err == -EINPROGRESS);
}

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
void lll_done_score(void *param, uint8_t result)
{
	struct lll_hdr *hdr = param;

	if (!hdr) {
		return;
	}

	if (result == DONE_COMPLETED) {
		hdr->score  = 0;
		hdr->latency = 0;
	} else {
		hdr->score++;
	}
}
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

#if defined(CONFIG_BT_OBSERVER)
static inline bool isr_scan_tgta_rpa_check(const struct lll_scan *lll,
					   uint8_t addr_type,
					   const uint8_t *addr,
					   bool *const dir_report);

bool lll_scan_isr_rx_check(const struct lll_scan *lll, uint8_t irkmatch_ok,
			   uint8_t devmatch_ok, uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return (((lll->filter_policy & SCAN_FP_FILTER) == 0U) &&
		(!devmatch_ok || ull_filter_lll_rl_idx_allowed(irkmatch_ok,
							       rl_idx))) ||
	       (((lll->filter_policy & SCAN_FP_FILTER) != 0U) &&
		(devmatch_ok || ull_filter_lll_irk_in_fal(rl_idx)));
#else
	return ((lll->filter_policy & SCAN_FP_FILTER) == 0U) ||
		devmatch_ok;
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

bool lll_scan_tgta_check(const struct lll_scan *lll, bool init,
			 uint8_t addr_type, const uint8_t *addr,
			 uint8_t rl_idx, bool *dir_report)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_addr_resolve(addr_type, addr, rl_idx)) {
		return true;
	} else if (init && lll->rpa_gen && ull_filter_lll_lrpa_get(rl_idx)) {
		/* Initiator generating RPAs, and could not resolve TargetA:
		 * discard
		 */
		return false;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return (((lll->init_addr_type == addr_type) &&
		 !memcmp(lll->init_addr, addr, BDADDR_SIZE))) ||
	       /* allow directed adv packets where TargetA address
		* is resolvable private address (scanner only)
		*/
	       isr_scan_tgta_rpa_check(lll, addr_type, addr, dir_report);
}

static inline bool isr_scan_tgta_rpa_check(const struct lll_scan *lll,
					   uint8_t addr_type,
					   const uint8_t *addr,
					   bool *const dir_report)
{
	if (((lll->filter_policy & SCAN_FP_EXT) != 0U) && (addr_type != 0U) &&
	    ((addr[5] & 0xc0) == 0x40)) {

		if (dir_report) {
			*dir_report = true;
		}

		return true;
	}

	return false;
}
#endif /* CONFIG_BT_OBSERVER */
