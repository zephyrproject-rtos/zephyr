/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_types.h>
#include <string.h>

#define ACTIVE     1
#define NOT_ACTIVE 0

static void zephyr_timer_wrapper(struct k_timer *timer);

K_MEM_SLAB_DEFINE(cmsis_rtos_timer_cb_slab, sizeof(struct cmsis_rtos_timer_cb),
		  CONFIG_CMSIS_V2_TIMER_MAX_COUNT, 4);

static const osTimerAttr_t init_timer_attrs = {
	.name = "ZephyrTimer",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
};

static void zephyr_timer_wrapper(struct k_timer *timer)
{
	struct cmsis_rtos_timer_cb *cm_timer;

	cm_timer = CONTAINER_OF(timer, struct cmsis_rtos_timer_cb, z_timer);
	(cm_timer->callback_function)(cm_timer->arg);
}

/**
 * @brief Create a Timer
 */
osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument,
		       const osTimerAttr_t *attr)
{
	struct cmsis_rtos_timer_cb *timer;

	if (type != osTimerOnce && type != osTimerPeriodic) {
		return NULL;
	}

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_timer_attrs;
	}

	if (attr->cb_mem != NULL) {
		__ASSERT(attr->cb_size == sizeof(struct cmsis_rtos_timer_cb), "Invalid cb_size\n");
		timer = (struct cmsis_rtos_timer_cb *)attr->cb_mem;
	} else if (k_mem_slab_alloc(&cmsis_rtos_timer_cb_slab, (void **)&timer, K_MSEC(100)) != 0) {
		return NULL;
	}
	(void)memset(timer, 0, sizeof(struct cmsis_rtos_timer_cb));
	timer->is_cb_dynamic_allocation = attr->cb_mem == NULL;

	timer->callback_function = func;
	timer->arg = argument;
	timer->type = type;
	timer->status = NOT_ACTIVE;

	k_timer_init(&timer->z_timer, zephyr_timer_wrapper, NULL);

	if (attr->name == NULL) {
		strncpy(timer->name, init_timer_attrs.name, sizeof(timer->name) - 1);
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
	struct cmsis_rtos_timer_cb *timer = (struct cmsis_rtos_timer_cb *)timer_id;

	if (timer == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timer->type == osTimerOnce) {
		k_timer_start(&timer->z_timer, K_TICKS(ticks), K_NO_WAIT);
	} else if (timer->type == osTimerPeriodic) {
		k_timer_start(&timer->z_timer, K_TICKS(ticks), K_TICKS(ticks));
	}

	timer->status = ACTIVE;
	return osOK;
}

/**
 * @brief Stop the Timer
 */
osStatus_t osTimerStop(osTimerId_t timer_id)
{
	struct cmsis_rtos_timer_cb *timer = (struct cmsis_rtos_timer_cb *)timer_id;

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
	struct cmsis_rtos_timer_cb *timer = (struct cmsis_rtos_timer_cb *)timer_id;

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

	if (timer->is_cb_dynamic_allocation) {
		k_mem_slab_free(&cmsis_rtos_timer_cb_slab, (void *)timer);
	}
	return osOK;
}

/**
 * @brief Get name of a timer.
 */
const char *osTimerGetName(osTimerId_t timer_id)
{
	struct cmsis_rtos_timer_cb *timer = (struct cmsis_rtos_timer_cb *)timer_id;

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
	struct cmsis_rtos_timer_cb *timer = (struct cmsis_rtos_timer_cb *)timer_id;

	if (k_is_in_isr() || (timer == NULL)) {
		return 0;
	}

	return !(!(k_timer_remaining_get(&timer->z_timer)));
}
