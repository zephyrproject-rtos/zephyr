/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_DW_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_DW_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dw_ictl_config_irq_t)(const struct device *dev);

struct dw_ictl_config {
	uint32_t base_addr;
	uint32_t numirqs;
	uint32_t isr_table_offset;
	dw_ictl_config_irq_t config_func;
};

struct dw_ictl_registers {
	uint32_t irq_inten_l;		/* offset 00 */
	uint32_t irq_inten_h;		/* offset 04 */
	uint32_t irq_intmask_l;		/* offset 08 */
	uint32_t irq_intmask_h;		/* offset 0C */
	uint32_t irq_intforce_l;		/* offset 10 */
	uint32_t irq_intforce_h;		/* offset 14 */
	uint32_t irq_rawstatus_l;		/* offset 18 */
	uint32_t irq_rawstatus_h;		/* offset 1c */
	uint32_t irq_status_l;		/* offset 20 */
	uint32_t irq_status_h;		/* offset 24 */
	uint32_t irq_maskstatus_l;		/* offset 28 */
	uint32_t irq_maskstatus_h;		/* offset 2c */
	uint32_t irq_finalstatus_l;	/* offset 30 */
	uint32_t irq_finalstatus_h;	/* offset 34 */
	uint32_t irq_vector;		/* offset 38 */
	uint32_t Reserved1;		/* offset 3c */
	uint32_t irq_vector_0;		/* offset 40 */
	uint32_t Reserved2;		/* offset 44 */
	uint32_t irq_vector_1;		/* offset 48 */
	uint32_t Reserved3;		/* offset 4c */
	uint32_t irq_vector_2;		/* offset 50 */
	uint32_t Reserved4;		/* offset 54 */
	uint32_t irq_vector_3;		/* offset 58 */
	uint32_t Reserved5;		/* offset 5c */
	uint32_t irq_vector_4;		/* offset 60 */
	uint32_t Reserved6;		/* offset 64 */
	uint32_t irq_vector_5;		/* offset 68 */
	uint32_t Reserved7;		/* offset 6c */
	uint32_t irq_vector_6;		/* offset 70 */
	uint32_t Reserved8;		/* offset 74 */
	uint32_t irq_vector_7;		/* offset 78 */
	uint32_t Reserved9;		/* offset 7c */
	uint32_t irq_vector_8;		/* offset 80 */
	uint32_t Reserved10;		/* offset 84 */
	uint32_t irq_vector_9;		/* offset 88 */
	uint32_t Reserved11;		/* offset 8c */
	uint32_t irq_vector_10;		/* offset 90 */
	uint32_t Reserved12;		/* offset 94 */
	uint32_t irq_vector_11;		/* offset 98 */
	uint32_t Reserved13;		/* offset 9c */
	uint32_t irq_vector_12;		/* offset a0 */
	uint32_t Reserved14;		/* offset a4 */
	uint32_t irq_vector_13;		/* offset a8 */
	uint32_t Reserved15;		/* offset ac */
	uint32_t irq_vector_14;		/* offset b0 */
	uint32_t Reserved16;		/* offset b4 */
	uint32_t irq_vector_15;		/* offset b8 */
	uint32_t Reserved17;		/* offset bc */
	uint32_t fiq_inten;		/* offset c0 */
	uint32_t fiq_intmask;		/* offset c4 */
	uint32_t fiq_intforce;		/* offset c8 */
	uint32_t fiq_rawstatus;		/* offset cc */
	uint32_t fiq_status;		/* offset d0 */
	uint32_t fiq_finalstatus;		/* offset d4 */
	uint32_t irq_plevel;		/* offset d8 */
	uint32_t Reserved18;		/* offset dc */
	uint32_t APB_ICTL_COMP_VERSION;	/* offset e0 */
	uint32_t Reserved19;		/* offset e4 */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_DW_H_ */
