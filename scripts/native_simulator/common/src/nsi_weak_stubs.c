/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsi_cpu_if.h"
#include "nsi_tracing.h"

/*
 * Stubbed embedded CPU images, which do nothing:
 *   The CPU does not boot, and interrupts are just ignored
 * These are all defined as weak, so if an actual image is present for that CPU,
 * that will be linked against.
 */

#define NSI_CPU_STUBBED_IMAGE(i)                                          \
__attribute__((weak)) void nsif_cpu##i##_pre_cmdline_hooks(void) { }      \
__attribute__((weak)) void nsif_cpu##i##_pre_hw_init_hooks(void) { }      \
__attribute__((weak)) void nsif_cpu##i##_boot(void)                       \
	{                                                                 \
		nsi_print_trace("Attempted boot of CPU %i without image. "\
				"CPU %i shut down permanently\n", i, i);  \
	}                                                                 \
__attribute__((weak)) int nsif_cpu##i##_cleanup(void) { return 0; }       \
__attribute__((weak)) void nsif_cpu##i##_irq_raised(void) { }             \
__attribute__((weak)) void nsif_cpu##i##_irq_raised_from_sw(void) { }

NSI_CPU_STUBBED_IMAGE(0)
NSI_CPU_STUBBED_IMAGE(1)
NSI_CPU_STUBBED_IMAGE(2)
NSI_CPU_STUBBED_IMAGE(3)
NSI_CPU_STUBBED_IMAGE(4)
NSI_CPU_STUBBED_IMAGE(5)
NSI_CPU_STUBBED_IMAGE(6)
NSI_CPU_STUBBED_IMAGE(7)
NSI_CPU_STUBBED_IMAGE(8)
NSI_CPU_STUBBED_IMAGE(9)
NSI_CPU_STUBBED_IMAGE(10)
NSI_CPU_STUBBED_IMAGE(11)
NSI_CPU_STUBBED_IMAGE(12)
NSI_CPU_STUBBED_IMAGE(13)
NSI_CPU_STUBBED_IMAGE(14)
NSI_CPU_STUBBED_IMAGE(15)
