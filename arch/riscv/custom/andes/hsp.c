/*
 * Copyright (c) 2026 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/irq.h>
#include <andes_csr.h>

/* HSP feature configuration in MMSC_CFG */
#define MMSC_CFG_HSP                 (1UL << 5)

/* Machine mode MHSP_CTL */
#define MHSP_CTL_OVF_EN              (1UL << 0)
#define MHSP_CTL_UDF_EN              (1UL << 1)
#define MHSP_CTL_SCHM_MASK           (3UL << 2)
#define MHSP_CTL_SCHM_TOS            (1UL << 2)
#define MHSP_CTL_SCHM_DETECT         (0   << 2)
#define MHSP_CTL_U_EN                (1UL << 3)
#define MHSP_CTL_S_EN                (1UL << 4)
#define MHSP_CTL_M_EN                (1UL << 5)

/* Machine Trap Cause exception code  */
#define TRAP_M_STACKOVF              32
#define TRAP_M_STACKUDF              33

/* Initialize the StackSafe extension for hardware stack protection */
static inline void hsp_init(void)
{
	unsigned long mmsc_cfg;

	mmsc_cfg = csr_read(NDS_MMSC_CFG);

	if (!(mmsc_cfg & MMSC_CFG_HSP)) {
		/* This CPU doesn't support StackSafe hardware stack protection */
		__ASSERT(0, "CPU doesn't support StackSafe extension. "
			    "Please disable CONFIG_RISCV_CUSTOM_CSR_ANDES_HSP\n");
		return;
	}

	/*
	 * Set operating scheme to stack overflow/underflow detection,
	 * and enable the stack protection in machine mode (and debug mode).
	 */
	csr_write(NDS_MHSP_CTL, MHSP_CTL_M_EN | MHSP_CTL_SCHM_DETECT);
}

/* Enable StackSafe stack overflow detection */
static inline void hsp_ovf_enable(void)
{
	csr_set(NDS_MHSP_CTL, MHSP_CTL_OVF_EN);
}

/* Disable StackSafe stack overflow detection */
static inline void hsp_ovf_disable(void)
{
	csr_clear(NDS_MHSP_CTL, MHSP_CTL_OVF_EN);
}

/* Set the StackSafe stack overflow bound */
static inline void hsp_ovf_bound_set(unsigned long bound)
{
	csr_write(NDS_MSP_BOUND, bound);
}

/* Check whether the current exception is a StackSafe stack overflow */
static inline bool hsp_is_stack_overflow(void)
{
	unsigned long mcause = csr_read(mcause);

	if (mcause & RISCV_MCAUSE_IRQ_BIT) {
		return false;
	}

	/* Check exception code for stack overflow */
	return (mcause & CONFIG_RISCV_MCAUSE_EXCEPTION_MASK) == TRAP_M_STACKOVF;
}

#ifdef CONFIG_CUSTOM_STACK_GUARD

/**
 * @brief Initialize custom stack guard.
 *
 * Performs the initial hardware setup required for the custom stack guard.
 */
void z_riscv_custom_stack_guard_init(void)
{
	unsigned long bound =
		(unsigned long)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[_current_cpu->id]);

	hsp_init();

	/* Set the stack guard for this CPU's interrupt stack */
	hsp_ovf_bound_set(bound);

	/* Enable stack overflow protection */
	hsp_ovf_enable();
}

/**
 * @brief Enable the custom stack guard.
 *
 * Configures and enables the custom stack guard for the stack associated
 * with the specified thread or interrupt context.
 *
 * @param thread Thread whose stack will be monitored by the stack guard.
 */
void z_riscv_custom_stack_guard_enable(struct k_thread *thread)
{
	unsigned long bound;

	if (arch_is_in_isr()) {
		/* In an ISR context, set the bound to the bottom of the interrupt stack */
		bound = (unsigned long)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[_current_cpu->id]);
	} else {
#ifdef CONFIG_MULTITHREADING
		/* Skip enabling the custom stack guard for dummy threads */
		if (thread->base.thread_state & _THREAD_DUMMY) {
			return;
		}

		/* Default: use the bottom of the thread stack */
		bound = thread->stack_info.start;

#ifdef CONFIG_USERSPACE
		/* For user threads, set bound to the bottom of the privileged stack */
		if (thread->base.user_options & K_USER) {
			bound = (unsigned long)K_KERNEL_STACK_BUFFER(
				(k_thread_stack_t *)thread->arch.priv_stack_start);
		}
#endif /* CONFIG_USERSPACE */
#else  /* !CONFIG_MULTITHREADING */
		bound = (unsigned long)K_KERNEL_STACK_BUFFER(z_main_stack);
#endif /* CONFIG_MULTITHREADING */
	}

	/* Set the stack limits to detect stack overflow */
	hsp_ovf_bound_set(bound);

	/* Enable stack overflow protection */
	hsp_ovf_enable();
}

/**
 * @brief Disable the custom stack guard.
 *
 * Turns off the custom stack guard.
 */
void z_riscv_custom_stack_guard_disable(void)
{
	hsp_ovf_disable();
}

/**
 * @brief Determine whether an exception was caused by a custom stack guard fault.
 *
 * Inspects the provided exception stack frame to identify whether
 * the trap was triggered by a custom stack guard violation.
 *
 * @param esf Pointer to the exception stack frame.
 *
 * @retval true  The fault was caused by a custom stack guard violation.
 * @retval false The fault was not related to the custom stack guard.
 */
bool z_riscv_custom_stack_guard_is_fault(struct arch_esf *esf)
{
	ARG_UNUSED(esf);
	return hsp_is_stack_overflow();
}

#endif /* CONFIG_CUSTOM_STACK_GUARD */
