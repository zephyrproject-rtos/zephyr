/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <irq_nextlevel.h>
#include <soc.h>
#include "mchp_xec_ictl.h"

/*
 * isr_base_offset = start address of GIRQn aggregated handlers.
 * _sw_isr_table base
 *	174 level 1 entries
 *	girq08 aggregated handler base
 *		32 aggregated handlers
 *	girq09 aggregated handler base
 *		32 aggregated handlers
 *	girq10 aggregated handler base
 *		...
 *	girq26 aggregated handler base
 *		...
 * We are only implementing aggregated GIRQ 8-12, 19, and 24-26
 * as all other interrupt sources can use direct mode. Current
 * MULTI_LEVEL interrupt support requires tables for all possible
 * level 1 aggregators.
 *
 * ISR implementation notes:
 * The intr_status we pass in is bitwise OR of interrupt enable
 * and status. GIRQ.RESULT register generates the bitwise OR.
 * The find_lsb_set() function calls __builtin_ffs() making use of
 * Cortex-Mx's bit reverse and count leading zeros instructions.
 */
static ALWAYS_INLINE void xec_ictl_dispatch_child_isrs(u32_t intr_status,
						       u32_t isr_base_offset)
{
	u32_t intr_bitpos, intr_offset;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (intr_status) {
		intr_bitpos = find_lsb_set(intr_status) - 1;
		intr_status &= ~(1 << intr_bitpos);
		intr_offset = isr_base_offset + intr_bitpos;
		_sw_isr_table[intr_offset].isr(
			_sw_isr_table[intr_offset].arg);
	}
}

static void xec_ictl_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct xec_ictl_runtime *context = port->driver_data;

	const struct xec_ictl_config *config = port->config->config_info;
	GIRQ_Type *regs = (GIRQ_Type *)context->girq_addr;

	xec_ictl_dispatch_child_isrs(regs->RESULT,
				     config->isr_table_offset);
}

static inline void xec_ictl_irq_enable(struct device *dev, unsigned int irq)
{
	struct xec_ictl_runtime *context = dev->driver_data;

	GIRQ_Type *regs = (GIRQ_Type *)context->girq_addr;

	regs->EN_SET = (1 << irq); /* TODO get bit position */
}

static inline void xec_ictl_irq_disable(struct device *dev, unsigned int irq)
{
	struct xec_ictl_runtime *context = dev->driver_data;

	GIRQ_Type *regs = (GIRQ_Type *)context->girq_addr;

	regs->EN_CLR = (1 << irq); /* TODO get bit position */
}

static inline unsigned int xec_ictl_irq_get_state(struct device *dev)
{
	struct xec_ictl_runtime *context = dev->driver_data;

	GIRQ_Type *regs = (GIRQ_Type *)context->girq_addr;

	/* When the bits of this register are clear, it means the
	 * corresponding interrupts are disabled. This function
	 * returns 0 only if ALL the interrupts are disabled.
	 */
	if (regs->EN_SET == 0) {
		return 0;
	}

	return 1;
}

static const struct irq_next_level_api xec_apis = {
	.intr_enable = xec_ictl_irq_enable,
	.intr_disable = xec_ictl_irq_disable,
	.intr_get_state = xec_ictl_irq_get_state,
};

/*
 * SoC initialization disconnects all aggregated GIRQ's only direct
 * GIRQ source are routed to NVIC.
 * This routine enables the aggregated output of the GIRQ routing it
 * to its corresponding NVIC external input.
 * Note: aggregated NVIC inputs are separate from direct NVIC inputs.
 */
static int xec_ictl_initialize(struct device *port)
{
	struct xec_ictl_runtime *context = port->driver_data;
	const struct xec_ictl_config *config = port->config->config_info;

	volatile struct xec_ctrl_registers *ctrl_regs =
		(volatile struct xec_ctrl_registers *)context->girq_ctrl_addr;

	ctrl_regs->girq_aggr_out_set_en = (1ul << config->girq_num);

	NVIC_EnableIRQ((IRQn_Type)(config->irq_num & 0xFF));

	return 0;
}

/* Aggregated GIRQ08 */
static int xec_ictl_0_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_0_irq(struct device *port);

#define ISR_TBL_OFFSET(lvl2_offset) (CONFIG_SOC_NUM_EXTERNAL_INTS +\
	((CONFIG_MAX_IRQ_PER_AGGREGATOR) * (u32_t)(lvl2_offset)))

static const struct xec_ictl_config xec_config_0 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_0_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_0_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_00_OFFSET),
	.config_func = xec_config_0_irq,
};

static struct xec_ictl_runtime xec_0_runtime = {	/* TODO add to fixup */
	.girq_addr = DT_MICROCHIP_XEC_INTC_0_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_0_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_0, DT_MICROCHIP_XEC_INTC_0_LABEL,
		    xec_ictl_0_initialize, &xec_0_runtime, &xec_config_0,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_0_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_0_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_0_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_0_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_0), 0);
}

/* Aggregated GIRQ09 */
static int xec_ictl_1_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_1_irq(struct device *port);

static const struct xec_ictl_config xec_config_1 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_1_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_1_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_01_OFFSET),
	.config_func = xec_config_1_irq,
};

static struct xec_ictl_runtime xec_1_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_1_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_1_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_1, DT_MICROCHIP_XEC_INTC_1_LABEL,
		    xec_ictl_1_initialize, &xec_1_runtime, &xec_config_1,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_1_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_1_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_1_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_1_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_1), 0);
}

/* Aggregated GIRQ10 */
static int xec_ictl_2_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_2_irq(struct device *port);

static const struct xec_ictl_config xec_config_2 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_2_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_2_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_02_OFFSET),
	.config_func = xec_config_2_irq,
};

static struct xec_ictl_runtime xec_2_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_2_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_2_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_2, DT_MICROCHIP_XEC_INTC_2_LABEL,
		    xec_ictl_2_initialize, &xec_2_runtime, &xec_config_2,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_2_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_2_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_2_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_2_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_2), 0);
}

/* Aggregated GIRQ11 */
static int xec_ictl_3_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_3_irq(struct device *port);

static const struct xec_ictl_config xec_config_3 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_3_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_3_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_03_OFFSET),
	.config_func = xec_config_3_irq,
};

static struct xec_ictl_runtime xec_3_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_3_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_3_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_3, DT_MICROCHIP_XEC_INTC_3_LABEL,
		    xec_ictl_3_initialize, &xec_3_runtime, &xec_config_3,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_3_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_3_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_3_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_3_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_3), 0);
}

/* Aggregated GIRQ12 */
static int xec_ictl_4_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_4_irq(struct device *port);

static const struct xec_ictl_config xec_config_4 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_4_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_4_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_04_OFFSET),
	.config_func = xec_config_4_irq,
};

static struct xec_ictl_runtime xec_4_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_4_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_4_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_4, DT_MICROCHIP_XEC_INTC_4_LABEL,
		    xec_ictl_4_initialize, &xec_4_runtime, &xec_config_4,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_4_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_4_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_4_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_4_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_4), 0);
}

/* Aggregated GIRQ19 */
static int xec_ictl_5_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_5_irq(struct device *port);

/* GIRQ19 is NVIC external input 11 use CONFIG_2ND_LVL_INTR_11_OFFSET */
static const struct xec_ictl_config xec_config_5 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_5_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_5_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_11_OFFSET),
	.config_func = xec_config_5_irq,
};

static struct xec_ictl_runtime xec_5_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_5_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_5_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_5, DT_MICROCHIP_XEC_INTC_5_LABEL,
		    xec_ictl_5_initialize, &xec_5_runtime, &xec_config_5,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_5_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_5_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_5_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_5_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_5), 0);
}

/* Aggregated GIRQ24 */
static int xec_ictl_6_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_6_irq(struct device *port);

/* GIRQ24 is NVIC external input 15 use CONFIG_2ND_LVL_INTR_15_OFFSET */
static const struct xec_ictl_config xec_config_6 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_6_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_6_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_15_OFFSET),
	.config_func = xec_config_6_irq,
};

static struct xec_ictl_runtime xec_6_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_6_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_6_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_6, DT_MICROCHIP_XEC_INTC_6_LABEL,
		    xec_ictl_6_initialize, &xec_6_runtime, &xec_config_6,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_6_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_6_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_6_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_6_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_6), 0);
}

/* Aggregated GIRQ25 */
static int xec_ictl_7_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_7_irq(struct device *port);

/* GIRQ25 is NVIC external input 16 use CONFIG_2ND_LVL_INTR_16_OFFSET */
static const struct xec_ictl_config xec_config_7 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_7_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_7_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_16_OFFSET),
	.config_func = xec_config_7_irq,
};

static struct xec_ictl_runtime xec_7_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_7_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_7_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_7, DT_MICROCHIP_XEC_INTC_7_LABEL,
		    xec_ictl_7_initialize, &xec_7_runtime, &xec_config_7,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_7_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_7_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_7_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_7_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_7), 0);
}

/* Aggregated GIRQ26 */
static int xec_ictl_8_initialize(struct device *port)
{
	return xec_ictl_initialize(port);
}

static void xec_config_8_irq(struct device *port);

/* GIRQ26 is NVIC external input 16 use CONFIG_2ND_LVL_INTR_17_OFFSET */
static const struct xec_ictl_config xec_config_8 = {
	.irq_num = DT_MICROCHIP_XEC_INTC_8_IRQ_0,
	.girq_num = DT_MICROCHIP_XEC_INTC_8_GIRQ_NUM,
	.isr_table_offset = ISR_TBL_OFFSET(CONFIG_2ND_LVL_INTR_17_OFFSET),
	.config_func = xec_config_8_irq,
};

static struct xec_ictl_runtime xec_8_runtime = {
	.girq_addr = DT_MICROCHIP_XEC_INTC_8_GIRQ_BASE_ADDRESS,
	.girq_ctrl_addr = DT_MICROCHIP_XEC_INTC_8_CTRL_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(xec_ictl_8, DT_MICROCHIP_XEC_INTC_8_LABEL,
		    xec_ictl_8_initialize, &xec_8_runtime, &xec_config_8,
		    POST_KERNEL, DT_MICROCHIP_XEC_INTC_8_IRQ_0_PRIORITY,
		    &xec_apis);

static void xec_config_8_irq(struct device *port)
{
	IRQ_CONNECT(DT_MICROCHIP_XEC_INTC_8_IRQ_0,
		    DT_MICROCHIP_XEC_INTC_8_IRQ_0_PRIORITY, xec_ictl_isr,
		    DEVICE_GET(xec_ictl_8), 0);
}
