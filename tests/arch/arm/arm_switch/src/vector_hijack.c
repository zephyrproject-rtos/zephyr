/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/ztest.h>
#include "vector_hijack.h"

void *vector_hijack(void (*my_svc_handler)(void))
{
	static uint32_t __aligned(1024) vectors[256];
	uint32_t *vtor = (uint32_t *)SCB->VTOR;

	printk("VTOR @%p\n", vtor);

	/* Vector count: _vector_start/end set by the linker. */
	size_t nv = ((uintptr_t)_vector_end - (uintptr_t)_vector_start) / sizeof(uint32_t);

	if (nv > ARRAY_SIZE(vectors)) {
		TC_ERROR("vector table exceeds test buffer\n");
		ztest_test_fail();
		return vtor;
	}
	for (size_t i = 0; i < nv; i++) {
		vectors[i] = vtor[i];
	}
	vectors[11] = (uint32_t)my_svc_handler;
	SCB->VTOR = (uint32_t)&vectors[0];
	__DSB();
	__ISB();

	return vtor;
}
