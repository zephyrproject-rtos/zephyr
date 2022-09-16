/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include "boot.h"

void z_arm64_el2_init(void);

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
	if (is_el_highest_implemented()) {
		write_cntfrq_el0(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	z_arm64_el_highest_plat_init();

	isb();
}

enum el3_next_el {
	EL3_TO_EL2,
	EL3_TO_EL1_NO_EL2,
	EL3_TO_EL1_SKIP_EL2
};

static inline enum el3_next_el el3_get_next_el(void)
{
	if (!is_el_implemented(2)) {
		return EL3_TO_EL1_NO_EL2;
	} else if (is_in_secure_state() && !is_el2_sec_supported()) {
		/*
		 * Is considered an illegal return "[..] a return to EL2 when EL3 is
		 * implemented and the value of the SCR_EL3.NS bit is 0 if
		 * ARMv8.4-SecEL2 is not implemented" (D1.11.2 from ARM DDI 0487E.a)
		 */
		return EL3_TO_EL1_SKIP_EL2;
	} else {
		return EL3_TO_EL2;
	}
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

#if defined(CONFIG_GIC_V3)
	reg = read_sysreg(ICC_SRE_EL3);
	reg |= (ICC_SRE_ELx_DFB_BIT |	/* Disable FIQ bypass */
		ICC_SRE_ELx_DIB_BIT |	/* Disable IRQ bypass */
		ICC_SRE_ELx_SRE_BIT |	/* System register interface is used */
		ICC_SRE_EL3_EN_BIT);	/* Enables lower Exception level access to ICC_SRE_EL1 */
	write_sysreg(reg, ICC_SRE_EL3);
#endif

	z_arm64_el3_plat_init();

	isb();

	if (el3_get_next_el() == EL3_TO_EL1_SKIP_EL2) {
		/*
		 * handle EL2 init in EL3, as it still needs to be done,
		 * but we are going to be skipping EL2.
		 */
		z_arm64_el2_init();
	}
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
#ifdef CONFIG_CPU_AARCH64_CORTEX_R
	zero_cnthps_ctl_el2();
#else
	zero_cnthp_ctl_el2();
#endif
	/*
	 * Enable this if/when we use the hypervisor timer.
	 * write_cnthp_cval_el2(~(uint64_t)0);
	 */

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
	reg |= CPACR_EL1_FPEN_NOTRAP;	/* Do not trap NEON/SIMD/FP initially */
					/* TODO: CONFIG_FLOAT_*_FORBIDDEN */
	write_cpacr_el1(reg);

	reg = read_sctlr_el1();
	reg |= (SCTLR_EL1_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el1(reg);

	write_cntv_cval_el0(~(uint64_t)0);
	/*
	 * Enable these if/when we use the corresponding timers.
	 * write_cntp_cval_el0(~(uint64_t)0);
	 * write_cntps_cval_el1(~(uint64_t)0);
	 */

	z_arm64_el1_plat_init();

	isb();
}

void z_arm64_el3_get_next_el(uint64_t switch_addr)
{
	uint64_t spsr;

	write_elr_el3(switch_addr);

	/* Mask the DAIF */
	spsr = SPSR_DAIF_MASK;

	if (el3_get_next_el() == EL3_TO_EL2) {
		/* Dropping into EL2 */
		spsr |= SPSR_MODE_EL2T;
	} else {
		/* Dropping into EL1 */
		spsr |= SPSR_MODE_EL1T;
	}

	write_spsr_el3(spsr);
}
