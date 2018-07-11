/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/reboot.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <ztest.h>
#include <tc_util.h>

/* on v8m arch the nmi pend bit is renamed to pend nmi map it to old name */
#ifndef SCB_ICSR_NMIPENDSET_Msk
#define SCB_ICSR_NMIPENDSET_Msk SCB_ICSR_PENDNMISET_Msk
#endif

extern void _NmiHandlerSet(void (*pHandler)(void));

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
 * @details this test is to validate _NmiHandlerSet() api.
 * First we configure the NMI isr using _NmiHandlerSet() api.
 * After wait for some time, and set the  Interrupt Control and
 * State Register(ICSR) of System control block (SCB).
 * The registered NMI isr should fire immediately.
 *
 * @see _NmiHandlerSet()
 */
void test_arm_runtime_nmi(void)
{
	u32_t i = 0;

	TC_START("nmi_test_isr");
	/* Configure the NMI isr */
	_NmiHandlerSet(nmi_test_isr);

	for (i = 0; i < 10; i++) {
		printk("Trigger NMI in 10s: %d s\n", i);
		k_sleep(1000);
	}

	/* Trigger NMI: Should fire immediately */
	SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
}
/**
 * @}
 */
