/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 Interrupt Unit device driver
 *
 * The ARCv2 interrupt unit has 16 allocated exceptions associated with
 * vectors 0 to 15 and 240 interrupts associated with vectors 16 to 255.
 * The interrupt unit is optional in the ARCv2-based processors. When
 * building a processor, you can configure the processor to include an
 * interrupt unit. The ARCv2 interrupt unit is highly programmable.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <init.h>

extern void *_VectorTable;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#include <power/power.h>
#include <kernel_structs.h>

#ifdef CONFIG_ARC_SECURE_FIRMWARE
#undef _ARC_V2_IRQ_VECT_BASE
#define _ARC_V2_IRQ_VECT_BASE _ARC_V2_IRQ_VECT_BASE_S
#endif

static uint32_t _arc_v2_irq_unit_device_power_state = DEVICE_PM_ACTIVE_STATE;
struct arc_v2_irq_unit_ctx {
	uint32_t irq_ctrl; /* Interrupt Context Saving Control Register. */
	uint32_t irq_vect_base; /* Interrupt Vector Base. */

	/*
	 * IRQ configuration:
	 * - IRQ Priority:BIT(6):BIT(2)
	 * - IRQ Trigger:BIT(1)
	 * - IRQ Enable:BIT(0)
	 */
	uint8_t irq_config[CONFIG_NUM_IRQS - 16];
};
static struct arc_v2_irq_unit_ctx ctx;
#endif

/*
 * @brief Initialize the interrupt unit device driver
 *
 * Initializes the interrupt unit device driver and the device
 * itself.
 *
 * Interrupts are still locked at this point, so there is no need to protect
 * the window between a write to IRQ_SELECT and subsequent writes to the
 * selected IRQ's registers.
 *
 * @return 0 for success
 */
static int arc_v2_irq_unit_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	int irq; /* the interrupt index */

	/* Interrupts from 0 to 15 are exceptions and they are ignored
	 * by IRQ auxiliary registers. For that reason we skip those
	 * values in this loop.
	 */
	for (irq = 16; irq < CONFIG_NUM_IRQS; irq++) {

		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
#ifdef CONFIG_ARC_SECURE_FIRMWARE
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
			 (CONFIG_NUM_IRQ_PRIO_LEVELS-1) |
			 _ARC_V2_IRQ_PRIORITY_SECURE); /* lowest priority */
#else
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
			 (CONFIG_NUM_IRQ_PRIO_LEVELS-1)); /* lowest priority */
#endif
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, _ARC_V2_INT_DISABLE);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	}

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

/*
 * @brief Suspend the interrupt unit device driver
 *
 * Suspends the interrupt unit device driver and the device
 * itself.
 *
 * @return 0 for success
 */
static int arc_v2_irq_unit_suspend(const struct device *dev)
{
	uint8_t irq;

	ARG_UNUSED(dev);

	/* Interrupts from 0 to 15 are exceptions and they are ignored
	 * by IRQ auxiliary registers. For that reason we skip those
	 * values in this loop.
	 */
	for (irq = 16U; irq < CONFIG_NUM_IRQS; irq++) {
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
		ctx.irq_config[irq - 16] =
			z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY) << 2;
		ctx.irq_config[irq - 16] |=
			z_arc_v2_aux_reg_read(_ARC_V2_IRQ_TRIGGER) << 1;
		ctx.irq_config[irq - 16] |=
			z_arc_v2_aux_reg_read(_ARC_V2_IRQ_ENABLE);
	}

	ctx.irq_ctrl = z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_CTRL);
	ctx.irq_vect_base = z_arc_v2_aux_reg_read(_ARC_V2_IRQ_VECT_BASE);

	_arc_v2_irq_unit_device_power_state = DEVICE_PM_SUSPEND_STATE;

	return 0;
}

/*
 * @brief Resume the interrupt unit device driver
 *
 * Resume the interrupt unit device driver and the device
 * itself.
 *
 * @return 0 for success
 */
static int arc_v2_irq_unit_resume(const struct device *dev)
{
	uint8_t irq;

	ARG_UNUSED(dev);

	/* Interrupts from 0 to 15 are exceptions and they are ignored
	 * by IRQ auxiliary registers. For that reason we skip those
	 * values in this loop.
	 */
	for (irq = 16U; irq < CONFIG_NUM_IRQS; irq++) {
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
#ifdef CONFIG_ARC_SECURE_FIRMWARE
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
				ctx.irq_config[irq - 16] >> 2 |
				_ARC_V2_IRQ_PRIORITY_SECURE);
#else
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
				ctx.irq_config[irq - 16] >> 2);
#endif
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER,
				(ctx.irq_config[irq - 16] >> 1) & BIT(0));
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE,
				ctx.irq_config[irq - 16] & BIT(0));
	}

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	/* \todo use sjli instruction to access irq_ctrl */
#else
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_CTRL, ctx.irq_ctrl);
#endif
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_VECT_BASE, ctx.irq_vect_base);

	_arc_v2_irq_unit_device_power_state = DEVICE_PM_ACTIVE_STATE;

	return 0;
}

/*
 * @brief Get the power state of interrupt unit
 *
 * @return the power state of interrupt unit
 */
static int arc_v2_irq_unit_get_state(const struct device *dev)
{
	ARG_UNUSED(dev);

	return _arc_v2_irq_unit_device_power_state;
}

/*
 * @brief  Implement the driver control of interrupt unit
 *
 * The operation on interrupt unit requires interrupt lock.
 * The *context may include IN data or/and OUT data
 *
 * @return operation result
 */
static int arc_v2_irq_unit_device_ctrl(const struct device *device,
				       uint32_t ctrl_command, void *context,
				       device_pm_cb cb, void *arg)
{
	int ret = 0;
	unsigned int key = arch_irq_lock();

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			ret = arc_v2_irq_unit_suspend(device);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = arc_v2_irq_unit_resume(device);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = arc_v2_irq_unit_get_state(device);
	}

	arch_irq_unlock(key);

	if (cb) {
		cb(device, ret, context, arg);
	}

	return ret;
}

SYS_DEVICE_DEFINE("arc_v2_irq_unit", arc_v2_irq_unit_init,
		  arc_v2_irq_unit_device_ctrl, PRE_KERNEL_1,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_INIT(arc_v2_irq_unit_init, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif   /* CONFIG_DEVICE_POWER_MANAGEMENT */
