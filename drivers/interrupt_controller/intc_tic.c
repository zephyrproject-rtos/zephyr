/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NOTE: This driver implements the GIC400 interfaces.
 */

#define DT_DRV_COMPAT tcc_tic

#include <stdint.h>

#include <zephyr/arch/arm/irq.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/dt-bindings/interrupt-controller/tcc-tic.h>
#include <zephyr/drivers/interrupt_controller/intc_tic.h>

LOG_MODULE_REGISTER(tic);

static tic_irq_func_ptr tic_intr_table[TIC_INT_SRC_CNT];

static void tic_irq_pri_set_internal(uint32_t irq, uint32_t pri)
{
	uint32_t reg_offset, reg_bits, intr_pri_reg;

	reg_offset = 0;
	reg_bits = 0;
	intr_pri_reg = 0;

	if ((pri < TIC_PRIORITY_NO_MEAN) && (irq < TIC_INT_SRC_CNT)) {
		reg_offset = (irq >> 2u);
		reg_bits = (irq & 0x03u);

		intr_pri_reg = tic_distributer->dist_intr_pri[reg_offset];
		intr_pri_reg = (uint32_t)(intr_pri_reg & ~(0xFFu << (reg_bits * 8u)));
		intr_pri_reg = (uint32_t)(intr_pri_reg | ((pri & 0xFFu) << (reg_bits * 8u)));

		tic_distributer->dist_intr_pri[reg_offset] = intr_pri_reg;
	}
}

static void tic_irq_config_set(uint32_t irq, uint8_t irq_type)
{
	uint32_t reg_offset, reg_mask, intr_config;

	reg_offset = 0;
	reg_mask = 0;
	intr_config = 0;

	if (irq < TIC_INT_SRC_CNT) {
		reg_offset = (irq >> 4u);
		reg_mask = (uint32_t)(0x2u << ((irq & 0xfu) * 2u));
		intr_config = tic_distributer->dist_intr_config[reg_offset];

		if (((irq_type & (uint8_t)TIC_INT_TYPE_LEVEL_HIGH) ==
		     (uint8_t)TIC_INT_TYPE_LEVEL_HIGH) ||
		    ((irq_type & (uint8_t)TIC_INT_TYPE_LEVEL_LOW) ==
		     (uint8_t)TIC_INT_TYPE_LEVEL_LOW)) {
			intr_config = (uint32_t)(intr_config & ~reg_mask);
		} else {
			intr_config = (uint32_t)(intr_config | reg_mask);
		}

		tic_distributer->dist_intr_config[reg_offset] = intr_config;
	}
}

void tic_irq_vector_set(uint32_t irq, uint32_t pri, uint8_t irq_type, tic_isr_func irq_func,
			void *irq_arg)
{
	uint32_t rsvd_irq;

	rsvd_irq = 0;

	if ((pri > TIC_PRIORITY_NO_MEAN) || (irq >= TIC_INT_SRC_CNT)) {
		return;
	}

	tic_irq_pri_set_internal(irq, pri);
	tic_irq_config_set(irq, irq_type);

	tic_intr_table[irq].if_func_ptr = irq_func;
	tic_intr_table[irq].if_arg_ptr = irq_arg;
	tic_intr_table[irq].if_is_both_edge = 0;

	if ((irq >= (uint32_t)TIC_EINT_START_INT) &&
	    (irq <= (uint32_t)TIC_EINT_END_INT) /* Set reversed external interrupt */
	    && (irq_type == (uint8_t)TIC_INT_TYPE_EDGE_BOTH)) { /* for supporting both edge. */

		rsvd_irq = (irq + TIC_EINT_NUM); /* add offset of IRQn */

		tic_irq_pri_set_internal(rsvd_irq, pri);
		tic_irq_config_set(rsvd_irq, irq_type);

		tic_intr_table[rsvd_irq].if_func_ptr = irq_func;
		tic_intr_table[rsvd_irq].if_arg_ptr = irq_arg;
		tic_intr_table[irq].if_is_both_edge = (1U);
	}
}

unsigned int z_tic_irq_get_active(void)
{
	uint32_t int_id;

	int_id = tic_cpu_if->cpu_intr_ack;

	return int_id;
}

void z_tic_irq_eoi(unsigned int irq)
{
	tic_cpu_if->cpu_end_intr = irq;
}

void z_tic_irq_init(void)
{
	unsigned long reg_offset;

	reg_offset = 0;

	/* Global TIC disable -> enable. */
	tic_distributer->dist_ctrl &= (unsigned long)(~ARM_BIT_TIC_DIST_ICDDCR_EN);
	tic_distributer->dist_ctrl |= (unsigned long)ARM_BIT_TIC_DIST_ICDDCR_EN;

	for (; reg_offset <= ((unsigned long)(TIC_INT_SRC_CNT - 1UL) / 4UL); reg_offset++) {
		tic_distributer->dist_intr_pri[reg_offset] = 0xFAFAFAFAUL;
	}

	tic_cpu_if->cpu_pri_mask = UNMASK_VALUE;
	tic_cpu_if->cpu_ctlr |=
		(TIC_CPUIF_CTRL_ENABLEGRP0 | TIC_CPUIF_CTRL_ENABLEGRP1 | TIC_CPUIF_CTRL_ACKCTL);

	LOG_DBG("TIC: Number of IRQs = %lu\n", (unsigned long)TIC_INT_SRC_CNT);
}

void z_tic_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	tic_irq_pri_set_internal(irq, prio);
}

void z_tic_irq_enable(unsigned int irq)
{
	uint32_t reg_offset, mask_bit_id;

	reg_offset = 0;
	mask_bit_id = 0;

	if (irq < TIC_INT_SRC_CNT) {
		reg_offset = (irq >> 5u);    /* Calculate the register offset. */
		mask_bit_id = (irq & 0x1Fu); /* Mask bit ID. */

		tic_distributer->dist_intr_set_en[reg_offset] = (1UL << mask_bit_id);

		if (tic_intr_table[irq].if_is_both_edge == (1UL)) {
			reg_offset = ((irq + 10UL) >> 5UL);    /* Calculate the register offset. */
			mask_bit_id = ((irq + 10UL) & 0x1FUL); /* Mask bit ID. */

			tic_distributer->dist_intr_set_en[reg_offset] = (1UL << mask_bit_id);
		}
	} else {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}
}

void z_tic_irq_disable(unsigned int irq)
{
	uint32_t reg_offset, mask_bit_id;

	reg_offset = 0;
	mask_bit_id = 0;

	if (irq < TIC_INT_SRC_CNT) {
		reg_offset = (irq >> 5UL);    /* Calculate the register offset. */
		mask_bit_id = (irq & 0x1FUL); /* Mask bit ID. */

		tic_distributer->dist_intr_clr_en[reg_offset] = (1UL << mask_bit_id);

		if (tic_intr_table[irq].if_is_both_edge == (1UL)) {
			reg_offset = ((irq + 10UL) >> 5UL);    /* Calculate the register offset. */
			mask_bit_id = ((irq + 10UL) & 0x1FUL); /* Mask bit ID. */

			tic_distributer->dist_intr_clr_en[reg_offset] = (1UL << mask_bit_id);
		}
	} else {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
		return;
	}
}

bool z_tic_irq_is_enabled(unsigned int irq)
{
	uint32_t reg_offset, mask_bit_id, enabler;

	reg_offset = 0;
	mask_bit_id = 0;

	if (irq < TIC_INT_SRC_CNT) {
		reg_offset = (irq >> 5u);    /* Calculate the register offset. */
		mask_bit_id = (irq & 0x1Fu); /* Mask bit ID. */

		enabler = tic_distributer->dist_intr_set_en[reg_offset];

		return (enabler & (1 << mask_bit_id)) != 0;
	}

	LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
	return false;
}

void z_tic_arm_enter_irq(int irq)
{
	uint32_t target_list_filter;
	uint32_t cpu_target_list, group_id;

	target_list_filter = TIC_SGI_TO_TARGETLIST;
	cpu_target_list = 0x1UL; /* bitfiled 0 : cpu #0, bitfield n : cpu #n, n : 0 ~ 7 */
	group_id = 0UL;          /* 0 : group 0 , 1: group 1 */

	if (irq <= 15UL) {
		tic_distributer->dist_sw_gen_intr = (uint32_t)((target_list_filter & 0x3UL) << 24) |
						    ((cpu_target_list & 0xffUL) << 16) |
						    ((group_id & 0x1UL) << 15) | (irq & 0xfUL);
	} else {
		LOG_ERR("%s: Invalid irq number = %u\n", __func__, irq);
	}
}

void tic_irq_handler(void *arg)
{
	uint32_t intr_ack_reg, irq;
	tic_isr_func func_isr;
	void *intr_arg_ptr;

	intr_ack_reg = z_tic_irq_get_active();
	irq = intr_ack_reg & 0x3FFU; /* Mask away the CPUID. */
	func_isr = (tic_isr_func)NULL;
	intr_arg_ptr = TCC_NULL_PTR;

	if (irq < TIC_INT_SRC_CNT) {
		func_isr = tic_intr_table[irq].if_func_ptr; /* Fetch ISR handler. */
		intr_arg_ptr = tic_intr_table[irq].if_arg_ptr;

		if (func_isr != (tic_isr_func)NULL) {
			(*func_isr)(intr_arg_ptr); /* Call ISR handler. */
		}

		z_tic_irq_eoi(intr_ack_reg);
	}
}
