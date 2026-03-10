/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon XMC7200 SOC.
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <cy_sysclk.h>

__attribute__((section(".dtcm_bss"))) struct _isr_table_entry sys_int_table[CPUSS_SYSTEM_INT_NR];

void enable_sys_int(uint32_t int_num, uint32_t priority, void (*isr)(const void *), const void *arg)
{
	/* IRQ_PRIO_LOWEST = 6 */
	if (priority <= IRQ_PRIO_LOWEST) {
		Cy_SysInt_SetInterruptSource(priority, int_num);
	} else {
		Cy_SysInt_SetInterruptSource(IRQ_PRIO_LOWEST + 1, int_num);
	}

	if (int_num < CPUSS_SYSTEM_INT_NR) {
		sys_int_table[int_num].arg = arg;
		sys_int_table[int_num].isr = isr;
	} else {
		k_fatal_halt(K_ERR_CPU_EXCEPTION);
	}
}

__attribute__((section(".itcm"))) void sys_int_handler(uint32_t intrNum)
{
	uint32_t system_int_idx;

#ifdef CONFIG_SOC_XMC7200_CORE_NAME_M7_0
	if ((_FLD2VAL(CPUSS_CM7_0_INT_STATUS_SYSTEM_INT_VALID, CPUSS_CM7_0_INT_STATUS[intrNum]))) {
		system_int_idx = _FLD2VAL(CPUSS_CM7_0_INT_STATUS_SYSTEM_INT_IDX,
					  CPUSS_CM7_0_INT_STATUS[intrNum]);
		struct _isr_table_entry *entry = &sys_int_table[system_int_idx];
		(entry->isr)(entry->arg);
	}
#endif
#ifdef CONFIG_SOC_XMC7200_CORE_NAME_M7_1
	if ((_FLD2VAL(CPUSS_CM7_1_INT_STATUS_SYSTEM_INT_VALID, CPUSS_CM7_1_INT_STATUS[intrNum]))) {
		system_int_idx = _FLD2VAL(CPUSS_CM7_1_INT_STATUS_SYSTEM_INT_IDX,
					  CPUSS_CM7_1_INT_STATUS[intrNum]);
		struct _isr_table_entry *entry = &sys_int_table[system_int_idx];
		(entry->isr)(entry->arg);
	}
#endif
	NVIC_ClearPendingIRQ((IRQn_Type)intrNum);
}

void system_irq_init(void)
{
	/* Set system interrupt table defaults */
	for (uint32_t index = 0; index < CPUSS_SYSTEM_INT_NR; index++) {
		sys_int_table[index].arg = (const void *)0x0;
		sys_int_table[index].isr = z_irq_spurious;
	}

	/* Connect System Interrupts (IRQ0-IRQ7) to handler */
	/*          irq  priority  handler          arg  flags */
	IRQ_CONNECT(0, 0, sys_int_handler, 0, 0);
	IRQ_CONNECT(1, 1, sys_int_handler, 1, 0);
	IRQ_CONNECT(2, 2, sys_int_handler, 2, 0);
	IRQ_CONNECT(3, 3, sys_int_handler, 3, 0);
	IRQ_CONNECT(4, 4, sys_int_handler, 4, 0);
	IRQ_CONNECT(5, 5, sys_int_handler, 5, 0);
	IRQ_CONNECT(6, 6, sys_int_handler, 6, 0);
	/* Priority 0 is reserved for processor faults.  So, the priority number here
	 * is incremented by 1 in the code associated with IRQ_CONNECT.  Which means that
	 * can not select priority 7, because that gets converted to 8, and doesn't fit
	 * in the 3-bit priority encoding.
	 *
	 * We will use this for additional interrupts that have any priority lower than
	 * the lowest level.
	 */
	IRQ_CONNECT(7, IRQ_PRIO_LOWEST, sys_int_handler, 7, 0);

	/* Enable System Interrupts (IRQ0-IRQ7) */
	irq_enable(0);
	irq_enable(1);
	irq_enable(2);
	irq_enable(3);
	irq_enable(4);
	irq_enable(5);
	irq_enable(6);
	irq_enable(7);
}

void soc_prep_hook(void)
{
	/* disable global interrupt */
	__disable_irq();

	/* Allow write access to Vector Table Offset Register and ITCM/DTCM configuration register
	 * (CPUSS_CM7_X_CTL.PPB_LOCK[3] and CPUSS_CM7_X_CTL.PPB_LOCK[1:0])
	 */
#ifdef CONFIG_SOC_XMC7200_CORE_NAME_M7_1
	CPUSS->CM7_1_CTL &= ~(0xB);
#elif CONFIG_SOC_XMC7200_CORE_NAME_M7_0
	CPUSS->CM7_0_CTL &= ~(0xB);
#else
#error "Not valid"
#endif

	__DSB();
	__ISB();

	/* Enable ITCM and DTCM */
	SCB->ITCMCR = SCB->ITCMCR | 0x7; /* Set ITCMCR.EN, .RMW and .RETEN fields */
	SCB->DTCMCR = SCB->DTCMCR | 0x7; /* Set DTCMCR.EN, .RMW and .RETEN fields */

#ifdef CONFIG_SOC_XMC7200_CORE_NAME_M7_0
	CPUSS_CM7_0_CTL |= (0x1 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x2 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x1 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x2 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);
#elif CONFIG_SOC_XMC7200_CORE_NAME_M7_1
	CPUSS_CM7_1_CTL |= (0x1 << CPUSS_CM7_1_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x2 << CPUSS_CM7_1_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x1 << CPUSS_CM7_1_CTL_INIT_RMW_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x2 << CPUSS_CM7_1_CTL_INIT_RMW_EN_Pos);
#else
#error "Not valid"
#endif

	/* ITCMCR EN/RMW/RETEN enabled to access ITCM */
	__UNALIGNED_UINT32_WRITE(((void const *)0xE000EF90), 0x2F);
	/* DTCMCR EN/RMW/RETEN enabled to access DTCM */
	__UNALIGNED_UINT32_WRITE(((void const *)0xE000EF94), 0x2F);

	__DSB();
	__ISB();
}

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	system_irq_init();
}
