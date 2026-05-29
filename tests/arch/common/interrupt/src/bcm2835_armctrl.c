/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/ztest.h>

#define BCM2835_ARMCTRL_BASE DT_REG_ADDR(DT_NODELABEL(intc))

#define BCM2835_ARM_TIMER_LOAD    (BCM2835_ARMCTRL_BASE + 0x400)
#define BCM2835_ARM_TIMER_CONTROL (BCM2835_ARMCTRL_BASE + 0x408)
#define BCM2835_ARM_TIMER_IRQ_CLR (BCM2835_ARMCTRL_BASE + 0x40c)

#define BCM2835_ARM_TIMER_CTRL_23BIT BIT(1)
#define BCM2835_ARM_TIMER_CTRL_INTEN BIT(5)
#define BCM2835_ARM_TIMER_CTRL_ENABLE BIT(7)

#define BCM2835_ARMCTRL_ARM_TIMER_IRQ 64

static K_SEM_DEFINE(bcm2835_arm_timer_sem, 0, 1);

static void bcm2835_arm_timer_stop(void)
{
	sys_write32(0U, BCM2835_ARM_TIMER_CONTROL);
	sys_write32(1U, BCM2835_ARM_TIMER_IRQ_CLR);
	irq_disable(BCM2835_ARMCTRL_ARM_TIMER_IRQ);
}

static void bcm2835_arm_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	sys_write32(1U, BCM2835_ARM_TIMER_IRQ_CLR);
	k_sem_give(&bcm2835_arm_timer_sem);
}

ZTEST(interrupt_feature, test_bcm2835_armctrl_basic_arm_timer)
{
	uint32_t control;

	BUILD_ASSERT(BCM2835_ARMCTRL_ARM_TIMER_IRQ < CONFIG_NUM_IRQS,
		     "BCM2835 ARM timer IRQ must be in range");

	bcm2835_arm_timer_stop();
	k_sem_reset(&bcm2835_arm_timer_sem);

	IRQ_CONNECT(BCM2835_ARMCTRL_ARM_TIMER_IRQ, 0, bcm2835_arm_timer_isr, NULL, 0);
	irq_enable(BCM2835_ARMCTRL_ARM_TIMER_IRQ);

	sys_write32(1000U, BCM2835_ARM_TIMER_LOAD);
	control = BCM2835_ARM_TIMER_CTRL_23BIT |
		  BCM2835_ARM_TIMER_CTRL_INTEN |
		  BCM2835_ARM_TIMER_CTRL_ENABLE;
	sys_write32(control, BCM2835_ARM_TIMER_CONTROL);

	zassert_ok(k_sem_take(&bcm2835_arm_timer_sem, K_MSEC(100)),
		   "BCM2835 ARMCTRL basic ARM timer IRQ did not fire");

	bcm2835_arm_timer_stop();
}
