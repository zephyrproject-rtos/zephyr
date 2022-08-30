/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_intmux

/**
 * @file
 * @brief RV32M1 INTMUX (interrupt multiplexer) driver
 *
 * This driver provides support for level 2 interrupts on the RV32M1
 * SoC using the INTMUX peripheral.
 *
 * Each of the RI5CY and ZERO-RISCY cores has an INTMUX peripheral;
 * INTMUX0 is wired to the RI5CY event unit interrupt table, while
 * INTMUX1 is used with ZERO-RISCY.
 *
 * For this reason, only a single intmux device is declared here. The
 * dtsi for each core needs to set up the intmux device and any
 * associated IRQ numbers to work with this driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/sw_isr_table.h>
#include <soc.h>
#include <zephyr/dt-bindings/interrupt-controller/openisa-intmux.h>

/*
 * CHn_VEC registers are offset by a value that is convenient if
 * you're dealing with a Cortex-M NVIC vector table; we're not, so it
 * needs to be subtracted out to get a useful value.
 */
#define VECN_OFFSET 48U

struct rv32m1_intmux_config {
	INTMUX_Type *regs;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct _isr_table_entry *isr_base;
};

#define DEV_REGS(dev) (((const struct rv32m1_intmux_config *)(dev->config))->regs)

/*
 * <irq_nextlevel.h> API
 */

static void rv32m1_intmux_irq_enable(const struct device *dev, uint32_t irq)
{
	INTMUX_Type *regs = DEV_REGS(dev);
	uint32_t channel = rv32m1_intmux_channel(irq);
	uint32_t line = rv32m1_intmux_line(irq);

	regs->CHANNEL[channel].CHn_IER_31_0 |= BIT(line);
}

static void rv32m1_intmux_irq_disable(const struct device *dev, uint32_t irq)
{
	INTMUX_Type *regs = DEV_REGS(dev);
	uint32_t channel = rv32m1_intmux_channel(irq);
	uint32_t line = rv32m1_intmux_line(irq);

	regs->CHANNEL[channel].CHn_IER_31_0 &= ~BIT(line);
}

static uint32_t rv32m1_intmux_get_state(const struct device *dev)
{
	INTMUX_Type *regs = DEV_REGS(dev);
	size_t i;

	for (i = 0; i < INTMUX_CHn_IER_31_0_COUNT; i++) {
		if (regs->CHANNEL[i].CHn_IER_31_0) {
			return 1;
		}
	}

	return 0;
}

static int rv32m1_intmux_get_line_state(const struct device *dev,
					unsigned int irq)
{
	INTMUX_Type *regs = DEV_REGS(dev);
	uint32_t channel = rv32m1_intmux_channel(irq);
	uint32_t line = rv32m1_intmux_line(irq);

	if ((regs->CHANNEL[channel].CHn_IER_31_0 & BIT(line)) != 0) {
		return 1;
	}

	return 0;
}

/*
 * IRQ handling.
 */

#define ISR_ENTRY(channel, line) \
	((channel) * CONFIG_MAX_IRQ_PER_AGGREGATOR + line)

static void rv32m1_intmux_isr(const void *arg)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct rv32m1_intmux_config *config = dev->config;
	INTMUX_Type *regs = DEV_REGS(dev);
	uint32_t channel = POINTER_TO_UINT(arg);
	uint32_t line = (regs->CHANNEL[channel].CHn_VEC >> 2);
	struct _isr_table_entry *isr_base = config->isr_base;
	struct _isr_table_entry *entry;

	/*
	 * Make sure the vector is valid, there is a note of page 1243~1244
	 * of chapter 36 INTMUX of RV32M1 RM,
	 * Note: Unlike the NVIC, the INTMUX does not latch pending source
	 * interrupts. This means that the INTMUX output channel ISRs must
	 * check for and handle a 0 value of the CHn_VEC register to
	 * account for spurious interrupts.
	 */
	if (line < VECN_OFFSET) {
		return;
	}

	entry = &isr_base[ISR_ENTRY(channel, (line - VECN_OFFSET))];
	entry->isr(entry->arg);
}

/*
 * Instance and initialization
 */

static const struct irq_next_level_api rv32m1_intmux_apis = {
	.intr_enable = rv32m1_intmux_irq_enable,
	.intr_disable = rv32m1_intmux_irq_disable,
	.intr_get_state = rv32m1_intmux_get_state,
	.intr_get_line_state = rv32m1_intmux_get_line_state,
};

static const struct rv32m1_intmux_config rv32m1_intmux_cfg = {
	.regs = (INTMUX_Type *)DT_INST_REG_ADDR(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = UINT_TO_POINTER(DT_INST_CLOCKS_CELL(0, name)),
	.isr_base = &_sw_isr_table[CONFIG_2ND_LVL_ISR_TBL_OFFSET],
};

static int rv32m1_intmux_init(const struct device *dev)
{
	const struct rv32m1_intmux_config *config = dev->config;
	INTMUX_Type *regs = DEV_REGS(dev);
	size_t i;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	/* Enable INTMUX clock. */
	clock_control_on(config->clock_dev, config->clock_subsys);

	/*
	 * Reset all channels, not just the ones we're configured to
	 * support. We don't want to continue to take level 2 IRQs
	 * enabled by bootloaders, for example.
	 */
	for (i = 0; i < INTMUX_CHn_CSR_COUNT; i++) {
		regs->CHANNEL[i].CHn_CSR |= INTMUX_CHn_CSR_RST_MASK;
	}

	/* Connect and enable level 1 (channel) interrupts. */
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_0
	IRQ_CONNECT(INTMUX_CH0_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(0), 0);
	irq_enable(INTMUX_CH0_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_1
	IRQ_CONNECT(INTMUX_CH1_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(1), 0);
	irq_enable(INTMUX_CH1_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_2
	IRQ_CONNECT(INTMUX_CH2_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(2), 0);
	irq_enable(INTMUX_CH2_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_3
	IRQ_CONNECT(INTMUX_CH3_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(3), 0);
	irq_enable(INTMUX_CH3_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_4
	IRQ_CONNECT(INTMUX_CH4_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(4), 0);
	irq_enable(INTMUX_CH4_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_5
	IRQ_CONNECT(INTMUX_CH5_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(5), 0);
	irq_enable(INTMUX_CH5_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_6
	IRQ_CONNECT(INTMUX_CH6_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(6), 0);
	irq_enable(INTMUX_CH6_IRQ);
#endif
#ifdef CONFIG_RV32M1_INTMUX_CHANNEL_7
	IRQ_CONNECT(INTMUX_CH7_IRQ, 0, rv32m1_intmux_isr,
		    UINT_TO_POINTER(7), 0);
	irq_enable(INTMUX_CH7_IRQ);
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &rv32m1_intmux_init, NULL, NULL,
		    &rv32m1_intmux_cfg, PRE_KERNEL_1,
		    CONFIG_RV32M1_INTMUX_INIT_PRIORITY, &rv32m1_intmux_apis);
