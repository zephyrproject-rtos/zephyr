/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys_clock.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>

#define DT_DRV_COMPAT renesas_rx_timer_cmt

#define CMT_NODE  DT_NODELABEL(cmt)
#define CMT0_NODE DT_NODELABEL(cmt0)
#define CMT1_NODE DT_NODELABEL(cmt1)

#define ICU_NODE DT_NODELABEL(icu)

#define ICU_IR_ADDR  DT_REG_ADDR_BY_NAME(ICU_NODE, IR)
#define ICU_IER_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, IER)
#define ICU_IPR_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, IPR)
#define ICU_FIR_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, FIR)
#define ICU_IR       ((volatile uint8_t *)ICU_IR_ADDR)

#define CMT0_IRQ_NUM DT_IRQ_BY_NAME(CMT0_NODE, cmi, irq)
#define CMT1_IRQ_NUM DT_IRQ_BY_NAME(CMT1_NODE, cmi, irq)

#define COUNTER_MAX 0x0000ffff

#define CYCLES_PER_SEC  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#define TICKS_PER_SEC   (CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define CYCLES_PER_TICK (CYCLES_PER_SEC / TICKS_PER_SEC)

#define CYCLES_CYCLE_TIMER (COUNTER_MAX + 1)
#define MSTPCR_ADDR        ((volatile uint32_t *)0x00080010) /* MSTPCR register address */
#define MSTP_CMT0_BIT      (1 << 15)                         /* Bit 15 controls MSTPA15 (CMT0) */

static const struct clock_control_rx_subsys_cfg cmt_clk_cfg = {
	.mstp = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, mstp),
	.stop_bit = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, stop_bit),
};

struct timer_rx_cfg {
	volatile uint16_t *cmstr;
	volatile uint16_t *cmcr;
	volatile uint16_t *cmcnt;
	volatile uint16_t *cmcor;
};

static const struct timer_rx_cfg tick_timer_cfg = {
	.cmstr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT_NODE, CMSTR0),
	.cmcr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCR),
	.cmcnt = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCNT),
	.cmcor = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCOR)};

static const struct timer_rx_cfg cycle_timer_cfg = {
	.cmstr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT_NODE, CMSTR0),
	.cmcr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCR),
	.cmcnt = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCNT),
	.cmcor = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCOR)};

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
typedef uint64_t cycle_t;
#define CYCLE_COUNT_MAX (0xffffffffffffffff)
#else
typedef uint32_t cycle_t;
#define CYCLE_COUNT_MAX (0xffffffff)
#endif
static cycle_t cycle_count;

static uint16_t clock_cycles_per_tick;

static volatile cycle_t announced_cycle_count;

static struct k_spinlock lock;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = CMT0_IRQ_NUM;
#endif

static cycle_t cmt1_elapsed(void)
{
	uint32_t val1 = (uint32_t)(*cycle_timer_cfg.cmcnt);
	uint8_t cmt_ir = ICU_IR[29];
	uint32_t val2 = (uint32_t)(*cycle_timer_cfg.cmcnt);

	if ((1 == cmt_ir) || (val1 > val2)) {
		cycle_count += CYCLES_CYCLE_TIMER;

		ICU_IR[29] = 0;
	}

	return (val2 + cycle_count);
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t ret = (uint32_t)cmt1_elapsed();

	k_spin_unlock(&lock, key);

	return ret;
}

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t ret = (uint64_t)cmt1_elapsed();

	k_spin_unlock(&lock, key);

	return ret;
}
#endif /* CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER */

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful operation */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t ret;
	cycle_t current_cycle_count;

	current_cycle_count = cmt1_elapsed();

	if (current_cycle_count < announced_cycle_count) {
		/* cycle_count overflowed */
		ret = (uint32_t)((current_cycle_count +
				  (CYCLE_COUNT_MAX - announced_cycle_count + 1)) /
				 CYCLES_PER_TICK);
	} else {
		ret = (uint32_t)((current_cycle_count - announced_cycle_count) / CYCLES_PER_TICK);
	}

	k_spin_unlock(&lock, key);

	return ret;
}

static void cmt0_isr(void)
{
	k_ticks_t dticks;
	cycle_t current_cycle_count;

	k_spinlock_key_t key = k_spin_lock(&lock);

	current_cycle_count = cmt1_elapsed();

	if (current_cycle_count < announced_cycle_count) {
		/* cycle_count overflowed */
		dticks = (k_ticks_t)((current_cycle_count +
				      (CYCLE_COUNT_MAX - announced_cycle_count + 1)) /
				     CYCLES_PER_TICK);
	} else {
		dticks = (k_ticks_t)((current_cycle_count - announced_cycle_count) /
				     CYCLES_PER_TICK);
	}

	announced_cycle_count = (current_cycle_count / CYCLES_PER_TICK) * CYCLES_PER_TICK;

	k_spin_unlock(&lock, key);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		sys_clock_announce(1);
	} else {
		sys_clock_announce(dticks);
	}
}

static int sys_clock_driver_init(void)
{
	const struct device *clk = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(0)));
	int ret;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&cmt_clk_cfg);
	if (ret < 0) {
		return ret;
	}

	*tick_timer_cfg.cmcr = 0x00C0;  /* enable CMT0 interrupt */
	*cycle_timer_cfg.cmcr = 0x00C0; /* enable CMT1 interrupt */

	clock_cycles_per_tick = (uint16_t)(CYCLES_PER_TICK);
	*tick_timer_cfg.cmcor = clock_cycles_per_tick - 1;
	*cycle_timer_cfg.cmcor = (uint16_t)COUNTER_MAX;

	IRQ_CONNECT(CMT0_IRQ_NUM, 0x01, cmt0_isr, NULL, 0);
	irq_enable(CMT0_IRQ_NUM);

	*tick_timer_cfg.cmstr = 0x0003; /* start cmt0,1 */

	return 0;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL) && idle && ticks == K_TICKS_FOREVER) {
		*tick_timer_cfg.cmstr = 0x0000; /* stop cmt0,1 */
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	/*
	 * By default, it is implemented as a no operation.
	 * Implement the processing as needed.
	 */
#endif /* CONFIG_TICKLESS_KERNEL */
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
