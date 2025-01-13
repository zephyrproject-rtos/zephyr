/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_itim_systick

/**
 * @file
 * @brief Nuvoton NPCM kernel device driver for  "system clock driver" interface
 *
 * This file contains a kernel device driver implemented by the internal
 * 32-bit timers in Nuvoton NPCM series.
 *
 */

#include <zephyr/init.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(npcm_itim_systick, LOG_LEVEL_ERR);

#define NPCM_ITIM32_MAX_CNT 0xffffffff
#define SYS_CYCLES_PER_TICK (sys_clock_hw_cycles_per_sec() \
					/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)

static struct itim32_reg *const sys_tmr = (struct itim32_reg *) DT_INST_REG_ADDR(0);

static const struct npcm_clk_cfg itim_clk_cfg = DT_INST_PHA(0, clocks, clk_cfg);

static struct k_spinlock lock;

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in npcm_itim32_sys_isr() and sys_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = sys_cycle_count + npcm_itim_elapsed();
 */
static uint64_t sys_cycle_count;

/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 *
 * Note:
 * Additions/subtractions/comparisons of 64-bits values on 32-bits systems
 * are very cheap. Divisions are not. Make sure the difference between
 * sys_cycle_count and sys_announced_cycles is stored in a 64-bit variable
 * before dividing it by SYS_CYCLES_PER_TICK.
 */
static uint64_t sys_announced_cycles;

/*
 * This local variable holds the amount of elapsed HW cycles due to
 * SysTick timer wraps ('overflows') and is used in the calculation
 * in npcm_itim_elapsed() function, as well as in the updates to
 * sys_cycle_count.
 *
 * Note:
 * Each time sys_cycle_count is updated with the value from
 * overflow_sys_cyc, the overflow_sys_cycs must be reset to zero.
 */
static volatile uint64_t overflow_sys_cycs;

/* Use to record last timeout value */
static uint64_t last_timeout_cycs;

/* ITIM local inline functions */
static inline uint64_t npcm_itim_elapsed(void)
{
	uint32_t val1 = sys_tmr->ITCNT32;
	uint8_t itsts = sys_tmr->ITCTS;
	uint32_t val2 = sys_tmr->ITCNT32;

	if (val1 == 0) {
		val1 = last_timeout_cycs;
	}

	if (val2 == 0) {
		val2 = last_timeout_cycs;
	}

	if ((itsts & BIT(NPCM_ITCTS_TO_STS))
			|| (val1 < val2)) {
		overflow_sys_cycs += last_timeout_cycs;

		/* Clear timeout event. Clear timeout event status may lead
		 * timer interrupt disappear, but next timer interrupt will
		 * add all overflow_sys_cycs value back to sys_cycle_count.
		 */
		sys_tmr->ITCTS |= BIT(NPCM_ITCTS_TO_STS);
	}

	return (last_timeout_cycs - val2) + overflow_sys_cycs;
}

static inline void npcm_itim_sys_enable(void)
{
	/* Enable sys timer and wait for it to take effect */
	sys_tmr->ITCTS |= BIT(NPCM_ITCTS_ITEN);

	/* Wait until sys timer enable */
	while (!IS_BIT_SET(sys_tmr->ITCTS, NPCM_ITCTS_ITEN));
}

static inline void npcm_itim_sys_disable(void)
{
	/* Disable sys timer and no need to wait for it to take effect */
	sys_tmr->ITCTS &= ~BIT(NPCM_ITCTS_ITEN);

	/* Wait until sys timer disable */
	while (IS_BIT_SET(sys_tmr->ITCTS, NPCM_ITCTS_ITEN));
}

/* ITIM local functions */
static int npcm_itim_start_sys_tmr_by_tick(int32_t ticks)
{
	k_spinlock_key_t key;
	uint64_t sys_cycs_timeout;
	uint64_t last_timeout_cycs_ = last_timeout_cycs;
	uint64_t next_cycs;
	uint64_t dcycles;
	uint64_t elapsed_cycles;
	uint32_t val1, val2;

	key = k_spin_lock(&lock);

	elapsed_cycles = npcm_itim_elapsed();
	sys_cycle_count += elapsed_cycles;
	overflow_sys_cycs = 0;

	val1 = sys_tmr->ITCNT32;

	/*
	 * Get desired cycles of sys timer from the requested ticks which
	 * round up to next tick boundary.
	 */
	if (ticks == K_TICKS_FOREVER) {
		sys_cycs_timeout = NPCM_ITIM32_MAX_CNT;
	}else {
		if (ticks <= 0) {
			ticks = 1;
		}

		/* Calculate next timeout cycles */
		next_cycs = (uint64_t)ticks;
		next_cycs = next_cycs * SYS_CYCLES_PER_TICK;
		next_cycs += (sys_announced_cycles + elapsed_cycles);

		if (unlikely(next_cycs <= sys_cycle_count)) {
			sys_cycs_timeout = 1;
		} else {
			dcycles = next_cycs - sys_cycle_count;
			sys_cycs_timeout = CLAMP(dcycles, 1, NPCM_ITIM32_MAX_CNT);
		}
	}

	val2 = sys_tmr->ITCNT32;

	last_timeout_cycs = sys_cycs_timeout;

	/* Add elapsed times */
	if ((val1 < val2)) {
		sys_cycle_count += (val1 + (last_timeout_cycs_ - val2));
	} else {
		sys_cycle_count += (val1 - val2);
	}

	/* Should we need another timer to record elapsed time from
	 * disable to enable sys timer?
	 */

	/* Disable timer and clear timeout if needed before configuring timer */
	if (IS_BIT_SET(sys_tmr->ITCTS, NPCM_ITCTS_ITEN)) {
		npcm_itim_sys_disable();

		if (sys_tmr->ITCTS & BIT(NPCM_ITCTS_TO_STS)) {
			sys_tmr->ITCTS |= BIT(NPCM_ITCTS_TO_STS);
		}
	}

	/* Upload counter of sys timer */
	sys_tmr->ITCNT32 = (sys_cycs_timeout - 1);

	k_spin_unlock(&lock, key);

	npcm_itim_sys_enable();

	return 0;
}

static void npcm_itim_sys_isr(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t delta_ticks;

	ARG_UNUSED(dev);

	/* Suppose NPCM_ITCTS_TO_STS bit clear by npcm_itim_elapsed()  */
	sys_cycle_count += npcm_itim_elapsed();
	overflow_sys_cycs = 0;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t delta_cycle;

		delta_cycle = sys_cycle_count - sys_announced_cycles;
		delta_ticks = delta_cycle / SYS_CYCLES_PER_TICK;
		sys_announced_cycles += (delta_ticks * SYS_CYCLES_PER_TICK);

	} else {
		/* Informs kernel one tick have elapsed */
		delta_ticks = 1;
	}

	k_spin_unlock(&lock, key);

	/* Informs kernel that specified number of ticks have elapsed */
	sys_clock_announce((uint32_t)delta_ticks);
}

/* System timer api functions */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Only for tickless kernel system */
		return;
	}

	LOG_DBG("timeout is %d idle = 0x%x\n", ticks, idle);
	/* Start a sys timer in ticks */
	npcm_itim_start_sys_tmr_by_tick(ticks);
}

uint32_t sys_clock_elapsed(void)
{
	uint64_t delta_cycle;
	uint64_t delta_ticks;
	k_spinlock_key_t key;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	key = k_spin_lock(&lock);

	delta_cycle = sys_cycle_count - sys_announced_cycles;
	delta_cycle += npcm_itim_elapsed();
	delta_ticks = (delta_cycle / SYS_CYCLES_PER_TICK);

	k_spin_unlock(&lock, key);

	/* Return how many ticks elapsed since last sys_clock_announce() call */
	return (uint32_t)delta_ticks;
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t current_cycs = sys_cycle_count;

	current_cycs += npcm_itim_elapsed();

	k_spin_unlock(&lock, key);

	/* Return how many cycles since system kernel timer start counting */
	return current_cycs;
}

uint32_t sys_clock_cycle_get_32(void)
{
	uint64_t current_cycs = sys_clock_cycle_get_64();

	/* Return how many cycles since system kernel timer start counting */
	return (uint32_t)(current_cycs);
}

static void npcm_itim_init_global_value(void)
{
	/* setup timeout cycles */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		last_timeout_cycs = SYS_CYCLES_PER_TICK;
        } else {
		last_timeout_cycs = NPCM_ITIM32_MAX_CNT;
	}

	/* intial current cycle count */
	sys_cycle_count = 0;

	/* initial announce to system cycle/ticks count */
	sys_announced_cycles = 0;

	/* initial overflow cycle count */
	overflow_sys_cycs = 0;
}

static int sys_clock_driver_init(void)
{
	int ret;
	uint32_t sys_tmr_rate;
	uint8_t itcts;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on itim module clock used for counting */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)
					itim_clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on timer %d clock failed.");
		return ret;
	}

	/*
	 * In npcm series, we use ITIM32 as system kernel timer. Its source
	 * clock frequency must equal to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC.
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
			itim_clk_cfg, &sys_tmr_rate);
	if (ret < 0) {
		LOG_ERR("Get ITIM clock rate failed %d", ret);
		return ret;
	}

	if (sys_tmr_rate != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LOG_ERR("CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC doesn't match "
			"ITIM clock frequency %d", sys_tmr_rate);
		return -EINVAL;
	}

	if (IS_BIT_SET(sys_tmr->ITCTS, NPCM_ITCTS_ITEN)) {
		npcm_itim_sys_disable();
	}

	/* No need div clock input source */
	sys_tmr->ITPRE = 0;

	/* Enable wakeup event and interrupt, clear status timeout event */
	itcts = BIT(NPCM_ITCTS_TO_WUE) | BIT(NPCM_ITCTS_TO_IE) |
		BIT(NPCM_ITCTS_TO_STS);

	/* Configure sys timer control and status */
	sys_tmr->ITCTS = itcts;

	/* Configure sys timer's ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
					npcm_itim_sys_isr, NULL, 0);
	/* Enable sys timer interrupt */
	irq_enable(DT_INST_IRQN(0));

	npcm_itim_init_global_value();

	/* Configure sys timer value */
	sys_tmr->ITCNT32 = (last_timeout_cycs - 1);

	/* Start sys timer */
	npcm_itim_sys_enable();

	return 0;
}
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
