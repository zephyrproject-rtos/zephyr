/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_DW_ICTL_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_DW_ICTL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dw_ictl_config_irq_t)(struct device *port);

struct dw_ictl_config {
	u32_t irq_num;
	u32_t numirqs;
	u32_t isr_table_offset;
	dw_ictl_config_irq_t config_func;
};

struct dw_ictl_runtime {
	u32_t base_addr;
};

struct dw_ictl_registers {
	u32_t irq_inten_l;		/* offset 00 */
	u32_t irq_inten_h;		/* offset 04 */
	u32_t irq_intmask_l;		/* offset 08 */
	u32_t irq_intmask_h;		/* offset 0C */
	u32_t irq_intforce_l;		/* offset 10 */
	u32_t irq_intforce_h;		/* offset 14 */
	u32_t irq_rawstatus_l;		/* offset 18 */
	u32_t irq_rawstatus_h;		/* offset 1c */
	u32_t irq_status_l;		/* offset 20 */
	u32_t irq_status_h;		/* offset 24 */
	u32_t irq_maskstatus_l;		/* offset 28 */
	u32_t irq_maskstatus_h;		/* offset 2c */
	u32_t irq_finalstatus_l;	/* offset 30 */
	u32_t irq_finalstatus_h;	/* offset 34 */
	u32_t irq_vector;		/* offset 38 */
	u32_t Reserved1;		/* offset 3c */
	u32_t irq_vector_0;		/* offset 40 */
	u32_t Reserved2;		/* offset 44 */
	u32_t irq_vector_1;		/* offset 48 */
	u32_t Reserved3;		/* offset 4c */
	u32_t irq_vector_2;		/* offset 50 */
	u32_t Reserved4;		/* offset 54 */
	u32_t irq_vector_3;		/* offset 58 */
	u32_t Reserved5;		/* offset 5c */
	u32_t irq_vector_4;		/* offset 60 */
	u32_t Reserved6;		/* offset 64 */
	u32_t irq_vector_5;		/* offset 68 */
	u32_t Reserved7;		/* offset 6c */
	u32_t irq_vector_6;		/* offset 70 */
	u32_t Reserved8;		/* offset 74 */
	u32_t irq_vector_7;		/* offset 78 */
	u32_t Reserved9;		/* offset 7c */
	u32_t irq_vector_8;		/* offset 80 */
	u32_t Reserved10;		/* offset 84 */
	u32_t irq_vector_9;		/* offset 88 */
	u32_t Reserved11;		/* offset 8c */
	u32_t irq_vector_10;		/* offset 90 */
	u32_t Reserved12;		/* offset 94 */
	u32_t irq_vector_11;		/* offset 98 */
	u32_t Reserved13;		/* offset 9c */
	u32_t irq_vector_12;		/* offset a0 */
	u32_t Reserved14;		/* offset a4 */
	u32_t irq_vector_13;		/* offset a8 */
	u32_t Reserved15;		/* offset ac */
	u32_t irq_vector_14;		/* offset b0 */
	u32_t Reserved16;		/* offset b4 */
	u32_t irq_vector_15;		/* offset b8 */
	u32_t Reserved17;		/* offset bc */
	u32_t fiq_inten;		/* offset c0 */
	u32_t fiq_intmask;		/* offset c4 */
	u32_t fiq_intforce;		/* offset c8 */
	u32_t fiq_rawstatus;		/* offset cc */
	u32_t fiq_status;		/* offset d0 */
	u32_t fiq_finalstatus;		/* offset d4 */
	u32_t irq_plevel;		/* offset d8 */
	u32_t Reserved18;		/* offset dc */
	u32_t APB_ICTL_COMP_VERSION;	/* offset e0 */
	u32_t Reserved19;		/* offset e4 */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_DW_ICTL_H_ */
