/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#include <hal/nrf_egu.h>
#include <hal/nrf_ppi.h>
#include <hal/nrf_timer.h>

#include <zephyr/ztest.h>

#define TIMER_NO 2
#define TIMER_INSTANCE NRFX_CONCAT_2(NRF_TIMER, TIMER_NO)
#define TIMER_IRQ TIMER2_IRQn
#define TIMER_INT NRF_TIMER_INT_COMPARE0_MASK
#define TIMER_PRIORITY 5
#define TIMER_DELAY_TICKS 100
#define EGU_DELAY_USEC 200

struct timer_isr_context {
	unsigned int egu_channel;
};

#define NRF_NEGUs 6
#define NRF_NEGU_NEVENTS 16
static const NRF_EGU_Type *EGU[NRF_NEGUs] = {
	NRF_EGU0, NRF_EGU1, NRF_EGU2, NRF_EGU3, NRF_EGU4, NRF_EGU5
};

ZTEST(nrf_egu_tests, test_channels_count)
{
	for (int i = 0; i < NRF_NEGUs; i++) {
		zassert_equal(16, nrf_egu_channel_count(EGU[i]),
				"NRF_EGU%d incorrect number of channels", i);
	}
}

ZTEST(nrf_egu_tests, test_task_address_get)
{
	nrf_egu_task_t egu_tasks[NRF_NEGU_NEVENTS] = {
			NRF_EGU_TASK_TRIGGER0, NRF_EGU_TASK_TRIGGER1, NRF_EGU_TASK_TRIGGER2,
			NRF_EGU_TASK_TRIGGER3, NRF_EGU_TASK_TRIGGER4, NRF_EGU_TASK_TRIGGER5,
			NRF_EGU_TASK_TRIGGER6, NRF_EGU_TASK_TRIGGER7, NRF_EGU_TASK_TRIGGER8,
			NRF_EGU_TASK_TRIGGER9, NRF_EGU_TASK_TRIGGER10, NRF_EGU_TASK_TRIGGER11,
			NRF_EGU_TASK_TRIGGER12, NRF_EGU_TASK_TRIGGER13, NRF_EGU_TASK_TRIGGER14,
			NRF_EGU_TASK_TRIGGER15
	};

	for (int i = 0; i < NRF_NEGUs; i++) {
		for (int j = 0; j < NRF_NEGU_NEVENTS; j++) {
			zassert_equal((size_t)&EGU[i]->TASKS_TRIGGER[j],
					nrf_egu_task_address_get(EGU[i], egu_tasks[j]),
					  "NRF_EGU_%i incorrect address of task trigger %i", i, j);
		}
	}
}

ZTEST(nrf_egu_tests, test_event_address_get)
{
	nrf_egu_event_t egu_events[NRF_NEGU_NEVENTS] = {
			NRF_EGU_EVENT_TRIGGERED0, NRF_EGU_EVENT_TRIGGERED1,
			NRF_EGU_EVENT_TRIGGERED2, NRF_EGU_EVENT_TRIGGERED3,
			NRF_EGU_EVENT_TRIGGERED4, NRF_EGU_EVENT_TRIGGERED5,
			NRF_EGU_EVENT_TRIGGERED6, NRF_EGU_EVENT_TRIGGERED7,
			NRF_EGU_EVENT_TRIGGERED8, NRF_EGU_EVENT_TRIGGERED9,
			NRF_EGU_EVENT_TRIGGERED10, NRF_EGU_EVENT_TRIGGERED11,
			NRF_EGU_EVENT_TRIGGERED12, NRF_EGU_EVENT_TRIGGERED13,
			NRF_EGU_EVENT_TRIGGERED14, NRF_EGU_EVENT_TRIGGERED15
	};

	for (int i = 0; i < NRF_NEGUs; i++) {
		for (int j = 0; j < NRF_NEGU_NEVENTS; j++) {
			zassert_equal((size_t)&EGU[i]->EVENTS_TRIGGERED[j],
					nrf_egu_event_address_get(EGU[i], egu_events[j]),
					  "NRF_EGU_%i incorrect address of event trigger %i", i, j);
		}
	}
}

ZTEST(nrf_egu_tests, test_channel_int_get)
{
	nrf_egu_int_mask_t egu_masks[NRF_NEGU_NEVENTS] = {
			NRF_EGU_INT_TRIGGERED0, NRF_EGU_INT_TRIGGERED1, NRF_EGU_INT_TRIGGERED2,
			NRF_EGU_INT_TRIGGERED3, NRF_EGU_INT_TRIGGERED4, NRF_EGU_INT_TRIGGERED5,
			NRF_EGU_INT_TRIGGERED6, NRF_EGU_INT_TRIGGERED7, NRF_EGU_INT_TRIGGERED8,
			NRF_EGU_INT_TRIGGERED9, NRF_EGU_INT_TRIGGERED10, NRF_EGU_INT_TRIGGERED11,
			NRF_EGU_INT_TRIGGERED12, NRF_EGU_INT_TRIGGERED13, NRF_EGU_INT_TRIGGERED14,
			NRF_EGU_INT_TRIGGERED15
	};

	for (int i = 0; i < NRF_NEGU_NEVENTS; i++) {
		zassert_equal(egu_masks[i], nrf_egu_channel_int_get(i),
				  "Incorrect offset for channel %i", i);
	}
}

struct SWI_trigger_assert_parameter {
	bool triggered[EGU5_CH_NUM];
	uint32_t call_count;
};

static struct SWI_trigger_assert_parameter event_triggered_flag;

void SWI5_trigger_function(const void *param)
{
	event_triggered_flag.call_count++;
	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		event_triggered_flag.triggered[i] = nrf_egu_event_check(NRF_EGU5, check_event);
		if (event_triggered_flag.triggered[i]) {
			nrf_egu_event_clear(NRF_EGU5, check_event);
		}
	}
}

ZTEST(nrf_egu_tests, test_task_trigger_not_int)
{
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED1);
	zassert_equal(0, nrf_egu_int_enable_check(NRF_EGU5, NRF_EGU_INT_TRIGGERED1),
			  "interrupt has not been disabled");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);
	nrf_egu_task_t task_to_trigger = nrf_egu_trigger_task_get(0);

	nrf_egu_task_trigger(NRF_EGU5, task_to_trigger);

	k_busy_wait(1000);
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED1);

	zassert_equal(0, event_triggered_flag.call_count, "interrupt has been called");
	for (int i = 0 ; i < NRF_NEGU_NEVENTS; i++) {
		zassert_false(event_triggered_flag.triggered[i], "Event %i has been triggered", i);
	}

	zassert_true(nrf_egu_event_check(NRF_EGU5, nrf_egu_triggered_event_get(0)),
			 "event has not been triggered");
	for (uint8_t i = 1; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		zassert_false(nrf_egu_event_check(NRF_EGU5, check_event),
				"event %d has been triggered, but it shouldn't", i);
	}
}

ZTEST(nrf_egu_tests, test_task_trigger)
{
	nrf_egu_int_enable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);
	zassert_equal(NRF_EGU_INT_TRIGGERED0,
			nrf_egu_int_enable_check(NRF_EGU5, NRF_EGU_INT_TRIGGERED0),
			"failed to enable interrupt");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);
	nrf_egu_task_t task_to_trigger = nrf_egu_trigger_task_get(0);

	nrf_egu_task_trigger(NRF_EGU5, task_to_trigger);

	k_busy_wait(1000);
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);

	zassert_equal(1, event_triggered_flag.call_count, "zassert failed count = %d ",
			  event_triggered_flag.call_count);

	zassert_true(event_triggered_flag.triggered[0], "Event 0 has not been triggered");
	for (int i = 1 ; i < NRF_NEGU_NEVENTS; i++) {
		zassert_false(event_triggered_flag.triggered[i], "Event %i has been triggered", i);
	}

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		zassert_false(nrf_egu_event_check(NRF_EGU5, check_event),
				"event %d has been triggered, but it shouldn't", i);
	}
}

ZTEST(nrf_egu_tests, test_task_configure_not_trigger)
{
	nrf_egu_int_mask_t egu_int_mask = nrf_egu_channel_int_get(0);

	zassert_equal(NRF_EGU_INT_TRIGGERED0, egu_int_mask, "interrupt mask is invalid");
	nrf_egu_int_enable(NRF_EGU5, egu_int_mask);
	zassert_equal(egu_int_mask, nrf_egu_int_enable_check(NRF_EGU5, egu_int_mask),
			  "failed to enable interrupt");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);

	k_busy_wait(1000);
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, egu_int_mask);

	zassert_equal(0, event_triggered_flag.call_count, "interrupt has been called");
	for (int i = 0 ; i < NRF_NEGU_NEVENTS; i++) {
		zassert_false(event_triggered_flag.triggered[i], "Event %i has been triggered", i);
	}

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		zassert_false(nrf_egu_event_check(NRF_EGU5, check_event),
				"event %d has been triggered, but it shouldn't", i);
	}
}

static void timer_isr(const void *timer_isr_ctx)
{
	struct timer_isr_context *ctx = (struct timer_isr_context *)timer_isr_ctx;

	if (ctx) {
		nrf_egu_task_trigger(NRF_EGU5, nrf_egu_trigger_task_get(ctx->egu_channel));
	}

	nrf_timer_event_clear(TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_disable(TIMER_INSTANCE, TIMER_INT);
}

ZTEST(nrf_egu_tests, test_trigger_from_another_irq)
{
	uint32_t call_cnt_expected = event_triggered_flag.call_count + 1;
	static struct timer_isr_context timer_isr_ctx = {
		.egu_channel = 0
	};

	/* Timer cleanup */
	nrf_timer_event_clear(TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_CLEAR);

	/* Timer setup */
	irq_connect_dynamic(TIMER_IRQ, TIMER_PRIORITY, timer_isr, &timer_isr_ctx, 0);
	irq_enable(TIMER_IRQ);

	nrf_timer_mode_set(TIMER_INSTANCE, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(TIMER_INSTANCE, TIMER_BITMODE_BITMODE_16Bit);
	nrf_timer_prescaler_set(TIMER_INSTANCE,
				NRF_TIMER_PRESCALER_CALCULATE(
					NRF_TIMER_BASE_FREQUENCY_GET(TIMER_INSTANCE),
					NRFX_MHZ_TO_HZ(1)));
	nrf_timer_cc_set(TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL0, TIMER_DELAY_TICKS);
	nrf_timer_int_enable(TIMER_INSTANCE, TIMER_INT);

	/* EGU setup */
	nrf_egu_int_enable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);

	nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_START);
	k_busy_wait(EGU_DELAY_USEC);

	zassert_equal(call_cnt_expected, event_triggered_flag.call_count,
			"interrupt called unexpected number of times %d",
			event_triggered_flag.call_count);
}

static struct SWI_trigger_assert_parameter SWI4_event_triggered_flag;

static void SWI4_trigger_function(const void *param)
{
	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU4); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		SWI4_event_triggered_flag.triggered[i] = nrf_egu_event_check(NRF_EGU4, check_event);
		if (SWI4_event_triggered_flag.triggered[i]) {
			nrf_egu_event_clear(NRF_EGU4, check_event);
		}
	}
}

ZTEST(nrf_egu_tests, test_trigger_by_PPI)
{
	nrf_ppi_channel_enable(NRF_PPI, NRF_PPI_CHANNEL0);
	nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL0,
		nrf_egu_event_address_get(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0),
		nrf_egu_task_address_get(NRF_EGU4, NRF_EGU_TASK_TRIGGER0));

	memset(&SWI4_event_triggered_flag, 0, sizeof(SWI4_event_triggered_flag));
	irq_connect_dynamic(SWI4_EGU4_IRQn, 0, SWI4_trigger_function, NULL, BIT(0));

	/* configure egu4 */
	nrf_egu_int_enable(NRF_EGU4, NRF_EGU_INT_TRIGGERED0);
	irq_enable(SWI4_EGU4_IRQn);

	/* trigger egu3 */
	nrf_egu_task_trigger(NRF_EGU3, NRF_EGU_TASK_TRIGGER0);

	k_busy_wait(1000);
	irq_disable(SWI4_EGU4_IRQn);
	nrf_egu_int_disable(NRF_EGU4, NRF_EGU_INT_TRIGGERED0);

	/* EGU3 should forward the trigger to EGU4 via PPI, and SWI4 IRQ is expected */
	/* IRQ for EGU3 is not enabled */

	zassert_true(SWI4_event_triggered_flag.triggered[0], "Event 0 has not been triggered");
	for (int i = 1 ; i < NRF_NEGU_NEVENTS; i++) {
		zassert_false(SWI4_event_triggered_flag.triggered[i],
				"Event %i has been triggered", i);
	}

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU4); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);

		zassert_false(nrf_egu_event_check(NRF_EGU4, check_event),
				"event %d has been triggered, but it shouldn't", i);
	}
}

static void test_clean_egu(void *ignored)
{
	ARG_UNUSED(ignored);
	memset(NRF_EGU5, 0, sizeof(*NRF_EGU5));
}

ZTEST_SUITE(nrf_egu_tests, NULL, NULL, test_clean_egu, test_clean_egu, NULL);
