/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/qemu_cortex_m3/sys.c
 * @brief	machine specific system primitives implementation.
 */

#include <metal/io.h>
#include <metal/sys.h>
#include <stdint.h>

/**
 * @brief poll function until some event happens
 */
void metal_weak metal_generic_default_poll(void)
{
	__asm__ __volatile__("wfi");
}
