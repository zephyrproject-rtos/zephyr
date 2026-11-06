/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_icum_grp_irq

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_icum_grp_irq.h>
#include <errno.h>
#include <bsp_api.h>

#define MAX_FACTOR_NUMBER 31

struct rx_grp_int_cfg {
	/*  Group Interrupt Request Enable Register (GENxxx)  */
	volatile uint32_t *gen;
	/* vector number */
	uint8_t vector;
	/* priority */
	uint8_t priority;
};

struct rx_grp_int_data {
	struct k_spinlock lock;
};

/* Handlers for group interrupts */
static void handler_group(const struct device *dev)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	uint32_t prev = g_current_isr_number;

	g_current_isr_number = cfg->vector;
	group_irq_isr();
	g_current_isr_number = prev;
}

int rx_icum_grp_irq_enable(const struct device *dev, uint8_t factor)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	uint32_t irq;
	uint32_t index;
	k_spinlock_key_t key;

	if (factor > MAX_FACTOR_NUMBER) {
		return -EINVAL;
	}

	index = bsp_group_irq_get_table_index((icu_event_t)cfg->vector);
	if (index >= BSP_FEATURE_GROUP_IRQ_NUM_INTERRUPTS) {
		return -EINVAL;
	}

	/* Set upper 8 bits as factor and lower 8 bits as vector number in irq */
	irq = (factor << 8) | cfg->vector;

	key = k_spin_lock(&data->lock);

	/* The HAL calls the factor callback without a NULL check */
	if (g_bsp_group_irq_callback_list[index][factor] == NULL) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	R_BSP_IrqEnable(irq);

	k_spin_unlock(&data->lock, key);

	return 0;
}

int rx_icum_grp_irq_disable(const struct device *dev, uint8_t factor)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	uint32_t irq;
	k_spinlock_key_t key;

	if (factor > MAX_FACTOR_NUMBER) {
		return -EINVAL;
	}

	/* Set upper 8 bits as factor and lower 8 bits as vector number in irq */
	irq = (factor << 8) | cfg->vector;

	key = k_spin_lock(&data->lock);

	R_BSP_IrqDisable(irq);

	k_spin_unlock(&data->lock, key);

	return 0;
}

int rx_icum_grp_irq_callback_set(const struct device *dev, uint8_t factor, void (*callback)(void),
				 void *context)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t irq;

	if ((factor > MAX_FACTOR_NUMBER) || (callback == NULL)) {
		return -EINVAL;
	}

	uint32_t index = bsp_group_irq_get_table_index((icu_event_t)cfg->vector);

	/* The HAL returns an out-of-range index when the vector is not a group interrupt */
	if (index >= BSP_FEATURE_GROUP_IRQ_NUM_INTERRUPTS) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	g_bsp_group_irq_callback_list[index][factor] = callback;

	irq = (factor << 8) | cfg->vector;
	R_BSP_IrqCfg(irq, cfg->priority, context);

	k_spin_unlock(&data->lock, key);

	return 0;
}

int rx_icum_grp_irq_gen_set(const struct device *dev, uint8_t factor, bool set)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;

	if (factor > MAX_FACTOR_NUMBER) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (set) {
		*cfg->gen |= (1U << factor);
	} else {
		*cfg->gen &= ~(1U << factor);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

#define GRP_INT_RX_INIT(index)                                                                     \
	BUILD_ASSERT(DT_INST_IRQN(index) < CONFIG_NUM_IRQS, "Invalid group interrupt vector");     \
	BUILD_ASSERT(DT_INST_IRQ(index, priority) < CONFIG_NUM_IRQ_PRIO_LEVELS,                    \
		     "Invalid group interrupt priority");                                          \
	static struct rx_grp_int_data rx_grp_int_##index##_data;                                   \
	static const struct rx_grp_int_cfg rx_grp_int_##index##_cfg = {                            \
		.vector = DT_INST_IRQN(index),                                                     \
		.priority = DT_INST_IRQ(index, priority),                                          \
		.gen = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(index, gen),                  \
	};                                                                                         \
	static int rx_grp_int_##index##_init(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), handler_group,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, rx_grp_int_##index##_init, NULL, &rx_grp_int_##index##_data,  \
			      &rx_grp_int_##index##_cfg, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(GRP_INT_RX_INIT);
