/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>

extern FUNC_NORETURN void z_cstart(void);

/* defined by the SoC in case of CONFIG_SOC_HAS_RUNTIME_NUM_CPUS=y */
extern void soc_num_cpus_init(void);

/* Make sure the platform configuration matches what the toolchain
 * thinks the hardware is doing.
 */
#ifdef CONFIG_DCACHE_LINE_SIZE
BUILD_ASSERT(CONFIG_DCACHE_LINE_SIZE == XCHAL_DCACHE_LINESIZE);
#endif

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
#if CONFIG_SOC_HAS_RUNTIME_NUM_CPUS
	soc_num_cpus_init();
#endif

	_cpu_t *cpu0 = &_kernel.cpus[0];

#ifdef CONFIG_KERNEL_COHERENCE
	/* Make sure we don't have live data for unexpected cached
	 * regions due to boot firmware
	 */
	sys_cache_data_flush_and_invd_all();

	/* Our cache top stash location might have junk in it from a
	 * pre-boot environment.  Must be zero or valid!
	 */
	XTENSA_WSR(ZSR_FLUSH_STR, 0);
#endif

	cpu0->nested = 0;

	/* The asm2 scheme keeps the kernel pointer in a scratch SR
	 * (see zsr.h for generation specifics) for easy access.  That
	 * saves 4 bytes of immediate value to store the address when
	 * compared to the legacy scheme.  But in SMP this record is a
	 * per-CPU thing and having it stored in a SR already is a big
	 * win.
	 */
	XTENSA_WSR(ZSR_CPU_STR, cpu0);

#ifdef CONFIG_INIT_STACKS
	char *stack_start = K_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]);
	size_t stack_sz = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]);
	char *stack_end = stack_start + stack_sz;

	uint32_t sp;

	__asm__ volatile("mov %0, sp" : "=a"(sp));

	/* Only clear the interrupt stack if the current stack pointer
	 * is not within the interrupt stack. Or else we would be
	 * wiping the in-use stack.
	 */
	if (((uintptr_t)sp < (uintptr_t)stack_start) ||
	    ((uintptr_t)sp >= (uintptr_t)stack_end)) {
		memset(stack_start, 0xAA, stack_sz);
	}
#endif
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif

#ifdef CONFIG_XTENSA_MMU
	xtensa_mmu_init();
#endif

#ifdef CONFIG_XTENSA_MPU
	xtensa_mpu_init();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
