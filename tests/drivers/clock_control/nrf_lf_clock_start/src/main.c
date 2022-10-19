/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static nrf_clock_lfclk_t type;
static bool on;
static uint32_t rtc_cnt;

static void xtal_check(bool on, nrf_clock_lfclk_t type)
{
	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)) {
		zassert_false(on, "Clock should be off");
	} else if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY)) {
		bool is_running =
			rtc_cnt || (on && (type == NRF_CLOCK_LFCLK_RC));

		zassert_true(is_running, "Clock should be on");
	} else {
		zassert_true(on, "Clock should be on");
		zassert_equal(type, NRF_CLOCK_LFCLK_Xtal);
	}
}

static void rc_check(bool on, nrf_clock_lfclk_t type)
{
	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)) {
		zassert_false(on, "Clock should be off");
	} else {
		zassert_true(on, "Clock should be on");
		zassert_equal(type, NRF_CLOCK_LFCLK_RC);
	}
}

static void synth_check(bool on, nrf_clock_lfclk_t type)
{
	#if !defined(CLOCK_LFCLKSRC_SRC_Synth) && \
	    !defined(CLOCK_LFCLKSRC_SRC_LFSYNT)
	#define NRF_CLOCK_LFCLK_Synth 0
	#endif

	if (!IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)) {
		zassert_true(on, "Clock should be on");
		zassert_equal(type, NRF_CLOCK_LFCLK_Synth);
	}
}

ZTEST(nrf_lf_clock_start, test_clock_check)
{
	bool xtal;

	xtal = IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL) |
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING) |
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING);

	if (xtal) {
		xtal_check(on, type);
	} else if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)) {
		rc_check(on, type);
	} else {
		synth_check(on, type);
	}
}

ZTEST(nrf_lf_clock_start, test_wait_in_thread)
{
	nrf_clock_lfclk_t t;
	bool o;

	if (!(IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT) &&
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL))) {
		return;
	}

	z_nrf_clock_control_lf_on(CLOCK_CONTROL_NRF_LF_START_AVAILABLE);
	o = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &t);
	zassert_false((t == NRF_CLOCK_LFCLK_Xtal) && o);
	k_busy_wait(35);
	zassert_true(k_cycle_get_32() > 0);

	z_nrf_clock_control_lf_on(CLOCK_CONTROL_NRF_LF_START_STABLE);
	o = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &t);
	zassert_true((t == NRF_CLOCK_LFCLK_Xtal) && o);
}

void *test_init(void)
{
	TC_PRINT("CLOCK_CONTROL_NRF_K32SRC=%s\n",
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)    ? "RC" :
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH) ? "SYNTH" :
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL)  ? "XTAL"
								  : "???");
	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)) {
		TC_PRINT("SYSTEM_CLOCK_NO_WAIT=y\n");
	}
	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY)) {
		TC_PRINT("SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY=y\n");
	}
	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_STABILITY)) {
		TC_PRINT("SYSTEM_CLOCK_WAIT_FOR_STABILITY=y\n");
	}
	return NULL;
}
ZTEST_SUITE(nrf_lf_clock_start, NULL, test_init, NULL, NULL, NULL);

/* This test needs to read the LF clock state soon after the system clock is
 * started (to check if the starting routine waits for the LF clock or not),
 * so do it at the beginning of the POST_KERNEL stage (the system clock is
 * started in PRE_KERNEL_2). Reading of the clock state in the ZTEST setup
 * function turns out to be too late.
 */
static int get_lfclk_state(void)
{

	/* Do clock state read as early as possible. When RC is already running
	 * and XTAL has been started then LFSRCSTAT register content might be
	 * not valid, in that case read system clock to check if it has
	 * progressed.
	 */
	on = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &type);
	k_busy_wait(100);
	rtc_cnt = k_cycle_get_32();

	return 0;
}
SYS_INIT(get_lfclk_state, POST_KERNEL, 0);
