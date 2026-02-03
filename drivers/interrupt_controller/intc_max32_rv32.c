/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_rv32_intc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_max32_rv32.h>

typedef struct {
	uint32_t enable;        /**< <tt>\b 0x00:<\tt> */
	uint32_t pending;       /**< <tt>\b 0x04:<\tt>  */
	uint32_t set_pending;   /**< <tt>\b 0x08:<\tt>  */
	uint32_t clear_pending; /**< <tt>\b 0x0c:<\tt>  */
} rv32_intc_grp_t;

typedef struct {
	rv32_intc_grp_t intr[2];
	rv32_intc_grp_t event[2];
} rv32_intc_regs_t;

static volatile rv32_intc_regs_t *get_intc_regs(void)
{
	return (rv32_intc_regs_t *)DT_INST_REG_ADDR(0);
}

void arch_irq_enable(unsigned int source)
{
	volatile rv32_intc_regs_t *regs = get_intc_regs();
	uint8_t grp = source / 32;
	uint32_t val = source % 32;
	volatile rv32_intc_grp_t *int_grp = &regs->intr[grp];
	volatile rv32_intc_grp_t *event_grp = &regs->event[grp];
	unsigned int key;

	key = arch_irq_lock();
	int_grp->enable |= BIT(val);
	event_grp->enable |= BIT(val);
	arch_irq_unlock(key);
}

void arch_irq_disable(unsigned int source)
{
	volatile rv32_intc_regs_t *regs = get_intc_regs();
	uint8_t grp = source / 32;
	uint32_t val = source % 32;
	volatile rv32_intc_grp_t *int_grp = &regs->intr[grp];
	volatile rv32_intc_grp_t *event_grp = &regs->event[grp];
	unsigned int key;

	key = arch_irq_lock();
	int_grp->enable &= ~BIT(val);
	event_grp->enable &= ~BIT(val);
	arch_irq_unlock(key);
}

int arch_irq_is_enabled(unsigned int source)
{
	volatile rv32_intc_regs_t *regs = get_intc_regs();
	uint8_t grp = source / 32;
	uint32_t val = source % 32;
	volatile rv32_intc_grp_t *int_grp = &regs->intr[grp];
	volatile rv32_intc_grp_t *event_grp = &regs->event[grp];
	unsigned int key;
	int ret;

	key = arch_irq_lock();
	ret = (int_grp->enable & BIT(val)) & (event_grp->enable & BIT(val));
	arch_irq_unlock(key);

	return ret;
}

void intc_max32_rv32_irq_clear_pending(int source)
{
	volatile rv32_intc_regs_t *regs = get_intc_regs();
	uint8_t grp = source / 32;
	uint32_t val = source % 32;
	volatile rv32_intc_grp_t *int_grp = &regs->intr[grp];
	volatile rv32_intc_grp_t *event_grp = &regs->event[grp];
	unsigned int key;

	key = arch_irq_lock();
	int_grp->clear_pending |= BIT(val);
	event_grp->clear_pending |= BIT(val);
	arch_irq_unlock(key);
}

uint32_t max32_rv32_intc_get_next_source(void)
{
	volatile rv32_intc_regs_t *regs = get_intc_regs();
	volatile rv32_intc_grp_t *int_grp = &regs->intr[0];
	volatile rv32_intc_grp_t *evt_grp = &regs->event[0];
	uint32_t status;
	uint32_t source;
	uint32_t clear;

	status = int_grp->pending & int_grp->enable;

	if (status) {
		clear = source = __builtin_ffs(status) - 1;
	} else {
		int_grp = &regs->intr[1];
		evt_grp = &regs->event[1];
		status = int_grp->pending & int_grp->enable;

		if (status) {
			clear = __builtin_ffs(status) - 1;
			source = (clear + 32);
		} else {
			clear = 0;
			source = 0;
			printk("No pending interrupt lines!\n");
		}
	}

	int_grp->clear_pending |= BIT(clear);
	evt_grp->clear_pending |= BIT(clear);

	return source;
}

static int max32_rv32_intc_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_INST_DEFINE(0, max32_rv32_intc_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
