/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/arch/security_partition.h>
#include <zephyr/arch/secure_domain.h>
#include <cmsis_core.h>

int arch_security_partition_configure(const struct arch_security_region *regions, uint8_t count)
{
	uint8_t max = (uint8_t)(SAU->TYPE & SAU_TYPE_SREGION_Msk);

	if (count > max) {
		return -ENOMEM;
	}

	/* Disable while reconfiguring to avoid transient mismatches. */
	SAU->CTRL = 0U;

	uint8_t hw_idx = 0U;

	for (uint8_t i = 0U; i < count; i++) {
		uintptr_t base = regions[i].base;
		size_t size = regions[i].size;

		/* ARCH_SECURITY_ATTR_S: S is the default for addresses not
		 * covered by an enabled SAU region — nothing to program.
		 */
		if (regions[i].attr == ARCH_SECURITY_ATTR_S) {
			continue;
		}

		if (size == 0U || (base & 0x1fU) != 0U || (size & 0x1fU) != 0U) {
			return -EINVAL;
		}

		/* Allow size that reaches exactly the end of 32-bit space (0x100000000). */
		if ((uint64_t)base + (uint64_t)size > 0x100000000ULL) {
			return -EINVAL;
		}

		uint32_t rlar = ((base + size - 1U) & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

		if (regions[i].attr == ARCH_SECURITY_ATTR_NSC) {
			rlar |= SAU_RLAR_NSC_Msk;
		}

		SAU->RNR = hw_idx;
		SAU->RBAR = base & SAU_RBAR_BADDR_Msk;
		SAU->RLAR = rlar;
		hw_idx++;
	}

	/* Clear any remaining hardware regions that are no longer needed. */
	for (uint8_t i = hw_idx; i < max; i++) {
		SAU->RNR = i;
		SAU->RBAR = 0U;
		SAU->RLAR = 0U;
	}

	return 0;
}

void arch_security_partition_enable(void)
{
	SAU->CTRL = SAU_CTRL_ENABLE_Msk;
	__DSB();
	__ISB();
}

void arch_security_partition_disable(void)
{
	SAU->CTRL = 0U;
	__DSB();
	__ISB();
}

int arch_security_partition_region_count(void)
{
	return (int)(SAU->TYPE & SAU_TYPE_SREGION_Msk);
}

/*
 * Secure-domain entry (arch_secure_domain_swap()).
 *
 * The generic ARMv8-M backend hooks below are weak so a SoC can override the
 * vector-table write and/or the world-crossing hand-off for silicon that
 * deviates from the architectural sequence (e.g. PSE84).
 */

static void secure_domain_enable_fpu(void)
{
#if defined(CONFIG_CPU_HAS_FPU)
	/*
	 * NSACR.CP10/CP11 (Secure-only) grant the Non-Secure state access to the
	 * FPU coprocessors; the Non-Secure CPACR then enables full NS access.
	 */
	SCB->NSACR |= (1U << 10) | (1U << 11);
	SCB_NS->CPACR |= (0xFUL << 20);
	__DSB();
	__ISB();
#endif
}

__weak void z_arch_secure_domain_set_vector_table(uintptr_t base)
{
	SCB_NS->VTOR = (uint32_t)base;
}

#if defined(CONFIG_ARM_SECURE_DOMAIN_ENTRY_SVC)

static void __attribute__((naked)) secure_domain_svc_entry(void)
{
	__asm__ volatile("ldr r0, =0xFFFFFFB9\n\t" /* EXC_RETURN: NS/Thread/MSP_NS/basic */
			 "bx r0\n\t");
}

/*
 * An ARMv8-M vector table holds the 16 architectural system exceptions
 * (initial SP, Reset, ..., SVCall at index 11, ..., SysTick) followed by the
 * CONFIG_NUM_IRQS external interrupt vectors.  The RAM copy must be able to
 * hold all of them.  VTOR requires the table base to be aligned to a power of
 * two no smaller than the table size; 0x400 (256 vectors) satisfies every
 * ARMv8-M IRQ count and is asserted below.
 */
#define SECURE_DOMAIN_SVC_VECTOR  11
#define SECURE_DOMAIN_NUM_VECTORS (16 + CONFIG_NUM_IRQS)

BUILD_ASSERT(SECURE_DOMAIN_NUM_VECTORS <= 256,
	     "Non-Secure vector table exceeds the 0x400 VTOR alignment");

static uint32_t secure_domain_vtable_ram[SECURE_DOMAIN_NUM_VECTORS] __aligned(0x400);

/*
 * Some silicon raises INVTRAN on BXNS even when every ARMv8-M precondition is
 * satisfied (a documented deviation, e.g. Infineon PSE84 / PSC3).  Enter the
 * Non-Secure world via an exception return instead:
 *   1. Build a Non-Secure exception frame at the top of MSP_NS.
 *   2. Clone the Secure vector table into RAM and replace the SVC handler with
 *      a stub that immediately performs EXC_RETURN to NS Thread mode.
 *   3. SVC -> Secure SVC handler -> EXC_RETURN 0xFFFFFFB9 pops the frame and
 *      resumes at the entry point in Non-Secure Thread mode on MSP_NS.
 */
__weak FUNC_NORETURN void z_arch_secure_domain_enter(uintptr_t entry, uintptr_t stack)
{
	uint32_t *frm = (uint32_t *)(stack - 32U);

	frm[0] = 0U;                    /* r0 */
	frm[1] = 0U;                    /* r1 */
	frm[2] = 0U;                    /* r2 */
	frm[3] = 0U;                    /* r3 */
	frm[4] = 0U;                    /* r12 */
	frm[5] = 0xFFFFFFFFU;           /* lr (uninitialised thread default) */
	frm[6] = (uint32_t)entry & ~1U; /* pc (T bit comes from xPSR) */
	frm[7] = 0x01000000U;           /* xPSR: T=1, Thread, no active ISR */
	__TZ_set_MSP_NS((uint32_t)frm);
	__DSB();

	const uint32_t *vt = (const uint32_t *)(SCB->VTOR);

	for (int i = 0; i < SECURE_DOMAIN_NUM_VECTORS; i++) {
		secure_domain_vtable_ram[i] = vt[i];
	}
	secure_domain_vtable_ram[SECURE_DOMAIN_SVC_VECTOR] = (uint32_t)secure_domain_svc_entry | 1U;
	SCB->VTOR = (uint32_t)secure_domain_vtable_ram;
	__DSB();
	__ISB();

	__asm__ volatile("svc #0");

	CODE_UNREACHABLE;
}

#else /* BXNS */

/*
 * On real Cortex-M33, bit 0 of the BXNS target selects the Thumb state and must
 * be 1.  QEMU's v7m_bxns instead interprets bit 0 as the target security state
 * (0 = Non-Secure), so under emulation it must be cleared.
 */
__weak FUNC_NORETURN void z_arch_secure_domain_enter(uintptr_t entry, uintptr_t stack)
{
	__TZ_set_MSP_NS((uint32_t)stack);
	__DSB();

#if defined(CONFIG_QEMU_TARGET)
	__asm__ volatile("bxns %0" : : "r"(entry & ~(uintptr_t)1U));
#else
	__asm__ volatile("bxns %0" : : "r"(entry | (uintptr_t)1U));
#endif

	CODE_UNREACHABLE;
}

#endif /* CONFIG_ARM_SECURE_DOMAIN_ENTRY_SVC */

FUNC_NORETURN void arch_secure_domain_swap(const struct arch_secure_domain *domain)
{
	if (domain->vector_table != 0U) {
		z_arch_secure_domain_set_vector_table(domain->vector_table);
	}
	if ((domain->flags & ARCH_SECURE_DOMAIN_ENABLE_FPU) != 0U) {
		secure_domain_enable_fpu();
	}

	z_arch_secure_domain_enter(domain->entry, domain->stack);

	CODE_UNREACHABLE;
}

__weak FUNC_NORETURN void arch_secure_call_oops(void)
{
	/*
	 * A secure-call argument failed validation, so Non-Secure code tried to
	 * reach Secure memory.  Halt rather than continue; a SoC may override
	 * this with a richer oops/reset handler.
	 */
	for (;;) {
		/* halt */
	}
}
