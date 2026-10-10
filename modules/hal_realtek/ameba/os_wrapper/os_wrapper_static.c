/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "os_wrapper_static_functions.h"

/* This wrapper has no static object pools; report empty statistics. */
void rtos_static_get_component_status(struct component_status *comp_status)
{
	memset(comp_status, 0, sizeof(*comp_status));
}
