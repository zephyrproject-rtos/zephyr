/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/counter.h>
#include <nrfx_clock.h>

#define UPDATE_INTERVAL_S 10

static const struct device *const clock0 = DEVICE_DT_GET_ONE(nordic_nrf_clock);
static const struct device *const timer0 = DEVICE_DT_GET(DT_NODELABEL(timer0));
static struct timeutil_sync_config sync_config;
static uint64_t counter_ref;
static struct timeutil_sync_state sync_state;
static struct k_work_delayable sync_work;

/* Convert local time in ticks to microseconds. */
uint64_t local_to_us(uint64_t local)
{
	return z_tmcvt(local, sync_config.local_Hz, USEC_PER_SEC, false,
		       false, false, false);
}

/* Convert HFCLK reference to microseconds. */
uint64_t ref_to_us(uint64_t ref)
{
	return z_tmcvt(ref, sync_config.ref_Hz, USEC_PER_SEC, false,
		       false, false, false);
}

/* Format a microsecond timestamp to text as D d HH:MM:SS.SSSSSS. */
static const char *us_to_text_r(uint64_t rem, char *buf, size_t len)
{
	char *bp = buf;
	char *bpe = bp + len;
	uint32_t us;
	uint32_t s;
	uint32_t min;
	uint32_t hr;
	uint32_t d;

	us = rem % USEC_PER_SEC;
	rem /= USEC_PER_SEC;
	s = rem % 60;
	rem /= 60;
	min = rem % 60;
	rem /= 60;
	hr = rem % 24;
	rem /= 24;
	d = rem;

	if (d > 0) {
		bp += snprintf(bp, bpe - bp, "%u d ", d);
	}
	bp += snprintf(bp, bpe - bp, "%02u:%02u:%02u.%06u",
		       hr, min, s, us);
	return buf;
}

static const char *us_to_text(uint64_t rem)
{
	static char ts_buf[32];

	return us_to_text_r(rem, ts_buf, sizeof(ts_buf));
}

/* Show status of various clocks */
static void show_clocks(const char *tag)
{
	static const char *const lfsrc_s[] = {
#if defined(CLOCK_LFCLKSRC_SRC_LFULP)
		[NRF_CLOCK_LFCLK_LFULP] = "LFULP",
#endif
		[NRF_CLOCK_LFCLK_RC] = "LFRC",
		[NRF_CLOCK_LFCLK_Xtal] = "LFXO",
		[NRF_CLOCK_LFCLK_Synth] = "LFSYNT",
	};
	static const char *const hfsrc_s[] = {
		[NRF_CLOCK_HFCLK_LOW_ACCURACY] = "HFINT",
		[NRF_CLOCK_HFCLK_HIGH_ACCURACY] = "HFXO",
	};
	static const char *const clkstat_s[] = {
		[CLOCK_CONTROL_STATUS_STARTING] = "STARTING",
		[CLOCK_CONTROL_STATUS_OFF] = "OFF",
		[CLOCK_CONTROL_STATUS_ON] = "ON",
		[CLOCK_CONTROL_STATUS_UNKNOWN] = "UNKNOWN",
	};
	union {
		unsigned int raw;
		nrf_clock_lfclk_t lf;
		nrf_clock_hfclk_t hf;
	} src;
	enum clock_control_status clkstat;
	bool running;

	clkstat = clock_control_get_status(clock0, CLOCK_CONTROL_NRF_SUBSYS_LF);
	running = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK,
				       &src.lf);
	printk("%s: LFCLK[%s]: %s %s ; ", tag, clkstat_s[clkstat],
	       running ? "Running" : "Off", lfsrc_s[src.lf]);
	clkstat = clock_control_get_status(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);
	running = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK,
				       &src.hf);
	printk("HFCLK[%s]: %s %s\n", clkstat_s[clkstat],
	       running ? "Running" : "Off", hfsrc_s[src.hf]);
}

static void sync_work_handler(struct k_work *work)
{
	uint32_t ctr;
	int rc = counter_get_value(timer0, &ctr);
	const struct timeutil_sync_instant *base = &sync_state.base;
	const struct timeutil_sync_instant *latest = &sync_state.latest;

	if (rc == 0) {
		struct timeutil_sync_instant inst;
		uint64_t ref_span_us;

		counter_ref += ctr - (uint32_t)counter_ref;
		inst.ref = counter_ref;
		inst.local = k_uptime_ticks();

		rc = timeutil_sync_state_update(&sync_state, &inst);
		printf("\nTy  Latest           Base             Span             Err\n");
		printf("HF  %s", us_to_text(ref_to_us(inst.ref)));
		if (rc > 0) {
			printf("  %s", us_to_text(ref_to_us(base->ref)));
			ref_span_us = ref_to_us(latest->ref - base->ref);
			printf("  %s", us_to_text(ref_span_us));
		}
		printf("\nLF  %s", us_to_text(local_to_us(inst.local)));
		if (rc > 0) {
			uint64_t err_us;
			uint64_t local_span_us;
			char err_sign = ' ';

			printf("  %s", us_to_text(local_to_us(base->local)));

			local_span_us = local_to_us(latest->local - base->local);
			printf("  %s", us_to_text(local_span_us));

			if (ref_span_us >= local_span_us) {
				err_us = ref_span_us - local_span_us;
				err_sign = '-';
			} else {
				err_us = local_span_us - ref_span_us;
			}
			printf(" %c%s", err_sign, us_to_text(err_us));
		}
		printf("\n");
		if (rc > 0) {
			float skew = timeutil_sync_estimate_skew(&sync_state);

			/* Create a state with the current skew estimate.  Use
			 * it to reconstruct the expected reference time from
			 * the latest local time, then display that time and
			 * its error from the latest reference time.
			 */
			uint64_t rec_ref;
			struct timeutil_sync_state st2 = sync_state;

			(void)timeutil_sync_state_set_skew(&st2, skew, NULL);
			(void)timeutil_sync_ref_from_local(&st2, latest->local,
							   &rec_ref);

			char err_sign = ' ';
			uint64_t err_us;

			if (rec_ref < latest->ref) {
				err_sign = '-';
				err_us = ref_to_us(latest->ref - rec_ref);
			} else {
				err_us = ref_to_us(rec_ref - latest->ref);
			}

			printf("RHF %s                                   ",
			       us_to_text(ref_to_us(rec_ref)));
			printf("%c%s\n", err_sign, us_to_text(err_us));

			printf("Skew %f ; err %d ppb\n", (double)skew,
			       timeutil_sync_skew_to_ppb(skew));
		} else if (rc < 0) {
			printf("Sync update error: %d\n", rc);
		}
	}
	(void)k_work_schedule(k_work_delayable_from_work(work),
			      K_SECONDS(UPDATE_INTERVAL_S));
}

void main(void)
{
	uint32_t top;
	int rc;

	/* Grab the clock driver */
	if (!device_is_ready(clock0)) {
		printk("%s: device not ready.\n", clock0->name);
		return;
	}

	show_clocks("Power-up clocks");

	if (IS_ENABLED(CONFIG_APP_ENABLE_HFXO)) {
		rc = clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);
		printk("Enable HFXO got %d\n", rc);
	}

	/* Grab the timer. */
	if (!device_is_ready(timer0)) {
		printk("%s: device not ready.\n", timer0->name);
		return;
	}

	/* Apparently there's no API to configure a frequency at
	 * runtime, so live with whatever we get.
	 */
	sync_config.ref_Hz = counter_get_frequency(timer0);
	if (sync_config.ref_Hz == 0) {
		printk("Timer %s has no fixed frequency\n",
			timer0->name);
		return;
	}

	top = counter_get_top_value(timer0);
	if (top != UINT32_MAX) {
		printk("Timer %s wraps at %u (0x%08x) not at 32 bits\n",
		       timer0->name, top, top);
		return;
	}

	rc = counter_start(timer0);
	printk("Start %s: %d\n", timer0->name, rc);

	show_clocks("Timer-running clocks");

	sync_config.local_Hz = CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	sync_state.cfg = &sync_config;

	printf("Checking %s at %u Hz against ticks at %u Hz\n",
	       timer0->name, sync_config.ref_Hz, sync_config.local_Hz);
	printf("Timer wraps every %u s\n",
	       (uint32_t)(BIT64(32) / sync_config.ref_Hz));

	k_work_init_delayable(&sync_work, sync_work_handler);
	rc = k_work_schedule(&sync_work, K_NO_WAIT);

	printk("Started sync: %d\n", rc);
}
