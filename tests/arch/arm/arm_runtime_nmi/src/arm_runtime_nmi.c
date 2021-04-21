/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <power/reboot.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <ztest.h>
#include <tc_util.h>

/* on v8m arch the nmi pend bit is renamed to pend nmi map it to old name */
#ifndef SCB_ICSR_NMIPENDSET_Msk
#define SCB_ICSR_NMIPENDSET_Msk SCB_ICSR_PENDNMISET_Msk
#endif

extern void z_NmiHandlerSet(void (*pHandler)(void));

static void nmi_test_isr(void)
{
	printk("NMI received (test_handler_isr)! Rebooting...\n");
	/* ISR triggered correctly: test passed! */
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}

/**
 * @brief Test the behavior of CONFIG_RUNTIME_NMI at runtime.
 * @addtogroup kernel_interrupt_tests
 * @ingroup all_tests
 * @{
 */


/**
 * @brief test the behavior of CONFIG_RUNTIME_NMI at run time
 *
 * @details this test is to validate z_NmiHandlerSet() api.
 * First we configure the NMI isr using z_NmiHandlerSet() api.
 * After wait for some time, and set the  Interrupt Control and
 * State Register(ICSR) of System control block (SCB).
 * The registered NMI isr should fire immediately.
 *
 * @see z_NmiHandlerSet()
 */
void test_arm_runtime_nmi(void)
{
	uint32_t i = 0U;

	TC_START("nmi_test_isr");
	/* Configure the NMI isr */
	z_NmiHandlerSet(nmi_test_isr);

	for (i = 0U; i < 10; i++) {
		printk("Trigger NMI in 10s: %d s\n", i);
		k_sleep(K_MSEC(1000));
	}

	/* Trigger NMI: Should fire immediately */
	SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
}
/**
 * @}
 */
