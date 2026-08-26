/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_pit_g1_timer

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>

#if defined(CONFIG_TICKLESS_KERNEL)
BUILD_ASSERT(0, "PIT driver for tickless kernel support DOES NOT implemented yet!");
#endif

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to a microchip,pit-g1-timer node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), microchip_pit_g1_timer),
	     "zephyr,system-timer must point to a microchip,pit-g1-timer compatible node");

#define NODE_SYSTICK    DT_CHOSEN(zephyr_system_timer)
#define TIMER_IRQ_NUM   DT_IRQN(NODE_SYSTICK)
#define TIMER_IRQ_PRIO  DT_IRQ(NODE_SYSTICK, priority)
#define TIMER_IRQ_FLAGS DT_IRQ(NODE_SYSTICK, flags)

#define CYCLES_PER_TICK		(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* PIV is 20 bits; the maximum period is all ones (counter runs PIV..0). */
#define PIT_MAX_PIV ((PIT_PIVR_CPIV_Msk >> PIT_PIVR_CPIV_Pos))

BUILD_ASSERT(CYCLES_PER_TICK > 0, "PIT CYCLES_PER_TICK must be greater than 0");
BUILD_ASSERT(CYCLES_PER_TICK <= PIT_MAX_PIV + 1,
	     "system tick period exceeds the maximum Periodic Interval Value");

/* Device constant configuration parameters */
struct mchp_pit_timer_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	struct sam_clk_cfg clock_cfg;
};

struct mchp_pit_timer_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	uint64_t accumulated_cycles;
	uint32_t piv;
};

#define DEV_CFG(_dev)  ((const struct mchp_pit_timer_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mchp_pit_timer_data *)(_dev)->data)

static const struct device *systick_timer_dev;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ_NUM;
#endif

static inline uint32_t mchp_pit_reg_read(uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(systick_timer_dev, reg_base) + reg);
}

static inline void mchp_pit_reg_write(uint32_t data, uint32_t reg, uint32_t mask)
{
	sys_write32((mchp_pit_reg_read(reg) & ~mask) | data,
		    DEVICE_MMIO_NAMED_GET(systick_timer_dev, reg_base) + reg);
}

static inline uint32_t timer_driver_cycle_get(void)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	uint32_t piir;

	piir = mchp_pit_reg_read(PIT_PIIR_REG_OFST);

	return data->accumulated_cycles + FIELD_GET(PIT_PIIR_CPIV_Msk, piir);
}

static inline void timer_driver_set_reload(uint32_t cycles)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	uint32_t piir = mchp_pit_reg_read(PIT_PIIR_REG_OFST);

	/* Fold the current period's elapsed count before reprogramming. */
	data->accumulated_cycles += FIELD_GET(PIT_PIIR_CPIV_Msk, piir);

	data->piv = cycles - 1;

	mchp_pit_reg_write(PIT_MR_PIV(data->piv), PIT_MR_REG_OFST, PIT_MR_PIV_Msk);
}

/*
 * A 20-bit reload counter, synthesized cycle count, immediate PIV reload.
 * The synthesized read touches state the ISR and set_reload also update, so
 * declare it non-atomic and let the core serialise under the clock lock.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 20
#define TIMER_CORE_COUNTER_NONATOMIC

#include "system_timer_generic.h"

static void mchp_pit_isr(const void *arg)
{
	ARG_UNUSED(arg);

	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	k_spinlock_key_t key;
	uint32_t pivr;

	/* If no pending event */
	if (FIELD_GET(PIT_SR_PITS_Msk, mchp_pit_reg_read(PIT_SR_REG_OFST)) == 0) {
		return;
	}

	key = sys_clock_lock();

	pivr = mchp_pit_reg_read(PIT_PIVR_REG_OFST);

	data->accumulated_cycles += FIELD_GET(PIT_PIVR_PICNT_Msk, pivr) * (data->piv + 1);

	timer_core_announce_from(key);
}

static int sys_clock_driver_init(void)
{
	const struct mchp_pit_timer_config *cfg;
	struct mchp_pit_timer_data *data;

	systick_timer_dev = DEVICE_DT_GET(NODE_SYSTICK);

	cfg  = systick_timer_dev->config;
	data = systick_timer_dev->data;

	/* Enable the PIT peripheral clock through the PMC before any register access. */
	(void)clock_control_on(DEVICE_DT_GET(DT_NODELABEL(pmc)),
			       (clock_control_subsys_t)&cfg->clock_cfg);

	data->accumulated_cycles = 0;
	data->piv = TIMER_CORE_CYC_PER_TICK - 1;

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

	/* Seed the announce baseline and arm the first tick/deadline. */
	timer_core_init();

	return 0;
}

#define MCHP_PIT_TIMER(n)									\
	static struct mchp_pit_timer_data mchp_pit_timer_data_##n;				\
	static const struct mchp_pit_timer_config mchp_pit_timer_config_##n = {			\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,							\
			      &mchp_pit_timer_data_##n,						\
			      &mchp_pit_timer_config_##n,					\
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MCHP_PIT_TIMER);

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
