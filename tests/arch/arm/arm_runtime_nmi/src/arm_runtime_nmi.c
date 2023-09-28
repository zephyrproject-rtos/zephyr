/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/barrier.h>
#include <cmsis_core.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

/* on v8m arch the nmi pend bit is renamed to pend nmi map it to old name */
#ifndef SCB_ICSR_NMIPENDSET_Msk
#define SCB_ICSR_NMIPENDSET_Msk SCB_ICSR_PENDNMISET_Msk
#endif

static bool nmi_triggered;

static void nmi_test_isr(void)
{
	printk("NMI triggered (test_handler_isr)!\n");
	/* ISR triggered correctly: test passed! */
	nmi_triggered = true;
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
 * @details this test is to validate z_arm_nmi_set_handler() api.
 * First we configure the NMI isr using z_arm_nmi_set_handler() api.
 * After wait for some time, and set the  Interrupt Control and
 * State Register(ICSR) of System control block (SCB).
 * The registered NMI isr should fire immediately.
 *
 * @see z_arm_nmi_set_handler()
 */
ZTEST(arm_runtime_nmi_fn, test_arm_runtime_nmi)
{
	uint32_t i = 0U;

	/* Configure the NMI isr */
	z_arm_nmi_set_handler(nmi_test_isr);

	for (i = 0U; i < 2; i++) {
		printk("Trigger NMI in 2s: %d s\n", i);
		k_sleep(K_MSEC(1000));
	}

	/* Trigger NMI: Should fire immediately */
	SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

#ifdef ARM_CACHEL1_ARMV7_H
	/* Flush Data Cache now if enabled */
	if (IS_ENABLED(CONFIG_DCACHE)) {
		SCB_CleanDCache();
	}
#endif /* ARM_CACHEL1_ARMV7_H */
	zassert_true(nmi_triggered, "Isr not triggered!\n");
}
/**
 * @}
 */
