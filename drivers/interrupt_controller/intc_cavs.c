/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_cavs_intc

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/sw_isr_table.h>
#include "intc_cavs.h"

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
#if defined(CONFIG_SOC_INTEL_CAVS_V25)
#define PER_CPU_OFFSET(x)	(0x40 * x)
#else
#error "Must define PER_CPU_OFFSET(x) for SoC"
#endif
#else
#define PER_CPU_OFFSET(x)	0
#endif

static ALWAYS_INLINE
struct cavs_registers *get_base_address(struct cavs_ictl_runtime *context)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	return UINT_TO_POINTER(context->base_addr +
			       PER_CPU_OFFSET(arch_curr_cpu()->id));
#else
	return UINT_TO_POINTER(context->base_addr);
#endif
}

static ALWAYS_INLINE void cavs_ictl_dispatch_child_isrs(uint32_t intr_status,
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

static void cavs_ictl_isr(const struct device *port)
{
	struct cavs_ictl_runtime *context = port->data;

	const struct cavs_ictl_config *config = port->config;

	volatile struct cavs_registers * const regs =  get_base_address(context);

	cavs_ictl_dispatch_child_isrs(regs->status_il,
				      config->isr_table_offset);
}

static void cavs_ictl_irq_enable(const struct device *dev,
					unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->data;

	volatile struct cavs_registers * const regs = get_base_address(context);

	regs->enable_il = 1 << irq;
}

static void cavs_ictl_irq_disable(const struct device *dev,
					 unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->data;

	volatile struct cavs_registers * const regs = get_base_address(context);

	regs->disable_il = 1 << irq;
}

static unsigned int cavs_ictl_irq_get_state(const struct device *dev)
{
	struct cavs_ictl_runtime *context = dev->data;

	volatile struct cavs_registers * const regs = get_base_address(context);

	/* When the bits of this register are set, it means the
	 * corresponding interrupts are disabled. This function
	 * returns 0 only if ALL the interrupts are disabled.
	 */
	return regs->disable_state_il != 0xFFFFFFFF;
}

static int cavs_ictl_irq_get_line_state(const struct device *dev,
					unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->data;

	volatile struct cavs_registers * const regs = get_base_address(context);

	if ((regs->disable_state_il & BIT(irq)) == 0) {
		return 1;
	}

	return 0;
}

static const struct irq_next_level_api cavs_apis = {
	.intr_enable = cavs_ictl_irq_enable,
	.intr_disable = cavs_ictl_irq_disable,
	.intr_get_state = cavs_ictl_irq_get_state,
	.intr_get_line_state = cavs_ictl_irq_get_line_state,
};

#define CAVS_ICTL_INIT(n)						\
	static int cavs_ictl_##n##_initialize(const struct device *port) \
	{								\
		struct cavs_ictl_runtime *context = port->data;		\
		volatile struct cavs_registers * const regs =		\
			get_base_address(context);			\
		regs->disable_il = ~0;					\
									\
		return 0;						\
	}								\
									\
	static void cavs_config_##n##_irq(const struct device *port);	\
									\
	static const struct cavs_ictl_config cavs_config_##n = {	\
		.irq_num = DT_INST_IRQN(n),				\
		.isr_table_offset = CONFIG_CAVS_ISR_TBL_OFFSET +	\
				    CONFIG_MAX_IRQ_PER_AGGREGATOR*n,	\
		.config_func = cavs_config_##n##_irq,			\
	};								\
									\
	static struct cavs_ictl_runtime cavs_##n##_runtime = {		\
		.base_addr = DT_INST_REG_ADDR(n),			\
	};								\
	DEVICE_DT_INST_DEFINE(n,					\
			    cavs_ictl_##n##_initialize,			\
			    NULL,					\
			    &cavs_##n##_runtime, &cavs_config_##n,	\
			    PRE_KERNEL_1,				\
			    CONFIG_CAVS_ICTL_INIT_PRIORITY, &cavs_apis);\
									\
	static void cavs_config_##n##_irq(const struct device *port)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    cavs_ictl_isr, DEVICE_DT_INST_GET(n),	\
			    DT_INST_IRQ(n, sense));			\
	}								\
	IRQ_PARENT_ENTRY_DEFINE(					\
		intc_cavs_##n, DEVICE_DT_INST_GET(n), DT_INST_IRQN(n),	\
		INTC_INST_ISR_TBL_OFFSET(n),				\
		DT_INST_INTC_GET_AGGREGATOR_LEVEL(n));

DT_INST_FOREACH_STATUS_OKAY(CAVS_ICTL_INIT)
