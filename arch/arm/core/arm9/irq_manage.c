/*
 * Copyright (C) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/exception.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	EXCEPTION_DUMP("Spurious IRQ Unhandled");
	while (1) {
	}
}

void z_arm_swi(void)
{
	EXCEPTION_DUMP("swi unhandled");
	while (1) {
	}
}

void z_arm_fiq(void)
{
	EXCEPTION_DUMP("fiq unhandled");
	while (1) {
	}
}
