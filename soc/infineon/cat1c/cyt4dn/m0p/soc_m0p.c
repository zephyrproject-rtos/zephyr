/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CAT1C SOC.
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <cy_wdt.h>
#include <cy_sysclk.h>
#include <cy_sysint.h>

#define M7_0_FREQ DT_PROP(DT_NODELABEL(m7_0), clock_frequency)
#define M7_1_FREQ DT_PROP(DT_NODELABEL(m7_1), clock_frequency)

#define IFX_FAST_CLOCK_DOMAIN_FREQ_MHZ (MAX(M7_0_FREQ, M7_1_FREQ) / 1000000)

/* IRQ0 and IRQ1 entries of the SROM vector table */
#define IFX_SROM_IRQ0_VECTOR_ADDR 0x40U
#define IFX_SROM_IRQ1_VECTOR_ADDR 0x44U

extern const uintptr_t _irq_vector_table[];

/*
 * IPC interrupt structure 0 is routed to IRQ2. Pending IRQ0 from here makes
 * the SROM handler always preempt from handler mode on the main stack,
 * whatever state the CM0+ was in (TRM 41.1, case 2).
 */
void ifx_srom_syscall_isr(void)
{
	NVIC_SetPendingIRQ(NvicMux0_IRQn);
	(void)NVIC->ISPR[0];
	NVIC_ClearPendingIRQ(NvicMux0_IRQn);
	(void)NVIC->ICPR[0];
}

void soc_prep_hook(void)
{
	Cy_WDT_Unlock();
	Cy_WDT_Disable();
	SystemCoreClockUpdate();
	Cy_SysLib_SetWaitStates(false, IFX_FAST_CLOCK_DOMAIN_FREQ_MHZ);
}

void soc_early_init_hook(void)
{
	/* The SROM handlers are part of the boot ROM and may move with the silicon revision */
	if ((_irq_vector_table[0] != sys_read32(IFX_SROM_IRQ0_VECTOR_ADDR)) ||
	    (_irq_vector_table[1] != sys_read32(IFX_SROM_IRQ1_VECTOR_ADDR))) {
		k_fatal_halt(K_ERR_CPU_EXCEPTION);
	}

	/* IRQ2 must be lower than IRQ0 and IRQ1 for the trampoline to pend IRQ0 */
	NVIC_SetPriority(NvicMux0_IRQn, 0U);
	NVIC_SetPriority(NvicMux1_IRQn, 0U);
	NVIC_SetPriority(NvicMux2_IRQn, 1U);

	Cy_SysInt_SetInterruptSource(NvicMux2_IRQn, cpuss_interrupts_ipc_0_IRQn);

	NVIC_EnableIRQ(NvicMux0_IRQn);
	NVIC_EnableIRQ(NvicMux1_IRQn);
	NVIC_EnableIRQ(NvicMux2_IRQn);
}

void soc_late_init_hook(void)
{
#if CONFIG_SOC_SERIES_CYT4DN_START_M7_0
	Cy_SysEnableCM7(CORE_CM7_0, DT_REG_ADDR(DT_NODELABEL(m7_0_partition)));
#endif
#if CONFIG_SOC_SERIES_CYT4DN_START_M7_1
	Cy_SysEnableCM7(CORE_CM7_1, DT_REG_ADDR(DT_NODELABEL(m7_1_partition)));
#endif
}
