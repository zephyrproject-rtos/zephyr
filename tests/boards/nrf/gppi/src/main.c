/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_egu.h>
#if defined(CONFIG_SOC_NRF54H20_CPURAD)
#include <hal/nrf_ecb.h>
#endif
#include <hal/nrf_lpcomp.h>
#include <helpers/nrfx_gppi.h>

NRF_TIMER_Type *timer0 = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(dut_timer0));
NRF_TIMER_Type *timer1 = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(dut_timer1));
NRF_TIMER_Type *timer2 = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(dut_timer2));

#if DT_NODE_EXISTS(DT_NODELABEL(comp)) && DT_NODE_HAS_STATUS(DT_NODELABEL(comp), reserved)
NRF_LPCOMP_Type *lpcomp = (NRF_LPCOMP_Type *)DT_REG_ADDR(DT_NODELABEL(comp));

static void sink_setup(void)
{
	nrf_lpcomp_task_trigger(lpcomp, NRF_LPCOMP_TASK_STOP);
	nrf_lpcomp_event_clear(lpcomp, NRF_LPCOMP_EVENT_READY);
	nrf_lpcomp_enable(lpcomp);
}

static void sink_cleanup(void)
{
	nrf_lpcomp_task_trigger(lpcomp, NRF_LPCOMP_TASK_STOP);
	nrf_lpcomp_disable(lpcomp);
}

static bool sink_evt_check(void)
{
	return nrf_lpcomp_event_check(lpcomp, NRF_LPCOMP_EVENT_READY);
}

static uint32_t sink_tsk_addr(void)
{
	return nrf_lpcomp_task_address_get(lpcomp, NRF_LPCOMP_TASK_START);
}

#elif defined(CONFIG_SOC_NRF54H20_CPURAD)

static uint32_t sink_tsk_addr(void)
{
	return nrf_ecb_task_address_get(NRF_ECB030, NRF_ECB_TASK_START);
}
static void sink_setup(void)
{
	nrf_ecb_event_clear(NRF_ECB030, NRF_ECB_EVENT_ERROR);
}

static void sink_cleanup(void)
{
	nrf_ecb_event_clear(NRF_ECB030, NRF_ECB_EVENT_ERROR);
}

static bool sink_evt_check(void)
{
	return nrf_ecb_event_check(NRF_ECB030, NRF_ECB_EVENT_ERROR);
}

#else
#error "Target not supported"
#endif

/* Setup a single PPI connection TIMER_COMPARE->sink task. Use various timers. */
static void test_single_connection(NRF_TIMER_Type *timer)
{
	uint32_t evt = nrf_timer_event_address_get(timer, NRF_TIMER_EVENT_COMPARE0);
	uint32_t tsk = sink_tsk_addr();
	nrfx_gppi_handle_t handle;
	int rv;

	nrf_timer_mode_set(timer, NRF_TIMER_MODE_TIMER);
	nrf_timer_cc_set(timer, NRF_TIMER_CC_CHANNEL0, 100);
	nrf_timer_event_clear(timer, NRF_TIMER_EVENT_COMPARE0);

	sink_setup();

	rv = nrfx_gppi_conn_alloc(evt, tsk, &handle);
	zassert_ok(rv);

	/* Enable PPI connection and validate that task-event connection is working. */
	nrfx_gppi_conn_enable(handle);

	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_START);
	k_busy_wait(1000);

	zassert_equal(nrf_timer_event_check(timer, NRF_TIMER_EVENT_COMPARE0), 1);
	zassert_true(sink_evt_check());

	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_STOP);
	nrf_timer_event_clear(timer, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_CLEAR);

	sink_setup();

	/* Disable PPI to check that task is not triggered. */
	nrfx_gppi_conn_disable(handle);

	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_START);
	k_busy_wait(1000);

	/* TIMER event is set but sink event is not which means that sink task START was not
	 * triggered.
	 */
	zassert_equal(nrf_timer_event_check(timer, NRF_TIMER_EVENT_COMPARE0), 1);
	zassert_false(sink_evt_check());

	/* Clean up. */
	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_STOP);
	nrf_timer_event_clear(timer, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_task_trigger(timer, NRF_TIMER_TASK_CLEAR);

	sink_cleanup();

	nrfx_gppi_conn_free(evt, tsk, handle);
}

ZTEST(gppi, test_basic)
{
	test_single_connection(timer0);
	test_single_connection(timer1);
	test_single_connection(timer2);
}

/* Test is checking that it is possible to attach task to a connection.
 *
 * Connection TIMER0_COMPARE0->sink_task
 * Attached TIMER1_CAPTURE0
 */
ZTEST(gppi, test_attach_task)
{
	uint32_t evt = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE0);
	uint32_t tsk = sink_tsk_addr();
	uint32_t tsk2 = nrf_timer_task_address_get(timer1, NRF_TIMER_TASK_CAPTURE0);
	nrfx_gppi_handle_t handle;
	int rv;

	/* Setup TIMER0 TIMER1 in timer mode, set CC0 to 100 on TIMER0 */
	nrf_timer_mode_set(timer0, NRF_TIMER_MODE_TIMER);
	nrf_timer_mode_set(timer1, NRF_TIMER_MODE_TIMER);
	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL0, 100);
	nrf_timer_cc_set(timer1, NRF_TIMER_CC_CHANNEL0, 0);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE0);

	/* Prepare sink */
	sink_setup();

	/* Setup PPI connection. */
	rv = nrfx_gppi_conn_alloc(evt, tsk, &handle);
	zassert_ok(rv);

	/* Attach task to the connection. */
	rv = nrfx_gppi_ep_attach(tsk2, handle);
	zassert_ok(rv);

	nrfx_gppi_conn_enable(handle);

	/* Start both timers. */
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_START);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_START);

	/* Wait and validate that COMPARE0 event occurred. */
	k_busy_wait(1000);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE0), 1);

	/* Validate that PPI connection triggered both tasks (sink task and TIMER CAPTURE). */
	zassert_true(sink_evt_check());
	zassert_true(nrf_timer_cc_get(timer1, NRF_TIMER_CC_CHANNEL0) != 0);

	/* Clean up. */
	nrfx_gppi_conn_disable(handle);

	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_CLEAR);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_CLEAR);
	sink_cleanup();

	nrfx_gppi_ep_clear(tsk2);
	nrfx_gppi_conn_free(evt, tsk, handle);
}

/* Test is checking that it is possible to attach events to a connection.
 *
 * Connection TIMER0_COMPARE0->TIMER1_COUNT
 * Attached TIMER0_COMPARE1
 */
ZTEST(gppi, test_attach_event)
{
	if (IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)) {
		ztest_test_skip();
	}

	uint32_t evt = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE0);
	uint32_t evt2 = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE1);
	uint32_t tsk = nrf_timer_task_address_get(timer1, NRF_TIMER_TASK_COUNT);
	nrfx_gppi_handle_t handle;
	int rv;

	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL0, 100);
	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL1, 200);
	nrf_timer_mode_set(timer1, NRF_TIMER_MODE_COUNTER);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE1);

	/* Setup  PPI connection. */
	rv = nrfx_gppi_conn_alloc(evt, tsk, &handle);
	zassert_ok(rv);

	rv = nrfx_gppi_ep_attach(evt2, handle);
	zassert_ok(rv);

	nrfx_gppi_conn_enable(handle);

	/* Start timers. */
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_START);
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_START);

	/* Wait and check that both COMPARE events expired. */
	k_busy_wait(1000);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE0), 1);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE1), 1);

	/* TIMER1 should be incremented twice by both events. */
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_CAPTURE0);
	zassert_equal(nrf_timer_cc_get(timer1, NRF_TIMER_CC_CHANNEL0), 2);

	/* Clean up. */
	nrfx_gppi_conn_disable(handle);
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_CLEAR);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE1);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_CLEAR);

	nrfx_gppi_ep_clear(evt2);
	nrfx_gppi_conn_free(evt, tsk, handle);
}

/* Test is checking PPI group functionality. Group can contain one or more PPI channel
 * and it has tasks for enabling and disabling all channel in the group.
 *
 * Test is using 2 TIMERs and has following connections:
 *
 * PPI connections that are included in a group:
 * 1a. TIMER0_COMPARE1->TIMER1_COUNT
 * 1b. TIMER0_COMPARE3->TIMER1_COUNT
 *
 * 2. TIMER0_COMPARE0->GROUP_EN
 * 3. TIMER0_COMPARE2->GROUP_DIS
 *
 * Compare channels in TIMER0 are set to 100, 110, 120 and 130.
 *
 * Expected behavior is that first event at 100 will enable the PPI group so that
 * the second compare event (at 110) will increment TIMER1 counter. Next event
 * (compare 2 at 120) will disable the group so that the last compare event (at 130)
 * will NOT increment TIMER1.
 */
ZTEST(gppi, test_group)
{
	uint32_t evt0 = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE0);
	uint32_t evt1 = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE1);
	uint32_t evt2 = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE2);
	uint32_t evt3 = nrf_timer_event_address_get(timer0, NRF_TIMER_EVENT_COMPARE3);
	uint32_t tsk = nrf_timer_task_address_get(timer1, NRF_TIMER_TASK_COUNT);
	uint32_t gtsk_en, gtsk_dis;
	nrfx_gppi_handle_t handle0;
	nrfx_gppi_handle_t handle1;
	nrfx_gppi_handle_t handle2;
	nrfx_gppi_handle_t handle3;
	nrfx_gppi_group_handle_t ghandle;
	uint32_t cc;
	int rv;

	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL0, 100);
	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL1, 110);
	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL2, 120);
	nrf_timer_cc_set(timer0, NRF_TIMER_CC_CHANNEL3, 130);
	nrf_timer_mode_set(timer1, NRF_TIMER_MODE_COUNTER);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE1);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE2);
	nrf_timer_event_clear(timer0, NRF_TIMER_EVENT_COMPARE3);

	/* PPI 1a. TIMER0_CC1->TIMER1_COUNT */
	rv = nrfx_gppi_conn_alloc(evt1, tsk, &handle0);
	zassert_ok(rv);

	/* Allocate a group and add connection 1 to the group. */
	rv = nrfx_gppi_group_alloc(nrfx_gppi_domain_id_get(evt1), &ghandle);
	zassert_ok(rv);

	rv = nrfx_gppi_group_ep_add(ghandle, evt1);
	zassert_ok(rv);

	if (IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)) {
		rv = nrfx_gppi_conn_alloc(evt3, tsk, &handle3);
		zassert_ok(rv);

		rv = nrfx_gppi_group_ep_add(ghandle, evt3);
		zassert_ok(rv);
	} else {
		/* PPI 1b. TIMER0_CC3->TIMER1_COUNT */
		rv = nrfx_gppi_ep_attach(evt3, handle0);
		zassert_ok(rv);
	}

	gtsk_en = nrfx_gppi_group_task_en_addr(ghandle);
	gtsk_dis = nrfx_gppi_group_task_dis_addr(ghandle);

	/* Allocate PPI 2. TIMER0_CC0->GROUP_EN */
	rv = nrfx_gppi_conn_alloc(evt0, gtsk_en, &handle1);
	zassert_ok(rv);

	/* Allocate PPI 3. TIMER0_CC2->GROUP_DIS */
	rv = nrfx_gppi_conn_alloc(evt2, gtsk_dis, &handle2);
	zassert_ok(rv);

	/* Enable connection but then disable the channel in the connection source.
	 * On single domain SoC it is redundant but on multi domain SoC it will enable
	 * channels used for a connection that uses multiple DPPIC and PPIB and disable
	 * the channel only for source. PPI group will then enable it.
	 */
	nrfx_gppi_conn_enable(handle0);
	nrfx_gppi_ep_chan_disable(evt1);

	/* Enable PPIs which enables and disables the group. Connection PPI 1 is now disabled. */
	nrfx_gppi_conn_enable(handle1);
	nrfx_gppi_conn_enable(handle2);

	/* Start both timers. */
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_START);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_CAPTURE0);
	zassert_equal(nrf_timer_cc_get(timer1, NRF_TIMER_CC_CHANNEL0), 0);

	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_START);

	/* Wait for all COMPARE events to expire. */
	k_busy_wait(1000);

	/* Stop timers and check that all events expired. */
	nrf_timer_task_trigger(timer0, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_STOP);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE0), 1);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE1), 1);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE2), 1);
	zassert_equal(nrf_timer_event_check(timer0, NRF_TIMER_EVENT_COMPARE3), 1);

	/* Validate that TIMER1 counter got incremented exactly once. */
	nrf_timer_task_trigger(timer1, NRF_TIMER_TASK_CAPTURE0);
	cc = nrf_timer_cc_get(timer1, NRF_TIMER_CC_CHANNEL0);
	zassert_equal(cc, 1, "Unexpected cc:%d (exp:%d)", cc, 1);

	/* Clean up. */
	nrfx_gppi_group_disable(ghandle);
	nrfx_gppi_ep_clear(evt3);
	nrfx_gppi_conn_disable(handle1);
	nrfx_gppi_conn_disable(handle2);
	nrfx_gppi_conn_free(evt1, tsk, handle0);
	nrfx_gppi_conn_free(evt0, gtsk_en, handle1);
	nrfx_gppi_conn_free(evt2, gtsk_dis, handle2);
	nrfx_gppi_group_free(ghandle);
}

#if defined(CONFIG_SOC_NRF54H20_CPURAD)
ZTEST(gppi, test_cpurad_slow_fast_domain)
{
	uint32_t eep0 = nrf_egu_event_address_get(NRF_EGU020, NRF_EGU_EVENT_TRIGGERED0);
	uint32_t tep0 = nrf_ecb_task_address_get(NRF_ECB030, NRF_ECB_TASK_START);
	uint32_t eep1 = nrf_ecb_event_address_get(NRF_ECB030, NRF_ECB_EVENT_ERROR);
	uint32_t tep1 = nrf_egu_task_address_get(NRF_EGU020, NRF_EGU_TASK_TRIGGER1);
	nrfx_gppi_handle_t handle[2];
	int rv;

	rv = nrfx_gppi_conn_alloc(eep0, tep0, &handle[0]);
	zassert_ok(rv);

	rv = nrfx_gppi_conn_alloc(eep1, tep1, &handle[1]);
	zassert_ok(rv);

	nrfx_gppi_conn_enable(handle[0]);
	nrfx_gppi_conn_enable(handle[1]);

	nrf_egu_event_clear(NRF_EGU020, NRF_EGU_EVENT_TRIGGERED1);
	nrf_egu_task_trigger(NRF_EGU020, NRF_EGU_TASK_TRIGGER0);

	k_busy_wait(10);

	zassert_true(nrf_egu_event_check(NRF_EGU020, NRF_EGU_EVENT_TRIGGERED1));
	nrf_egu_event_clear(NRF_EGU020, NRF_EGU_EVENT_TRIGGERED1);
	nrf_ecb_event_clear(NRF_ECB030, NRF_ECB_EVENT_ERROR);

	nrfx_gppi_conn_disable(handle[0]);
	nrfx_gppi_conn_disable(handle[1]);
	nrfx_gppi_conn_free(eep0, tep0, handle[0]);
	nrfx_gppi_conn_free(eep1, tep1, handle[1]);
}
#endif

ZTEST_SUITE(gppi, NULL, NULL, NULL, NULL, NULL);
