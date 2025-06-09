/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE_NUM_SPIN_RELAX_NOPS
void arch_spin_relax(void)
{
	register uint32_t remaining = CONFIG_SOC_SERIES_INTEL_ADSP_ACE_NUM_SPIN_RELAX_NOPS;

	while (remaining > 0) {
#if (CONFIG_SOC_SERIES_INTEL_ADSP_ACE_NUM_SPIN_RELAX_NOPS % 4) == 0
		remaining -= 4;

		/*
		 * Note the xcc/xt-clang likes to "truncate"
		 * continuous NOPs to max 4 NOPs. So this is
		 * the most we can do in one loop.
		 */
		__asm__("nop.n; nop.n; nop.n; nop.n;");
#else
		remaining--;
		__asm__("nop.n");
#endif
	}
}
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE_NUM_SPIN_RELAX_NOPS */
