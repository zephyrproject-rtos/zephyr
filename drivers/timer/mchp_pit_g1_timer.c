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

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to a microchip,pit-g1-timer node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), microchip_pit_g1_timer),
	     "zephyr,system-timer must point to a microchip,pit-g1-timer compatible node");

#define NODE_SYSTICK    DT_CHOSEN(zephyr_system_timer)
#define TIMER_IRQ_NUM   DT_IRQN(NODE_SYSTICK)
#define TIMER_IRQ_PRIO  DT_IRQ(NODE_SYSTICK, priority)
#define TIMER_IRQ_FLAGS DT_IRQ(NODE_SYSTICK, flags)

#define CYCLES_PER_TICK		(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* PIV is 20 bits; the maximum period is all ones (counter counts up 0..PIV). */
#define PIT_MAX_PIV (PIT_PIVR_CPIV_Msk >> PIT_PIVR_CPIV_Pos)

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
	/* Cycle count at CPIV == 0 of the period CPIV is currently in. */
	uint32_t accumulated_cycles;
	/* PIV that every not-yet-folded period ran under, which is not the
	 * register's value across an arm that has not been confirmed yet.
	 */
	uint32_t piv;
};

#define DEV_CFG(_dev)  ((const struct mchp_pit_timer_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mchp_pit_timer_data *)(_dev)->data)

static const struct device *systick_timer_dev;
static bool mchp_pit_mmio_mapped;

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

#ifdef CONFIG_TICKLESS_KERNEL

/*
 * Consume the periods PICNT has counted, which by the invariant above all ran
 * under data->piv, and hand back the live CPIV. Clears PICNT and PITS, so a
 * match this swallows is left to the PITS test in the ISR.
 */
static inline uint32_t mchp_pit_fold(struct mchp_pit_timer_data *data)
{
	uint32_t pivr = mchp_pit_reg_read(PIT_PIVR_REG_OFST);

	/* PICNT is 12 bits and PIV 20, so the product stays inside 32. */
	data->accumulated_cycles += FIELD_GET(PIT_PIVR_PICNT_Msk, pivr) * (data->piv + 1);

	return FIELD_GET(PIT_PIVR_CPIV_Msk, pivr);
}

static inline uint32_t timer_driver_cycle_get(void)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	uint32_t piir;

	if (!mchp_pit_mmio_mapped) {
		return 0;
	}

	/* PIIR leaves PICNT alone, so the periods it reports are still to be
	 * folded and have to be added here.
	 */
	piir = mchp_pit_reg_read(PIT_PIIR_REG_OFST);

	return data->accumulated_cycles +
	       FIELD_GET(PIT_PIIR_PICNT_Msk, piir) * (data->piv + 1) +
	       FIELD_GET(PIT_PIIR_CPIV_Msk, piir);
}

/* Bound for the arming retry below. Reached only if the compare cannot be
 * placed ahead of the count, which the minimum arm is there to prevent.
 */
#define PIT_ARM_RETRY_MAX 8U
static void timer_driver_set_reload(uint32_t cycles)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;

	for (uint32_t retry = 0U; retry < PIT_ARM_RETRY_MAX; retry++) {
		uint32_t cpiv = mchp_pit_fold(data);
		uint32_t piv = MIN(cpiv + cycles, PIT_MAX_PIV);
		uint32_t piir;

		/*
		 * PIV is the count CPIV matches at and the write does not restart
		 * CPIV, so the delay has to be expressed from the current count.
		 * A PIV at or below CPIV never matches: CPIV runs to its 20-bit
		 * end, wraps and climbs back, and that period is accounted as one
		 * PIV, losing the wrap from the timebase for good.
		 */
		mchp_pit_reg_write(PIT_MR_PIV(piv), PIT_MR_REG_OFST, PIT_MR_PIV_Msk);
		piir = mchp_pit_reg_read(PIT_PIIR_REG_OFST);
		if ((FIELD_GET(PIT_PIIR_PICNT_Msk, piir) == 0U) &&
		    (FIELD_GET(PIT_PIIR_CPIV_Msk, piir) < piv)) {
			/* Ahead, and nothing pending ran under the old value. */
			data->piv = piv;
			return;
		}
		/* Either the write landed behind, which leaves a whole wrap to
		 * correct it in, or a period ended meanwhile and still belongs to
		 * data->piv, which the next fold accounts for.
		 */
	}

	__ASSERT(false, "PIT compare could not be placed ahead of the count");
}

/*
 * A 20-bit counter that resets to 0 whenever it reaches PIV, so PIV is a period
 * and this is a RELOAD backend. The count it presents is synthesized from
 * accumulated_cycles, PICNT and CPIV, which the ISR and the arm path both
 * write, hence non-atomic.
 *
 * TIMER_CORE_ALARM_MIN_CYCLES has to exceed the PIT_MR write plus the PIIR read
 * back, in MCK/16 cycles: below that the compare can land at or behind CPIV.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 32
#define TIMER_CORE_ALARM_MAX_CYCLES PIT_MAX_PIV
#define TIMER_CORE_ALARM_MIN_CYCLES 8U
#define TIMER_CORE_COUNTER_NONATOMIC

#include "system_timer_generic.h"

static void mchp_pit_isr(const void *arg)
{
	ARG_UNUSED(arg);

	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	k_spinlock_key_t key;

	/* If no pending event */
	if (FIELD_GET(PIT_SR_PITS_Msk, mchp_pit_reg_read(PIT_SR_REG_OFST)) == 0) {
		return;
	}

	key = sys_clock_lock();
	(void)mchp_pit_fold(data);
	timer_core_announce_from(key);
}

#else /* !CONFIG_TICKLESS_KERNEL */

static uint32_t mchp_pit_get_cycles(uint32_t reg)
{
	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	uint32_t piir;
	uint32_t cpiv;
	uint32_t picnt;

	piir = mchp_pit_reg_read(reg);

	cpiv = FIELD_GET(PIT_PIIR_CPIV_Msk, piir);
	picnt = FIELD_GET(PIT_PIIR_PICNT_Msk, piir);

	return data->accumulated_cycles + picnt * (data->piv + 1) + cpiv;
}

static void mchp_pit_isr(const void *arg)
{
	ARG_UNUSED(arg);

	struct mchp_pit_timer_data *data = systick_timer_dev->data;
	k_spinlock_key_t key;
	uint32_t elapsed_ticks;

	/* If no pending event */
	if (FIELD_GET(PIT_SR_PITS_Msk, mchp_pit_reg_read(PIT_SR_REG_OFST)) == 0) {
		return;
	}

	key = sys_clock_lock();

	elapsed_ticks = FIELD_GET(PIT_PIVR_PICNT_Msk, mchp_pit_reg_read(PIT_PIVR_REG_OFST));

	data->accumulated_cycles += elapsed_ticks * CYCLES_PER_TICK;

	sys_clock_announce_locked(elapsed_ticks, key);
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
	k_spinlock_key_t key = sys_clock_lock();
	uint32_t cycles = mchp_pit_get_cycles(PIT_PIIR_REG_OFST);

	sys_clock_unlock(key);
	return cycles;
}

#endif /* CONFIG_TICKLESS_KERNEL */

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
	data->piv = CYCLES_PER_TICK - 1;

	DEVICE_MMIO_NAMED_MAP(systick_timer_dev, reg_base, K_MEM_CACHE_NONE);
	mchp_pit_mmio_mapped = true;

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

#ifdef CONFIG_TICKLESS_KERNEL
	/* Seed the announce baseline and arm the first tick/deadline. */
	timer_core_init();
#endif /* CONFIG_TICKLESS_KERNEL */

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
