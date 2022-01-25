/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_oom.h"
#include <zephyr.h>

void *event_manager_alloc(size_t size)
{
	void *event = k_malloc(size);

	if (unlikely(!event)) {
		oom_error_handler();
	}

	return event;
}

void event_manager_free(void *addr)
{
	k_free(addr);
}
