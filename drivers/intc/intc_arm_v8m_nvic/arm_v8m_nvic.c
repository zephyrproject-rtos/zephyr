/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/intc.h>
#include <zephyr/drivers/intc/vector_table.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

#define DT_DRV_COMPAT arm_v8m_nvic

struct intc_arm_v8m_nvic_data {
	struct k_spinlock lock;
};

struct intc_arm_v8m_nvic_config {
	uint8_t max_irq_priority;
	uint16_t interrupt_lines;
};

typedef void (*intc_arm_v8m_nvic_vector_t)(void);

#ifdef CONFIG_TRACING_ISR
extern void sys_trace_isr_enter(void);
extern void sys_trace_isr_exit(void);
#endif

#ifdef CONFIG_PM
extern void _arch_isr_direct_pm(void);
#endif

extern void z_arm_int_exit(void);

static inline void arm_v8m_nvic_vector_enter(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif
}

static inline void arm_v8m_nvic_vector_exit(void)
{
#ifdef CONFIG_PM
	_arch_isr_direct_pm(void);
#endif

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif

	z_arm_int_exit();
}

static int arm_v8m_nvic_configure_irq(const struct device *dev, uint16_t irq, uint32_t flags)
{
	struct intc_arm_v8m_nvic_data *data = dev->data;
	const struct intc_arm_v8m_nvic_config *config = dev->config;

	if (irq >= config->interrupt_lines) {
		return -ENODEV;
	}

	if (flags > config->max_irq_priority) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		NVIC_DisableIRQ(irq);
		NVIC_ClearPendingIRQ(irq);
		NVIC_SetPriority(irq, flags);
	}

	return 0;
}

static int arm_v8m_nvic_enable_irq(const struct device *dev, uint16_t irq)
{
	struct intc_arm_v8m_nvic_data *data = dev->data;
	const struct intc_arm_v8m_nvic_config *config = dev->config;

	if (irq >= config->interrupt_lines) {
		return -ENODEV;
	}

	K_SPINLOCK(&data->lock) {
		NVIC_EnableIRQ(irq);
	}

	return 0;
}

static int arm_v8m_nvic_disable_irq(const struct device *dev, uint16_t irq)
{
	struct intc_arm_v8m_nvic_data *data = dev->data;
	const struct intc_arm_v8m_nvic_config *config = dev->config;
	int ret = -ENODEV;

	if (irq >= config->interrupt_lines) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		ret = NVIC_GetEnableIRQ(irq);
		NVIC_DisableIRQ(irq);
	}

	return ret;
}

static int arm_v8m_nvic_trigger_irq(const struct device *dev, uint16_t irq)
{
	struct intc_arm_v8m_nvic_data *data = dev->data;
	const struct intc_arm_v8m_nvic_config *config = dev->config;

	if (irq >= config->interrupt_lines) {
		return -ENODEV;
	}

	K_SPINLOCK(&data->lock) {
		NVIC_SetPendingIRQ(irq);
	}

	return 0;
}

static int arm_v8m_nvic_clear_irq(const struct device *dev, uint16_t irq)
{
	struct intc_arm_v8m_nvic_data *data = dev->data;
	const struct intc_arm_v8m_nvic_config *config = dev->config;
	int ret = -ENODEV;

	if (irq >= config->interrupt_lines) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		ret = NVIC_GetPendingIRQ(irq);
		NVIC_ClearPendingIRQ(irq);
	}

	return ret;
}

static const struct intc_driver_api api = {
	.configure_irq = arm_v8m_nvic_configure_irq,
	.enable_irq = arm_v8m_nvic_enable_irq,
	.disable_irq = arm_v8m_nvic_disable_irq,
	.trigger_irq = arm_v8m_nvic_trigger_irq,
	.clear_irq = arm_v8m_nvic_clear_irq,
};

#define ARM_V8M_NVIC_VECTOR_DEFINE(intln, _)							\
	__attribute__((interrupt("IRQ")))							\
	__weak void INTC_DT_INST_VECTOR_SYMBOL(0, intln)(void)					\
	{											\
		arm_v8m_nvic_vector_enter();							\
		INTC_DT_INST_INTL_HANDLER_SYMBOL(0, intln)();					\
		arm_v8m_nvic_vector_exit();							\
	}

#define ARM_V8M_NVIC_VECTORS_DEFINE(intlns)							\
	_Pragma("GCC diagnostic push")								\
	_Pragma("GCC diagnostic ignored \"-Wattributes\"")					\
	LISTIFY(intlns, ARM_V8M_NVIC_VECTOR_DEFINE, (), _)					\
	_Pragma("GCC diagnostic pop")

#define ARM_V8M_NVIC_VECTOR_TABLE_ENTRY_DEFINE(intln, _) \
	INTC_DT_VECTOR_SYMBOL(DT_DRV_INST(0), intln)

#define ARM_V8M_NVIC_VECTOR_TABLE_ENTRIES_DEFINE(n) \
	LISTIFY(n, ARM_V8M_NVIC_VECTOR_TABLE_ENTRY_DEFINE, (,), _)

ARM_V8M_NVIC_VECTORS_DEFINE(DT_INST_PROP(0, interrupt_lines))

intc_arm_v8m_nvic_vector_t __irq_vector_table _irq_vector_table[] = {
	ARM_V8M_NVIC_VECTOR_TABLE_ENTRIES_DEFINE(DT_INST_PROP(0, interrupt_lines))
};

static struct intc_arm_v8m_nvic_data data;
static struct intc_arm_v8m_nvic_config config = {
	.max_irq_priority = BIT_MASK(DT_INST_PROP(0, arm_num_irq_priority_bits)),
	.interrupt_lines = DT_INST_PROP(0, interrupt_lines),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &data, &config, PRE_KERNEL_1, 0, &api);
