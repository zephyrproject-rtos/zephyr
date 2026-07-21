/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_timer);

typedef struct {
	struct k_timer z_timer;
	uint32_t timer_id;
	uint32_t interval_ms;
	uint8_t reload;
} k_timer_wrapper_t;

int rtos_timer_create(rtos_timer_t *pp_handle, const char *p_timer_name, uint32_t timer_id,
		      uint32_t interval_ms, uint8_t reload, void (*p_timer_callback)(void *))
{
	k_timer_wrapper_t *p_timer;

	ARG_UNUSED(p_timer_name);
	if (pp_handle == NULL || p_timer_callback == NULL) {
		return RTK_FAIL;
	}

#if (K_HEAP_MEM_POOL_SIZE > 0)
	p_timer = k_malloc(sizeof(k_timer_wrapper_t));
	if (p_timer == NULL) {
		return RTK_FAIL;
	}
#else
	LOG_ERR("k_malloc not support.");
	return RTK_FAIL;
#endif

	k_timer_init((struct k_timer *)p_timer, (k_timer_expiry_t)p_timer_callback, NULL);
	p_timer->timer_id = timer_id;
	p_timer->interval_ms = interval_ms;
	p_timer->reload = reload;

	*pp_handle = p_timer;
	return RTK_SUCCESS;
}

int rtos_timer_delete(rtos_timer_t p_handle, uint32_t wait_ms)
{
	struct k_timer *p_timer = p_handle;

	ARG_UNUSED(wait_ms);
	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	k_timer_stop(p_timer);
	k_free(p_timer);
	return RTK_SUCCESS;
}

int rtos_timer_create_static(rtos_timer_t *pp_handle, const char *p_timer_name, uint32_t timer_id,
			     uint32_t interval_ms, uint8_t reload, void (*p_timer_callback)(void *))
{
	return rtos_timer_create(pp_handle, p_timer_name, timer_id, interval_ms, reload,
				 p_timer_callback);
}

int rtos_timer_delete_static(rtos_timer_t p_handle, uint32_t wait_ms)
{
	return rtos_timer_delete(p_handle, wait_ms);
}

int rtos_timer_start(rtos_timer_t p_handle, uint32_t wait_ms)
{
	k_timer_wrapper_t *p_timer = p_handle;

	ARG_UNUSED(wait_ms);
	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (p_timer->reload) {
		/* Called Periodically Until Stopped */
		k_timer_start((struct k_timer *)p_timer, K_MSEC(p_timer->interval_ms),
			      K_MSEC(p_timer->interval_ms));
	} else {
		/* Called Once Only */
		k_timer_start((struct k_timer *)p_timer, K_MSEC(p_timer->interval_ms), K_NO_WAIT);
	}

	return RTK_SUCCESS;
}

int rtos_timer_stop(rtos_timer_t p_handle, uint32_t wait_ms)
{
	ARG_UNUSED(wait_ms);

	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	k_timer_stop(p_handle);
	return RTK_SUCCESS;
}

int rtos_timer_change_period(rtos_timer_t p_handle, uint32_t interval_ms, uint32_t wait_ms)
{
	k_timer_wrapper_t *p_timer = p_handle;

	ARG_UNUSED(wait_ms);
	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (p_timer->reload) {
		/* Called Periodically Until Stopped */
		k_timer_start((struct k_timer *)p_timer, K_MSEC(interval_ms), K_MSEC(interval_ms));
	} else {
		/* Called Once Only */
		k_timer_start((struct k_timer *)p_timer, K_MSEC(interval_ms), K_NO_WAIT);
	}

	p_timer->interval_ms = interval_ms;
	return RTK_SUCCESS;
}

uint32_t rtos_timer_is_timer_active(rtos_timer_t p_handle)
{
	struct k_timer *p_timer = p_handle;

	if (p_handle == NULL) {
		return 0;
	}

	return (k_timer_remaining_get(p_timer) != 0u) ? TRUE : FALSE;
}

uint32_t rtos_timer_get_id(rtos_timer_t p_handle)
{
	k_timer_wrapper_t *p_timer = p_handle;

	if (p_handle == NULL) {
		return 0;
	}

	return p_timer->timer_id;
}

_WEAK void init_timer_wrapper(void)
{
	LOG_ERR("Not Support");
}
