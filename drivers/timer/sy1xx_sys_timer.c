/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#define DT_DRV_COMPAT sy1xx_sys_timer

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <soc.h>
#include <zephyr/irq.h>

#define SY1XX_SYS_TIMER_NODE      DT_NODELABEL(systick)
#define SY1XX_SYS_TIMER_BASE_ADDR DT_REG_ADDR(SY1XX_SYS_TIMER_NODE)

#define SY1XX_MINIMUM_ALLOWED_TICK 1000

#define REG_TIMER_CMP_LO_OFFS 0x10

/* config bits */
#define PLP_TIMER_ENABLE_BIT           0
#define PLP_TIMER_RESET_BIT            1
#define PLP_TIMER_IRQ_ENABLE_BIT       2
#define PLP_TIMER_IEM_BIT              3
#define PLP_TIMER_CMP_CLR_BIT          4
#define PLP_TIMER_ONE_SHOT_BIT         5
#define PLP_TIMER_PRESCALER_ENABLE_BIT 6
#define PLP_TIMER_CLOCK_SOURCE_BIT     7
#define PLP_TIMER_PRESCALER_VALUE_BIT  8
#define PLP_TIMER_PRESCALER_VALUE_BITS 8
#define PLP_TIMER_64_BIT               31

/* config flags */
#define PLP_TIMER_ACTIVE 1
#define PLP_TIMER_IDLE   0

#define PLP_TIMER_RESET_ENABLED  1
#define PLP_TIMER_RESET_DISABLED 0

#define PLP_TIMER_IRQ_ENABLED  1
#define PLP_TIMER_IRQ_DISABLED 0

#define PLP_TIMER_IEM_ENABLED  1
#define PLP_TIMER_IEM_DISABLED 0

#define PLP_TIMER_CMPCLR_ENABLED  1
#define PLP_TIMER_CMPCLR_DISABLED 0

#define PLP_TIMER_ONE_SHOT_ENABLED  1
#define PLP_TIMER_ONE_SHOT_DISABLED 0

#define PLP_TIMER_REFCLK_ENABLED  1
#define PLP_TIMER_REFCLK_DISABLED 0

#define PLP_TIMER_PRESCALER_ENABLED  1
#define PLP_TIMER_PRESCALER_DISABLED 0

#define PLP_TIMER_MODE_64_ENABLED  1
#define PLP_TIMER_MODE_64_DISABLED 0

static volatile uint32_t current_sys_clock;

struct timer_cfg {
	uint32_t tick_us;
};

static inline unsigned int timer_conf_prep(int enable, int reset, int irq_enable, int event_mask,
					   int cmp_clr, int one_shot, int clk_source,
					   int prescaler_enable, int prescaler, int mode_64)
{
	return (enable << PLP_TIMER_ENABLE_BIT) | (reset << PLP_TIMER_RESET_BIT) |
	       (irq_enable << PLP_TIMER_IRQ_ENABLE_BIT) | (event_mask << PLP_TIMER_IEM_BIT) |
	       (cmp_clr << PLP_TIMER_CMP_CLR_BIT) | (one_shot << PLP_TIMER_ONE_SHOT_BIT) |
	       (clk_source << PLP_TIMER_CLOCK_SOURCE_BIT) |
	       (prescaler_enable << PLP_TIMER_PRESCALER_ENABLE_BIT) |
	       (prescaler << PLP_TIMER_PRESCALER_VALUE_BIT) | (mode_64 << PLP_TIMER_64_BIT);
}

static void sy1xx_sys_timer_reload(uint32_t base, uint32_t reload_timer_ticks)
{
	sys_write32(reload_timer_ticks, (base + REG_TIMER_CMP_LO_OFFS));
}

static void sy1xx_sys_timer_cfg_auto_reload(uint32_t base)
{

	uint32_t conf =
		timer_conf_prep(PLP_TIMER_ACTIVE, PLP_TIMER_RESET_ENABLED, PLP_TIMER_IRQ_ENABLED,
				PLP_TIMER_IEM_DISABLED, PLP_TIMER_CMPCLR_ENABLED,
				PLP_TIMER_ONE_SHOT_DISABLED, PLP_TIMER_REFCLK_ENABLED,
				PLP_TIMER_PRESCALER_DISABLED, 0, PLP_TIMER_MODE_64_DISABLED);

	sys_write32(conf, base);
}

static void sy1xx_sys_timer_irq_enable(void)
{
	soc_enable_irq(DT_IRQN(SY1XX_SYS_TIMER_NODE));
}

static void sy1xx_sys_timer_irq_disable(void)
{
	soc_disable_irq(DT_IRQN(SY1XX_SYS_TIMER_NODE));
}

static int32_t sy1xx_sys_timer_config(uint32_t base, struct timer_cfg *cfg)
{

	/* global irq disable */
	uint32_t isr_state = arch_irq_lock();

	if (cfg->tick_us < SY1XX_MINIMUM_ALLOWED_TICK) {
		cfg->tick_us = SY1XX_MINIMUM_ALLOWED_TICK;
	}

	/* expect 1.0ms resolution => tick_us = 1000 */
	uint32_t us = cfg->tick_us;
	volatile double ticks_f =
		(((double)us / (double)1000000) * (double)soc_get_rts_clock_frequency()) + 1.0;

	volatile uint32_t timer_ticks = (uint32_t)ticks_f;

	printk("timer [%d] expected %u (%d)\n", soc_get_rts_clock_frequency(), cfg->tick_us,
	       timer_ticks);

	sy1xx_sys_timer_reload(base, timer_ticks);

	sy1xx_sys_timer_cfg_auto_reload(base);

	/* we always start timer irq disabled */
	sy1xx_sys_timer_irq_disable();

	/* restore global irq */
	arch_irq_unlock(isr_state);

	return 0;
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return current_sys_clock;
}

void sy1xx_sys_timer_callback(const void *user_data)
{
	current_sys_clock += 1;

	sys_clock_announce(1);
}

static int sy1xx_sys_timer_init(void)
{
	printk("starting sys_timer\n");

	struct timer_cfg timerCfg0 = {
		.tick_us = DT_PROP(SY1XX_SYS_TIMER_NODE, ticks_us),
	};

	sy1xx_sys_timer_config(SY1XX_SYS_TIMER_BASE_ADDR, &timerCfg0);

	uint32_t irq = arch_irq_lock();

	/* register interrupt routine with zephyr */
	irq_connect_dynamic(DT_IRQN(SY1XX_SYS_TIMER_NODE), 0, sy1xx_sys_timer_callback, NULL, 0);

	sy1xx_sys_timer_irq_enable();

	arch_irq_unlock(irq | 0x1);

	return 0;
}

SYS_INIT(sy1xx_sys_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
