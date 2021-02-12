/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include "vector_table.h"

void __weak z_arm64_el_highest_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el3_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el2_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el1_plat_init(void)
{
	/* do nothing */
}

void z_arm64_el_highest_init(void)
{
	write_cntfrq_el0(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	z_arm64_el_highest_plat_init();

	isb();
}

void z_arm64_el3_init(void)
{
	uint64_t reg;

	/* Setup vector table */
	write_vbar_el3((uint64_t)_vector_table);
	isb();

	reg = 0U;			/* Mostly RES0 */
	reg &= ~(CPTR_TTA_BIT |		/* Do not trap sysreg accesses */
		 CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT);	/* Do not trap CPTR_EL2 / CPACR_EL1 accesses */
	write_cptr_el3(reg);

	reg = 0U;			/* Reset */
#ifdef CONFIG_ARMV8_A_NS
	reg |= SCR_NS_BIT;		/* EL2 / EL3 non-secure */
#endif
	reg |= (SCR_RES1 |		/* RES1 */
		SCR_RW_BIT |		/* EL2 execution state is AArch64 */
		SCR_ST_BIT |		/* Do not trap EL1 accesses to timer */
		SCR_HCE_BIT |		/* Do not trap HVC */
		SCR_SMD_BIT);		/* Do not trap SMC */
	write_scr_el3(reg);

	z_arm64_el3_plat_init();

	isb();
}

void z_arm64_el2_init(void)
{
	uint64_t reg;

	reg = read_sctlr_el2();
	reg |= (SCTLR_EL2_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el2(reg);

	reg = read_hcr_el2();
	reg |= HCR_RW_BIT;		/* EL1 Execution state is AArch64 */
	write_hcr_el2(reg);

	reg = 0U;			/* RES0 */
	reg |= CPTR_EL2_RES1;		/* RES1 */
	reg &= ~(CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT);	/* Do not trap CPACR_EL1 accesses */
	write_cptr_el2(reg);

	zero_cntvoff_el2();		/* Set 64-bit virtual timer offset to 0 */
	zero_cnthctl_el2();
	zero_cnthp_ctl_el2();

	z_arm64_el2_plat_init();

	isb();
}

void z_arm64_el1_init(void)
{
	uint64_t reg;

	/* Setup vector table */
	write_vbar_el1((uint64_t)_vector_table);
	isb();

	reg = 0U;			/* RES0 */
	reg |= CPACR_EL1_FPEN_NOTRAP;	/* Do not trap NEON/SIMD/FP */
					/* TODO: CONFIG_FLOAT_*_FORBIDDEN */
	write_cpacr_el1(reg);

	reg = read_sctlr_el1();
	reg |= (SCTLR_EL1_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el1(reg);

	z_arm64_el1_plat_init();

	isb();
}

void z_arm64_el3_get_next_el(uint64_t switch_addr)
{
	uint64_t spsr;

	write_elr_el3(switch_addr);

	/* Mask the DAIF */
	spsr = SPSR_DAIF_MASK;

	/*
	 * Is considered an illegal return "[..] a return to EL2 when EL3 is
	 * implemented and the value of the SCR_EL3.NS bit is 0 if
	 * ARMv8.4-SecEL2 is not implemented" (D1.11.2 from ARM DDI 0487E.a)
	 */
	if (is_el_implemented(2) &&
	   ((is_in_secure_state() && is_el2_sec_supported()) || !is_in_secure_state())) {
		/* Dropping into EL2 */
		spsr |= SPSR_MODE_EL2T;
	} else {
		/* Dropping into EL1 */
		spsr |= SPSR_MODE_EL1T;
	}

	write_spsr_el3(spsr);
}
