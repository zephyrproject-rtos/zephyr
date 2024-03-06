/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the RISCV processor architecture.
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>
#include <pmp.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void arch_kernel_init(void)
{
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	__asm__ volatile ("li tp, 0");
#endif
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	csr_write(mscratch, &_kernel.cpus[0]);
#endif
#ifdef CONFIG_SMP
	_kernel.cpus[0].arch.hartid = csr_read(mhartid);
	_kernel.cpus[0].arch.online = true;
#endif
#if ((CONFIG_MP_MAX_NUM_CPUS) > 1)
	unsigned int cpu_node_list[] = {
		DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(cpus), DT_REG_ADDR, (,))
	};
	unsigned int cpu_num, hart_x;

	for (cpu_num = 1, hart_x = 0; cpu_num < arch_num_cpus(); cpu_num++) {
		if (cpu_node_list[hart_x] == _kernel.cpus[0].arch.hartid) {
			hart_x++;
		}
		_kernel.cpus[cpu_num].arch.hartid = cpu_node_list[hart_x];
		hart_x++;
	}
#endif
#ifdef CONFIG_RISCV_PMP
	z_riscv_pmp_init();
#endif
}

static ALWAYS_INLINE void
arch_switch(void *switch_to, void **switched_from)
{
	extern void z_riscv_switch(struct k_thread *new, struct k_thread *old);
	struct k_thread *new = switch_to;
	struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
					    switch_handle);
#ifdef CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL
	arch_syscall_invoke2((uintptr_t)new, (uintptr_t)old, RV_ECALL_SCHEDULE);
#else
	z_riscv_switch(new, old);
#endif
}

/* Thin wrapper around z_riscv_fatal_error_csf */
FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf);

FUNC_NORETURN void z_riscv_fatal_error_csf(unsigned int reason, const z_arch_esf_t *esf,
					   const _callee_saved_t *csf);

static inline bool arch_is_in_isr(void)
{
#ifdef CONFIG_SMP
	unsigned int key = arch_irq_lock();
	bool ret = arch_curr_cpu()->nested != 0U;

	arch_irq_unlock(key);
	return ret;
#else
	return _kernel.cpus[0].nested != 0U;
#endif
}

extern FUNC_NORETURN void z_riscv_userspace_enter(k_thread_entry_t user_entry,
						 void *p1, void *p2, void *p3,
						 uint32_t stack_end,
						 uint32_t stack_start);

#ifdef CONFIG_IRQ_OFFLOAD
int z_irq_do_offload(void);
#endif

#ifdef CONFIG_FPU_SHARING
void arch_flush_local_fpu(void);
void arch_flush_fpu_ipi(unsigned int cpu);
#endif

#ifndef CONFIG_MULTITHREADING
extern FUNC_NORETURN void z_riscv_switch_to_main_no_multithreading(
	k_thread_entry_t main_func, void *p1, void *p2, void *p3);

#define ARCH_SWITCH_TO_MAIN_NO_MULTITHREADING \
	z_riscv_switch_to_main_no_multithreading

#endif /* !CONFIG_MULTITHREADING */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_FUNC_H_ */
