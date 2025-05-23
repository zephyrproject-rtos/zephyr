/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 * Copyright (C) 2025 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_vim

#include <stdint.h>

#include <zephyr/arch/arm/irq.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_vim.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "zephyr/sys/__assert.h"
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(vim);

unsigned int z_vim_irq_get_active(void)
{
	uint32_t irq_group_num, irq_bit_num;
	uint32_t actirq, vec_addr;

	/* Reading IRQVEC register, ACTIRQ gets loaded with valid IRQ values */
	vec_addr = sys_read32(VIM_IRQVEC);

	/* ACTIRQ register should be read only after reading IRQVEC register */
	actirq = sys_read32(VIM_ACTIRQ);

	/* Check if the irq number is valid, else return invalid irq number.
	 * which will be considered as spurious interrupt
	 */
	if ((actirq & (VIM_ACTIRQ_VALID_MASK)) == 0) {
		return CONFIG_NUM_IRQS + 1;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(actirq & VIM_PRIIRQ_NUM_MASK);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(actirq & VIM_PRIIRQ_NUM_MASK);

	/* Ack the interrupt in IRQSTS register */
	sys_write32(BIT(irq_bit_num), VIM_IRQSTS(irq_group_num));

	if (irq_group_num > VIM_MAX_GROUP_NUM) {
		return (CONFIG_NUM_IRQS + 1);
	}

	return (actirq & VIM_ACTIRQ_NUM_MASK);
}

void z_vim_irq_eoi(unsigned int irq)
{
	sys_write32(0, VIM_IRQVEC);
}

void z_vim_irq_init(void)
{
	uint32_t num_of_irqs = sys_read32(VIM_INFO) & VIM_INFO_INTERRUPTS_MASK;

	__ASSERT(CONFIG_NUM_IRQS == num_of_irqs,
		 "Number of configured interrupts (%d) doesn't match reported "
		 "(%" PRIu32 ") interrupts",
		 CONFIG_NUM_IRQS, num_of_irqs);
	LOG_DBG("VIM: Number of IRQs = %u\n", num_of_irqs);
}

void z_vim_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	uint32_t irq_group_num, irq_bit_num, regval;

	if (irq > CONFIG_NUM_IRQS || prio > VIM_PRI_INT_MAX ||
	    (flags != IRQ_TYPE_EDGE && flags != IRQ_TYPE_LEVEL)) {
		LOG_ERR("%s: Invalid argument irq = %u prio = %u flags = %u\n",
			__func__, irq, prio, flags);
		return;
	}

	sys_write8(prio, VIM_PRI_INT(irq));

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	regval = sys_read32(VIM_INTTYPE(irq_group_num));

	if (flags == IRQ_TYPE_EDGE) {
		regval |= (BIT(irq_bit_num));
	} else {
		regval &= ~(BIT(irq_bit_num));
	}

	sys_write32(regval, VIM_INTTYPE(irq_group_num));
}

void z_vim_irq_enable(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num;

	if (irq > CONFIG_NUM_IRQS) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	sys_write32(BIT(irq_bit_num), VIM_INTR_EN_SET(irq_group_num));
}

void z_vim_irq_disable(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num;

	if (irq > CONFIG_NUM_IRQS) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	sys_write32(BIT(irq_bit_num), VIM_INTR_EN_CLR(irq_group_num));
}

int z_vim_irq_is_enabled(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num, regval;

	if (irq > CONFIG_NUM_IRQS) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return -EINVAL;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	regval = sys_read32(VIM_INTR_EN_SET(irq_group_num));

	return !!(regval & (BIT(irq_bit_num)));
}

void z_vim_arm_enter_irq(int irq)
{
	uint32_t irq_group_num, irq_bit_num;

	if (irq > CONFIG_NUM_IRQS) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	sys_write32(BIT(irq_bit_num), VIM_RAW(irq_group_num));
}
