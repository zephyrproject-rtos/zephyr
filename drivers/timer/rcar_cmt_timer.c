/*
 * Copyright (c) 2020 IoT.bzh <julien.massot@iot.bzh>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>

#define DT_DRV_COMPAT renesas_rcar_cmt

#define TIMER_IRQ              DT_INST_IRQN(0)
#define TIMER_BASE_ADDR        DT_INST_REG_ADDR(0)
#define TIMER_CLOCK_FREQUENCY  DT_INST_PROP(0, clock_frequency)

#define CLOCK_SUBSYS           DT_INST_CLOCKS_CELL(0, module)

#define CYCLES_PER_SEC         TIMER_CLOCK_FREQUENCY
#define CYCLES_PER_TICK        (CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_INST(0, renesas_rcar_cmt));
#endif
static struct rcar_cpg_clk mod_clk = {
	.module = DT_INST_CLOCKS_CELL(0, module),
	.domain = DT_INST_CLOCKS_CELL(0, domain),
};

BUILD_ASSERT(CYCLES_PER_TICK > 1,
	     "CYCLES_PER_TICK must be greater than 1");

#define CMCOR0_OFFSET                   0x018   /* constant register 0 */
#define CMCNT0_OFFSET                   0x014   /* counter 0 */
#define CMCSR0_OFFSET                   0x010   /* control/status register 0 */

#define CMCOR1_OFFSET                   0x118   /* constant register 1 */
#define CMCNT1_OFFSET                   0x114   /* counter 1 */
#define CMCSR1_OFFSET                   0x110   /* control/status register 1 */

#define CMCLKE                          0xB00   /* CLK enable register */
#define CLKEN0                          BIT(5)  /* Enable Clock for channel 0 */
#define CLKEN1                          BIT(6)  /* Enable Clock for channel 1 */

#define CMSTR0_OFFSET                   0x000   /* Timer start register 0 */
#define CMSTR1_OFFSET                   0x100   /* Timer start register 1 */
#define START_BIT                       BIT(0)

#define CSR_CLK_DIV_1                   0x00000007
#define CSR_ENABLE_COUNTER_IN_DEBUG     BIT(3)
#define CSR_ENABLE_INTERRUPT            BIT(5)
#define CSR_FREE_RUN                    BIT(8)
#define CSR_WRITE_FLAG                  BIT(13)
#define CSR_OVERFLOW_FLAG               BIT(14)
#define CSR_MATCH_FLAG                  BIT(15)

static void cmt_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t reg_val;

	/* clear the interrupt */
	reg_val = sys_read32(TIMER_BASE_ADDR + CMCSR0_OFFSET);
	reg_val &= ~CSR_MATCH_FLAG;
	sys_write32(reg_val, TIMER_BASE_ADDR + CMCSR0_OFFSET);

	/* Announce to the kernel */
	sys_clock_announce(1);
}

uint32_t sys_clock_elapsed(void)
{
	/* Always return 0 for tickful operation */
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(TIMER_BASE_ADDR + CMCNT1_OFFSET);
}

/*
 * Initialize both channels at same frequency,
 * Set the first one to generates interrupt at CYCLES_PER_TICK.
 * The second one is used for cycles count, the match value is set
 * at max uint32_t.
 */
static int sys_clock_driver_init(const struct device *dev)
{
	const struct device *clk;
	uint32_t reg_val;
	int i, ret;

	ARG_UNUSED(dev);
	clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0));
	if (clk == NULL) {
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t *)&mod_clk);
	if (ret < 0) {
		return ret;
	}

	/* Supply clock for both channels */
	sys_write32(CLKEN0 | CLKEN1, TIMER_BASE_ADDR + CMCLKE);

	/* Stop both channels */
	reg_val = sys_read32(TIMER_BASE_ADDR + CMSTR0_OFFSET);
	reg_val &= ~START_BIT;
	sys_write32(reg_val, TIMER_BASE_ADDR + CMSTR0_OFFSET);

	reg_val = sys_read32(TIMER_BASE_ADDR + CMSTR1_OFFSET);
	reg_val &= ~START_BIT;
	sys_write32(reg_val, TIMER_BASE_ADDR + CMSTR1_OFFSET);

	/* Set the timers as 32-bit, with RCLK/1 clock */
	sys_write32(CSR_FREE_RUN | CSR_CLK_DIV_1 | CSR_ENABLE_INTERRUPT,
		    TIMER_BASE_ADDR + CMCSR0_OFFSET);

	/* Do not enable interrupts for the second channel */
	sys_write32(CSR_FREE_RUN | CSR_CLK_DIV_1,
		    TIMER_BASE_ADDR + CMCSR1_OFFSET);

	/* Set the first channel match to CYCLES Per tick*/
	sys_write32(CYCLES_PER_TICK, TIMER_BASE_ADDR + CMCOR0_OFFSET);

	/* Set the second channel match to max uint32 */
	sys_write32(0xffffffff, TIMER_BASE_ADDR + CMCOR1_OFFSET);

	/* Reset the counter for first channel, check WRFLG first */
	while (sys_read32(TIMER_BASE_ADDR + CMCSR0_OFFSET) & CSR_WRITE_FLAG)
		;
	sys_write32(0, TIMER_BASE_ADDR + CMCNT0_OFFSET);

	for (i = 0; i < 1000; i++) {
		if (!sys_read32(TIMER_BASE_ADDR + CMCNT0_OFFSET)) {
			break;
		}
	}

	__ASSERT(sys_read32(TIMER_BASE_ADDR + CMCNT0_OFFSET) == 0,
			"Fail to clear CMCNT0 register");

	/* Connect timer interrupt for channel 0*/
	IRQ_CONNECT(TIMER_IRQ, 0, cmt_isr, 0, 0);
	irq_enable(TIMER_IRQ);

	/* Start the timers */
	sys_write32(START_BIT, TIMER_BASE_ADDR + CMSTR0_OFFSET);
	sys_write32(START_BIT, TIMER_BASE_ADDR + CMSTR1_OFFSET);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
