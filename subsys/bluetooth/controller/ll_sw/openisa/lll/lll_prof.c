/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/memq.h"

#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"

static uint8_t latency_min = (uint8_t) -1;
static uint8_t latency_max;
static uint8_t latency_prev;
static uint8_t cputime_min = (uint8_t) -1;
static uint8_t cputime_max;
static uint8_t cputime_prev;
static uint32_t timestamp_latency;

void lll_prof_latency_capture(void)
{
	/* sample the packet timer, use it to calculate ISR latency
	 * and generate the profiling event at the end of the ISR.
	 */
	radio_tmr_sample();
}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
static uint32_t timestamp_radio_end;

uint32_t lll_prof_radio_end_backup(void)
{
	/* PA enable is overwriting packet end used in ISR profiling, hence
	 * back it up for later use.
	 */
	timestamp_radio_end = radio_tmr_end_get();

	return timestamp_radio_end;
}
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

void lll_prof_cputime_capture(void)
{
	/* get the ISR latency sample */
	timestamp_latency = radio_tmr_sample_get();

	/* sample the packet timer again, use it to calculate ISR execution time
	 * and use it in profiling event
	 */
	radio_tmr_sample();
}

void lll_prof_send(void)
{
	uint8_t latency, cputime, prev;
	uint8_t chg = 0U;

	/* calculate the elapsed time in us since on-air radio packet end
	 * to ISR entry
	 */
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	latency = timestamp_latency - timestamp_radio_end;
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	latency = timestamp_latency - radio_tmr_end_get();
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

	/* check changes in min, avg and max of latency */
	if (latency > latency_max) {
		latency_max = latency;
		chg = 1U;
	}
	if (latency < latency_min) {
		latency_min = latency;
		chg = 1U;
	}

	/* check for +/- 1us change */
	prev = ((uint16_t)latency_prev + latency) >> 1;
	if (prev != latency_prev) {
		latency_prev = latency;
		chg = 1U;
	}

	/* calculate the elapsed time in us since ISR entry */
	cputime = radio_tmr_sample_get() - timestamp_latency;

	/* check changes in min, avg and max */
	if (cputime > cputime_max) {
		cputime_max = cputime;
		chg = 1U;
	}

	if (cputime < cputime_min) {
		cputime_min = cputime;
		chg = 1U;
	}

	/* check for +/- 1us change */
	prev = ((uint16_t)cputime_prev + cputime) >> 1;
	if (prev != cputime_prev) {
		cputime_prev = cputime;
		chg = 1U;
	}

	/* generate event if any change */
	if (chg) {
		struct node_rx_pdu *rx;

		/* NOTE: enqueue only if rx buffer available, else ignore */
		rx = ull_pdu_rx_alloc_peek(3);
		if (rx) {
			struct pdu_data *pdu;
			struct profile *p;

			ull_pdu_rx_alloc();

			rx->hdr.type = NODE_RX_TYPE_PROFILE;
			rx->hdr.handle = 0xFFFF;

			pdu = (void *)rx->pdu;
			p = &pdu->profile;
			p->lcur = latency;
			p->lmin = latency_min;
			p->lmax = latency_max;
			p->cur = cputime;
			p->min = cputime_min;
			p->max = cputime_max;

			ull_rx_put_sched(rx->hdr.link, rx);
		}
	}
}
