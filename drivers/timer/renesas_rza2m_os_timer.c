/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>

#define DT_DRV_COMPAT renesas_rza2m_ostm

DEVICE_MMIO_TOPLEVEL_STATIC(ostm_base, DT_DRV_INST(0));

/* The interrupt numbers in the device tree are interrupt IDs and need to be converted to SPI
 * interrupt numbers
 */
#define OSTM_IRQ_NUM (DT_INST_IRQN(0) - GIC_SPI_INT_BASE)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = OSTM_IRQ_NUM;
#endif

#define cycle_diff_t   uint32_t
#define CYCLE_DIFF_MAX (~(cycle_diff_t)0)

#define OSTM_REG_ADDR(off) ((mm_reg_t)(DEVICE_MMIO_TOPLEVEL_GET(ostm_base) + (off)))

#define OSTM_CMP_OFFSET 0x0 /* Compare register */
#define OSTM_CNT_OFFSET 0x4 /* Counter register */

#define OSTM_TE_OFFSET 0x10   /* Count enable status register */
#define OSTM_TE_ENABLE BIT(0) /* Timer enabled */

#define OSTM_TS_OFFSET 0x14   /* Count start trigger register */
#define OSTM_TS_START  BIT(0) /* Trigger start of the timer */

#define OSTM_TT_OFFSET 0x18   /* Count stop trigger register */
#define OSTM_TT_STOP   BIT(0) /* Trigger stop of the timer */

#define OSTM_CTL_OFFSET            0x20 /* Control register */
/*
 * Bit 0 of CTL controls enabling/disabling of OSTMnTINT interrupt requests when counting starts
 *    0: Disables the interrupts when counting starts
 *    1: Enables the interrupts when counting starts
 */
#define OSTM_CTL_TRIG_IRQ_ON_START 1
/*
 * Bit 1 of CTL specifies the operating mode for the counter
 *    0: Interval timer mode
 *    1: Free-running comparison mode
 */
#define OSTM_CTL_INTERVAL          0
#define OSTM_CTL_FREERUN           2

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
#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYC_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX_5 (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

/* Precompute CYCLES_MAX and CYC_PER_TICK at driver init to avoid runtime double divisions */
static uint64_t cycles_max;
static uint32_t cyc_per_tick;
#define CYCLES_MAX   cycles_max
#define CYC_PER_TICK cyc_per_tick

static struct k_spinlock lock;
static uint64_t last_cycle;
static uint64_t last_tick;
static uint32_t last_elapsed;
extern unsigned int z_clock_hw_cycles_per_sec;

static void ostm_irq_handler(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t delta_cycles = sys_clock_cycle_get_32() - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	last_cycle += (cycle_diff_t)delta_ticks * CYC_PER_TICK;
	last_tick += delta_ticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next_cycle = last_cycle + CYC_PER_TICK;

		sys_write32(next_cycle, OSTM_REG_ADDR(OSTM_CMP_OFFSET));
	} else {
		irq_disable(OSTM_IRQ_NUM);
	}

	/* Announce to the kernel */
	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (idle && ticks == K_TICKS_FOREVER) {
		return;
	}

	uint32_t next_cycle;

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (ticks == K_TICKS_FOREVER) {
		next_cycle = last_cycle + CYCLES_MAX;
	} else {
		next_cycle = (last_tick + last_elapsed + ticks) * CYC_PER_TICK;
		if ((next_cycle - last_cycle) > CYCLES_MAX) {
			next_cycle = last_cycle + CYCLES_MAX;
		}
	}

	sys_write32(next_cycle, OSTM_REG_ADDR(OSTM_CMP_OFFSET));
	irq_enable(OSTM_IRQ_NUM);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	uint32_t delta_cycles = sys_clock_cycle_get_32() - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	last_elapsed = delta_ticks;

	return delta_ticks;
}

void sys_clock_disable(void)
{
	if ((sys_read8(OSTM_REG_ADDR(OSTM_TE_OFFSET)) & OSTM_TE_ENABLE) != OSTM_TE_ENABLE) {
		return;
	}

	sys_write8(OSTM_TT_STOP, OSTM_REG_ADDR(OSTM_TT_OFFSET));
	while ((sys_read8(OSTM_REG_ADDR(OSTM_TE_OFFSET)) & OSTM_TE_ENABLE) == OSTM_TE_ENABLE) {
		;
	}
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ostm_cnt = sys_read32(OSTM_REG_ADDR(OSTM_CNT_OFFSET));

	k_spin_unlock(&lock, key);

	return ostm_cnt;
}

static int sys_clock_driver_init(void)
{
	int ret;
	const struct device *clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0));
	uint32_t clock_subsys = DT_INST_CLOCKS_CELL(0, clk_id);

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(clock_dev, (clock_control_subsys_t)&clock_subsys);

	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(clock_dev, (clock_control_subsys_t)&clock_subsys,
				     &z_clock_hw_cycles_per_sec);
	if (ret < 0) {
		return ret;
	}

	last_tick = 0;
	last_cycle = 0;
	cyc_per_tick = sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	cycles_max = CYCLES_MAX_5;

	DEVICE_MMIO_TOPLEVEL_MAP(ostm_base, K_MEM_CACHE_NONE);

	IRQ_CONNECT(OSTM_IRQ_NUM, DT_INST_IRQ(0, priority), ostm_irq_handler, NULL,
		    DT_INST_IRQ(0, flags));

	/* Restarting the timer will cause reset of CNT register in free-running mode */
	sys_clock_disable();

	sys_write32(cyc_per_tick, OSTM_REG_ADDR(OSTM_CMP_OFFSET));
	sys_write8(OSTM_CTL_FREERUN, OSTM_REG_ADDR(OSTM_CTL_OFFSET));
	sys_write8(OSTM_TS_START, OSTM_REG_ADDR(OSTM_TS_OFFSET));

	irq_enable(OSTM_IRQ_NUM);
	return 0;
}
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
