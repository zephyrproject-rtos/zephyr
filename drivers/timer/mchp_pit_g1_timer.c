/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_pit_g1_timer

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_TICKLESS_KERNEL)
BUILD_ASSERT(0, "PIT driver for tickless kernel support DOES NOT implemented yet!");
#endif

BUILD_ASSERT(DT_HAS_ALIAS(systick_timer), "No alias named systick_timer found in the device tree");
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(systick_timer), okay),
	     "systick_timer NOT enabled in the device tree");
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one enabled microchip,pit-g1-timer instance is required");

#define NODE_SYSTICK    DT_ALIAS(systick_timer)
#define TIMER_IRQ_NUM   DT_IRQN(NODE_SYSTICK)
#define TIMER_IRQ_PRIO  DT_IRQ(NODE_SYSTICK, priority)
#define TIMER_IRQ_FLAGS DT_IRQ(NODE_SYSTICK, flags)

#define SYSTICK_FREQ_HZ		DT_PROP(NODE_SYSTICK, clock_frequency)

#define CYCLES_PER_TICK		(SYSTICK_FREQ_HZ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_PERIOD_CYCLES	((PIT_PIVR_CPIV_Msk >> PIT_PIVR_CPIV_Pos) + 1)
#define MAX_TICKS		((k_ticks_t)(MAX_PERIOD_CYCLES / CYCLES_PER_TICK))

BUILD_ASSERT(SYSTICK_FREQ_HZ > 0, "PIT clock-frequency must be greater than 0");

BUILD_ASSERT((SYSTICK_FREQ_HZ % CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0,
	     "system tick clock-frequency must be divisible by CONFIG_SYS_CLOCK_TICKS_PER_SEC");

BUILD_ASSERT(CYCLES_PER_TICK > 0, "PIT CYCLES_PER_TICK must be greater than 0");

BUILD_ASSERT(CYCLES_PER_TICK <= MAX_PERIOD_CYCLES,
	     "system tick period exceeds the maximum Periodic Interval Value");

BUILD_ASSERT(MAX_TICKS > 0, "system tick MAX_TICKS must be greater than 0");

/* Device constant configuration parameters */
struct mchp_pit_timer_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
};

struct mchp_pit_timer_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct k_spinlock lock;
	uint64_t accumulated_cycles;
	uint64_t last_cycle;
	uint32_t piv;
};

#define DEV_CFG(_dev)  ((const struct mchp_pit_timer_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mchp_pit_timer_data *)(_dev)->data)

static const struct device *systick_timer_dev;

static inline uint32_t mchp_pit_reg_read(uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(systick_timer_dev) + reg);
}

static inline void mchp_pit_reg_write(uint32_t data, uint32_t reg, uint32_t mask)
{
	sys_write32((mchp_pit_reg_read(reg) & ~mask) | data,
		    DEVICE_MMIO_GET(systick_timer_dev) + reg);
}

static uint64_t mchp_pit_get_cycles(uint32_t reg, bool commit)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	k_spinlock_key_t key;
	uint64_t cycles;
	uint32_t piir;
	uint32_t cpiv;
	uint32_t picnt;

	key = k_spin_lock(&data->lock);

	piir = mchp_pit_reg_read(reg);

	cpiv = FIELD_GET(PIT_PIIR_CPIV_Msk, piir);
	picnt = FIELD_GET(PIT_PIIR_PICNT_Msk, piir);

	cycles = data->accumulated_cycles;
	cycles += (uint64_t)picnt * (data->piv + 1);

	if (commit) {
		data->accumulated_cycles = cycles;
	}

	cycles += cpiv;

	k_spin_unlock(&data->lock, key);

	return cycles;
}

static void mchp_pit_isr(const void *arg)
{
	ARG_UNUSED(arg);

	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	uint32_t elapsed_ticks;
	uint64_t curr_cycle;

	/* If no pending event */
	if (FIELD_GET(PIT_SR_PITS_Msk, mchp_pit_reg_read(PIT_SR_REG_OFST)) == 0) {
		return;
	}

	curr_cycle = mchp_pit_get_cycles(PIT_PIVR_REG_OFST, true);

	elapsed_ticks = (curr_cycle - data->last_cycle) / CYCLES_PER_TICK;

	data->last_cycle += (uint64_t)elapsed_ticks * CYCLES_PER_TICK;

	sys_clock_announce(elapsed_ticks);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
	/* do nothing for tickful kernel system */
}

uint32_t sys_clock_elapsed(void)
{
	/* Always return 0 for tickful kernel system */
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)mchp_pit_get_cycles(PIT_PIIR_REG_OFST, false);
}

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
uint64_t sys_clock_cycle_get_64(void)
{
	return mchp_pit_get_cycles(PIT_PIIR_REG_OFST, false);
}
#endif

static int sys_clock_driver_init(void)
{
	struct mchp_pit_timer_data *data;

	systick_timer_dev = DEVICE_DT_GET(NODE_SYSTICK);

	data = systick_timer_dev->data;

	data->accumulated_cycles = 0;
	data->last_cycle = 0;
	data->piv = CYCLES_PER_TICK - 1;

	DEVICE_MMIO_NAMED_MAP(systick_timer_dev, reg_base, K_MEM_CACHE_NONE);

	/* Read PIT_PIVR and clear PITS in PIT_SR */
	(void)mchp_pit_reg_read(PIT_PIVR_REG_OFST);

	/* Disable PIT */
	mchp_pit_reg_write(0, PIT_MR_REG_OFST, PIT_MR_PITIEN_Msk | PIT_MR_PITEN_Msk);

	/* IRQ initialize */
	IRQ_CONNECT(TIMER_IRQ_NUM, TIMER_IRQ_PRIO, mchp_pit_isr, NULL, TIMER_IRQ_FLAGS);
	irq_enable(TIMER_IRQ_NUM);

	/* Set Periodic Interval Value */
	mchp_pit_reg_write(PIT_MR_PIV(data->piv), PIT_MR_REG_OFST, PIT_MR_PIV_Msk);

	/* Enable Period Interval Timer Interrupt */
	mchp_pit_reg_write(PIT_MR_PITIEN_Msk, PIT_MR_REG_OFST, PIT_MR_PITIEN_Msk);

	/* Enable Period Interval Timer */
	mchp_pit_reg_write(PIT_MR_PITEN_Msk, PIT_MR_REG_OFST, PIT_MR_PITEN_Msk);

	return 0;
}

#define MCHP_PIT_TIMER(n)									\
	static struct mchp_pit_timer_data mchp_pit_timer_data_##n;				\
	static const struct mchp_pit_timer_config mchp_pit_timer_config_##n = {			\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,							\
			      &mchp_pit_timer_data_##n,						\
			      &mchp_pit_timer_config_##n,					\
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MCHP_PIT_TIMER);

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
