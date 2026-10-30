/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_grp_intc_fsp

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_grp_int_fsp.h>
#include <errno.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0

#define MIN_FACTOR_NUMBER 0
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

bsp_grp_irq_cb_t g_bsp_group_irq_callback_list[BSP_FEATURE_GROUP_IRQ_NUM_INTERRUPTS]
					      [BSP_FEATURE_GROUP_IRQ_NUM_FACTORS];

/* Handlers for group interrupts */
static void handler_group(const struct device *dev)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	uint32_t prev = g_current_isr_number;

	g_current_isr_number = cfg->vector;
	group_irq_isr();
	g_current_isr_number = prev;
}

/* GROUP INTERRUPT APIs */
/* Enable GEN register and IER register*/
int rx_grp_intc_fsp_enable(const struct device *dev, uint16_t factor)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	uint32_t irq;
	k_spinlock_key_t key;

	if (((factor < MIN_FACTOR_NUMBER) || (factor > MAX_FACTOR_NUMBER))) {
		return -EINVAL;
	}

	/* Set upper 8 bits as factor and lower 8 bits as vector number in irq */
	irq = (factor << 8) | cfg->vector;

	key = k_spin_lock(&data->lock);

	R_BSP_IrqEnable(irq);

	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Disable GEN register, and disable IER register if no other factors are set.*/
int rx_grp_intc_fsp_disable(const struct device *dev, uint16_t factor)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	uint32_t irq;
	k_spinlock_key_t key;

	if (((factor < MIN_FACTOR_NUMBER) || (factor > MAX_FACTOR_NUMBER))) {
		return -EINVAL;
	}

	/* Set upper 8 bits as factor and lower 8 bits as vector number in irq */
	irq = (factor << 8) | cfg->vector;

	key = k_spin_lock(&data->lock);

	R_BSP_IrqDisable(irq);

	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Set callback function.*/
int rx_grp_intc_fsp_callback_set(const struct device *dev, uint16_t factor, void (*callback)(void),
				 void *context)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t irq;

	if ((factor < MIN_FACTOR_NUMBER) || (factor > MAX_FACTOR_NUMBER) || (callback == NULL)) {
		return -EINVAL;
	}

	if (cfg->vector > CONFIG_NUM_IRQS - 1) {
		return -EINVAL;
	}

	if (cfg->priority > CONFIG_NUM_IRQ_PRIO_LEVELS - 1) {
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

int rx_grp_intc_fsp_set_gen(const struct device *dev, uint8_t factor, bool set)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;

	if (factor > 31) {
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
	static struct rx_grp_int_data rx_grp_int_##index##_data;                                   \
	static const struct rx_grp_int_cfg rx_grp_int_##index##_cfg = {                            \
		.vector = DT_INST_IRQN(index),                                                     \
		.priority = DT_INST_IRQ(index, priority),                                          \
		.gen = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(index, GEN),                  \
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

#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0 */
