/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/exception.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef __cplusplus
extern "C" {
#endif

void z_arm_undef_instruction(void)
{
	EXCEPTION_DUMP("exception undefined instruction not handled");
	while (1) {
	}
}

void z_arm_prefetch_abort(void)
{
	EXCEPTION_DUMP("exception perfetch abort not handled");
	while (1) {
	}
}

void z_arm_data_abort(void)
{
	EXCEPTION_DUMP("exception data abort not handled");
	while (1) {
	}
}

#ifdef __cplusplus
}
#endif
