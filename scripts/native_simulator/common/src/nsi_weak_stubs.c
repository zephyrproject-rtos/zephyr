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
 *
 * This exists in case the total device image is assembled lacking some of the embedded CPU images
 */

static void nsi_boot_warning(const char *func)
{
	nsi_print_trace("%s: Attempted boot of CPU without image. "
			"CPU shut down permanently\n", func);
}

/*
 * These will define N functions like
 * int nsif_cpu<n>_cleanup(void) { return 0; }
 */
F_TRAMP_BODY_LIST(__attribute__((weak)) void nsif_cpu, _pre_cmdline_hooks(void) { })
F_TRAMP_BODY_LIST(__attribute__((weak)) void nsif_cpu, _pre_hw_init_hooks(void) { })
F_TRAMP_BODY_LIST(__attribute__((weak)) void nsif_cpu,
		  _boot(void) { nsi_boot_warning(__func__); })
F_TRAMP_BODY_LIST(__attribute__((weak)) int nsif_cpu, _cleanup(void) { return 0; })
F_TRAMP_BODY_LIST(__attribute__((weak)) void nsif_cpu, _irq_raised(void) { })
F_TRAMP_BODY_LIST(__attribute__((weak)) void nsif_cpu, _irq_raised_from_sw(void) { })
F_TRAMP_BODY_LIST(__attribute__((weak)) int nsif_cpu, _test_hook(void *p) { return 0; })
