/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#define TIMEOUT		(100)
#define SIGNAL1		(0x00000020)
#define SIGNAL2		(0x00000004)
#define SIGNAL		(SIGNAL1 | SIGNAL2)

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
	zassert_not_equal(signals, 0x80000000, "");

	/* wait for SIGNAL1. It should timeout here as the signal
	 * though triggered, gets cleared in the previous step.
	 */
	evt = osSignalWait(SIGNAL1, TIMEOUT);
	zassert_equal(evt.status, osEventTimeout, "");
}

void test_signal_events_signalled(void)
{
	osThreadId id1, id2;
	osEvent evt;

	id1 = osThreadCreate(osThread(Thread_1), osThreadGetId());
	zassert_true(id1 != NULL, "Thread creation failed");

	id2 = osThreadCreate(osThread(Thread_2), osThreadGetId());
	zassert_true(id2 != NULL, "Thread creation failed");

	/* wait for a signal */
	evt = osSignalWait(SIGNAL, TIMEOUT);
	zassert_equal(evt.status, osEventSignal, "");
	zassert_equal((evt.value.signals & SIGNAL), SIGNAL, "");
}
