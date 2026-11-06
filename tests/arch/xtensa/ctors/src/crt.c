/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifdef CRT_BEGIN
const uint32_t crt_begin __attribute__((section(".ctors"))) = UINT32_MAX;

extern void crt_init(void);

/* Modern CRT initializer callbacks must remain callable. */
void (*const crt_callback)(void) __attribute__((section(".init_array"))) = crt_init;
#else
const uint32_t crt_end __attribute__((section(".ctors"))) = 0U;
#endif
