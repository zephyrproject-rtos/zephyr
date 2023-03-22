/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <da1469x_pdc.h>

#define COUNTER_SPAN BIT(24)
#define CYC_PER_TICK k_ticks_to_cyc_ceil32(1)
#define TICK_TO_CYC(tick) k_ticks_to_cyc_ceil32(tick)
#define CYC_TO_TICK(cyc) k_cyc_to_ticks_ceil32(cyc)
#define MAX_TICKS (((COUNTER_SPAN / 2) - CYC_PER_TICK) / (CYC_PER_TICK))

static uint32_t last_timer_val_reg;
static uint32_t timer_val_31_24;

static uint32_t last_isr_val;
static uint32_t last_isr_val_rounded;
static uint32_t announced_ticks;

static void set_reload(uint32_t val)
{
	TIMER2->TIMER2_RELOAD_REG = val & TIMER2_TIMER2_RELOAD_REG_TIM_RELOAD_Msk;
}

static uint32_t timer_val_32(void)
{
	uint32_t timer_val_reg;
	uint32_t val;

	timer_val_reg = TIMER2->TIMER2_TIMER_VAL_REG &
		TIMER2_TIMER2_TIMER_VAL_REG_TIM_TIMER_VALUE_Msk;
	if (timer_val_reg < last_timer_val_reg) {
		timer_val_31_24 += COUNTER_SPAN;
	}
	last_timer_val_reg = timer_val_reg;

	val = timer_val_31_24 + timer_val_reg;

	return val;
}

static uint32_t timer_val_32_noupdate(void)
{
	uint32_t timer_val_reg;
	uint32_t val;

	timer_val_reg = TIMER2->TIMER2_TIMER_VAL_REG &
		TIMER2_TIMER2_TIMER_VAL_REG_TIM_TIMER_VALUE_Msk;
	val = timer_val_31_24 + timer_val_reg;
	if (timer_val_reg < last_timer_val_reg) {
		val += COUNTER_SPAN;
	}

	return val;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	uint32_t target_val;
	uint32_t timer_val;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		/* FIXME we could disable timer here */
	}

	/*
	 * When Watchdog is NOT enabled but power management is, system
	 * starts watchdog before PD_SYS is powered off.
	 * Watchdog default reload value is 0x1FFF (~82s for RC32K and 172s for RCX).
	 * After this time watchdog will reset system if not woken up before.
	 * When Watchdog is not configured power management freezes watchdog
	 * as soon as system is awaken. Following code makes sure that
	 * system never goes to sleep for longer time that watchdog reload value.
	 */
	if (!IS_ENABLED(CONFIG_WDT_SMARTBOND) && IS_ENABLED(CONFIG_PM)) {
		uint32_t watchdog_expire_ticks;

		if (CRG_TOP->CLK_RCX_REG & CRG_TOP_CLK_RCX_REG_RCX_ENABLE_Msk) {
			watchdog_expire_ticks = SYS_WDOG->WATCHDOG_REG * 21 *
				CONFIG_SYS_CLOCK_TICKS_PER_SEC / 1000;
		} else {
			watchdog_expire_ticks = SYS_WDOG->WATCHDOG_REG *
				CONFIG_SYS_CLOCK_TICKS_PER_SEC / 100;
		}
		if (watchdog_expire_ticks - 2 < ticks) {
			ticks = watchdog_expire_ticks - 2;
		}
	}
	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	timer_val = timer_val_32_noupdate();

	/* Calculate target timer value and align to full tick */
	target_val = timer_val + TICK_TO_CYC(ticks);
	target_val = ((target_val + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;

	set_reload(target_val);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	return CYC_TO_TICK(timer_val_32_noupdate() - last_isr_val);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return timer_val_32_noupdate();
}

void sys_clock_idle_exit(void)
{
	TIMER2->TIMER2_CTRL_REG |= TIMER2_TIMER2_CTRL_REG_TIM_EN_Msk;
}

void sys_clock_disable(void)
{
	TIMER2->TIMER2_CTRL_REG &= ~TIMER2_TIMER2_CTRL_REG_TIM_EN_Msk;
}

static void timer2_isr(const void *arg)
{
	uint32_t val;
	int32_t delta;
	int32_t dticks;

	ARG_UNUSED(arg);

	TIMER2->TIMER2_CLEAR_IRQ_REG = 1;

	val = timer_val_32();
	delta = (int32_t)(val - last_isr_val_rounded);
	last_isr_val = val;
	dticks = CYC_TO_TICK(delta);
	last_isr_val_rounded += TICK_TO_CYC(dticks);
	announced_ticks += dticks;
	sys_clock_announce(dticks);
}

static int sys_clock_driver_init(void)
{
#if CONFIG_PM
	uint8_t pdc_idx;
	uint8_t en_xtal;

	en_xtal = DT_NODE_HAS_STATUS(DT_NODELABEL(xtal32m), okay) ? MCU_PDC_EN_XTAL : 0;

	/* Enable wakeup by TIMER2 */
	pdc_idx = da1469x_pdc_add(MCU_PDC_TRIGGER_TIMER2, MCU_PDC_MASTER_M33, en_xtal);
	__ASSERT_NO_MSG(pdc_idx >= 0);
	da1469x_pdc_set(pdc_idx);
	da1469x_pdc_ack(pdc_idx);
#endif

	TIMER2->TIMER2_CTRL_REG = 0;
	TIMER2->TIMER2_PRESCALER_REG = 0;
	TIMER2->TIMER2_CTRL_REG |= TIMER2_TIMER2_CTRL_REG_TIM_CLK_EN_Msk;
	TIMER2->TIMER2_CTRL_REG |= TIMER2_TIMER2_CTRL_REG_TIM_FREE_RUN_MODE_EN_Msk |
				   TIMER2_TIMER2_CTRL_REG_TIM_IRQ_EN_Msk |
				   TIMER2_TIMER2_CTRL_REG_TIM_EN_Msk;

	IRQ_CONNECT(TIMER2_IRQn, _IRQ_PRIO_OFFSET, timer2_isr, 0, 0);
	irq_enable(TIMER2_IRQn);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
