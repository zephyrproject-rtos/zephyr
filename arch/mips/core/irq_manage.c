/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kswap.h>
#include <tracing/tracing.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);


#include <soc.h>

uint32_t mips_cp0_status_int_mask;


FUNC_NORETURN void z_irq_spurious(const void *sp)
{
	uint32_t cause;

	cause = (mips_getcr() & SOC_CAUSE_EXP_MASK) >> 2;

	LOG_ERR("Spurious interrupt detected! CAUSE: %d\n", (int)cause);

	z_mips_fatal_error(K_ERR_CPU_EXCEPTION, sp);
}

void arch_irq_enable(unsigned int irq)
{
	unsigned int key;

	key = irq_lock();

	mips_cp0_status_int_mask |= (SR_SINT0 << irq);
	mips_bissr(SR_SINT0 << irq);

	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	unsigned int key;

	key = irq_lock();

	mips_cp0_status_int_mask &= ~(SR_SINT0 << irq);
	mips_bicsr(SR_SINT0 << irq);

	irq_unlock(key);
};

int arch_irq_is_enabled(unsigned int irq)
{
	return mips_getsr() & BIT(irq + CR_IP_SHIFT);
}

void z_mips_enter_irq(uint32_t ipending)
{
	int index;

	_current_cpu->nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	z_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_enter();
#endif

		index = find_lsb_set(ipending) - 1;
		ipending &= ~BIT(index);

		ite = &_sw_isr_table[index];

		ite->isr(ite->arg);
#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_exit();
#endif
	}

	_current_cpu->nested--;
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif
}
