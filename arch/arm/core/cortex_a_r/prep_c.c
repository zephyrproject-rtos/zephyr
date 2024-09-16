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
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>
#include <zephyr/platform/hooks.h>

#if defined(CONFIG_ARMV7_R) || defined(CONFIG_ARMV7_A)
#include <cortex_a_r/stack.h>
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

#ifdef CONFIG_ARM_MPU
extern void z_arm_mpu_init(void);
extern void z_arm_configure_static_mpu_regions(void);
#elif defined(CONFIG_ARM_AARCH32_MMU)
extern int z_arm_mmu_init(void);
#endif

#if defined(CONFIG_AARCH32_ARMV8_R)

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

static inline void relocate_vector_table(void)
{
	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	barrier_isync_fence_full();
}

#else
#define VECTOR_ADDRESS 0

void __weak relocate_vector_table(void)
{
#if defined(CONFIG_XIP) && (CONFIG_FLASH_BASE_ADDRESS != 0) || \
	!defined(CONFIG_XIP) && (CONFIG_SRAM_BASE_ADDRESS != 0)
	write_sctlr(read_sctlr() & ~HIVECS);
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	(void)memcpy(VECTOR_ADDRESS, _vector_start, vector_size);
#elif defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
	_vector_table_pointer = _vector_start;
#endif
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* CONFIG_AARCH32_ARMV8_R */

#if defined(CONFIG_CPU_HAS_FPU)

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
	barrier_isync_fence_full();

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

#endif /* CONFIG_CPU_HAS_FPU */

extern FUNC_NORETURN void z_cstart(void);

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 */
void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif
	/* Initialize tpidruro with our struct _cpu instance address */
	write_tpidruro((uintptr_t)&_kernel.cpus[0]);

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
#ifdef CONFIG_ARM_MPU
	z_arm_mpu_init();
	z_arm_configure_static_mpu_regions();
#elif defined(CONFIG_ARM_AARCH32_MMU)
	z_arm_mmu_init();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
