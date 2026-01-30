/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#include "IfxStm_reg.h"

#define TIMER_BASE_ADDR DT_REG_ADDR_BY_IDX(DT_CHOSEN(infineon_system_timer), 0)

#define TICKS_PER_SEC   CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CYCLES_PER_SEC  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CYCLES_PER_TICK (CYCLES_PER_SEC / TICKS_PER_SEC)

/* the unsigned long cast limits divisions to native CPU register width */
#define cycle_diff_t   unsigned long
#define CYCLE_DIFF_MAX (~(cycle_diff_t)0)

/*
 * We have two constraints on the maximum number of cycles we can wait for.
 *
 * 1) sys_clock_announce() accepts at most INT32_MAX ticks.
 *
 * 2) The number of cycles between two reports must fit in a cycle_diff_t
 *    variable before converting it to ticks.
 *
 * Then:
 *
 * 3) Pick the smallest between (1) and (2).
 *
 * 4) Take into account some room for the unavoidable IRQ servicing latency.
 *    Let's use 3/4 of the max range.
 *
 * Finally let's add the LSB value to the result so to clear out a bunch of
 * consecutive set bits coming from the original max values to produce a
 * nicer literal for assembly generation.
 */
#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYCLES_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX   (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

static struct k_spinlock lock;
static uint64_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQ_BY_IDX(DT_CHOSEN(infineon_system_timer), 0, irq);
#endif

static uint32_t get_time32(void)
{

	return sys_read32(TIMER_BASE_ADDR + offsetof(Ifx_STM, TIM0));
}

static uint64_t get_time64(void)
{
	uint64_t ticks = sys_read32(TIMER_BASE_ADDR + offsetof(Ifx_STM, TIM0SV));

	ticks |= ((uint64_t)sys_read32(TIMER_BASE_ADDR + offsetof(Ifx_STM, TIM0SV) + 4) << 32);

	return ticks;
}

static void set_compare(uint32_t cmp)
{
	sys_write32(cmp, TIMER_BASE_ADDR + offsetof(Ifx_STM, CMP));
}

static void set_compare_irq(bool enabled)
{
	Ifx_STM_ICR icr = {.U = sys_read32(TIMER_BASE_ADDR + offsetof(Ifx_STM, ICR))};

	icr.B.CMP0EN = enabled;
	sys_write32(icr.U, TIMER_BASE_ADDR + offsetof(Ifx_STM, ICR));
}

static void sys_clock_isr(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	Ifx_STM_ISCR iscr = {.B.CMP0IRR = 1};

	sys_write32(iscr.U, TIMER_BASE_ADDR + offsetof(Ifx_STM, ISCR));

	uint64_t now = get_time64();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYCLES_PER_TICK;

	last_count += (cycle_diff_t)dticks * CYCLES_PER_TICK;
	last_ticks += dticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next = last_count + CYCLES_PER_TICK;

		/* Even though we use only the lower 32bits for compare an overflow is
		 * not possible as we make sure that the CYCLES_PER_TICK is smaller than 32 bits
		 */
		set_compare((uint32_t)next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(dticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t cyc;

	if (ticks == K_TICKS_FOREVER) {
		cyc = last_count + CYCLES_MAX;
	} else {
		cyc = (last_ticks + last_elapsed + ticks) * CYCLES_PER_TICK;
		if ((cyc - last_count) > CYCLES_MAX) {
			cyc = last_count + CYCLES_MAX;
		}
	}
	if (idle) {
		set_compare_irq(false);
	} else {
		set_compare_irq(true);
	}
	set_compare(cyc);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = get_time64();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYCLES_PER_TICK;

	last_elapsed = dticks;
	k_spin_unlock(&lock, key);
	return dticks;
}
uint32_t sys_clock_cycle_get_32(void)
{
	/* Return the current counter value */
	return get_time32();
}

uint64_t sys_clock_cycle_get_64(void)
{
	/* Return the current counter value */
	return get_time64();
}

void sys_clock_disable(void)
{
	set_compare_irq(false);
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(DT_IRQ_BY_IDX(DT_CHOSEN(infineon_system_timer), 0, irq),
		    DT_IRQ_BY_IDX(DT_CHOSEN(infineon_system_timer), 0, priority), sys_clock_isr,
		    NULL, 0);

	/* Initialise internal states */
	last_ticks = get_time64() / CYCLES_PER_TICK;
	last_count = last_ticks * CYCLES_PER_TICK;

	/* Set debug freeze if selected */
#if DT_PROP(DT_CHOSEN(infineon_system_timer), freeze)
	sys_write32(0x12000000, TIMER_BASE_ADDR + offsetof(Ifx_STM, OCS));
#endif
	/* Set compare window */
	Ifx_STM_CMCON cmcon = {.B.MSIZE0 = 31, .B.MSTART0 = 0};

	sys_write32(cmcon.U, TIMER_BASE_ADDR + offsetof(Ifx_STM, CMCON));

	/* Set irq line */
	Ifx_STM_ICR icr = {.B.CMP0OS = 0};

	sys_write32(icr.U, TIMER_BASE_ADDR + offsetof(Ifx_STM, ICR));

	/* Clear irq */
	Ifx_STM_ISCR iscr = {.B.CMP0IRR = 1};

	sys_write32(iscr.U, TIMER_BASE_ADDR + offsetof(Ifx_STM, ISCR));

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_compare((uint32_t)(last_count + CYCLES_PER_TICK));
		set_compare_irq(true);
	}
	irq_enable(DT_IRQ_BY_IDX(DT_CHOSEN(infineon_system_timer), 0, irq));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
