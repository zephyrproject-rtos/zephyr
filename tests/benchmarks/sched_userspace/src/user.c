/*
 * Copyright (c) 2022 Huawei Technologies SASU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#include "user.h"

void context_switch_yield(void *p1, void *p2, void *p3)
{
	uint32_t nb_threads = (uint32_t)(uintptr_t) p1;
	uint32_t rounds = NB_YIELDS / nb_threads;

	while (rounds--) {
		k_yield();
	}
}
