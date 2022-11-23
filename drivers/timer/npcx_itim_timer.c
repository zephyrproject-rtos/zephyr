/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_itim_timer

/**
 * @file
 * @brief Nuvoton NPCX kernel device driver for  "system clock driver" interface
 *
 * This file contains a kernel device driver implemented by the internal
 * 64/32-bit timers in Nuvoton NPCX series. Via these two kinds of timers, the
 * driver provides an standard "system clock driver" interface.
 *
 * It includes:
 * - A system timer based on an ITIM64 (Internal 64-bit timer) instance, clocked
 *   by APB2 which freq is the same as CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC.
 * - Provide a 64-bit cycles reading and ticks computation based on it.
 * - Its prescaler is set to 1 and provide the kernel cycles reading without
 *   handling overflow mechanism.
 * - After ec entered "sleep/deep sleep" power state which is used for better
 *   power consumption, then its clock will stop.
 *
 * - A event timer based on an ITIM32 (Internal 32-bit timer) instance, clocked
 *   by LFCLK which frequency is 32KHz and still activated when ec entered
 *   "sleep/deep sleep" power state.
 * - Provide a system clock timeout notification. In its ISR, the driver informs
 *   the kernel that the specified number of ticks have elapsed.
 * - Its prescaler is set to 1 and the formula between event timer's cycles and
 *   ticks is 'cycles = (ticks * 32768) / CONFIG_SYS_CLOCK_TICKS_PER_SEC'
 * - Compensate reading of ITIM64 which clock is gating after ec entered
 *   "sleep/deep sleep" power state if CONFIG_PM is enabled.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(itim, LOG_LEVEL_ERR);

#define NPCX_ITIM32_MAX_CNT 0xffffffff
#define NPCX_ITIM64_MAX_HALF_CNT 0xffffffff
#define EVT_CYCLES_PER_SEC LFCLK /* 32768 Hz */
#define SYS_CYCLES_PER_TICK (sys_clock_hw_cycles_per_sec() \
					/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define SYS_CYCLES_PER_USEC (sys_clock_hw_cycles_per_sec() / 1000000)
#define EVT_CYCLES_FROM_TICKS(ticks) \
	DIV_ROUND_UP(ticks * EVT_CYCLES_PER_SEC, \
			 CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define NPCX_ITIM_CLK_SEL_DELAY 92 /* Delay for clock selection (Unit:us) */
/* Timeout for enabling ITIM module: 100us (Unit:cycles) */
#define NPCX_ITIM_EN_TIMEOUT_CYCLES (100 * SYS_CYCLES_PER_USEC)

/* Instance of system and event timers */
static struct itim64_reg *const sys_tmr = (struct itim64_reg *)
					DT_INST_REG_ADDR_BY_NAME(0, sys_itim);
static struct itim32_reg *const evt_tmr = (struct itim32_reg *)
					DT_INST_REG_ADDR_BY_NAME(0, evt_itim);

static const struct npcx_clk_cfg itim_clk_cfg[] = NPCX_DT_CLK_CFG_ITEMS_LIST(0);

static struct k_spinlock lock;
/* Announced cycles in system timer before executing sys_clock_announce() */
static uint64_t cyc_sys_announced;
/* Current target cycles of time-out signal in event timer */
static uint32_t cyc_evt_timeout;
/* Total cycles of system timer stopped in "sleep/deep sleep" mode */
__unused static uint64_t cyc_sys_compensated;
/* Current cycles in event timer when ec entered "sleep/deep sleep" mode */
__unused static uint32_t cyc_evt_enter_deep_idle;

/* ITIM local inline functions */
static inline uint64_t npcx_itim_get_sys_cyc64(void)
{
	uint32_t cnt64h, cnt64h_check, cnt64l;

	/* Read 64-bit counter value from two 32-bit registers */
	do {
		cnt64h_check = sys_tmr->ITCNT64H;
		cnt64l = sys_tmr->ITCNT64L;
		cnt64h = sys_tmr->ITCNT64H;
	} while (cnt64h != cnt64h_check);

	cnt64h = NPCX_ITIM64_MAX_HALF_CNT - cnt64h;
	cnt64l = NPCX_ITIM64_MAX_HALF_CNT - cnt64l + 1;

	/* Return current value of 64-bit counter value of system timer */
	if (IS_ENABLED(CONFIG_PM)) {
		return ((((uint64_t)cnt64h) << 32) | cnt64l) +
							cyc_sys_compensated;
	} else {
		return (((uint64_t)cnt64h) << 32) | cnt64l;
	}
}

static inline int npcx_itim_evt_enable(void)
{
	uint64_t cyc_start;

	/* Enable event timer and wait for it to take effect */
	evt_tmr->ITCTS32 |= BIT(NPCX_ITCTSXX_ITEN);

	/*
	 * Usually, it need one clock (30.5 us) to take effect since
	 * asynchronization between core and itim32's source clock (LFCLK).
	 */
	cyc_start = npcx_itim_get_sys_cyc64();
	while (!IS_BIT_SET(evt_tmr->ITCTS32, NPCX_ITCTSXX_ITEN)) {
		if (npcx_itim_get_sys_cyc64() - cyc_start >
						NPCX_ITIM_EN_TIMEOUT_CYCLES) {
			/* ITEN bit is still unset? */
			if (!IS_BIT_SET(evt_tmr->ITCTS32, NPCX_ITCTSXX_ITEN)) {
				LOG_ERR("Timeout: enabling EVT timer!");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static inline void npcx_itim_evt_disable(void)
{
	/* Disable event timer and no need to wait for it to take effect */
	evt_tmr->ITCTS32 &= ~BIT(NPCX_ITCTSXX_ITEN);
}

/* ITIM local functions */
static int npcx_itim_start_evt_tmr_by_tick(int32_t ticks)
{
	/*
	 * Get desired cycles of event timer from the requested ticks which
	 * round up to next tick boundary.
	 */
	if (ticks == K_TICKS_FOREVER) {
		cyc_evt_timeout = NPCX_ITIM32_MAX_CNT;
	} else {
		if (ticks <= 0) {
			ticks = 1;
		}
		cyc_evt_timeout = MIN(EVT_CYCLES_FROM_TICKS(ticks),
				      NPCX_ITIM32_MAX_CNT);
	}
	LOG_DBG("ticks %x, cyc_evt_timeout %x", ticks, cyc_evt_timeout);

	/* Disable event timer if needed before configuring counter */
	if (IS_BIT_SET(evt_tmr->ITCTS32, NPCX_ITCTSXX_ITEN)) {
		npcx_itim_evt_disable();
	}

	/* Upload counter of event timer */
	evt_tmr->ITCNT32 = MAX(cyc_evt_timeout - 1, 1);

	/* Enable event timer and start ticking */
	return npcx_itim_evt_enable();

}

static void npcx_itim_evt_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Disable ITIM event module first */
	npcx_itim_evt_disable();
	/* Clear timeout status for event */
	evt_tmr->ITCTS32 |= BIT(NPCX_ITCTSXX_TO_STS);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		k_spinlock_key_t key = k_spin_lock(&lock);
		uint32_t delta_ticks = (uint32_t)((npcx_itim_get_sys_cyc64() -
				cyc_sys_announced) / SYS_CYCLES_PER_TICK);

		/* Store announced cycles of system timer */
		cyc_sys_announced = npcx_itim_get_sys_cyc64();
		k_spin_unlock(&lock, key);

		/* Informs kernel that specified number of ticks have elapsed */
		sys_clock_announce(delta_ticks);
	} else {
		/* Enable event timer for ticking and wait to it take effect */
		npcx_itim_evt_enable();

		/* Informs kernel that one tick has elapsed */
		sys_clock_announce(1);
	}
}

#if defined(CONFIG_PM)
static inline uint32_t npcx_itim_get_evt_cyc32(void)
{
	uint32_t cnt1, cnt2;

	cnt1 = evt_tmr->ITCNT32;
	/*
	 * Wait for two consecutive equal values are read since the source clock
	 * of event timer is 32KHz.
	 */
	while ((cnt2 = evt_tmr->ITCNT32) != cnt1)
		cnt1 = cnt2;

	/* Return current value of 32-bit counter of event timer  */
	return cnt2;
}

static uint32_t npcx_itim_evt_elapsed_cyc32(void)
{
	uint32_t cnt1 = npcx_itim_get_evt_cyc32();
	uint8_t  sys_cts = evt_tmr->ITCTS32;
	uint16_t cnt2 = npcx_itim_get_evt_cyc32();

	/* Event has been triggered but timer ISR doesn't handle it */
	if (IS_BIT_SET(sys_cts, NPCX_ITCTSXX_TO_STS) || (cnt2 > cnt1)) {
		cnt2 = cyc_evt_timeout;
	} else {
		cnt2 = cyc_evt_timeout - cnt2;
	}

	/* Return elapsed cycles of 32-bit counter of event timer  */
	return cnt2;
}
#endif /* CONFIG_PM */

/* System timer api functions */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Only for tickless kernel system */
		return;
	}

	LOG_DBG("timeout is %d", ticks);
	/* Start a event timer in ticks */
	npcx_itim_start_evt_tmr_by_tick(ticks);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t delta_cycle = npcx_itim_get_sys_cyc64() - cyc_sys_announced;

	k_spin_unlock(&lock, key);

	/* Return how many ticks elapsed since last sys_clock_announce() call */
	return (uint32_t)(delta_cycle / SYS_CYCLES_PER_TICK);
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t current = npcx_itim_get_sys_cyc64();

	k_spin_unlock(&lock, key);

	/* Return how many cycles since system kernel timer start counting */
	return (uint32_t)(current);
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t current = npcx_itim_get_sys_cyc64();

	k_spin_unlock(&lock, key);

	/* Return how many cycles since system kernel timer start counting */
	return current;
}

/* Platform specific system timer functions */
#if defined(CONFIG_PM)
void npcx_clock_capture_low_freq_timer(void)
{
	cyc_evt_enter_deep_idle = npcx_itim_evt_elapsed_cyc32();
}

void npcx_clock_compensate_system_timer(void)
{
	uint32_t cyc_evt_elapsed_in_deep = npcx_itim_evt_elapsed_cyc32() -
							cyc_evt_enter_deep_idle;

	cyc_sys_compensated += ((uint64_t)cyc_evt_elapsed_in_deep *
			sys_clock_hw_cycles_per_sec()) / EVT_CYCLES_PER_SEC;
}

uint64_t npcx_clock_get_sleep_ticks(void)
{
	return  cyc_sys_compensated / SYS_CYCLES_PER_TICK;
}
#endif /* CONFIG_PM */

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;
	uint32_t sys_tmr_rate;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on all itim module clocks used for counting */
	for (int i = 0; i < ARRAY_SIZE(itim_clk_cfg); i++) {
		ret = clock_control_on(clk_dev, (clock_control_subsys_t)
				&itim_clk_cfg[i]);
		if (ret < 0) {
			LOG_ERR("Turn on timer %d clock failed.", i);
			return ret;
		}
	}

	/*
	 * In npcx series, we use ITIM64 as system kernel timer. Its source
	 * clock frequency must equal to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC.
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
			&itim_clk_cfg[1], &sys_tmr_rate);
	if (ret < 0) {
		LOG_ERR("Get ITIM64 clock rate failed %d", ret);
		return ret;
	}

	if (sys_tmr_rate != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LOG_ERR("CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC doesn't match "
			"ITIM64 clock frequency %d", sys_tmr_rate);
		return -EINVAL;
	}

	/*
	 * Step 1. Use a ITIM64 timer as system kernel timer for counting.
	 * Configure 64-bit timer counter and its prescaler to 1 first.
	 */
	sys_tmr->ITPRE64 = 0;
	sys_tmr->ITCNT64L = NPCX_ITIM64_MAX_HALF_CNT;
	sys_tmr->ITCNT64H = NPCX_ITIM64_MAX_HALF_CNT;
	/*
	 * Select APB2 clock which freq is CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	 * and clear timeout status bit before enabling the whole module.
	 */
	sys_tmr->ITCTS64 = BIT(NPCX_ITCTSXX_TO_STS);
	/* Enable 64-bit timer and start ticking */
	sys_tmr->ITCTS64 |= BIT(NPCX_ITCTSXX_ITEN);

	/*
	 * Step 2. Use a ITIM32 timer for event handling (ex. timeout event).
	 * Configure 32-bit timer's prescaler to 1 first.
	 */
	evt_tmr->ITPRE32 = 0;
	/*
	 * Select low frequency clock source (The freq is 32kHz), enable its
	 * interrupt/wake-up sources, and clear timeout status bit before
	 * enabling it.
	 */
	evt_tmr->ITCTS32 = BIT(NPCX_ITCTSXX_CKSEL) | BIT(NPCX_ITCTSXX_TO_WUE)
			 | BIT(NPCX_ITCTSXX_TO_IE) | BIT(NPCX_ITCTSXX_TO_STS);

	/* A delay for ITIM source clock selection */
	k_busy_wait(NPCX_ITIM_CLK_SEL_DELAY);

	/* Configure event timer's ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
					npcx_itim_evt_isr, NULL, 0);
	/* Enable event timer interrupt */
	irq_enable(DT_INST_IRQN(0));

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Start a event timer in one tick */
		ret = npcx_itim_start_evt_tmr_by_tick(1);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
