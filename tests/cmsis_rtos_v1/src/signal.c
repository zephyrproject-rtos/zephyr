/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#include <irq_offload.h>
#include <kernel_structs.h>

#define TIMEOUT		(100)
#define SIGNAL1		(0x00000020)
#define SIGNAL2		(0x00000004)
#define SIGNAL		(SIGNAL1 | SIGNAL2)

#define SIGNAL_FLAG 0x00000001
#define ISR_SIGNAL 0x50

#define SIGNAL_ALL_FLAGS ((1 << osFeature_Signals) - 1)
#define SIGNAL_OUTOFLIMIT_FLAG ((1 << (osFeature_Signals + 1))-1)

void Thread_1(void const *arg)
{
	int signals = osSignalSet((osThreadId)arg, SIGNAL1);

	zassert_not_equal(signals, 0x80000000, "");
}

void Thread_2(void const *arg)
{
	int signals = osSignalSet((osThreadId)arg, SIGNAL2);

	zassert_not_equal(signals, 0x80000000, "");
}

void test_multiple_signal_flags(void const *thread_id)
{
	int max_signal_cnt = osFeature_Signals;
	int signals, sig_cnt;

	for (sig_cnt = 0; sig_cnt < max_signal_cnt; sig_cnt++) {
		/* create unique signal flags to set one flag at a time */
		s32_t signal_each_flag = SIGNAL_FLAG << sig_cnt;
		/* check if each signal flag is set properly */
		signals = osSignalSet((osThreadId)thread_id, signal_each_flag);
		zassert_not_equal(signals, 0x80000000,
				  "Setting each signal flag failed");
	}
	/* clear all the bits to check next scenario */
	signals = osSignalClear((osThreadId)thread_id, SIGNAL_ALL_FLAGS);
	zassert_not_equal(signals, 0x80000000, "");

	/* signal all the available flags at one shot */
	signals = osSignalSet((osThreadId)thread_id, SIGNAL_ALL_FLAGS);
	zassert_not_equal(signals, 0x80000000,
					  "Signal flags maximum limit failed");
	/* clear all the bits to check next scenario */
	signals = osSignalClear((osThreadId)thread_id, SIGNAL_ALL_FLAGS);
	zassert_not_equal(signals, 0x80000000, "");

	/* signal invalid flag to validate permissible flag limit */
	signals = osSignalSet((osThreadId)thread_id, SIGNAL_OUTOFLIMIT_FLAG);
	zassert_equal(signals, 0x80000000, "Signal flags set unexpectedly");
}

osThreadDef(Thread_1, osPriorityHigh, 3, 0);
osThreadDef(Thread_2, osPriorityHigh, 1, 0);

void test_signal_events_no_wait(void)
{
	osThreadId id1;
	osEvent evt;

	id1 = osThreadCreate(osThread(Thread_1), osThreadGetId());
	zassert_true(id1 != NULL, "Thread creation failed");

	/* Let id1 run to trigger SIGNAL1 */
	osDelay(10);

	/* wait for SIGNAL1. It should return immediately as it is
	 * already triggered.
	 */
	evt = osSignalWait(SIGNAL1, 0);
	zassert_equal(evt.status, osEventSignal, "");
	zassert_equal((evt.value.signals & SIGNAL1), SIGNAL1, "");
	osThreadTerminate(id1);
}

void test_signal_events_timeout(void)
{
	osThreadId id1;
	int signals;
	osEvent evt;

	id1 = osThreadCreate(osThread(Thread_1), osThreadGetId());
	zassert_true(id1 != NULL, "Thread creation failed");

	/* Let id1 run to trigger SIGNAL1 */
	osDelay(10);

	signals = osSignalClear(osThreadGetId(), SIGNAL1);
	zassert_not_equal(signals, 0x80000000, "signal clear failed");

	/* wait for SIGNAL1. It should timeout here as the signal
	 * though triggered, gets cleared in the previous step.
	 */
	evt = osSignalWait(SIGNAL1, TIMEOUT);
	zassert_equal(evt.status, osEventTimeout, "signal timeout failed");
	osThreadTerminate(id1);
}

void test_signal_events_signalled(void)
{
	osThreadId id1, id2;
	osEvent evt;
	int signals;

	id1 = osThreadCreate(osThread(Thread_1), osThreadGetId());
	zassert_true(id1 != NULL, "Thread creation failed");

	id2 = osThreadCreate(osThread(Thread_2), osThreadGetId());
	zassert_true(id2 != NULL, "Thread creation failed");

	/* wait for multiple signals */
	evt = osSignalWait(SIGNAL, TIMEOUT);
	zassert_equal(evt.status, osEventSignal,
		      "wait signal returned unexpected error");
	zassert_equal((evt.value.signals & SIGNAL), SIGNAL,
		      "wait signal failed unexpectedly");

	signals = osSignalClear(osThreadGetId(), SIGNAL);
	zassert_not_equal(signals, 0x80000000, "clear signal failed");

	/* Set any single signal */
	signals = osSignalSet(osThreadGetId(), SIGNAL1);
	zassert_not_equal(signals, 0x80000000, "set any signal failed");

	/* wait for any single signal flag */
	evt = osSignalWait(0, TIMEOUT);
	zassert_equal(evt.status, osEventSignal, "wait for single flag failed");
	zassert_equal(evt.value.signals, SIGNAL1,
				  "wait single flag returned invalid value");

	/* validate by passing invalid parameters */
	zassert_equal(osSignalSet(NULL, 0), 0x80000000,
				  "NULL signal set unexpectedly");
	zassert_equal(osSignalClear(NULL, 0), 0x80000000,
				  "NULL signal cleared unexpectedly");
	/* cannot wait for Flag mask with MSB set */
	zassert_equal(osSignalWait((int32_t)0x80010000, 0).status, osErrorValue,
				  "signal wait passed unexpectedly");
	zassert_equal(osSignalSet(osThreadGetId(), (int32_t)0x80010000),
				  0x80000000, "signal set unexpectedly");
	zassert_equal(osSignalClear(osThreadGetId(), (int32_t)0x80010000),
				    0x80000000, "signal cleared unexpectedly");

	test_multiple_signal_flags(osThreadGetId());
}

/* IRQ offload function handler to set signal flag */
static void offload_function(void *param)
{
	u32_t tid = (u32_t)param;
	int signals;

	/* Make sure we're in IRQ context */
	zassert_true(_is_in_isr(), "Not in IRQ context!");

	signals = osSignalSet((osThreadId)tid, ISR_SIGNAL);
	zassert_not_equal(signals, 0x80000000, "signal set failed in ISR");
}

void test_signal_from_isr(void const *thread_id)
{
	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (void *)thread_id);
}

osThreadDef(test_signal_from_isr, osPriorityHigh, 1, 0);

void test_signal_events_isr(void)
{
	osThreadId id;
	osEvent evt;

	id = osThreadCreate(osThread(test_signal_from_isr), osThreadGetId());
	zassert_true(id != NULL, "Thread creation failed");
	evt = osSignalWait(ISR_SIGNAL, TIMEOUT);
	zassert_equal(evt.status, osEventSignal,
		      "signal wait failed unexpectedly");
	zassert_equal((evt.value.signals & ISR_SIGNAL),
		       ISR_SIGNAL, "unexpected signal wait value");
}
