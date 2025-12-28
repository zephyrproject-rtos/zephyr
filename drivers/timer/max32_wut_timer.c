/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_wut_timer

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

#include <wut.h>
#include <wrap_max32_lp.h>
#include <wrap_max32_sys.h>

#define WUT_NODE       DT_INST_PARENT(0)
#define WUT_REGS       ((mxc_wut_regs_t *)DT_REG_ADDR(WUT_NODE))
#define WUT_PRESCALER  DT_PROP(WUT_NODE, prescaler)
#define WUT_CLK_SRC    DT_PROP_OR(WUT_NODE, clock_source, ADI_MAX32_PRPH_CLK_SRC_ERTCO)

#define WUT_CLOCK_FREQ (32768 / WUT_PRESCALER)
#define CYC_PER_TICK   (WUT_CLOCK_FREQ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC        UINT32_MAX
#define MAX_TICKS      ((MAX_CYC / 2) / CYC_PER_TICK)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(WUT_NODE);
#endif

/*
 * Allow at least three timer cycles before next tick
 */
#define MIN_DELAY_CYC  3

BUILD_ASSERT(CYC_PER_TICK >= MIN_DELAY_CYC,
	"CONFIG_SYS_CLOCK_TICKS_PER_SEC is too high for WUT clock frequency");

static struct k_spinlock lock;

/* Counter value when last announcement was made */
static uint32_t last_count;

static void max32_wut_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	MXC_WUT_ClearFlags(WUT_REGS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr = MXC_WUT_GetCount(WUT_REGS);
	uint32_t delta_ticks = (curr - last_count) / CYC_PER_TICK;

	last_count += delta_ticks * CYC_PER_TICK;

#if !IS_ENABLED(CONFIG_TICKLESS_KERNEL)
	/* Set up next compare for non-tickless mode */
	uint32_t next_cyc = last_count + CYC_PER_TICK;

	if ((next_cyc - curr) < MIN_DELAY_CYC) {
		next_cyc += CYC_PER_TICK;
	}
	MXC_WUT_SetCompare(WUT_REGS, next_cyc);
#endif

	k_spin_unlock(&lock, key);

	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		ticks = MAX_TICKS;
	}

	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr = MXC_WUT_GetCount(WUT_REGS);
	uint32_t next;
	uint32_t adj, cyc = ticks * CYC_PER_TICK;

	adj = (curr - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	next = last_count + cyc;

	if ((next - curr) < MIN_DELAY_CYC) {
		next = curr + MIN_DELAY_CYC;
	}

	MXC_WUT_SetCompare(WUT_REGS, next);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr = MXC_WUT_GetCount(WUT_REGS);
	uint32_t delta_ticks = (curr - last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);

	return delta_ticks;
}

uint64_t sys_clock_cycle_get_64(void)
{
	uint32_t cnt = MXC_WUT_GetCount(WUT_REGS);

	return (uint64_t)cnt * sys_clock_hw_cycles_per_sec() / WUT_CLOCK_FREQ;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)sys_clock_cycle_get_64();
}

static void max32_wut_irq_config(void)
{
	IRQ_CONNECT(DT_IRQN(WUT_NODE), DT_IRQ(WUT_NODE, priority),
		    max32_wut_timer_isr, NULL, 0);
	irq_enable(DT_IRQN(WUT_NODE));
}

#ifdef CONFIG_PM_S2RAM
void sys_clock_idle_exit(void)
{
	if (!irq_is_enabled(DT_IRQN(WUT_NODE))) {
		max32_wut_irq_config();

		if (DT_NODE_HAS_PROP(WUT_NODE, wakeup_source)) {
			MXC_LP_EnableWUTAlarmWakeup();
		}
	}
}
#endif /* CONFIG_PM_S2RAM */

static int sys_clock_driver_init(void)
{
	uint8_t prescaler_lo, prescaler_hi;
	mxc_wut_pres_t pres;
	mxc_wut_cfg_t wut_cfg;

	/* Select 32kHz clock source */
	Wrap_MXC_SYS_Select32KClockSource(WUT_CLK_SRC);

	/* Calculate prescaler register values */
	prescaler_lo = FIELD_GET(GENMASK(2, 0), LOG2(WUT_PRESCALER));
	prescaler_hi = FIELD_GET(BIT(3), LOG2(WUT_PRESCALER));

	pres = (prescaler_hi << MXC_F_WUT_CTRL_PRES3_POS) |
	       (prescaler_lo << MXC_F_WUT_CTRL_PRES_POS);

	/* Initialize WUT peripheral */
	MXC_WUT_Init(WUT_REGS, pres);

	/* Configure for Compare mode */
	wut_cfg.mode = MXC_WUT_MODE_COMPARE;
#if IS_ENABLED(CONFIG_TICKLESS_KERNEL)
	wut_cfg.cmp_cnt = CYC_PER_TICK * MAX_TICKS;
#else
	wut_cfg.cmp_cnt = CYC_PER_TICK;
#endif
	MXC_WUT_Config(WUT_REGS, &wut_cfg);
	last_count = 0;
	MXC_WUT_SetCount(WUT_REGS, 0);

	/* Setup interrupt */
	max32_wut_irq_config();

	/* Enable as wakeup source if configured */
	if (DT_NODE_HAS_PROP(WUT_NODE, wakeup_source)) {
		MXC_LP_EnableWUTAlarmWakeup();
	}

	/* Start the timer */
	MXC_WUT_Enable(WUT_REGS);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
