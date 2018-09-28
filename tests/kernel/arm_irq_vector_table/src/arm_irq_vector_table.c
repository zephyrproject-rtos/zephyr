/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

/**
 * @defgroup kernel_interrupt_tests Interrupts
 * @ingroup all_tests
 * @{
 */


/**
 * @brief Test installation of ISRs directly in the vector table
 *
 * @details Test validates the arm irq vector table. We create a
 * irq vector table with the address of the interrupt handler. We write
 * into the Software Trigger Interrupt Register(STIR) or calling
 * NVIC_SetPendingIRQ(), to trigger the pending interrupt. And we check
 * that the corresponding interrupt handler is getting called or not.
 *
 * @see irq_enable(), _irq_priority_set(), NVIC_SetPendingIRQ()
 *
 */
void test_arm_irq_vector_table(void)
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
		/* the QEMU does not simulate the
		 * STIR register: this is a workaround
		 */
		NVIC_SetPendingIRQ(ii);
#else
#if defined(CONFIG_SOC_SERIES_NRF52X)
		/* The customized solution for nRF52X-based platforms
		 * requires that the RTC1 IRQ line equals 17 and is larger
		 * than the CONFIG_NUM_IRQS.
		 */
		__ASSERT(RTC1_IRQn == 17,
			 "RTC1_IRQn != 17. Consider rework manual vector table.");
		__ASSERT(RTC1_IRQn >= CONFIG_NUM_IRQS,
			 "RTC1_IRQn < NUM_IRQs. Consider rework manual vector table.");
#endif          /* CONFIG_SOC_SERIES_NRF52X */
		NVIC->STIR = ii;
#endif
	}

	zassert_false((k_sem_take(&sem[0], K_NO_WAIT) ||
		       k_sem_take(&sem[1], K_NO_WAIT) ||
		       k_sem_take(&sem[2], K_NO_WAIT)), NULL);

}

#if defined(CONFIG_SOC_SERIES_NRF52X)
/* nRF52X-based platforms employ a Hardware RTC peripheral
 * to implement the Kernel system timer, instead of the ARM Cortex-M
 * SysTick. Therefore, a pointer to the timer ISR needs to be added in
 * the custom vector table to handle the timer "tick" interrupts.
 */
void rtc1_nrf5_isr(void);
typedef void (*vth)(void); /* Vector Table Handler */
vth __irq_vector_table _irq_vector_table[RTC1_IRQn + 1] = {
	isr0, isr1, isr2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	rtc1_nrf5_isr
};
#else
typedef void (*vth)(void); /* Vector Table Handler */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	isr0, isr1, isr2
};
#endif /* CONFIG_SOC_SERIES_NRF52X */

/**
 * @}
 */
