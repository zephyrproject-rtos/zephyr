/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/tracing/tracing.h>

#include <ksched.h>
#include <kswap.h>
#include <microblaze/mb_interface.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static uint32_t emulated_irq_pending;

FUNC_NORETURN void z_irq_spurious(const void *param)
{
	LOG_ERR("Spurious interrupt detected!\n"
		"\tmsr: %x\n"
		"\tesr: %x\n"
		"\tear: %x\n"
		"\tedr: %x\n"
		"\tparam: %x\n",
		(unsigned int)mfmsr(), (unsigned int)mfesr(), (unsigned int)mfear(),
		(unsigned int)mfedr(), (unsigned int)param);

	z_microblaze_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
	CODE_UNREACHABLE;
}

/**
 * @brief Returns if IE bit is enabled
 * Defined weak so soc/boards can override it.
 */
__weak int arch_irq_is_enabled(unsigned int irq)
{
	ARG_UNUSED(irq);
	return mfmsr() & MSR_IE_MASK;
}

/**
 * @brief Simply unlocks all IRQs.
 * Defined weak so soc/boards can override it.
 */
__weak void arch_irq_enable(unsigned int irq)
{
	ARG_UNUSED(irq);
	arch_irq_unlock(1);
};

/**
 * @brief Simply locks all IRQs.
 * Defined weak so soc/boards can override it.
 */
__weak void arch_irq_disable(unsigned int irq)
{
	ARG_UNUSED(irq);
	arch_irq_lock();
};

/**
 * @brief Returns the currently pending interrupts.
 * This function should be overridden if an AXI interrupt
 * controller is placed inside the SoC.
 * Since there's no way to tell if a barebones MicroBlaze is
 * pending on an interrupt signal, this function will return 1 on first call,
 * and returns 0 on second call, which is enough to
 * make the _enter_irq break its while(ipending) loop.
 * This is a logically correct hack to make _enter_irq below work for
 * barebones MicroBlaze without introducing extra logic to the _enter_irq logic.
 * Obviously, this function shouldn't be used for generic purposes and is merely
 * a weak stub for soc/boards to override with more meaningful implementations.
 *
 * @return Pending IRQ bitmask. Pending IRQs will have their bitfield set to 1. 0 if no interrupt is
 * pending.
 */
__weak uint32_t arch_irq_pending(void)
{
	static uint32_t call_count;

	/* xor with 1 should simply invert the value */
	call_count ^= 1;
	return call_count;
};

__weak uint32_t arch_irq_pending_vector(uint32_t irq_pending)
{
	return find_lsb_set(irq_pending) - 1;
}

/**
 * @brief * Even in the presence of an interrupt controller, once the real mode
 * is enabled, there is no way to emulate hw interrupts. So this routine provides
 * a software "triggering" capability to MicroBlaze. This routine MUST be called
 * either with IRQs locked or interrupts disabled otherwise a real IRQ could fire.
 * Also see emulate_isr.h
 *
 * @param irq IRQ number to enable.
 * @return uint32_t returns the final emulated irq_pending mask.
 */
ALWAYS_INLINE uint32_t arch_irq_set_emulated_pending(uint32_t irq)
{
	return emulated_irq_pending |= BIT(irq);
}

/**
 * @brief Called by _interrupt_handler in isr.S
 */
void _enter_irq(void)
{
	_kernel.cpus[0].nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	z_irq_do_offload();
#endif

	uint32_t real_irq_pending = 0;

	while (true) {
		struct _isr_table_entry *ite;

		real_irq_pending = arch_irq_pending();

		if (real_irq_pending == 0 && emulated_irq_pending == 0) {
			break;
		}

#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_enter();
#endif

		/* From pg099 AXI Interrupt Controller (INTC) product guide:
		 * The least significant bit (LSB, in this case bit 0) has the highest priority.
		 */
		const uint32_t index = (real_irq_pending != 0)
			? arch_irq_pending_vector(real_irq_pending)
			: find_lsb_set(emulated_irq_pending) - 1;

		ite = &_sw_isr_table[index];

		ite->isr(ite->arg);

		/* In this implementation it's the ISR's responsibility to clear irq flags.
		 * But _enter_irq does clear the emulated IRQs automatically since this is a port
		 * provided functionality and also needed to pass unit tests without altering tests.
		 */
		emulated_irq_pending &= ~BIT(index);

#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_exit();
#endif
	}

	_kernel.cpus[0].nested--;
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
	void (*routine)(const void *parameter), const void *parameter,
	uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
