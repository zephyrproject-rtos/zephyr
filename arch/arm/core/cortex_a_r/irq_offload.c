/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - ARM Cortex-A/R implementation
 *
 * On Cortex-A/R, irq_offload() is triggered via an svc that is dispatched by
 * z_arm_svc() (see switch.S / swap_helper.S), which invokes z_irq_do_offload()
 * in interrupt context on the initiating CPU.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>

/*
 * Per-CPU offload slot. Written in arch_irq_offload() and read back in
 * z_irq_do_offload() on the same CPU, with the caller pinned by arch_irq_lock().
 */
static struct {
	irq_offload_routine_t routine;
	const void *param;
} offload[CONFIG_MP_MAX_NUM_CPUS];

/* Called by z_arm_svc, in interrupt context on the initiating CPU. */
void z_irq_do_offload(void)
{
	uint8_t cpu_id = _current_cpu->id;

	offload[cpu_id].routine(offload[cpu_id].param);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	unsigned int key;
	uint8_t cpu_id;

	/*
	 * Use arch_irq_lock() to pin this context to its CPU across the slot
	 * write and the svc; it stays legal when called from interrupt context
	 * (CONFIG_IRQ_OFFLOAD_NESTED). The lock also guarantees the slot write
	 * and its consumption by the svc are atomic with respect to this CPU.
	 * Without it, an interrupt taken in this window whose ISR itself calls
	 * irq_offload() would overwrite the slot before our svc consumes it.
	 */
	key = arch_irq_lock();
	cpu_id = _current_cpu->id;

	offload[cpu_id].routine = routine;
	offload[cpu_id].param = parameter;

	/*
	 * Clobber "lr": on the !CONFIG_USE_SWITCH path ISRs run in SVC mode, so
	 * a nested irq_offload() (CONFIG_IRQ_OFFLOAD_NESTED) runs this function
	 * in SVC mode with its return address in lr_svc, which the svc then
	 * overwrites. Without the clobber the compiler assumes lr survived and
	 * this leaf function returns into itself right after the svc, hanging.
	 */
	__asm__ volatile ("svc %[id]"
			  :
			  : [id] "i" (_SVC_CALL_IRQ_OFFLOAD)
			  : "memory", "lr");

	arch_irq_unlock(key);
}

void arch_irq_offload_init(void)
{
}
