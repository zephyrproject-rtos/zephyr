/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_types.h>
#include <string.h>

K_MEM_SLAB_DEFINE(cmsis_rtos_event_cb_slab, sizeof(struct cmsis_rtos_event_cb),
		  CONFIG_CMSIS_V2_EVT_FLAGS_MAX_COUNT, 4);

static const osEventFlagsAttr_t init_event_flags_attrs = {
	.name = "ZephyrEvent",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
};

#define DONT_CARE (0)

/**
 * @brief Create and Initialize an Event Flags object.
 */
osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
{
	struct cmsis_rtos_event_cb *events;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_event_flags_attrs;
	}

	if (attr->cb_mem != NULL) {
		__ASSERT(attr->cb_size == sizeof(struct cmsis_rtos_event_cb), "Invalid cb_size\n");
		events = (struct cmsis_rtos_event_cb *)attr->cb_mem;
	} else if (k_mem_slab_alloc(&cmsis_rtos_event_cb_slab, (void **)&events, K_MSEC(100)) !=
		   0) {
		return NULL;
	}
	memset(events, 0, sizeof(struct cmsis_rtos_event_cb));

	k_event_init(&events->z_event);
	events->is_cb_dynamic_allocation = (attr->cb_mem == NULL);
	events->name = (attr->name == NULL) ? init_event_flags_attrs.name : attr->name;

	return (osEventFlagsId_t)events;
}

/**
 * @brief Set the specified Event Flags.
 */
uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;
	if ((ef_id == NULL) || (flags & osFlagsError)) {
		return osFlagsErrorParameter;
	}

	k_event_post(&events->z_event, flags);

	return k_event_test(&events->z_event, 0xFFFFFFFF);
}

/**
 * @brief Clear the specified Event Flags.
 */
uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;
	uint32_t rv;

	if ((ef_id == NULL) || (flags & osFlagsError)) {
		return osFlagsErrorParameter;
	}

	rv = k_event_test(&events->z_event, 0xFFFFFFFF);
	k_event_clear(&events->z_event, flags);

	return rv;
}

/**
 * @brief Wait for one or more Event Flags to become signaled.
 */
uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options,
			  uint32_t timeout)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;
	uint32_t rv;
	k_timeout_t event_timeout;

	/*
	 * Return unknown error if called from ISR with a non-zero timeout
	 * or if flags is zero.
	 */
	if (((timeout > 0U) && k_is_in_isr()) || (flags == 0U)) {
		return osFlagsErrorUnknown;
	}

	if ((ef_id == NULL) || (flags & osFlagsError)) {
		return osFlagsErrorParameter;
	}

	if (timeout == osWaitForever) {
		event_timeout = K_FOREVER;
	} else if (timeout == 0U) {
		event_timeout = K_NO_WAIT;
	} else {
		event_timeout = K_TICKS(timeout);
	}

	if ((options & osFlagsWaitAll) != 0) {
		rv = k_event_wait_all(&events->z_event, flags, false, event_timeout);
	} else {
		rv = k_event_wait(&events->z_event, flags, false, event_timeout);
	}

	if ((options & osFlagsNoClear) == 0) {
		k_event_clear(&events->z_event, flags);
	}

	if (rv != 0U) {
		return rv;
	}

	return (timeout == 0U) ? osFlagsErrorResource : osFlagsErrorTimeout;
}

/**
 * @brief Get name of an Event Flags object.
 * This function may be called from Interrupt Service Routines.
 */
const char *osEventFlagsGetName(osEventFlagsId_t ef_id)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;

	if (events == NULL) {
		return NULL;
	}
	return events->name;
}

/**
 * @brief Get the current Event Flags.
 */
uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;

	if (ef_id == NULL) {
		return 0;
	}

	return k_event_test(&events->z_event, 0xFFFFFFFF);
}

/**
 * @brief Delete an Event Flags object.
 */
osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)
{
	struct cmsis_rtos_event_cb *events = (struct cmsis_rtos_event_cb *)ef_id;

	if (ef_id == NULL) {
		return osErrorResource;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorParameter" (the value of the parameter
	 * ef_id is incorrect) is not supported in Zephyr.
	 */
	if (events->is_cb_dynamic_allocation) {
		k_mem_slab_free(&cmsis_rtos_event_cb_slab, (void *)events);
	}
	return osOK;
}
