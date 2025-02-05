/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vm_irq.h>
#include <zephyr/zvm/vdev/pt_device.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const) \
	(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

#define PTDEV_CFG(dev) \
	((const struct pass_through_device_config * const) \
	(DEV_CFG(dev)->device_config))

/*Device init function when system bootup. */
static int pass_through_device_init(const struct device *dev)
{
	/*set device type to res.*/
	dev->state->init_res |= VM_DEVICE_INIT_RES;

	/*init the configuration for interrupt.*/
	if (PTDEV_CFG(dev)->irq_config_func) {
		PTDEV_CFG(dev)->irq_config_func(dev);
	}

	printk("PT-DEVICE: Initialized pass-through device: %s.\n", dev->name);
	return 0;
}

static int vm_ptdevice_init(const struct device *dev, struct z_vm *vm, struct z_virt_dev *vdev_desc)
{
	struct z_virt_dev *vdev;

	vdev = allocate_device_to_vm(dev, vm, vdev_desc, true, false);
	if (!vdev) {
		printk("Init virt serial device error\n");
		return -ENODEV;
	}

	if (DEV_DATA(dev)->device_data) {
		struct z_virt_dev *vdev_tmp = DEV_DATA(dev)->device_data;

		printk("Device data is not NULL, please check the device: %s\n", vdev_tmp->name);
	} else {
		DEV_DATA(dev)->device_data = vdev;
	}

	/*set special function for vm device init*/
	if (PTDEV_CFG(dev)->ptdev_spec_init_func) {
		PTDEV_CFG(dev)->ptdev_spec_init_func(vdev);
	}

	return 0;
}

static void pass_through_device_isr(const struct device *dev)
{
	/*irq handler.*/
	if (DEV_DATA(dev)->device_data) {
		vm_device_callback_func(dev, NULL, DEV_DATA(dev)->device_data);
	} else {
		printk("irq handle error, vdev is NULL, please check the device: %s\n", dev->name);
	}

	/*set special function for vm device irq route*/
	if (PTDEV_CFG(dev)->ptdev_spec_irq_func) {
		PTDEV_CFG(dev)->ptdev_spec_irq_func(dev);
	}
}

static const struct virt_device_api virt_ptdevice_api = {
	.init_fn = vm_ptdevice_init,
};


/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-------------------sample for adding pt device-------------------------*/
/*
 *static void ptdevice_irq_config_func_1(const struct device *dev)
 *{
 *	IRQ_CONNECT(DT_IRQN(DT_ALIAS(ptdevice1)),
 *			DT_IRQ(DT_ALIAS(ptdevice1), priority),
 *			pass_through_device_isr,
 *			DEVICE_DT_GET(DT_ALIAS(ptdevice1)),
 *			0);
 *	irq_enable(DT_IRQN(DT_ALIAS(ptdevice1)));
 *}
 *
 *static struct pass_through_device_config ptdevice_cfg_port_1 = {
 *	.irq_config_func = ptdevice_irq_config_func_1,
 *	.ptdev_spec_init_func = NULL,
 *	.ptdev_spec_irq_func = NULL,
 *};
 *
 *static struct virt_device_config virt_ptdevice_cfg_1 = {
 *	.reg_base = DT_REG_ADDR(DT_ALIAS(ptdevice1)),
 *	.reg_size = DT_REG_SIZE(DT_ALIAS(ptdevice1)),
 *	.hirq_num = DT_IRQN(DT_ALIAS(ptdevice1)),
 *	.device_config = &ptdevice_cfg_port_1,
 *};
 *
 *static struct virt_device_data virt_ptdevice_data_port_1 = {
 *	.device_data = NULL,
 *};
 *
 *DEVICE_DT_DEFINE(DT_ALIAS(ptdevice1),
 *			&pass_through_device_init,
 *			NULL, &virt_ptdevice_data_port_1, &virt_ptdevice_cfg_1,
 *			POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,
 *			&virt_ptdevice_api);
 */
/*------------------------cut line---------------------------------------*/
