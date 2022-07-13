/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
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

#define DONT_CARE        (0)

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
	unsigned int key;

	if ((ef_id == NULL) || (flags & osFlagsError)) {
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
	unsigned int key;
	uint32_t sig;

	if ((ef_id == NULL) || (flags & osFlagsError)) {
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
	k_timeout_t poll_timeout;
	uint64_t time_stamp_start, ticks_elapsed;
	bool flags_are_set;

	/* Can be called from ISRs only if timeout is set to 0 */
	if (timeout > 0 && k_is_in_isr()) {
		return osFlagsErrorUnknown;
	}

	if ((ef_id == NULL) || (flags & osFlagsError)) {
		return osFlagsErrorParameter;
	}

	time_stamp_start = (uint64_t)k_uptime_ticks();

	for (;;) {

		flags_are_set = false;

		key = irq_lock();

		if (options & osFlagsWaitAll) {
			/* Check if all events we are waiting on have
			 * been signalled
			 */
			if ((events->signal_results & flags) == flags) {
				flags_are_set = true;
			}
		} else {
			/* Check if any of events we are waiting on have
			 * been signalled
			 */
			if (events->signal_results & flags) {
				flags_are_set = true;
			}
		}

		if (flags_are_set) {
			sig = events->signal_results;

			if (!(options & osFlagsNoClear)) {
				/* Clear signal flags as the thread is ready now */
				events->signal_results &= ~(flags);
			}

			irq_unlock(key);

			break;
		}

		/* Reset the states to facilitate the next trigger */
		events->poll_event.signal->signaled = 0U;
		events->poll_event.state = K_POLL_STATE_NOT_READY;

		irq_unlock(key);

		if (timeout == 0) {
			return osFlagsErrorTimeout;
		} else if (timeout == osWaitForever) {
			poll_timeout = Z_FOREVER;
		} else {
			/* If we need to wait on more signals, we need to
			 * adjust the timeout value accordingly based on
			 * the time that has already elapsed.
			 */
			ticks_elapsed =
				(uint64_t)k_uptime_ticks() - time_stamp_start;

			if (ticks_elapsed < (uint64_t)timeout) {
				poll_timeout = Z_TIMEOUT_TICKS((k_ticks_t)(
					timeout - (uint32_t)ticks_elapsed));
			} else {
				return osFlagsErrorTimeout;
			}
		}

		retval = k_poll(&events->poll_event, 1, poll_timeout);

		if (retval == -EAGAIN) {
			/* k_poll signaled timeout. */
			return osFlagsErrorTimeout;
		} else if (retval != 0) {
			return osFlagsErrorUnknown;
		}

		/* retval is zero.
		 * k_poll found some raised signal then loop again and check flags.
		 */
		__ASSERT(events->poll_event.state == K_POLL_STATE_SIGNALED,
			 "event state not signalled!");
		__ASSERT(events->poll_event.signal->signaled == 1U,
			 "event signaled is not 1");
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
