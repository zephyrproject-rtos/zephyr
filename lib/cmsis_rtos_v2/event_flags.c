/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include "wrapper.h"

K_MEM_SLAB_DEFINE(cv2_event_flags_slab, sizeof(struct cv2_event_flags),
		  CONFIG_CMSIS_V2_EVT_FLAGS_MAX_COUNT, 4);

static const osEventFlagsAttr_t init_event_flags_attrs = {
	.name = "ZephyrEvent",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
};

#define DONT_CARE               (0)
#define NSEC_PER_MSEC           (NSEC_PER_USEC * USEC_PER_MSEC)

/**
 * @brief Create and Initialize an Event Flags object.
 */
osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
{
	struct cv2_event_flags *events;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_event_flags_attrs;
	}

	if (k_mem_slab_alloc(&cv2_event_flags_slab, (void **)&events, K_MSEC(100))
	    == 0) {
		memset(events, 0, sizeof(struct cv2_event_flags));
	} else {
		return NULL;
	}

	k_poll_signal_init(&events->poll_signal);
	k_poll_event_init(&events->poll_event, K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &events->poll_signal);
	events->signal_results = 0U;

	if (attr->name == NULL) {
		strncpy(events->name, init_event_flags_attrs.name,
			sizeof(events->name) - 1);
	} else {
		strncpy(events->name, attr->name, sizeof(events->name) - 1);
	}

	return (osEventFlagsId_t)events;
}

/**
 * @brief Set the specified Event Flags.
 */
uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;
	int key;

	if ((ef_id == NULL) || (flags & 0x80000000)) {
		return osFlagsErrorParameter;
	}

	key = irq_lock();
	events->signal_results |= flags;
	irq_unlock(key);

	k_poll_signal_raise(&events->poll_signal, DONT_CARE);

	return events->signal_results;
}

/**
 * @brief Clear the specified Event Flags.
 */
uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;
	int key;
	uint32_t sig;

	if ((ef_id == NULL) || (flags & 0x80000000)) {
		return osFlagsErrorParameter;
	}

	key = irq_lock();
	sig = events->signal_results;
	events->signal_results &= ~(flags);
	irq_unlock(key);

	return sig;
}

/**
 * @brief Wait for one or more Event Flags to become signaled.
 */
uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags,
			  uint32_t options, uint32_t timeout)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;
	int retval, key;
	uint32_t sig;
	uint32_t time_delta_ms, timeout_ms = k_ticks_to_ms_floor64(timeout);
	uint64_t time_stamp_start, hwclk_cycles_delta, time_delta_ns;

	/* Can be called from ISRs only if timeout is set to 0 */
	if (timeout > 0 && k_is_in_isr()) {
		return osFlagsErrorUnknown;
	}

	if ((ef_id == NULL) || (flags & 0x80000000)) {
		return osFlagsErrorParameter;
	}

	for (;;) {

		time_stamp_start = (uint64_t)k_cycle_get_32();

		switch (timeout) {
		case 0:
			retval = k_poll(&events->poll_event, 1, K_NO_WAIT);
			break;
		case osWaitForever:
			retval = k_poll(&events->poll_event, 1, K_FOREVER);
			break;
		default:
			retval = k_poll(&events->poll_event, 1,
					K_MSEC(timeout_ms));
			break;
		}

		switch (retval) {
		case 0:
			break;
		case -EAGAIN:
			return osFlagsErrorTimeout;
		default:
			return osFlagsErrorUnknown;
		}

		__ASSERT(events->poll_event.state == K_POLL_STATE_SIGNALED,
			 "event state not signalled!");
		__ASSERT(events->poll_event.signal->signaled == 1U,
			 "event signaled is not 1");

		/* Reset the states to facilitate the next trigger */
		events->poll_event.signal->signaled = 0U;
		events->poll_event.state = K_POLL_STATE_NOT_READY;

		if (options & osFlagsWaitAll) {

			/* Check if all events we are waiting on have
			 * been signalled
			 */
			if ((events->signal_results & flags) == flags) {
				break;
			}

			/* If we need to wait on more signals, we need to
			 * adjust the timeout value accordingly based on
			 * the time that has already elapsed.
			 */
			hwclk_cycles_delta =
				(uint64_t)k_cycle_get_32() - time_stamp_start;

			time_delta_ns =
				(uint32_t)k_cyc_to_ns_floor64(hwclk_cycles_delta);

			time_delta_ms = (uint32_t)time_delta_ns / NSEC_PER_MSEC;

			if (timeout_ms > time_delta_ms) {
				timeout_ms -= time_delta_ms;
			} else {
				timeout_ms = 0U;
			}
		} else {
			break;
		}
	}

	sig = events->signal_results;
	if (!(options & osFlagsNoClear)) {

		/* Clear signal flags as the thread is ready now */
		key = irq_lock();
		events->signal_results &= ~(flags);
		irq_unlock(key);
	}

	return sig;
}

/**
 * @brief Get name of an Event Flags object.
 */
const char *osEventFlagsGetName(osEventFlagsId_t ef_id)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;

	if (!k_is_in_isr() && (ef_id != NULL)) {
		return events->name;
	} else {
		return NULL;
	}
}

/**
 * @brief Get the current Event Flags.
 */
uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;

	if (ef_id == NULL) {
		return 0;
	}

	return events->signal_results;
}

/**
 * @brief Delete an Event Flags object.
 */
osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)
{
	struct cv2_event_flags *events = (struct cv2_event_flags *)ef_id;

	if (ef_id == NULL) {
		return osErrorResource;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorParameter" (the value of the parameter
	 * ef_id is incorrect) is not supported in Zephyr.
	 */

	k_mem_slab_free(&cv2_event_flags_slab, (void *)&events);

	return osOK;
}
