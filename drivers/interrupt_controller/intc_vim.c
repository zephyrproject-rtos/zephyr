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
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(vim);

/*
 * Physical VIM line count from VIM_INFO. CONFIG_NUM_IRQS may be larger when
 * multilevel ISR table slots are packed above native VIM lines (e.g. TI SCI
 * interrupt-router inputs). Hardware register ops must use this value, not
 * CONFIG_NUM_IRQS.
 */
static uint32_t vim_num_irqs;

static bool z_vim_irq_in_range(unsigned int irq)
{
	return vim_num_irqs > 0U && irq < vim_num_irqs;
}

unsigned int z_vim_irq_get_active(void)
{
	uint32_t irq_group_num, irq_bit_num;
	uint32_t actirq, vec_addr, irq_num;

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

	irq_num = actirq & VIM_ACTIRQ_NUM_MASK;
	if (!z_vim_irq_in_range(irq_num)) {
		return CONFIG_NUM_IRQS + 1;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq_num);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq_num);

	/* Ack the interrupt in IRQSTS register */
	sys_write32(BIT(irq_bit_num), VIM_IRQSTS(irq_group_num));

	return irq_num;
}

void z_vim_irq_eoi(unsigned int irq)
{
	sys_write32(0, VIM_IRQVEC);
}

void z_vim_irq_init(void)
{
	uint32_t max_group;

	vim_num_irqs = sys_read32(VIM_INFO) & VIM_INFO_INTERRUPTS_MASK;
	__ASSERT(vim_num_irqs > 0U, "VIM reported zero interrupt lines");
	LOG_DBG("VIM: hw IRQs = %u, CONFIG_NUM_IRQS = %u\n", vim_num_irqs, CONFIG_NUM_IRQS);

	/* Start with all hardware interrupt groups masked */
	max_group = DIV_ROUND_UP(vim_num_irqs, VIM_MAX_IRQ_PER_GROUP);
	for (uint32_t i = 0; i < max_group; i++) {
		sys_write32(VIM_GRP_INTR_EN_CLR_MSK_MASK, VIM_INTR_EN_CLR(i));
	}
}

void z_vim_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	uint32_t irq_group_num, irq_bit_num, regval;

	if (!z_vim_irq_in_range(irq) || prio > VIM_PRI_INT_MAX ||
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

	if (!z_vim_irq_in_range(irq)) {
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

	if (!z_vim_irq_in_range(irq)) {
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

	if (!z_vim_irq_in_range(irq)) {
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
	z_vim_irq_set_pending(irq);
}

void z_vim_irq_set_pending(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num;

	if (!z_vim_irq_in_range(irq)) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	/* Write-1-to-set the raw status bit (software interrupt). */
	sys_write32(BIT(irq_bit_num), VIM_RAW(irq_group_num));
}

bool z_vim_irq_is_pending(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num, regval;

	if (!z_vim_irq_in_range(irq)) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return false;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	regval = sys_read32(VIM_RAW(irq_group_num));

	return (regval & BIT(irq_bit_num)) != 0U;
}

void z_vim_irq_clear_pending(unsigned int irq)
{
	uint32_t irq_group_num, irq_bit_num;

	if (!z_vim_irq_in_range(irq)) {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}

	irq_group_num = VIM_GET_IRQ_GROUP_NUM(irq);
	irq_bit_num = VIM_GET_IRQ_BIT_NUM(irq);

	/* Write-1-to-clear raw/status bits left after a masked edge. */
	sys_write32(BIT(irq_bit_num), VIM_STS(irq_group_num));
	sys_write32(BIT(irq_bit_num), VIM_IRQSTS(irq_group_num));
}
