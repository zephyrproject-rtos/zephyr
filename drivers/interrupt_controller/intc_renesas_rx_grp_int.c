/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_grp_intc

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_grp_int.h>
#include <errno.h>

extern void group_bl0_handler_isr(void);
extern void group_bl1_handler_isr(void);
extern void group_bl2_handler_isr(void);
extern void group_al0_handler_isr(void);
extern void group_al1_handler_isr(void);

#define VECT_GROUP_BL0 DT_IRQN(DT_NODELABEL(group_irq_bl0))
#define VECT_GROUP_BL1 DT_IRQN(DT_NODELABEL(group_irq_bl1))
#define VECT_GROUP_BL2 DT_IRQN(DT_NODELABEL(group_irq_bl2))
#define VECT_GROUP_AL0 DT_IRQN(DT_NODELABEL(group_irq_al0))
#define VECT_GROUP_AL1 DT_IRQN(DT_NODELABEL(group_irq_al1))

struct rx_grp_int_cfg {
	/* address of the Group Interrupt Request Enable Register (GENxxx)  */
	volatile uint32_t *gen;
	/* vector number of the interrupt */
	const uint8_t vect;
	/* priority of the interrupt */
	const uint8_t prio;
};

struct rx_grp_int_data {
	struct k_spinlock lock;
};

int rx_grp_intc_set_callback(const struct device *dev, bsp_int_src_t vector, bsp_int_cb_t callback,
			     void *context)
{
	ARG_UNUSED(dev);
	bsp_int_err_t err;

	err = R_BSP_InterruptWrite_EX(vector, callback, context);
	if (err != BSP_INT_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

int rx_grp_intc_set_grp_int(const struct device *dev, bsp_int_src_t vector, bool set)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;
	volatile bsp_int_ctrl_t group_priority;
	bsp_int_err_t err;

	group_priority.ipl = cfg->prio;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (set) {
		err = R_BSP_InterruptControl(vector, BSP_INT_CMD_GROUP_INTERRUPT_ENABLE,
					     (void *)&group_priority);
	} else {
		err = R_BSP_InterruptControl(vector, BSP_INT_CMD_GROUP_INTERRUPT_DISABLE, NULL);
	}

	if (err != BSP_INT_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

int rx_grp_intc_set_gen(const struct device *dev, uint8_t vector_num, bool set)
{
	const struct rx_grp_int_cfg *cfg = dev->config;
	struct rx_grp_int_data *data = dev->data;

	if (vector_num > 31) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (set) {
		*cfg->gen |= (1U << vector_num);
	} else {
		*cfg->gen &= ~(1U << vector_num);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rx_grp_intc_init(const struct device *dev)
{
	const struct rx_grp_int_cfg *cfg = dev->config;

	switch (cfg->vect) {
	case VECT_GROUP_BL0:
		IRQ_CONNECT(VECT_GROUP_BL0, cfg->prio, group_bl0_handler_isr, NULL, 0);
		break;
	case VECT_GROUP_BL1:
		IRQ_CONNECT(VECT_GROUP_BL1, cfg->prio, group_bl1_handler_isr, NULL, 0);
		break;
	case VECT_GROUP_BL2:
		IRQ_CONNECT(VECT_GROUP_BL2, cfg->prio, group_bl2_handler_isr, NULL, 0);
		break;
	case VECT_GROUP_AL0:
		IRQ_CONNECT(VECT_GROUP_AL0, cfg->prio, group_al0_handler_isr, NULL, 0);
		break;
	case VECT_GROUP_AL1:
		IRQ_CONNECT(VECT_GROUP_AL1, cfg->prio, group_al1_handler_isr, NULL, 0);
		break;
	default:
		/* ERROR */
		return -EINVAL;
	}

	irq_enable(cfg->vect);

	return 0;
}

#define GRP_INT_RX_INIT(index)                                                                     \
	static struct rx_grp_int_cfg rx_grp_int_##index##_cfg = {                                  \
		.gen = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(index, GEN),                  \
		.vect = DT_INST_IRQN(index),                                                       \
		.prio = DT_INST_IRQ(index, priority),                                              \
	};                                                                                         \
	static struct rx_grp_int_data rx_grp_int_##index##_data;                                   \
	static int rx_grp_int_##index##_init(const struct device *dev)                             \
	{                                                                                          \
		return rx_grp_intc_init(dev);                                                      \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, rx_grp_int_##index##_init, NULL, &rx_grp_int_##index##_data,  \
			      &rx_grp_int_##index##_cfg, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(GRP_INT_RX_INIT);
