/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <ipi.h>
#include <cy_device.h>
#include <cy_ipc_drv.h>
#include <cy_sysint.h>

/* CPUSS_IDENTITY.MS is 14 on M7_0 and 13 on M7_1 */
#define CPUSS_IDENTITY_ADDR 0x40200000
#define IDENTITY_MS_M7_0    14

#define M7_1_HWID DT_REG_ADDR(DT_NODELABEL(m7_1))

#define CM7_PWR_CTL_KEY 0x05FAU
#define CM7_PWR_MODE_ON 3U

#define IPI_STRUCT(cpu) (4U + (cpu))
#define IPI_MUX(cpu)    (4U + (cpu))
#define IPI_IRQ(cpu)    (IRQ_TO_L2(IPI_STRUCT(cpu)) | IPI_MUX(cpu))
#define IPI_NOTIFY      BIT(0)

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 2, "CYT4DN has two Cortex-M7 cores");

__attribute__((naked)) uint32_t soc_cpu_index(void)
{
	__asm__ volatile("ldr r0, =%c0\n\t"
			 "ldr r0, [r0]\n\t"
			 "ubfx r0, r0, #8, #4\n\t"
			 "rsb r0, r0, %1\n\t"
			 "bx lr\n\t"
			 ".ltorg\n\t"
			 :
			 : "i"(CPUSS_IDENTITY_ADDR), "i"(IDENTITY_MS_M7_0));
}

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	uint32_t ctl;

	if (cpuid != M7_1_HWID) {
		return -EINVAL;
	}

	CPUSS->CM7_1_VECTOR_TABLE_BASE = entry_point;

	ctl = CPUSS->CM7_1_PWR_CTL &
	      ~(CPUSS_CM7_1_PWR_CTL_VECTKEYSTAT_Msk | CPUSS_CM7_1_PWR_CTL_PWR_MODE_Msk);
	CPUSS->CM7_1_PWR_CTL = ctl | _VAL2FLD(CPUSS_CM7_1_PWR_CTL_VECTKEYSTAT, CM7_PWR_CTL_KEY) |
			       _VAL2FLD(CPUSS_CM7_1_PWR_CTL_PWR_MODE, CM7_PWR_MODE_ON);
	while ((CPUSS->CM7_1_STATUS & CPUSS_CM7_1_STATUS_PWR_DONE_Msk) == 0U) {
	}

	CPUSS->CM7_1_CTL &= ~CPUSS_CM7_1_CTL_CPU_WAIT_Msk;

	return 0;
}

#ifdef CONFIG_SMP
static void ipi_isr(const void *arg)
{
	ARG_UNUSED(arg);

	Cy_IPC_Drv_ClearInterrupt(Cy_IPC_Drv_GetIntrBaseAddr(IPI_STRUCT(soc_cpu_index())), 0U,
				  IPI_NOTIFY);
	z_sched_ipi();
}

static void ipi_send(uint32_t cpu_bitmap)
{
	uint32_t self = soc_cpu_index();

	for (uint32_t cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		if ((cpu != self) && ((cpu_bitmap & BIT(cpu)) != 0U)) {
			Cy_IPC_Drv_SetInterrupt(Cy_IPC_Drv_GetIntrBaseAddr(IPI_STRUCT(cpu)), 0U,
						IPI_NOTIFY);
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

#define MUX_PRIORITY_SET(node_id)                                                                  \
	arm_irq_priority_set(DT_IRQN(node_id), DT_IRQ(node_id, priority), 0)

void cyt4dn_smp_cpu_init(void)
{
	uint32_t cpu = soc_cpu_index();

	/* CPU 0 gets these from the sysintc driver */
	if (cpu != 0U) {
		DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_NODELABEL(cpuss_intc), MUX_PRIORITY_SET, (;));
	}

	Cy_IPC_Drv_SetInterruptMask(Cy_IPC_Drv_GetIntrBaseAddr(IPI_STRUCT(cpu)), 0U, IPI_NOTIFY);
	Cy_SysInt_SetInterruptSource((IRQn_Type)IPI_MUX(cpu), (cy_en_intr_t)IPI_STRUCT(cpu));
	arm_irq_enable(IPI_MUX(cpu));
}

int arch_smp_init(void)
{
	IRQ_CONNECT(IPI_IRQ(0), 0, ipi_isr, NULL, 0);
	IRQ_CONNECT(IPI_IRQ(1), 0, ipi_isr, NULL, 0);

	return 0;
}
#endif /* CONFIG_SMP */
