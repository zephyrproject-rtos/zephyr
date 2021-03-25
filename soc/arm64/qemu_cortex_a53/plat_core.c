/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

void z_arm64_el3_plat_init(void)
{
	uint64_t reg = 0;

	reg = (ICC_SRE_ELx_DFB_BIT | ICC_SRE_ELx_DIB_BIT |
	       ICC_SRE_ELx_SRE_BIT | ICC_SRE_EL3_EN_BIT);

	write_sysreg(reg, ICC_SRE_EL3);
}
