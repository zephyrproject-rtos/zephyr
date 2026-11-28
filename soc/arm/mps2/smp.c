/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <ipi.h>
#include <soc.h>

/* Secure system control */
#define SSE_200_SYSTEM_CTRL_S_BASE     0x50021000
#define SSE_200_SYSTEM_CTRL_INITSVTOR1 (SSE_200_SYSTEM_CTRL_S_BASE + 0x114)
#define SSE_200_SYSTEM_CTRL_CPU_WAIT   (SSE_200_SYSTEM_CTRL_S_BASE + 0x118)
#define SSE_200_CPU_ID_UNIT_BASE       0x5001F000

/* MHU 0 carries the scheduler IPI; each CPU gets its own interrupt from it */
#define MHU_BASE           DT_REG_ADDR(DT_NODELABEL(mhu0))
#define MHU_IRQ            DT_IRQN(DT_NODELABEL(mhu0))
#define MHU_INTR_STAT(cpu) (MHU_BASE + 0x00 + (cpu) * 0x10)
#define MHU_INTR_SET(cpu)  (MHU_BASE + 0x04 + (cpu) * 0x10)
#define MHU_INTR_CLR(cpu)  (MHU_BASE + 0x08 + (cpu) * 0x10)

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 2, "the SSE-200 has two Cortex-M33 cores");

/* Called from reset before any stack exists: r0-r3 only */
__attribute__((naked)) uint32_t soc_cpu_index(void)
{
	__asm__ volatile("ldr r0, =%c0\n\t"
			 "ldr r0, [r0]\n\t"
			 "bx lr\n\t"
			 ".ltorg\n\t"
			 :
			 : "i"(SSE_200_CPU_ID_UNIT_BASE));
}

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	if (cpuid != 1) {
		return -EINVAL;
	}

	sys_write32(entry_point, SSE_200_SYSTEM_CTRL_INITSVTOR1);
	barrier_dsync_fence_full();
	sys_write32(0, SSE_200_SYSTEM_CTRL_CPU_WAIT);

	return 0;
}

#ifdef CONFIG_SMP
static void ipi_isr(const void *arg)
{
	ARG_UNUSED(arg);

	sys_write32(1, MHU_INTR_CLR(soc_cpu_index()));
	z_sched_ipi();
}

static void ipi_send(uint32_t cpu_bitmap)
{
	uint32_t self = soc_cpu_index();

	for (uint32_t cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		if (cpu != self && (cpu_bitmap & BIT(cpu)) != 0U) {
			sys_write32(1, MHU_INTR_SET(cpu));
		}
	}
}

void arch_sched_broadcast_ipi(void)
{
	ipi_send(IPI_ALL_CPUS_MASK);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	ipi_send(cpu_bitmap);
}

int arch_smp_init(void)
{
	IRQ_CONNECT(MHU_IRQ, DT_IRQ(DT_NODELABEL(mhu0), priority), ipi_isr, NULL, 0);

	return 0;
}
#endif /* CONFIG_SMP */

void soc_per_core_init_hook(void)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		irq_enable(MHU_IRQ);
	}
}
