/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <cmsis_os.h>
#include <string.h>

#define ACTIVE 1
#define NOT_ACTIVE 0

static void zephyr_timer_wrapper(struct k_timer *timer);

struct timer_obj {
	struct k_timer ztimer;
	os_timer_type type;
	uint32_t status;
	void (*callback_function)(void const *argument);
	void *arg;
};

K_MEM_SLAB_DEFINE(cmsis_timer_slab, sizeof(struct timer_obj),
		  CONFIG_CMSIS_TIMER_MAX_COUNT, __alignof__(struct timer_obj));

static void zephyr_timer_wrapper(struct k_timer *timer)
{
	struct timer_obj *cm_timer;

	cm_timer = CONTAINER_OF(timer, struct timer_obj, ztimer);
	(cm_timer->callback_function)(cm_timer->arg);
}

/**
 * @brief Create a Timer
 */
osTimerId osTimerCreate(const osTimerDef_t *timer_def, os_timer_type type,
			void *argument)
{
	struct timer_obj *timer;

	if (timer_def == NULL) {
		return NULL;
	}

	if (type != osTimerOnce && type != osTimerPeriodic) {
		return NULL;
	}

	if (k_mem_slab_alloc(&cmsis_timer_slab, (void **)&timer, K_MSEC(100)) == 0) {
		(void)memset(timer, 0, sizeof(struct timer_obj));
	} else {
		return NULL;
	}

	timer->callback_function = timer_def->ptimer;
	timer->arg = argument;
	timer->type = type;
	timer->status = NOT_ACTIVE;

	k_timer_init(&timer->ztimer, zephyr_timer_wrapper, NULL);

	return (osTimerId)timer;
}

/**
 * @brief Start or restart a Timer
 */
osStatus osTimerStart(osTimerId timer_id, uint32_t millisec)
{
	struct timer_obj *timer = (struct timer_obj *) timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->type == osTimerOnce) {
		k_timer_start(&timer->ztimer, K_MSEC(millisec), K_NO_WAIT);
	} else if (timer->type == osTimerPeriodic) {
		k_timer_start(&timer->ztimer, K_MSEC(millisec),
			      K_MSEC(millisec));
	}

	timer->status = ACTIVE;
	return osOK;
}

/**
 * @brief Stop the Timer
 */
osStatus osTimerStop(osTimerId timer_id)
{
	struct timer_obj *timer = (struct timer_obj *) timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->status == NOT_ACTIVE) {
		return osErrorResource;
	}

	k_timer_stop(&timer->ztimer);
	timer->status = NOT_ACTIVE;
	return osOK;
}

/**
 * @brief Delete the timer that was created by osTimerCreate
 */
osStatus osTimerDelete(osTimerId timer_id)
{
	struct timer_obj *timer = (struct timer_obj *) timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->status == ACTIVE) {
		k_timer_stop(&timer->ztimer);
		timer->status = NOT_ACTIVE;
	}

	k_mem_slab_free(&cmsis_timer_slab, (void *) &timer);
	return osOK;
}
