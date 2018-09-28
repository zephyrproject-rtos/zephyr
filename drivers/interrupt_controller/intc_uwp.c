/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT unisoc_uwp_intc

#include <device.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <irq_nextlevel.h>

#include "uwp_hal.h"

/* convenience defines */
#define DEV_DATA(dev) \
	((struct uwp_ictl_data * const)(dev)->driver_data)
#define DEV_CFG(dev) \
	((struct uwp_ictl_config * const)(dev)->config_info)
#define INTC_STRUCT(dev) \
	((volatile struct uwp_intc *)(DEV_DATA(dev))->base_addr)

#define INTC(idx) DT_NODELABEL(intc##idx)

typedef void (*uwp_ictl_config_irq_t)(struct device *dev);

struct uwp_ictl_config {
	uint32_t	irq_num;
	uint32_t	isr_table_offset;
	uwp_ictl_config_irq_t	config_func;
};

struct uwp_ictl_data {
	uint32_t	base_addr;
};

static ALWAYS_INLINE void uwp_dispatch_child_isrs(uint32_t intr_status,
						      uint32_t isr_base_offset)
{
	uint32_t intr_bitpos, intr_offset;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (intr_status) {
		intr_bitpos = find_lsb_set(intr_status) - 1;
		intr_status &= ~(1 << intr_bitpos);
		intr_offset = isr_base_offset + intr_bitpos;
		_sw_isr_table[intr_offset].isr(
			_sw_isr_table[intr_offset].arg);
	}
}

static void uwp_ictl_isr(void *arg)
{
	struct device *dev = arg;

	struct uwp_ictl_config *config = DEV_CFG(dev);

	volatile struct uwp_intc *intc = INTC_STRUCT(dev);

	uwp_dispatch_child_isrs(uwp_intc_status(intc),
			config->isr_table_offset);
}

static inline void uwp_ictl_irq_enable(struct device *dev, unsigned int irq)
{
	volatile struct uwp_intc *intc = INTC_STRUCT(dev);

	uwp_intc_enable(intc, irq);
}

static inline void uwp_ictl_irq_disable(struct device *dev, unsigned int irq)
{
	volatile struct uwp_intc *intc = INTC_STRUCT(dev);

	uwp_intc_disable(intc, irq);
}

static inline uint32_t uwp_ictl_irq_get_state(struct device *dev)
{
	volatile struct uwp_intc *intc = INTC_STRUCT(dev);

	return uwp_intc_status(intc);
}

static const struct irq_next_level_api uwp_ictl_apis = {
	.intr_enable = uwp_ictl_irq_enable,
	.intr_disable = uwp_ictl_irq_disable,
	.intr_get_state = uwp_ictl_irq_get_state,
};

#ifdef CONFIG_UWP_ICTL_0
static void uwp_config_0_irq(struct device *dev);
static struct device DEVICE_NAME_GET(uwp_ictl_0);
static struct uwp_ictl_config uwp_ictl_0_config = {
	.irq_num = DT_IRQN(INTC(0)),
	.isr_table_offset = CONFIG_UWP_ICTL_0_OFFSET,
	.config_func = uwp_config_0_irq,
};

static struct uwp_ictl_data uwp_ictl_0_data = {
	.base_addr = DT_REG_ADDR(INTC(0)),
};

static void uwp_config_0_irq(struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(INTC(0)), DT_IRQ(INTC(0), priority),
		    uwp_ictl_isr, DEVICE_GET(uwp_ictl_0), 0);
	irq_enable(DT_IRQN(INTC(0)));
}

static int uwp_ictl_0_init(struct device *dev)
{
	struct uwp_ictl_config *config = DEV_CFG(dev);

	uwp_sys_enable(BIT(APB_EB_INTC));
	uwp_sys_reset(BIT(APB_EB_INTC));

	config->config_func(dev);

	return 0;
}

DEVICE_AND_API_INIT(uwp_ictl_0, DT_INST_LABEL(0),
		    uwp_ictl_0_init, &uwp_ictl_0_data, &uwp_ictl_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&uwp_ictl_apis);
#endif /* CONFIG_UWP_ICTL_0 */

#ifdef CONFIG_UWP_ICTL_1
static void uwp_config_1_irq(struct device *dev);
static struct device DEVICE_NAME_GET(uwp_ictl_1);
static struct uwp_ictl_config uwp_ictl_1_config = {
	.irq_num = DT_IRQN(INTC(1)),
	.isr_table_offset = CONFIG_UWP_ICTL_1_OFFSET,
	.config_func = uwp_config_1_irq,
};

static struct uwp_ictl_data uwp_ictl_1_data = {
	.base_addr = DT_REG_ADDR(INTC(1)),
};

static void uwp_config_1_irq(struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(INTC(1)), DT_IRQ(INTC(1), priority),
		    uwp_ictl_isr, DEVICE_GET(uwp_ictl_1), 0);
	irq_enable(DT_IRQN(INTC(1)));
}

static int uwp_ictl_1_init(struct device *dev)
{
	struct uwp_ictl_config *config = DEV_CFG(dev);

	uwp_sys_enable(BIT(APB_EB_INTC));
	uwp_sys_reset(BIT(APB_EB_INTC));

	config->config_func(dev);

	return 0;
}

DEVICE_AND_API_INIT(uwp_ictl_1, DT_INST_LABEL(1),
		    uwp_ictl_1_init, &uwp_ictl_1_data, &uwp_ictl_1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&uwp_ictl_apis);
#endif /* CONFIG_UWP_ICTL_1 */

#ifdef CONFIG_UWP_ICTL_2
static void uwp_config_2_irq(struct device *dev);
static struct device DEVICE_NAME_GET(uwp_ictl_2);
static struct uwp_ictl_config uwp_ictl_2_config = {
	.irq_num = DT_IRQN(DT_NODELABEL(aon_intc)),
	.isr_table_offset = CONFIG_UWP_ICTL_2_OFFSET,
	.config_func = uwp_config_2_irq,
};

static struct uwp_ictl_data uwp_ictl_2_data = {
	.base_addr = DT_REG_ADDR(DT_NODELABEL(aon_intc)),
};

static void uwp_config_2_irq(struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(aon_intc)),
		    DT_IRQ(DT_NODELABEL(aon_intc), priority),
		    uwp_ictl_isr, DEVICE_GET(uwp_ictl_2), 0);
	irq_enable(DT_IRQN(DT_NODELABEL(aon_intc)));
}

static int uwp_ictl_2_init(struct device *dev)
{
	struct uwp_ictl_config *config = DEV_CFG(dev);

	uwp_aon_enable(BIT(AON_EB_INTC));
	uwp_aon_reset(BIT(AON_RST_INTC));

	config->config_func(dev);

	return 0;
}

DEVICE_AND_API_INIT(uwp_ictl_2, DT_INST_LABEL(2),
		    uwp_ictl_2_init, &uwp_ictl_2_data, &uwp_ictl_2_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&uwp_ictl_apis);
#endif /* CONFIG_UWP_ICTL_2 */
