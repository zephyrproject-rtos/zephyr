/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/ztest.h>
#include "vector_hijack.h"

void *vector_hijack(void (*my_svc_handler)(void))
{
	static uint32_t __aligned(1024) vectors[256];
	uint32_t *vtor_p = (void *)0xe000ed08;
	uint32_t *vtor = (void *)*vtor_p;

	printk("VTOR @%p\n", vtor);

	/* Vector count: _vector_start/end set by the linker. */
	int nv = (&_vector_end[0] - &_vector_start[0]) / sizeof(uint32_t);

	for (int i = 0; i < nv; i++) {
		vectors[i] = vtor[i];
	}
	*vtor_p = (uint32_t)&vectors[0];
	vtor = (void *)*vtor_p;
	printk("VTOR now @%p\n", vtor);

	/* And hook the SVC call with our own function above, allowing
	 * us direct access to interrupt entry
	 */
	vtor[11] = (int)my_svc_handler;
	printk("vtor[11] == %p (my_svc == %p)\n", (void *)vtor[11], my_svc_handler);

	return NULL;
}