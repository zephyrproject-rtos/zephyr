/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/linker/sections.h>


/*
 * Offset (starting from the beginning of the vector table)
 * of the location where the ISRs will be manually installed.
 */
#define _ISR_OFFSET 0

#if defined(CONFIG_SOC_FAMILY_NRF)
#undef _ISR_OFFSET
#if defined(CONFIG_BOARD_QEMU_CORTEX_M0)
/* For the nRF51-based QEMU Cortex-M0 platform, the first set of consecutive
 * implemented interrupts that can be used by this test starts right after
 * the TIMER0 IRQ line, which is used by the system timer.
 */
#define _ISR_OFFSET (TIMER0_IRQn + 1)
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
/* For nRF54L Series, use SWI00-02 interrupt lines. */
#define _ISR_OFFSET SWI00_IRQn
#else
/* For other nRF targets, use TIMER0-2 interrupt lines. */
#define _ISR_OFFSET TIMER0_IRQn
#endif
#endif /* CONFIG_SOC_FAMILY_NRF */

struct k_sem sem[3];

/**
 *
 * @brief ISR for IRQ0
 *
 */

void isr0(void)
{
	printk("%s ran!\n", __func__);
	k_sem_give(&sem[0]);
	z_arm_int_exit();
}

/**
 *
 * @brief ISR for IRQ1
 *
 */

void isr1(void)
{
	printk("%s ran!\n", __func__);
	k_sem_give(&sem[1]);
	z_arm_int_exit();
}

/**
 *
 * @brief ISR for IRQ2
 *
 */

void isr2(void)
{
	printk("%s ran!\n", __func__);
	k_sem_give(&sem[2]);
	z_arm_int_exit();
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
 * @see irq_enable(), z_irq_priority_set(), NVIC_SetPendingIRQ()
 *
 */
ZTEST(vector_table, test_arm_irq_vector_table)
{
	printk("Test Cortex-M IRQs installed directly in the vector table\n");

	for (int ii = 0; ii < 3; ii++) {
		irq_enable(_ISR_OFFSET + ii);
		z_arm_irq_priority_set(_ISR_OFFSET + ii, 0, 0);
		k_sem_init(&sem[ii], 0, K_SEM_MAX_LIMIT);
	}

	zassert_true((k_sem_take(&sem[0], K_NO_WAIT) ||
		      k_sem_take(&sem[1], K_NO_WAIT) ||
		      k_sem_take(&sem[2], K_NO_WAIT)), NULL);

	for (int ii = 0; ii < 3; ii++) {
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || \
	defined(CONFIG_SOC_TI_LM3S6965_QEMU)
		/* the QEMU does not simulate the
		 * STIR register: this is a workaround
		 */
		NVIC_SetPendingIRQ(_ISR_OFFSET + ii);
#else
		NVIC->STIR = _ISR_OFFSET + ii;
#endif
	}

	zassert_false((k_sem_take(&sem[0], K_NO_WAIT) ||
		       k_sem_take(&sem[1], K_NO_WAIT) ||
		       k_sem_take(&sem[2], K_NO_WAIT)), NULL);

}

typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_SOC_FAMILY_NRF)
/* nRF5X- and nRF91X-based platforms employ a Hardware RTC peripheral
 * to implement the Kernel system timer, instead of the ARM Cortex-M
 * SysTick. Therefore, a pointer to the timer ISR needs to be added in
 * the custom vector table to handle the timer "tick" interrupts.
 *
 * The same applies to the CLOCK Control peripheral, which may trigger
 * IRQs that would need to be serviced.
 *
 * Note: qemu_cortex_m0 uses TIMER0 to implement system timer.
 */
void nrfx_power_clock_irq_handler(void);
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
#define POWER_CLOCK_IRQ_NUM	POWER_CLOCK_IRQn
#else
#define POWER_CLOCK_IRQ_NUM	CLOCK_POWER_IRQn
#endif

#if defined(CONFIG_BOARD_QEMU_CORTEX_M0)
void timer0_nrf_isr(void);
#define TIMER_IRQ_HANDLER	timer0_nrf_isr
#define TIMER_IRQ_NUM		TIMER0_IRQn
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
void nrfx_grtc_irq_handler(void);
#define TIMER_IRQ_HANDLER	nrfx_grtc_irq_handler
#define TIMER_IRQ_NUM		GRTC_0_IRQn
#else
void rtc_nrf_isr(void);
#define TIMER_IRQ_HANDLER	rtc_nrf_isr
#define TIMER_IRQ_NUM		RTC1_IRQn
#endif

#define IRQ_VECTOR_TABLE_SIZE (MAX(POWER_CLOCK_IRQ_NUM, MAX(TIMER_IRQ_NUM, _ISR_OFFSET + 2)) + 1)

vth __irq_vector_table _irq_vector_table[IRQ_VECTOR_TABLE_SIZE] = {
	[POWER_CLOCK_IRQ_NUM] = nrfx_power_clock_irq_handler,
	[TIMER_IRQ_NUM] = TIMER_IRQ_HANDLER,
	[_ISR_OFFSET] = isr0, isr1, isr2,
};
#elif defined(CONFIG_SOC_SERIES_CC13X2_CC26X2) || defined(CONFIG_SOC_SERIES_CC13X2X7_CC26X2X7)
/* TI CC13x2/CC26x2 based platforms also employ a Hardware RTC peripheral
 * to implement the Kernel system timer, instead of the ARM Cortex-M
 * SysTick. Therefore, a pointer to the timer ISR needs to be added in
 * the custom vector table to handle the timer "tick" interrupts.
 */
extern void rtc_isr(void);
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2, 0,
	rtc_isr
};
#elif defined(CONFIG_SOC_SERIES_IMX_RT6XX) || defined(CONFIG_SOC_SERIES_IMX_RT5XX) && \
	defined(CONFIG_MCUX_OS_TIMER)
/* MXRT685 employs a OS Event timer to implement the Kernel system
 * timer, instead of the ARM Cortex-M SysTick. Therefore, a pointer to
 * the timer ISR needs to be added in the custom vector table to handle
 * the timer "tick" interrupts.
 */
extern void mcux_lpc_ostick_isr(void);
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	mcux_lpc_ostick_isr
};
#elif defined(CONFIG_SOC_SERIES_IMX_RT) && defined(CONFIG_MCUX_GPT_TIMER)
/** MXRT parts employ a GPT timer peripheral to implement the Kernel system
 * timer, instead of the ARM Cortex-M Systick. Thereforce, a pointer to the
 * timer ISR need to be added in the custom vector table to handle
 * the timer "tick" interrupts.
 */
extern void mcux_imx_gpt_isr(void);
#if defined(CONFIG_SOC_MIMXRT1011)
/* RT1011 GPT timer interrupt is at offset 30 */
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, mcux_imx_gpt_isr
};
#elif defined(CONFIG_SOC_SERIES_IMX_RT10XX)
/* RT10xx GPT timer interrupt is at offset 100 */
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, mcux_imx_gpt_isr
};
#elif defined(CONFIG_SOC_SERIES_IMX_RT11XX)
/* RT11xx GPT timer interrupt is at offset 119 */
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	mcux_imx_gpt_isr
};
#else
#error "GPT timer enabled, but no known SOC selected. ISR table needs rework"
#endif
#else
vth __irq_vector_table _irq_vector_table[] = {
	isr0, isr1, isr2
};
#endif /* CONFIG_SOC_FAMILY_NRF */

/**
 * @}
 */
