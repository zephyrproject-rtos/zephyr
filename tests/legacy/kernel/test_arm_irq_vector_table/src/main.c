/* main.c - test IRQs installed in vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
Set up three software IRQs: the ISR for each will print that it runs and then
release a semaphore. The task then verifies it can obtain all three
semaphores.

The ISRs are installed at build time, directly in the vector table.
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error project can only run on Cortex-M
#endif

#include <arch/cpu.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <tc_util.h>
#include <sections.h>

struct nano_sem sem[3];

/**
 *
 * @brief ISR for IRQ0
 *
 * @return N/A
 */

void isr0(void)
{
	printk("%s ran!\n", __func__);
	nano_isr_sem_give(&sem[0]);
	_IntExit();
}

/**
 *
 * @brief ISR for IRQ1
 *
 * @return N/A
 */

void isr1(void)
{
	printk("%s ran!\n", __func__);
	nano_isr_sem_give(&sem[1]);
	_IntExit();
}

/**
 *
 * @brief ISR for IRQ2
 *
 * @return N/A
 */

void isr2(void)
{
	printk("%s ran!\n", __func__);
	nano_isr_sem_give(&sem[2]);
	_IntExit();
}

/**
 *
 * @brief Task entry point
 *
 * @return N/A
 */

void main(void)
{
	TC_START("Test Cortex-M3 IRQ installed directly in vector table");

	for (int ii = 0; ii < 3; ii++) {
		irq_enable(ii);
		_irq_priority_set(ii, 0, 0);
		nano_sem_init(&sem[ii]);
	}

	int rv;
	rv = nano_task_sem_take(&sem[0], TICKS_NONE) ||
		 nano_task_sem_take(&sem[1], TICKS_NONE) ||
		 nano_task_sem_take(&sem[2], TICKS_NONE) ? TC_FAIL : TC_PASS;

	if (TC_FAIL == rv) {
		goto get_out;
	}

	for (int ii = 0; ii < 3; ii++) {
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
		/* the QEMU does not simulate the STIR register: this is a workaround */
		NVIC_SetPendingIRQ(ii);
#else
		NVIC->STIR = ii;
#endif
	}

	rv = nano_task_sem_take(&sem[0], TICKS_NONE) &&
		 nano_task_sem_take(&sem[1], TICKS_NONE) &&
		 nano_task_sem_take(&sem[2], TICKS_NONE) ? TC_PASS : TC_FAIL;

get_out:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

typedef void (*vth)(void); /* Vector Table Handler */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	isr0, isr1, isr2
	};
