/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 *
 * Set up three software IRQs: the ISR for each will print that it runs and
 * then release a semaphore. The task then verifies it can obtain all three
 * semaphores.
 *
 * The ISRs are installed at build time, directly in the vector table.
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error project can only run on Cortex-M
#endif

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <linker/sections.h>


struct k_sem sem[3];

/**
 *
 * @brief ISR for IRQ0
 *
 * @return N/A
 */

void isr0(void)
{
	printk("%s ran!\n", __func__);
	k_sem_give(&sem[0]);
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
	k_sem_give(&sem[1]);
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
	k_sem_give(&sem[2]);
	_IntExit();
}

void test_irq_vector_table(void)
{
	printk("Test Cortex-M3 IRQ installed directly in vector table\n");

	for (int ii = 0; ii < 3; ii++) {
		irq_enable(ii);
		_irq_priority_set(ii, 0, 0);
		k_sem_init(&sem[ii], 0, UINT_MAX);
	}

	zassert_true((k_sem_take(&sem[0], K_NO_WAIT) ||
		      k_sem_take(&sem[1], K_NO_WAIT) ||
		      k_sem_take(&sem[2], K_NO_WAIT)), NULL);

	for (int ii = 0; ii < 3; ii++) {
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
		/* the QEMU does not simulate the STIR register: this is a workaround */
		NVIC_SetPendingIRQ(ii);
#else
		NVIC->STIR = ii;
#endif
	}

	zassert_false((k_sem_take(&sem[0], K_NO_WAIT) ||
		       k_sem_take(&sem[1], K_NO_WAIT) ||
		       k_sem_take(&sem[2], K_NO_WAIT)), NULL);

}

void test_main(void)
{
	ztest_test_suite(vector_table_test,
			 ztest_unit_test(test_irq_vector_table)
			 );

	ztest_run_test_suite(vector_table_test);
}


typedef void (*vth)(void); /* Vector Table Handler */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	isr0, isr1, isr2
};
