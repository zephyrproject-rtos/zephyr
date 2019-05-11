/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <cmsis_os.h>

#define NSEC_PER_MSEC		(NSEC_PER_USEC * USEC_PER_MSEC)
#define MAX_VALID_SIGNAL_VAL	((1 << osFeature_Signals) - 1)

void *k_thread_other_custom_data_get(struct k_thread *thread_id)
{
	return thread_id->custom_data;
}

/**
 * @brief Set the specified Signal Flags of an active thread.
 */
int32_t osSignalSet(osThreadId thread_id, int32_t signals)
{
	int sig, key;

	if ((thread_id == NULL) || (!signals) ||
		(signals & 0x80000000) || (signals > MAX_VALID_SIGNAL_VAL)) {
		return 0x80000000;
	}

	osThreadDef_t *thread_def =
		(osThreadDef_t *)k_thread_other_custom_data_get(
						(struct k_thread *)thread_id);

	key = irq_lock();
	sig = thread_def->signal_results;
	thread_def->signal_results |= signals;
	irq_unlock(key);

	k_poll_signal_raise(thread_def->poll_signal, signals);

	return sig;
}

/**
 * @brief Clear the specified Signal Flags of an active thread.
 */
int32_t osSignalClear(osThreadId thread_id, int32_t signals)
{
	int sig, key;

	if (k_is_in_isr() || (thread_id == NULL) || (!signals) ||
		(signals & 0x80000000) || (signals > MAX_VALID_SIGNAL_VAL)) {
		return 0x80000000;
	}

	osThreadDef_t *thread_def =
		(osThreadDef_t *)k_thread_other_custom_data_get(
						(struct k_thread *)thread_id);

	key = irq_lock();
	sig = thread_def->signal_results;
	thread_def->signal_results &= ~(signals);
	irq_unlock(key);

	return sig;
}

/**
 * @brief Wait for one or more Signal Flags to become signalled for the
 * current running thread.
 */
osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
	int retval, key;
	osEvent evt;
	u32_t time_delta_ms, timeout = millisec;
	u64_t time_stamp_start, hwclk_cycles_delta, time_delta_ns;

	if (k_is_in_isr()) {
		evt.status = osErrorISR;
		return evt;
	}

	/* Check if signals is within the permitted range */
	if ((signals & 0x80000000) || (signals > MAX_VALID_SIGNAL_VAL)) {
		evt.status = osErrorValue;
		return evt;
	}

	osThreadDef_t *thread_def = k_thread_custom_data_get();

	for (;;) {

		time_stamp_start = (u64_t)k_cycle_get_32();

		switch (millisec) {
		case 0:
			retval = k_poll(thread_def->poll_event, 1, K_NO_WAIT);
			break;
		case osWaitForever:
			retval = k_poll(thread_def->poll_event, 1, K_FOREVER);
			break;
		default:
			retval = k_poll(thread_def->poll_event, 1, timeout);
			break;
		}

		switch (retval) {
		case 0:
			evt.status = osEventSignal;
			break;
		case -EAGAIN:
			if (millisec == 0U) {
				evt.status = osOK;
			} else {
				evt.status = osEventTimeout;
			}
			return evt;
		default:
			evt.status = osErrorValue;
			return evt;
		}

		__ASSERT(thread_def->poll_event->state
				== K_POLL_STATE_SIGNALED,
			"event state not signalled!");
		__ASSERT(thread_def->poll_event->signal->signaled == 1,
			"event signaled is not 1");

		/* Reset the states to facilitate the next trigger */
		thread_def->poll_event->signal->signaled = 0;
		thread_def->poll_event->state = K_POLL_STATE_NOT_READY;

		/* Check if all events we are waiting on have been signalled */
		if ((thread_def->signal_results & signals) == signals) {
			break;
		}

		/* If we need to wait on more signals, we need to adjust the
		 * timeout value accordingly based on the time that has
		 * already elapsed.
		 */
		hwclk_cycles_delta = (u64_t)k_cycle_get_32() - time_stamp_start;
		time_delta_ns =
			(u32_t)SYS_CLOCK_HW_CYCLES_TO_NS(hwclk_cycles_delta);
		time_delta_ms =	(u32_t)time_delta_ns/NSEC_PER_MSEC;

		if (timeout > time_delta_ms) {
			timeout -= time_delta_ms;
		} else {
			timeout = 0U;
		}
	}

	evt.value.signals = thread_def->signal_results;

	/* Clear signal flags as the thread is ready now */
	key = irq_lock();
	thread_def->signal_results &= ~(signals);
	irq_unlock(key);

	return evt;
}
