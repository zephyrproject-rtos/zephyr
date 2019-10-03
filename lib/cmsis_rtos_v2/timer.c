/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include "wrapper.h"

#define ACTIVE 1
#define NOT_ACTIVE 0

static void zephyr_timer_wrapper(struct k_timer *timer);

K_MEM_SLAB_DEFINE(cv2_timer_slab, sizeof(struct cv2_timer),
		  CONFIG_CMSIS_V2_TIMER_MAX_COUNT, 4);

static const osTimerAttr_t init_timer_attrs = {
	.name = "ZephyrTimer",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
};

static void zephyr_timer_wrapper(struct k_timer *timer)
{
	struct cv2_timer *cm_timer;

	cm_timer = CONTAINER_OF(timer, struct cv2_timer, z_timer);
	(cm_timer->callback_function)(cm_timer->arg);
}

/**
 * @brief Create a Timer
 */
osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type,
		       void *argument, const osTimerAttr_t *attr)
{
	struct cv2_timer *timer;

	if (type != osTimerOnce && type != osTimerPeriodic) {
		return NULL;
	}

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_timer_attrs;
	}

	if (k_mem_slab_alloc(&cv2_timer_slab, (void **)&timer, K_MSEC(100)) == 0) {
		(void)memset(timer, 0, sizeof(struct cv2_timer));
	} else {
		return NULL;
	}

	timer->callback_function = func;
	timer->arg = argument;
	timer->type = type;
	timer->status = NOT_ACTIVE;

	k_timer_init(&timer->z_timer, zephyr_timer_wrapper, NULL);

	if (attr->name == NULL) {
		strncpy(timer->name, init_timer_attrs.name,
			sizeof(timer->name) - 1);
	} else {
		strncpy(timer->name, attr->name, sizeof(timer->name) - 1);
	}

	return (osTimerId_t)timer;
}

/**
 * @brief Start or restart a Timer
 */
osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
{
	struct cv2_timer *timer = (struct cv2_timer *)timer_id;
	u32_t millisec = __ticks_to_ms(ticks);

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->type == osTimerOnce) {
		k_timer_start(&timer->z_timer, millisec, K_NO_WAIT);
	} else if (timer->type == osTimerPeriodic) {
		k_timer_start(&timer->z_timer, K_NO_WAIT, millisec);
	}

	timer->status = ACTIVE;
	return osOK;
}

/**
 * @brief Stop the Timer
 */
osStatus_t osTimerStop(osTimerId_t timer_id)
{
	struct cv2_timer *timer = (struct cv2_timer *)timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->status == NOT_ACTIVE) {
		return osErrorResource;
	}

	k_timer_stop(&timer->z_timer);
	timer->status = NOT_ACTIVE;
	return osOK;
}

/**
 * @brief Delete the timer that was created by osTimerCreate
 */
osStatus_t osTimerDelete(osTimerId_t timer_id)
{
	struct cv2_timer *timer = (struct cv2_timer *) timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->status == ACTIVE) {
		k_timer_stop(&timer->z_timer);
		timer->status = NOT_ACTIVE;
	}

	k_mem_slab_free(&cv2_timer_slab, (void *) &timer);
	return osOK;
}

/**
 * @brief Get name of a timer.
 */
const char *osTimerGetName(osTimerId_t timer_id)
{
	struct cv2_timer *timer = (struct cv2_timer *)timer_id;

	if (k_is_in_isr() || (timer == NULL)) {
		return NULL;
	}

	return timer->name;
}

/**
 * @brief Check if a timer is running.
 */
uint32_t osTimerIsRunning(osTimerId_t timer_id)
{
	struct cv2_timer *timer = (struct cv2_timer *)timer_id;

	if (k_is_in_isr() || (timer == NULL)) {
		return 0;
	}

	return !(!(k_timer_remaining_get(&timer->z_timer)));
}
