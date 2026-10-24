/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "hal/swi.h"

#include "util/memq.h"

#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_tim_internal.h"
#include "lll_radio.h"

#include "hal/debug.h"

static struct {
	/* Pending radio model operation, 0 if none */
	uint32_t id;
	lll_radio_cb_t cb;
	void *param;

	/* Callback of a stopped operation, run from the next radio ISR */
	lll_radio_cb_t stop_cb;
	void *stop_param;
} r;

/* A counter value is taken as the closest model time, so an operation starts
 * less than 2^31 us ahead.
 */
static uint64_t model_time_get(uint32_t cntr)
{
	uint64_t now = nbr_cntr_get();
	int64_t t = (int64_t)now + (int32_t)(cntr - (uint32_t)now);

	/* Early in the simulation, a time in the past may be before its start.
	 * The model rejects 0 as too late, as any time that is not in the future.
	 */
	return (t > 0) ? (uint64_t)t : 0U;
}

void lll_radio_init(void)
{
	(void)memset(&r, 0, sizeof(r));

	nbr_init(HAL_RADIO_IRQn);
}

void lll_radio_isr(void)
{
	struct lll_radio_evt e;
	struct nbr_evt evt;

	if (r.stop_cb != NULL) {
		lll_radio_cb_t cb = r.stop_cb;

		r.stop_cb = NULL;

		(void)memset(&e, 0, sizeof(e));
		e.status = NBR_STATUS_ABORTED;
		e.ts_end = lll_radio_now();

		cb(&e, r.stop_param);
	}

	while (nbr_evt_get(&evt)) {
		lll_radio_cb_t cb;

		/* Ignore the end of an operation that has been stopped since */
		if ((r.id == 0U) || (evt.id != r.id)) {
			continue;
		}

		r.id = 0U;
		cb = r.cb;

		e.ts_start = (uint32_t)evt.ts_start;
		e.ts_aa_end = (uint32_t)evt.ts_aa_end;
		e.ts_end = (uint32_t)evt.ts_end;
		e.status = evt.status;
		e.rssi = evt.rssi;

		cb(&e, r.param);
	}
}

void lll_radio_tx(const struct nbr_pkt_cfg *cfg, uint32_t at, const void *pdu,
		  lll_radio_cb_t cb, void *param)
{
	LL_ASSERT_DBG((r.id == 0U) && (r.stop_cb == NULL));

	r.cb = cb;
	r.param = param;
	r.id = nbr_tx(cfg, model_time_get(at), pdu);
	if (r.id == 0U) {
		/* Too late to start it */
		lll_radio_stop(cb, param);
	}
}

void lll_radio_rx(const struct nbr_pkt_cfg *cfg, uint32_t start, uint32_t window_us,
		  void *buf, lll_radio_cb_t cb, void *param)
{
	LL_ASSERT_DBG((r.id == 0U) && (r.stop_cb == NULL));

	r.cb = cb;
	r.param = param;
	r.id = nbr_rx(cfg, model_time_get(start), window_us, buf);
	if (r.id == 0U) {
		/* Too late to start it */
		lll_radio_stop(cb, param);
	}
}

void lll_radio_stop(lll_radio_cb_t cb, void *param)
{
	/* A stop that is pending is replaced, as when a radio that is being
	 * disabled gets a new ISR.
	 */
	lll_radio_abort();

	r.stop_cb = cb;
	r.stop_param = param;

	/* As a radio that is disabled, end the operation in the radio ISR */
	posix_sw_set_pending_IRQ(HAL_RADIO_IRQn);
}

void lll_radio_abort(void)
{
	nbr_abort();

	r.id = 0U;
	r.stop_cb = NULL;
}

uint32_t lll_radio_is_idle(void)
{
	return (r.id == 0U) && (r.stop_cb == NULL);
}
