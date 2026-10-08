/*
 * Copyright The Zephyr Project Contributors
 *
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

void lll_radio_init(void)
{
	(void)memset(&r, 0, sizeof(r));

	bsr_init(HAL_RADIO_IRQn, HAL_CNTR_IRQn);
}

void lll_radio_isr(void)
{
	struct bsr_evt evt;

	if (r.stop_cb != NULL) {
		lll_radio_cb_t cb = r.stop_cb;

		r.stop_cb = NULL;

		(void)memset(&evt, 0, sizeof(evt));
		evt.status = BSR_STATUS_ABORTED;
		evt.ts_end = lll_radio_now();

		cb(&evt, r.stop_param);
	}

	while (bsr_evt_get(&evt)) {
		lll_radio_cb_t cb;

		/* Ignore the end of an operation that has been stopped since */
		if ((r.id == 0U) || (evt.id != r.id)) {
			continue;
		}

		r.id = 0U;
		cb = r.cb;
		cb(&evt, r.param);
	}
}

void lll_radio_tx(const struct bsr_pkt_cfg *cfg, uint32_t at, const void *pdu,
		  lll_radio_cb_t cb, void *param)
{
	LL_ASSERT_DBG((r.id == 0U) && (r.stop_cb == NULL));

	r.cb = cb;
	r.param = param;
	r.id = bsr_tx(cfg, at, pdu);
	if (r.id == 0U) {
		/* Too late to start it */
		lll_radio_stop(cb, param);
	}
}

void lll_radio_rx(const struct bsr_pkt_cfg *cfg, uint32_t start, uint32_t window_us,
		  void *buf, lll_radio_cb_t cb, void *param)
{
	LL_ASSERT_DBG((r.id == 0U) && (r.stop_cb == NULL));

	r.cb = cb;
	r.param = param;
	r.id = bsr_rx(cfg, start, window_us, buf);
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
	bsr_abort();

	r.id = 0U;
	r.stop_cb = NULL;
}

uint32_t lll_radio_is_idle(void)
{
	return (r.id == 0U) && (r.stop_cb == NULL);
}
