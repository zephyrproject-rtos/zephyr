/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include <hal/nrf_egu.h>
#include <hal/nrf_ppi.h>
#include <hal/nrf_timer.h>

#include <ztest.h>

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

void test_egu_channels_count(void)
{
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU0), "NRF_EGU0 incorrect number of channels");
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU1), "NRF_EGU1 incorrect number of channels");
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU2), "NRF_EGU2 incorrect number of channels");
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU3), "NRF_EGU3 incorrect number of channels");
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU4), "NRF_EGU4 incorrect number of channels");
	zassert_equal(16, nrf_egu_channel_count(NRF_EGU5), "NRF_EGU5 incorrect number of channels");
}

void test_nrf_egu_task_address_get(void)
{

	zassert_equal((size_t)&NRF_EGU0->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU0,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_0 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU0->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU0,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_0 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU0->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU0,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_0 incorrect address of task trigger 15");

	zassert_equal((size_t)&NRF_EGU1->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU1,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_1 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU1->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU1,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_1 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU1->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU1,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_1 incorrect address of task trigger 15");

	zassert_equal((size_t)&NRF_EGU2->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU2,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_2 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU2->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU2,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_2 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU2->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU2,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_2 incorrect address of task trigger 15");

	zassert_equal((size_t)&NRF_EGU3->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU3,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_3 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU3->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU3,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_3 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU3->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU3,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_3 incorrect address of task trigger 15");

	zassert_equal((size_t)&NRF_EGU4->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU4,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_4 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU4->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU4,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_4 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU4->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU4,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_4 incorrect address of task trigger 15");

	zassert_equal((size_t)&NRF_EGU5->TASKS_TRIGGER[0], nrf_egu_task_address_get(NRF_EGU5,
											NRF_EGU_TASK_TRIGGER0),
			  "NRF_EGU_5 incorrect address of task trigger 0");
	zassert_equal((size_t)&NRF_EGU5->TASKS_TRIGGER[1], nrf_egu_task_address_get(NRF_EGU5,
											NRF_EGU_TASK_TRIGGER1),
			  "NRF_EGU_5 incorrect address of task trigger 1");
	zassert_equal((size_t)&NRF_EGU5->TASKS_TRIGGER[15], nrf_egu_task_address_get(NRF_EGU5,
											 NRF_EGU_TASK_TRIGGER15),
			  "NRF_EGU_5 incorrect address of task trigger 15");
}

void test_nrf_egu_event_address_get(void)
{
	zassert_equal((size_t)&NRF_EGU0->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_0 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU0->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_0 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU0->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU0,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_0 incorrect address of event trigger 15");

	zassert_equal((size_t)&NRF_EGU1->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU1, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_1 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU1->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU1, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_1 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU1->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU1,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_1 incorrect address of event trigger 15");

	zassert_equal((size_t)&NRF_EGU2->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU2, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_2 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU2->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU2, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_2 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU2->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU2,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_2 incorrect address of event trigger 15");

	zassert_equal((size_t)&NRF_EGU3->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_3 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU3->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_3 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU3->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU3,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_3 incorrect address of event trigger 15");

	zassert_equal((size_t)&NRF_EGU4->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU4, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_4 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU4->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU4, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_4 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU4->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU4,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_4 incorrect address of event trigger 15");

	zassert_equal((size_t)&NRF_EGU5->EVENTS_TRIGGERED[0],
			  nrf_egu_event_address_get(NRF_EGU5, NRF_EGU_EVENT_TRIGGERED0),
			  "NRF_EGU_5 incorrect address of event trigger 0");
	zassert_equal((size_t)&NRF_EGU5->EVENTS_TRIGGERED[1],
			  nrf_egu_event_address_get(NRF_EGU5, NRF_EGU_EVENT_TRIGGERED1),
			  "NRF_EGU_5 incorrect address of event trigger 1");
	zassert_equal((size_t)&NRF_EGU5->EVENTS_TRIGGERED[15],
			  nrf_egu_event_address_get(NRF_EGU5,
						NRF_EGU_EVENT_TRIGGERED15),
			  "NRF_EGU_5 incorrect address of event trigger 15");
}

void test_nrf_egu_channel_int_get()
{
	zassert_equal(NRF_EGU_INT_TRIGGERED0, nrf_egu_channel_int_get(0),
			  "Incorrect offset for channel 0");
	zassert_equal(NRF_EGU_INT_TRIGGERED1, nrf_egu_channel_int_get(1),
			  "Incorrect offset for channel 1");
	zassert_equal(NRF_EGU_INT_TRIGGERED2, nrf_egu_channel_int_get(2),
			  "Incorrect offset for channel 2");
	zassert_equal(NRF_EGU_INT_TRIGGERED3, nrf_egu_channel_int_get(3),
			  "Incorrect offset for channel 3");
	zassert_equal(NRF_EGU_INT_TRIGGERED4, nrf_egu_channel_int_get(4),
			  "Incorrect offset for channel 4");
	zassert_equal(NRF_EGU_INT_TRIGGERED5, nrf_egu_channel_int_get(5),
			  "Incorrect offset for channel 5");
	zassert_equal(NRF_EGU_INT_TRIGGERED6, nrf_egu_channel_int_get(6),
			  "Incorrect offset for channel 6");
	zassert_equal(NRF_EGU_INT_TRIGGERED7, nrf_egu_channel_int_get(7),
			  "Incorrect offset for channel 7");
	zassert_equal(NRF_EGU_INT_TRIGGERED8, nrf_egu_channel_int_get(8),
			  "Incorrect offset for channel 8");
	zassert_equal(NRF_EGU_INT_TRIGGERED9, nrf_egu_channel_int_get(9),
			  "Incorrect offset for channel 9");
	zassert_equal(NRF_EGU_INT_TRIGGERED10, nrf_egu_channel_int_get(10),
			  "Incorrect offset for channel 10");
	zassert_equal(NRF_EGU_INT_TRIGGERED11, nrf_egu_channel_int_get(11),
			  "Incorrect offset for channel 11");
	zassert_equal(NRF_EGU_INT_TRIGGERED12, nrf_egu_channel_int_get(12),
			  "Incorrect offset for channel 12");
	zassert_equal(NRF_EGU_INT_TRIGGERED13, nrf_egu_channel_int_get(13),
			  "Incorrect offset for channel 13");
	zassert_equal(NRF_EGU_INT_TRIGGERED14, nrf_egu_channel_int_get(14),
			  "Incorrect offset for channel 14");
	zassert_equal(NRF_EGU_INT_TRIGGERED15, nrf_egu_channel_int_get(15),
			  "Incorrect offset for channel 15");
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

void test_nrf_egu_task_trigger_not_int(void)
{
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED1);
	zassert_equal(0, nrf_egu_int_enable_check(NRF_EGU5, NRF_EGU_INT_TRIGGERED1),
			  "interrupt has not been disabled");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);
	nrf_egu_task_t task_to_trigger = nrf_egu_trigger_task_get(0);

	nrf_egu_task_trigger(NRF_EGU5, task_to_trigger);

	// k_sleep(K_MSEC(1));
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED1);

	zassert_equal(0, event_triggered_flag.call_count, "interrupt has been called");

	zassert_false(event_triggered_flag.triggered[0], "Event 0 has been triggered");
	zassert_false(event_triggered_flag.triggered[1], "Event 1 has been triggered");
	zassert_false(event_triggered_flag.triggered[2], "Event 2 has been triggered");
	zassert_false(event_triggered_flag.triggered[3], "Event 3 has been triggered");
	zassert_false(event_triggered_flag.triggered[4], "Event 4 has been triggered");
	zassert_false(event_triggered_flag.triggered[5], "Event 5 has been triggered");
	zassert_false(event_triggered_flag.triggered[6], "Event 6 has been triggered");
	zassert_false(event_triggered_flag.triggered[7], "Event 7 has been triggered");
	zassert_false(event_triggered_flag.triggered[8], "Event 8 has been triggered");
	zassert_false(event_triggered_flag.triggered[9], "Event 9 has been triggered");
	zassert_false(event_triggered_flag.triggered[10], "Event 10 has been triggered");
	zassert_false(event_triggered_flag.triggered[11], "Event 11 has been triggered");
	zassert_false(event_triggered_flag.triggered[12], "Event 12 has been triggered");
	zassert_false(event_triggered_flag.triggered[13], "Event 13 has been triggered");
	zassert_false(event_triggered_flag.triggered[14], "Event 14 has been triggered");
	zassert_false(event_triggered_flag.triggered[15], "Event 15 has been triggered");

	zassert_true(nrf_egu_event_check(NRF_EGU5, nrf_egu_triggered_event_get(0)),
			 "event has not been triggered");
	for (uint8_t i = 1; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);
		zassert_false(nrf_egu_event_check(NRF_EGU5,
						  check_event), "event %d has been triggered, but it shouldn't", i);
	}
}

void test_nrf_egu_task_trigger(void)
{

	nrf_egu_int_enable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);
	zassert_equal(NRF_EGU_INT_TRIGGERED0, nrf_egu_int_enable_check(NRF_EGU5,
									   NRF_EGU_INT_TRIGGERED0),
			  "failed to enable interrupt");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);
	nrf_egu_task_t task_to_trigger = nrf_egu_trigger_task_get(0);
	nrf_egu_task_trigger(NRF_EGU5, task_to_trigger);

	// k_sleep(K_MSEC(1));
	k_busy_wait(1000);
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);

	zassert_equal(1, event_triggered_flag.call_count, "zassert failed count = %d ",
			  event_triggered_flag.call_count);

	zassert_true(event_triggered_flag.triggered[0], "Event 0 has not been triggered");
	zassert_false(event_triggered_flag.triggered[1], "Event 1 has been triggered");
	zassert_false(event_triggered_flag.triggered[2], "Event 2 has been triggered");
	zassert_false(event_triggered_flag.triggered[3], "Event 3 has been triggered");
	zassert_false(event_triggered_flag.triggered[4], "Event 4 has been triggered");
	zassert_false(event_triggered_flag.triggered[5], "Event 5 has been triggered");
	zassert_false(event_triggered_flag.triggered[6], "Event 6 has been triggered");
	zassert_false(event_triggered_flag.triggered[7], "Event 7 has been triggered");
	zassert_false(event_triggered_flag.triggered[8], "Event 8 has been triggered");
	zassert_false(event_triggered_flag.triggered[9], "Event 9 has been triggered");
	zassert_false(event_triggered_flag.triggered[10], "Event 10 has been triggered");
	zassert_false(event_triggered_flag.triggered[11], "Event 11 has been triggered");
	zassert_false(event_triggered_flag.triggered[12], "Event 12 has been triggered");
	zassert_false(event_triggered_flag.triggered[13], "Event 13 has been triggered");
	zassert_false(event_triggered_flag.triggered[14], "Event 14 has been triggered");
	zassert_false(event_triggered_flag.triggered[15], "Event 15 has been triggered");

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);
		zassert_false(nrf_egu_event_check(NRF_EGU5,
						  check_event), "event %d has been triggered, but it shouldn't", i);
	}
}

void test_nrf_egu_task_configure_not_trigger(void)
{
	nrf_egu_int_mask_t egu_int_mask = nrf_egu_channel_int_get(0);

	zassert_equal(NRF_EGU_INT_TRIGGERED0, egu_int_mask, "interrupt mask is invalid");
	nrf_egu_int_enable(NRF_EGU5, egu_int_mask);
	zassert_equal(egu_int_mask, nrf_egu_int_enable_check(NRF_EGU5, egu_int_mask),
			  "failed to enable interrupt");
	memset(&event_triggered_flag, 0, sizeof(event_triggered_flag));
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);

	// k_sleep(K_MSEC(1));
	k_busy_wait(1000);
	irq_disable(SWI5_EGU5_IRQn);
	nrf_egu_int_disable(NRF_EGU5, egu_int_mask);

	zassert_equal(0, event_triggered_flag.call_count, "interrupt has been called");

	zassert_false(event_triggered_flag.triggered[0], "Event 0 has been triggered");
	zassert_false(event_triggered_flag.triggered[1], "Event 1 has been triggered");
	zassert_false(event_triggered_flag.triggered[2], "Event 2 has been triggered");
	zassert_false(event_triggered_flag.triggered[3], "Event 3 has been triggered");
	zassert_false(event_triggered_flag.triggered[4], "Event 4 has been triggered");
	zassert_false(event_triggered_flag.triggered[5], "Event 5 has been triggered");
	zassert_false(event_triggered_flag.triggered[6], "Event 6 has been triggered");
	zassert_false(event_triggered_flag.triggered[7], "Event 7 has been triggered");
	zassert_false(event_triggered_flag.triggered[8], "Event 8 has been triggered");
	zassert_false(event_triggered_flag.triggered[9], "Event 9 has been triggered");
	zassert_false(event_triggered_flag.triggered[10], "Event 10 has been triggered");
	zassert_false(event_triggered_flag.triggered[11], "Event 11 has been triggered");
	zassert_false(event_triggered_flag.triggered[12], "Event 12 has been triggered");
	zassert_false(event_triggered_flag.triggered[13], "Event 13 has been triggered");
	zassert_false(event_triggered_flag.triggered[14], "Event 14 has been triggered");
	zassert_false(event_triggered_flag.triggered[15], "Event 15 has been triggered");

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU5); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);
		zassert_false(nrf_egu_event_check(NRF_EGU5,
						  check_event), "event %d has been triggered, but it shouldn't", i);
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

void test_nrf_egu_trigger_from_another_irq(void)
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
	nrf_timer_frequency_set(TIMER_INSTANCE, NRF_TIMER_FREQ_1MHz);
	nrf_timer_cc_set(TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL0, TIMER_DELAY_TICKS);
	nrf_timer_int_enable(TIMER_INSTANCE, TIMER_INT);

	/* EGU setup */
	nrf_egu_int_enable(NRF_EGU5, NRF_EGU_INT_TRIGGERED0);
	irq_connect_dynamic(SWI5_EGU5_IRQn, 0, SWI5_trigger_function, NULL, BIT(0));
	irq_enable(SWI5_EGU5_IRQn);

	nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_START);
	k_busy_wait(EGU_DELAY_USEC);

	zassert_equal(call_cnt_expected, event_triggered_flag.call_count,
			  "interrupt called unexpected number of times %d", event_triggered_flag.call_count);
}


static struct SWI_trigger_assert_parameter SWI4_event_triggered_flag;

void SWI4_trigger_function(const void *param)
{
	SWI4_event_triggered_flag.call_count++;
	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU4); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);
		SWI4_event_triggered_flag.triggered[i] = nrf_egu_event_check(NRF_EGU4, check_event);
		if (SWI4_event_triggered_flag.triggered[i]) {
			nrf_egu_event_clear(NRF_EGU4, check_event);
		}
	}
}

void test_nrf_egu_trigger_by_PPI(void)
{
	nrf_ppi_channel_enable(NRF_PPI, NRF_PPI_CHANNEL0);
	nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL0, 
		nrf_egu_event_address_get(NRF_EGU3, NRF_EGU_EVENT_TRIGGERED0), 
		nrf_egu_task_address_get(NRF_EGU4, NRF_EGU_TASK_TRIGGER0));

	memset(&SWI4_event_triggered_flag, 0, sizeof(SWI4_event_triggered_flag));
	irq_connect_dynamic(SWI4_EGU4_IRQn, 0, SWI4_trigger_function, NULL, BIT(0));

	// configure egu4
	nrf_egu_int_enable(NRF_EGU4, NRF_EGU_INT_TRIGGERED0);
	irq_enable(SWI4_EGU4_IRQn);
	
	// trigger egu3
	nrf_egu_task_trigger(NRF_EGU3, NRF_EGU_TASK_TRIGGER0);
	
	k_busy_wait(1000);
	irq_disable(SWI4_EGU4_IRQn);
	nrf_egu_int_disable(NRF_EGU4, NRF_EGU_INT_TRIGGERED0);

	// EGU3 should forward the trigger to EGU4 via PPI, and SWI4 IRQ is expected
	// IRQ for EGU3 is not enabled

	zassert_true(SWI4_event_triggered_flag.triggered[0], "Event 0 has not been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[1], "Event 1 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[2], "Event 2 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[3], "Event 3 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[4], "Event 4 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[5], "Event 5 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[6], "Event 6 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[7], "Event 7 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[8], "Event 8 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[9], "Event 9 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[10], "Event 10 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[11], "Event 11 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[12], "Event 12 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[13], "Event 13 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[14], "Event 14 has been triggered");
	zassert_false(SWI4_event_triggered_flag.triggered[15], "Event 15 has been triggered");

	for (uint8_t i = 0; i < nrf_egu_channel_count(NRF_EGU4); i++) {
		const nrf_egu_event_t check_event = nrf_egu_triggered_event_get(i);
		zassert_false(nrf_egu_event_check(NRF_EGU4,
						  check_event), "event %d has been triggered, but it shouldn't", i);
	}

}

static void test_clean_egu()
{
	memset(NRF_EGU5, 0, sizeof(*NRF_EGU5));
}

void test_main(void)
{

	ztest_test_suite(egu_tests,
			ztest_unit_test_setup_teardown(test_egu_channels_count,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_egu_channels_count,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_task_address_get,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_event_address_get,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_channel_int_get,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_task_trigger_not_int,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_task_trigger,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_task_configure_not_trigger,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_trigger_from_another_irq,
						test_clean_egu, test_clean_egu),
			ztest_unit_test_setup_teardown(test_nrf_egu_trigger_by_PPI,
						test_clean_egu, test_clean_egu)
						
			);

	ztest_run_test_suite(egu_tests);
}
