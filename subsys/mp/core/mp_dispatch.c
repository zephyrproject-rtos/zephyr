/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/mp/core/mp_caps.h"
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>

#include <zephyr/mp/core/mp_dispatch.h>

void mp_dispatch_init(struct mp_dispatch *dispatch, uint8_t type, struct mp_caps *caps)
{
	if (dispatch == NULL) {
		return;
	}

	memset(dispatch, 0, sizeof(*dispatch));
	dispatch->type = type;

	if (caps != NULL) {
		dispatch->caps = mp_caps_ref(caps);
	}
}

void mp_dispatch_clear(struct mp_dispatch *dispatch)
{
	if (dispatch == NULL) {
		return;
	}

	if (dispatch->caps != NULL) {
		mp_caps_unref(dispatch->caps);
	}

	memset(dispatch, 0, sizeof(*dispatch));
}

struct mp_caps *mp_dispatch_get_caps(struct mp_dispatch *dispatch)
{
	if (dispatch == NULL) {
		return NULL;
	}

	return mp_caps_ref(dispatch->caps);
}

int mp_dispatch_set_caps(struct mp_dispatch *dispatch, struct mp_caps *caps)
{
	if (dispatch == NULL) {
		return -EINVAL;
	}

	return mp_caps_replace(&dispatch->caps, caps);
}

int mp_dispatch_set_pool(struct mp_dispatch *dispatch, struct mp_buffer_pool *pool)
{
	if (dispatch == NULL) {
		return -EINVAL;
	}

	dispatch->pool = pool;

	return 0;
}

int mp_dispatch_set_pool_config(struct mp_dispatch *dispatch, struct mp_buffer_pool_config *config)
{
	if (dispatch == NULL || config == NULL) {
		return -EINVAL;
	}

	dispatch->pool_cfg = *config;

	return 0;
}

struct mp_buffer_pool *mp_dispatch_get_pool(struct mp_dispatch *dispatch)
{
	if (dispatch == NULL) {
		return NULL;
	}

	return dispatch->pool;
}

struct mp_buffer_pool_config *mp_dispatch_get_pool_config(struct mp_dispatch *dispatch)
{
	if (dispatch == NULL) {
		return NULL;
	}

	return &dispatch->pool_cfg;
}
