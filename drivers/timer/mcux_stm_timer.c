/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include <fsl_stm.h>

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,stm node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), nxp_stm),
	     "zephyr,system-timer must point to an nxp,stm compatible node");

/* Devicetree node selected as system timer via zephyr,system-timer chosen */
#define STM_NODE DT_CHOSEN(zephyr_system_timer)

#define STM_BASE ((STM_Type *)(DT_REG_ADDR(STM_NODE)))
#define STM_IRQN DT_IRQN(STM_NODE)
#define STM_IRQ_PRIORITY DT_IRQ(STM_NODE, priority)
#define STM_PRESCALER DT_PROP(STM_NODE, prescaler)

#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() / \
				    (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

BUILD_ASSERT(CYCLES_PER_TICK > 0, "CYCLES_PER_TICK must be > 0");

#define MAX_CYC INT_MAX
#define MAX_TICKS ((MAX_CYC - CYCLES_PER_TICK) / CYCLES_PER_TICK)

static struct k_spinlock lock;
static uint32_t last_count;
static uint32_t next_compare;
static bool wait_forever;

static void stm_set_compare(uint32_t compare)
{
	uint32_t now = STM_GetTimerCount(STM_BASE);
	uint32_t delta = compare - now;

	/*
	 * Keep compare in the future.
	 *
	 * - If delta == 0, compare is "now".
	 * - If compare is in the past, unsigned subtraction underflows and delta is
	 *   large (near UINT32_MAX).
	 *
	 * All valid compares are programmed within MAX_CYC cycles into the future,
	 * so any delta > MAX_CYC is treated as "in the past".
	 */
	if ((delta == 0U) || (delta > (uint32_t)MAX_CYC)) {
		/*
		 * Recover by programming the next tick boundary relative to last_count,
		 * to avoid introducing long-term drift from sub-tick phase shifts.
		 */
		uint32_t elapsed_cycles = now - last_count;
		uint32_t elapsed_ticks = elapsed_cycles / CYCLES_PER_TICK;

		compare = last_count + (elapsed_ticks + 1U) * CYCLES_PER_TICK;
	}

	next_compare = compare;
	/* STM_SetCompare also enables the compare channel. */
	STM_SetCompare(STM_BASE, kSTM_Channel_0, compare);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Tickful mode: periodic ticks are always enabled. */
		ARG_UNUSED(ticks);
		ARG_UNUSED(idle);
		return;
	}

	if (idle && (ticks == K_TICKS_FOREVER)) {
		k_spinlock_key_t key = k_spin_lock(&lock);

		wait_forever = true;
		STM_DisableCompareChannel(STM_BASE, kSTM_Channel_0);
		k_spin_unlock(&lock, key);
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	/*
	 * Convert to a delta in ticks from the current time, using the legacy
	 * convention where ticks==1 means "announce next tick".
	 */
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = STM_GetTimerCount(STM_BASE);
	uint32_t adj;
	uint32_t cyc = (uint32_t)ticks * CYCLES_PER_TICK;

	wait_forever = false;

	/* Round up to next tick boundary. */
	adj = (uint32_t)(now - last_count) + (CYCLES_PER_TICK - 1U);
	if (cyc <= (uint32_t)MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = (uint32_t)MAX_CYC;
	}
	cyc = (cyc / CYCLES_PER_TICK) * CYCLES_PER_TICK;

	stm_set_compare(last_count + cyc);
	k_spin_unlock(&lock, key);
}

void sys_clock_idle_exit(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!wait_forever) {
		k_spin_unlock(&lock, key);
		return;
	}

	wait_forever = false;

	uint32_t now = STM_GetTimerCount(STM_BASE);

	last_count = now;
	stm_set_compare(now + CYCLES_PER_TICK);
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = STM_GetTimerCount(STM_BASE);
	uint32_t elapsed = (now - last_count) / CYCLES_PER_TICK;

	k_spin_unlock(&lock, key);

	return elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return STM_GetTimerCount(STM_BASE);
}

void sys_clock_disable(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	STM_DisableCompareChannel(STM_BASE, kSTM_Channel_0);
	STM_StopTimer(STM_BASE);
	k_spin_unlock(&lock, key);
}

static void mcux_stm_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((STM_GetStatusFlags(STM_BASE, kSTM_Channel_0) & STM_CIR_CIF_MASK) != 0U) {
		STM_ClearStatusFlags(STM_BASE, kSTM_Channel_0);
	}

	uint32_t now = STM_GetTimerCount(STM_BASE);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Tickful mode: advance the time base to the last programmed compare
		 * value (not 'now') so late ISR servicing doesn’t accumulate drift.
		 * next_compare mirrors the compare register we programmed.
		 */
		last_count = next_compare;
		stm_set_compare(last_count + CYCLES_PER_TICK);
		k_spin_unlock(&lock, key);
		sys_clock_announce(1);
		return;
	}

	/* Tickless mode: announce elapsed ticks since last_count. */
	uint32_t elapsed_cycles = now - last_count;
	uint32_t elapsed_ticks = elapsed_cycles / CYCLES_PER_TICK;

	if (elapsed_ticks == 0U) {
		elapsed_ticks = 1U;
	}

	last_count += elapsed_ticks * CYCLES_PER_TICK;

	/*
	 * Always move the compare forward so it is not left in the past. The kernel
	 * will typically reprogram the compare via sys_clock_set_timeout() during
	 * sys_clock_announce().
	 */
	stm_set_compare(last_count + CYCLES_PER_TICK);

	k_spin_unlock(&lock, key);

	sys_clock_announce((int32_t)elapsed_ticks);
}

static int sys_clock_driver_init(void)
{
	stm_config_t config;

	STM_GetDefaultConfig(&config);
	config.enableIRQ = true;
	config.prescale = (uint8_t)STM_PRESCALER;

	STM_Init(STM_BASE, &config);
	STM_StartTimer(STM_BASE);

	last_count = STM_GetTimerCount(STM_BASE);
	stm_set_compare(last_count + CYCLES_PER_TICK);

	IRQ_CONNECT(STM_IRQN, STM_IRQ_PRIORITY, mcux_stm_timer_isr, NULL, 0);
	irq_enable(STM_IRQN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
