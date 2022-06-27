/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/linker/linker-defs.h>

#if defined(CONFIG_ARMV7_R) || defined(CONFIG_ARMV7_A)
#include <aarch32/cortex_a_r/stack.h>
#endif

#if defined(__GNUC__)
/*
 * GCC can detect if memcpy is passed a NULL argument, however one of
 * the cases of relocate_vector_table() it is valid to pass NULL, so we
 * suppress the warning for this case.  We need to do this before
 * string.h is included to get the declaration of memcpy.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

#include <string.h>

#if defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
Z_GENERIC_SECTION(.vt_pointer_section) __attribute__((used))
void *_vector_table_pointer;
#endif

#ifdef CONFIG_CPU_CORTEX_M_HAS_VTOR

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

static inline void relocate_vector_table(void)
{
	SCB->VTOR = VECTOR_ADDRESS & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
}

#elif defined(CONFIG_AARCH32_ARMV8_R)

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

static inline void relocate_vector_table(void)
{
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	__ISB();
}

#else
#define VECTOR_ADDRESS 0

void __weak relocate_vector_table(void)
{
#if defined(CONFIG_XIP) && (CONFIG_FLASH_BASE_ADDRESS != 0) || \
    !defined(CONFIG_XIP) && (CONFIG_SRAM_BASE_ADDRESS != 0)
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	(void)memcpy(VECTOR_ADDRESS, _vector_start, vector_size);
#elif defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
	_vector_table_pointer = _vector_start;
#endif
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* CONFIG_CPU_CORTEX_M_HAS_VTOR */

#if defined(CONFIG_CPU_HAS_FPU)
#if defined(CONFIG_CPU_CORTEX_M)
static inline void z_arm_floating_point_init(void)
{
	/*
	 * Upon reset, the Co-Processor Access Control Register is, normally,
	 * 0x00000000. However, it might be left un-cleared by firmware running
	 * before Zephyr boot.
	 */
	SCB->CPACR &= (~(CPACR_CP10_Msk | CPACR_CP11_Msk));

#if defined(CONFIG_FPU)
	/*
	 * Enable CP10 and CP11 Co-Processors to enable access to floating
	 * point registers.
	 */
#if defined(CONFIG_USERSPACE)
	/* Full access */
	SCB->CPACR |= CPACR_CP10_FULL_ACCESS | CPACR_CP11_FULL_ACCESS;
#else
	/* Privileged access only */
	SCB->CPACR |= CPACR_CP10_PRIV_ACCESS | CPACR_CP11_PRIV_ACCESS;
#endif /* CONFIG_USERSPACE */
	/*
	 * Upon reset, the FPU Context Control Register is 0xC0000000
	 * (both Automatic and Lazy state preservation is enabled).
	 */
#if defined(CONFIG_MULTITHREADING) && !defined(CONFIG_FPU_SHARING)
	/* Unshared FP registers (multithreading) mode. We disable the
	 * automatic stacking of FP registers (automatic setting of
	 * FPCA bit in the CONTROL register), upon exception entries,
	 * as the FP registers are to be used by a single context (and
	 * the use of FP registers in ISRs is not supported). This
	 * configuration improves interrupt latency and decreases the
	 * stack memory requirement for the (single) thread that makes
	 * use of the FP co-processor.
	 */
	FPU->FPCCR &= (~(FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk));
#else
	/*
	 * FP register sharing (multithreading) mode or single-threading mode.
	 *
	 * Enable both automatic and lazy state preservation of the FP context.
	 * The FPCA bit of the CONTROL register will be automatically set, if
	 * the thread uses the floating point registers. Because of lazy state
	 * preservation the volatile FP registers will not be stacked upon
	 * exception entry, however, the required area in the stack frame will
	 * be reserved for them. This configuration improves interrupt latency.
	 * The registers will eventually be stacked when the thread is swapped
	 * out during context-switch or if an ISR attempts to execute floating
	 * point instructions.
	 */
	FPU->FPCCR = FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
#endif /* CONFIG_FPU_SHARING */

	/* Make the side-effects of modifying the FPCCR be realized
	 * immediately.
	 */
	__DSB();
	__ISB();

	/* Initialize the Floating Point Status and Control Register. */
#if defined(CONFIG_ARMV8_1_M_MAINLINE)
	/*
	 * For ARMv8.1-M with FPU, the FPSCR[18:16] LTPSIZE field must be set
	 * to 0b100 for "Tail predication not applied" as it's reset value
	 */
	__set_FPSCR(4 << FPU_FPDSCR_LTPSIZE_Pos);
#else
	__set_FPSCR(0);
#endif

	/*
	 * Note:
	 * The use of the FP register bank is enabled, however the FP context
	 * will be activated (FPCA bit on the CONTROL register) in the presence
	 * of floating point instructions.
	 */

#endif /* CONFIG_FPU */

	/*
	 * Upon reset, the CONTROL.FPCA bit is, normally, cleared. However,
	 * it might be left un-cleared by firmware running before Zephyr boot.
	 * We must clear this bit to prevent errors in exception unstacking.
	 *
	 * Note:
	 * In Sharing FP Registers mode CONTROL.FPCA is cleared before switching
	 * to main, so it may be skipped here (saving few boot cycles).
	 *
	 * If CONFIG_INIT_ARCH_HW_AT_BOOT is set, CONTROL is cleared at reset.
	 */
#if (!defined(CONFIG_FPU) || !defined(CONFIG_FPU_SHARING)) && \
	(!defined(CONFIG_INIT_ARCH_HW_AT_BOOT))

	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
#endif
}

#else

static inline void z_arm_floating_point_init(void)
{
#if defined(CONFIG_FPU)
	uint32_t reg_val = 0;

	/*
	 * CPACR : Coprocessor Access Control Register -> CP15 1/0/2
	 * comp. ARM Architecture Reference Manual, ARMv7-A and ARMv7-R edition,
	 * chap. B4.1.40
	 *
	 * Must be accessed in >= PL1!
	 * [23..22] = CP11 access control bits,
	 * [21..20] = CP10 access control bits.
	 * 11b = Full access as defined for the respective CP,
	 * 10b = UNDEFINED,
	 * 01b = Access at PL1 only,
	 * 00b = No access.
	 */
	reg_val = __get_CPACR();
	/* Enable PL1 access to CP10, CP11 */
	reg_val |= (CPACR_CP10(CPACR_FA) | CPACR_CP11(CPACR_FA));
	__set_CPACR(reg_val);
	__ISB();

#if !defined(CONFIG_FPU_SHARING)
	/*
	 * FPEXC: Floating-Point Exception Control register
	 * comp. ARM Architecture Reference Manual, ARMv7-A and ARMv7-R edition,
	 * chap. B6.1.38
	 *
	 * Must be accessed in >= PL1!
	 * [31] EX bit = determines which registers comprise the current state
	 *               of the FPU. The effects of setting this bit to 1 are
	 *               subarchitecture defined. If EX=0, the following
	 *               registers contain the complete current state
	 *               information of the FPU and must therefore be saved
	 *               during a context switch:
	 *               * D0-D15
	 *               * D16-D31 if implemented
	 *               * FPSCR
	 *               * FPEXC.
	 * [30] EN bit = Advanced SIMD/Floating Point Extensions enable bit.
	 * [29..00]    = Subarchitecture defined -> not relevant here.
	 */
	__set_FPEXC(FPEXC_EN);
#endif
#endif
}

#endif /* CONFIG_CPU_CORTEX_M */
#endif /* CONFIG_CPU_HAS_FPU */

extern FUNC_NORETURN void z_cstart(void);

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 */
void z_arm_prep_c(void)
{
	relocate_vector_table();
#if defined(CONFIG_CPU_HAS_FPU)
	z_arm_floating_point_init();
#endif
	z_bss_zero();
	z_data_copy();
#if ((defined(CONFIG_ARMV7_R) || defined(CONFIG_ARMV7_A)) && defined(CONFIG_INIT_STACKS))
	z_arm_init_stacks();
#endif
	z_arm_interrupt_init();
	z_cstart();
	CODE_UNREACHABLE;
}
