/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/cache.h>
#include <zephyr/sys/barrier.h>
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

	barrier_isync_fence_full();
}


#if !defined(CONFIG_ARMV8_R)
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
	barrier_isync_fence_full();

	reg = 0U;			/* Mostly RES0 */
	reg &= ~(CPTR_TTA_BIT |		/* Do not trap sysreg accesses */
		 CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT);	/* Do not trap CPTR_EL2 / CPACR_EL1 accesses */

#ifdef CONFIG_ARM64_SVE
	/* Enable SVE for EL2 and below if SVE is implemented */
	if (is_sve_implemented()) {
		reg |= CPTR_EZ_BIT;		/* Enable SVE access for lower ELs */
		write_cptr_el3(reg);

		/* Initialize ZCR_EL3 for full SVE vector length */
		/* ZCR_EL3.LEN = 0x1ff means full hardware vector length */
		write_zcr_el3(0x1ff);
	} else {
		write_cptr_el3(reg);
	}
#else
	write_cptr_el3(reg);
#endif

	reg = 0U;			/* Reset */
#ifdef CONFIG_ARMV8_A_NS
	reg |= SCR_NS_BIT;		/* EL2 / EL3 non-secure */
#else
	if (is_in_secure_state() && is_el2_sec_supported()) {
		reg |= SCR_EEL2_BIT;    /* Enable EL2 secure */
	}
#endif
	reg |= (SCR_RES1 |		/* RES1 */
		SCR_RW_BIT |		/* EL2 execution state is AArch64 */
		SCR_ST_BIT |		/* Do not trap EL1 accesses to timer */
		SCR_HCE_BIT |		/* Do not trap HVC */
		SCR_SMD_BIT);		/* Do not trap SMC */
#ifdef CONFIG_ARM_PAC
	reg |= (SCR_APK_BIT |		/* Do not trap pointer authentication key accesses */
		SCR_API_BIT);		/* Do not trap pointer authentication instructions */
#endif
	write_scr_el3(reg);

#if defined(CONFIG_GIC_V3)
	reg = read_sysreg(ICC_SRE_EL3);
	reg |= (ICC_SRE_ELx_DFB_BIT |	/* Disable FIQ bypass */
		ICC_SRE_ELx_DIB_BIT |	/* Disable IRQ bypass */
		ICC_SRE_ELx_SRE_BIT |	/* System register interface is used */
		ICC_SRE_ELx_EN_BIT);	/* Enables lower Exception level access to ICC_SRE_EL1 */
	write_sysreg(reg, ICC_SRE_EL3);
#endif

	z_arm64_el3_plat_init();

	barrier_isync_fence_full();

	if (el3_get_next_el() == EL3_TO_EL1_SKIP_EL2) {
		/*
		 * handle EL2 init in EL3, as it still needs to be done,
		 * but we are going to be skipping EL2.
		 */
		z_arm64_el2_init();
	}
}
#endif /* CONFIG_ARMV8_R */

void z_arm64_el2_init(void)
{
	uint64_t reg;

	reg = read_sctlr_el2();
#ifdef CONFIG_ARM64_BOOT_DISABLE_DCACHE
	/* Disable D-Cache if it is enabled, it will re-enabled when MMU is enabled at EL1 */
	if (reg & SCTLR_C_BIT) {
		/* Clean and invalidate the data cache before disabling it to ensure memory
		 * remains coherent.
		 */
		arch_dcache_flush_and_invd_all();
		barrier_isync_fence_full();
		/* Disable D-Cache and MMU for EL2 */
		reg &= ~(SCTLR_C_BIT | SCTLR_M_BIT);
		write_sctlr_el2(reg);
		/* Invalidate TLB entries */
		__asm__ volatile("dsb ishst; tlbi alle2; dsb ish; isb" : : : "memory");
	}
#endif
	reg |= (SCTLR_EL2_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el2(reg);

#if defined(CONFIG_GIC_V3)
	if (!is_in_secure_state() || is_el2_sec_supported()) {
		reg = read_sysreg(ICC_SRE_EL2);
		reg |= (ICC_SRE_ELx_DFB_BIT |   /* Disable FIQ bypass */
			ICC_SRE_ELx_DIB_BIT |   /* Disable IRQ bypass */
			ICC_SRE_ELx_SRE_BIT |   /* System register interface is used */
			ICC_SRE_ELx_EN_BIT);    /* Enables Exception access to ICC_SRE_EL1 */
		write_sysreg(reg, ICC_SRE_EL2);
	}
#endif

	reg = read_hcr_el2();
	/* when EL2 is enable in current security status:
	 * Clear TGE bit: All exceptions that would not be routed to EL2;
	 * Clear AMO bit: Physical SError interrupts are not taken to EL2 and EL3.
	 * Clear IMO bit: Physical IRQ interrupts are not taken to EL2 and EL3.
	 */
	reg &= ~(HCR_IMO_BIT | HCR_AMO_BIT | HCR_TGE_BIT);
	reg |= HCR_RW_BIT;		/* EL1 Execution state is AArch64 */

#ifdef CONFIG_ARM_PAC
	/* Do not trap pointer authentication instructions and key registers */
	reg |= (HCR_API_BIT | HCR_APK_BIT);
#endif

	write_hcr_el2(reg);

	reg = 0U;			/* RES0 */
	reg |= CPTR_EL2_RES1;		/* RES1 */
	reg &= ~(CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT |	/* Do not trap CPACR_EL1 accesses */
		 CPTR_EL2_TZ_BIT);	/* Do not trap SVE to EL2 */
#ifdef CONFIG_ARM64_SVE
	/* Enable SVE for EL1 and EL0 if SVE is implemented */
	if (is_sve_implemented()) {
		reg &= ~CPTR_EL2_ZEN_MASK;
		reg |= (CPTR_EL2_ZEN_EL1_EN | CPTR_EL2_ZEN_EL0_EN);
		write_cptr_el2(reg);

		/* Initialize ZCR_EL2 for full SVE vector length */
		/* ZCR_EL2.LEN = 0x1ff means full hardware vector length */
		write_zcr_el2(0x1ff);
	} else {
		write_cptr_el2(reg);
	}
#else
	write_cptr_el2(reg);
#endif

	zero_cntvoff_el2();		/* Set 64-bit virtual timer offset to 0 */
	zero_cnthctl_el2();
#ifdef CONFIG_CPU_AARCH64_CORTEX_R
	zero_cnthps_ctl_el2();
#else
	zero_cnthp_ctl_el2();
#endif

#ifdef CONFIG_ARM64_SET_VMPIDR_EL2
	reg = read_mpidr_el1();
	write_vmpidr_el2(reg);
#endif

	/*
	 * Enable this if/when we use the hypervisor timer.
	 * write_cnthp_cval_el2(~(uint64_t)0);
	 */

	z_arm64_el2_plat_init();

	barrier_isync_fence_full();
}

#ifdef CONFIG_ARM_PAC
__attribute__((target("branch-protection=none")))
#endif
void z_arm64_el1_init(void)
{
	uint64_t reg;

	/* Setup vector table */
	write_vbar_el1((uint64_t)_vector_table);
	barrier_isync_fence_full();

	reg = 0U;			/* RES0 */
	reg |= CPACR_EL1_FPEN;		/* Do not trap NEON/SIMD/FP initially */
					/* TODO: CONFIG_FLOAT_*_FORBIDDEN */
#ifdef CONFIG_ARM64_SVE
	/* Enable SVE access if SVE is implemented */
	if (is_sve_implemented()) {
		reg |= CPACR_EL1_ZEN;	/* Do not trap SVE initially */
		write_cpacr_el1(reg);

		/* Initialize ZCR_EL1 SVE vector length */
		write_zcr_el1(CONFIG_ARM64_SVE_VL_MAX/16 - 1);
	} else {
		write_cpacr_el1(reg);
	}
#else
	write_cpacr_el1(reg);
#endif

	reg = read_sctlr_el1();
	reg |= (SCTLR_EL1_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_C_BIT |		/* Enable d-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */

#ifdef CONFIG_ARM_PAC
	/* Set a default APIA key BEFORE enabling PAC */
	write_apiakeylo_el1(0x0123456789ABCDEFULL);
	write_apiakeyhi_el1(0xFEDCBA9876543210ULL);
	/* Now enable Pointer Authentication */
	reg |= SCTLR_EnIA_BIT;		/* Enable instruction address signing using key A */
#endif

#ifdef CONFIG_ARM_BTI
	/* Enable Branch Target Identification */
	reg |= SCTLR_BT0_BIT;		/* Enable BTI for EL0 (userspace threads) */
	reg |= SCTLR_BT1_BIT;		/* Enable BTI for EL1 (kernel) */
#endif

	write_sctlr_el1(reg);
	barrier_isync_fence_full();

	write_cntv_cval_el0(~(uint64_t)0);
	/*
	 * Enable these if/when we use the corresponding timers.
	 * write_cntp_cval_el0(~(uint64_t)0);
	 * write_cntps_cval_el1(~(uint64_t)0);
	 */

	z_arm64_el1_plat_init();

	barrier_isync_fence_full();
}

#if !defined(CONFIG_ARMV8_R)
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
#endif /* CONFIG_ARMV8_R */
