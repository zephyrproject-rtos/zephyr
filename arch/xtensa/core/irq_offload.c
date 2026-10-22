/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/zsr.h>
#include <zephyr/irq.h>

static struct {
	irq_offload_routine_t fn;
	const void *arg;
} offload_params[CONFIG_MP_MAX_NUM_CPUS];

static void irq_offload_isr(const void *param)
{
	ARG_UNUSED(param);
	uint8_t cpu_id = _current_cpu->id;

	offload_params[cpu_id].fn(offload_params[cpu_id].arg);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	IRQ_CONNECT(ZSR_IRQ_OFFLOAD_INT, 0, irq_offload_isr, NULL, 0);

	unsigned int intenable, key = arch_irq_lock();
	uint8_t cpu_id = _current_cpu->id;

	offload_params[cpu_id].fn = routine;
	offload_params[cpu_id].arg = parameter;

#if XCHAL_NUM_INTERRUPTS > 32
	switch ((ZSR_IRQ_OFFLOAD_INT) >> 5) {
	case 0:
		__asm__ volatile("rsr.intenable  %0" : "=r"(intenable));
		break;
	case 1:
		__asm__ volatile("rsr.intenable1 %0" : "=r"(intenable));
		break;
#if XCHAL_NUM_INTERRUPTS > 64
	case 2:
		__asm__ volatile("rsr.intenable2 %0" : "=r"(intenable));
		break;
#endif
#if XCHAL_NUM_INTERRUPTS > 96
	case 3:
		__asm__ volatile("rsr.intenable3 %0" : "=r"(intenable));
		break;
#endif
	default:
		break;
	}
#else
	__asm__ volatile("rsr.intenable  %0" : "=r"(intenable));
#endif

	intenable |= BIT((ZSR_IRQ_OFFLOAD_INT & 31U));

#if XCHAL_NUM_INTERRUPTS > 32
	switch ((ZSR_IRQ_OFFLOAD_INT) >> 5) {
	case 0:
		__asm__ volatile("wsr.intenable %0; wsr.intset %0; rsync"
				 :: "r"(intenable), "r"(BIT((ZSR_IRQ_OFFLOAD_INT & 31U))));
		break;
	case 1:
		__asm__ volatile("wsr.intenable1 %0; wsr.intset1 %0; rsync"
				 :: "r"(intenable), "r"(BIT((ZSR_IRQ_OFFLOAD_INT & 31U))));
		break;
#if XCHAL_NUM_INTERRUPTS > 64
	case 2:
		__asm__ volatile("wsr.intenable2 %0; wsr.intset2 %0; rsync"
				 :: "r"(intenable), "r"(BIT((ZSR_IRQ_OFFLOAD_INT & 31U))));
		break;
#endif
#if XCHAL_NUM_INTERRUPTS > 96
	case 3:
		__asm__ volatile("wsr.intenable3 %0; wsr.intset3 %0; rsync"
				 :: "r"(intenable), "r"(BIT((ZSR_IRQ_OFFLOAD_INT & 31U))));
		break;
#endif
	default:
		break;
	}
#else
	__asm__ volatile("wsr.intenable %0; wsr.intset %0; rsync"
			 :: "r"(intenable), "r"(BIT((ZSR_IRQ_OFFLOAD_INT & 31U))));
#endif

	arch_irq_unlock(key);
}

void arch_irq_offload_init(void)
{
}
