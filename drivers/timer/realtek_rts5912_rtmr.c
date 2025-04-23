/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_rtmr

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>

#include <reg/reg_rtmr.h>
#include <reg/reg_system.h>

#define RTS5912_SCCON_REG_BASE ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Realtek RTOS timer is not supported multiple instances");

#define RTMR_REG ((RTOSTMR_Type *)DT_INST_REG_ADDR(0))

#define SLWTMR_REG ((RTOSTMR_Type *)(DT_REG_ADDR(DT_NODELABEL(slwtmr0))))

#define SSCON_REG ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

#define RTMR_COUNTER_MAX   0x0ffffffful
#define RTMR_COUNTER_MSK   0x0ffffffful
#define RTMR_TIMER_STOPPED 0xf0000000ul

#define MAX_TICKS ((k_ticks_t)(RTMR_COUNTER_MAX / CYCLES_PER_TICK) - 1)

/* Adjust cycle count programmed into timer for HW restart latency */
#define RTMR_ADJUST_LIMIT  8
#define RTMR_ADJUST_CYCLES 7

static struct k_spinlock lock;
static uint32_t accumulated_cycles;
static uint32_t previous_cnt;      /* Record the counter set into RTMR */
static uint32_t last_announcement; /* Record the last tick announced to system */

static void rtmr_restart(uint32_t counter)
{
	RTMR_REG->CTRL = 0ul;
	RTMR_REG->LDCNT = counter;
	RTMR_REG->CTRL = RTOSTMR_CTRL_INTEN_Msk | RTOSTMR_CTRL_EN_Msk;
}

static uint32_t rtmr_get_counter(void)
{
	uint32_t counter = RTMR_REG->CNT;

	if ((counter == 0) && (RTMR_REG->CTRL & RTOSTMR_CTRL_EN_Msk)) {
		counter = previous_cnt;
	}

	return counter;
}

static void rtmr_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t cycles;
	int32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	rtmr_restart(RTMR_COUNTER_MAX * CYCLES_PER_TICK);

	cycles = previous_cnt;
	previous_cnt = RTMR_COUNTER_MAX * CYCLES_PER_TICK;

	accumulated_cycles += cycles;

	if (accumulated_cycles > RTMR_COUNTER_MSK) {
		accumulated_cycles &= RTMR_COUNTER_MSK;
	}

	ticks = accumulated_cycles - last_announcement;
	ticks &= RTMR_COUNTER_MSK;
	ticks /= CYCLES_PER_TICK;

	last_announcement = accumulated_cycles;

	k_spin_unlock(&lock, key);

	sys_clock_announce(ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	uint32_t cur_cnt, temp;
	int full_ticks;
	uint32_t full_cycles;
	uint32_t partial_cycles;

	if (idle && (ticks == K_TICKS_FOREVER)) {
		RTMR_REG->CTRL = 0U;
		previous_cnt = RTMR_TIMER_STOPPED;
		return;
	}

	if (ticks < 1) {
		full_ticks = 0;
	} else if ((ticks == K_TICKS_FOREVER) || (ticks > MAX_TICKS)) {
		full_ticks = MAX_TICKS - 1;
	} else {
		full_ticks = ticks - 1;
	}

	full_cycles = full_ticks * CYCLES_PER_TICK;

	k_spinlock_key_t key = k_spin_lock(&lock);

	cur_cnt = rtmr_get_counter();

	RTMR_REG->CTRL = 0U;

	temp = accumulated_cycles;
	temp += previous_cnt - cur_cnt;
	temp &= RTMR_COUNTER_MSK;
	accumulated_cycles = temp;

	partial_cycles = CYCLES_PER_TICK - (accumulated_cycles % CYCLES_PER_TICK);
	previous_cnt = full_cycles + partial_cycles;
	/* adjust for up to one 32KHz cycle startup time */
	temp = previous_cnt;
	if (temp > RTMR_ADJUST_LIMIT) {
		temp -= RTMR_ADJUST_CYCLES;
	}
	rtmr_restart(temp);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t cur_cnt;
	uint32_t ticks;
	int32_t elapsed;

	k_spinlock_key_t key = k_spin_lock(&lock);

	cur_cnt = rtmr_get_counter();

	elapsed = (int32_t)accumulated_cycles - (int32_t)last_announcement;
	if (elapsed < 0) {
		elapsed = -1 * elapsed;
	}
	ticks = (uint32_t)elapsed;
	ticks += previous_cnt - cur_cnt;
	ticks /= CYCLES_PER_TICK;
	ticks &= RTMR_COUNTER_MSK;

	k_spin_unlock(&lock, key);

	return ticks;
}

void sys_clock_idle_exit(void)
{
	if (previous_cnt == RTMR_TIMER_STOPPED) {
		previous_cnt = CYCLES_PER_TICK;
		rtmr_restart(previous_cnt);
	}
}

void sys_clock_disable(void)
{
	/* Disable RTMR. */
	RTMR_REG->CTRL = 0ul;
}

uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t ret;
	uint32_t cur_cnt;

	k_spinlock_key_t key = k_spin_lock(&lock);

	cur_cnt = rtmr_get_counter();
	ret = (accumulated_cycles + (previous_cnt - cur_cnt)) & RTMR_COUNTER_MSK;

	k_spin_unlock(&lock, key);

	return ret;
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT

void arch_busy_wait(uint32_t n_usec)
{
	if (n_usec == 0) {
		return;
	}

	uint32_t start = SLWTMR_REG->CNT;

	for (;;) {
		uint32_t curr = SLWTMR_REG->CNT;

		if ((start - curr) >= n_usec) {
			break;
		}
	}
}
#endif

static int sys_clock_driver_init(void)
{
	/* Enable RTMR clock power */

	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;

	sys_reg->PERICLKPWR1 |= SYSTEM_PERICLKPWR1_RTMRCLKPWR_Msk;

	/* Enable RTMR interrupt. */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtmr_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Trigger RTMR and wait it start to counting */
	previous_cnt = RTMR_COUNTER_MAX;

	rtmr_restart(previous_cnt);
	while (RTMR_REG->CNT == 0) {
	};

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
	/* Enable SLWTMR0 clock power */
	SSCON_REG->PERICLKPWR1 |= BIT(SYSTEM_PERICLKPWR1_SLWTMR0CLKPWR_Pos);

	/* Enable SLWTMR0 */
	SLWTMR_REG->LDCNT = UINT32_MAX;
	SLWTMR_REG->CTRL = RTOSTMR_CTRL_MDSEL_Msk | RTOSTMR_CTRL_EN_Msk;
#endif

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
