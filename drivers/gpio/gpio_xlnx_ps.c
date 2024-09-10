/*
 * Xilinx Processor System MIO / EMIO GPIO controller driver
 * Parent (IRQ handler) module
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_xlnx_ps.h"
#include "gpio_xlnx_ps_bank.h"

#define LOG_MODULE_NAME gpio_xlnx_ps
#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT xlnx_ps_gpio

#define DEV_CFG(_dev) ((const struct gpio_xlnx_ps_dev_cfg *)(_dev)->config)
#define DEV_DATA(_dev) ((struct gpio_xlnx_ps_dev_data *const)(_dev)->data)

/*
 * An API is required for this driver, but as no pin access is provided at
 * this level, use the default API contents provided by the driver subsystem.
 */
static DEVICE_API(gpio, gpio_xlnx_ps_default_apis);

/**
 * @brief Initialize a Xilinx PS GPIO controller parent device
 *
 * Initialize a Xilinx PS GPIO controller parent device, whose task it is
 * to handle the IRQ line of each controller instance, while the configuration,
 * status and data acquisition of each MIO / EMIO GPIO pin associated with
 * the parent controller instance is handled via the respective GPIO pin
 * bank's child device.
 *
 * @param dev Pointer to the PS GPIO controller's device.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_init(const struct device *dev)
{
	const struct gpio_xlnx_ps_dev_cfg *dev_conf = DEV_CFG(dev);
	struct gpio_xlnx_ps_dev_data *dev_data = DEV_DATA(dev);
	uint32_t bank;

	/* Perform the actual memory map operation in the parent device */
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	dev_data->base = DEVICE_MMIO_NAMED_GET(dev, reg_base);
	__ASSERT(dev_data->base != 0, "%s map register space failed", dev->name);

	/* Propagate the virtual base address to the bank devices */
	for (bank = 0; bank < dev_conf->num_banks; bank++) {
		struct gpio_xlnx_ps_bank_dev_data *bank_data =
			dev_conf->bank_devices[bank]->data;
		__ASSERT(bank_data != NULL, "%s bank %u data unresolved", dev->name, bank);
		bank_data->base = dev_data->base;
	}

	/* Initialize the device's interrupt */
	dev_conf->config_func(dev);

	return 0;
}

/**
 * @brief Xilinx PS GPIO controller parent device ISR
 *
 * Interrupt service routine for the Xilinx PS GPIO controller's
 * IRQ. The ISR iterates all associated MIO / EMIO GPIO pink bank
 * child device instances and checks each bank's interrupt status.
 * If any pending interrupt is detected within a GPIO pin bank,
 * the callbacks registered for the respective bank are triggered
 * using the functionality provided by the GPIO sub-system.
 *
 * @param dev Pointer to the PS GPIO controller's device.
 */
static void gpio_xlnx_ps_isr(const struct device *dev)
{
	const struct gpio_xlnx_ps_dev_cfg *dev_conf = DEV_CFG(dev);

	const struct gpio_driver_api *api;
	struct gpio_xlnx_ps_bank_dev_data *bank_data;

	uint32_t bank;
	uint32_t int_mask;

	for (bank = 0; bank < dev_conf->num_banks; bank++) {
		api = dev_conf->bank_devices[bank]->api;
		int_mask = 0;

		if (api != NULL) {
			int_mask = api->get_pending_int(dev_conf->bank_devices[bank]);
		}
		if (int_mask) {
			bank_data = (struct gpio_xlnx_ps_bank_dev_data *)
				dev_conf->bank_devices[bank]->data;
			gpio_fire_callbacks(&bank_data->callbacks,
				dev_conf->bank_devices[bank], int_mask);
		}
	}
}

/* Device definition macros */

/*
 * Macros generating a list of all associated GPIO pin bank child
 * devices for the parent controller device's config data struct
 * specified in the device tree.
 */

#define GPIO_XLNX_PS_GEN_BANK_ARRAY(idx)\
static const struct device *const gpio_xlnx_ps##idx##_banks[] = {\
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(idx, DEVICE_DT_GET, (,))\
};

/* Device config & run-time data struct creation macros */
#define GPIO_XLNX_PS_DEV_DATA(idx)\
static struct gpio_xlnx_ps_dev_data gpio_xlnx_ps##idx##_data = {\
	.base = 0x0,\
};

#define GPIO_XLNX_PS_DEV_CONFIG(idx)\
static const struct gpio_xlnx_ps_dev_cfg gpio_xlnx_ps##idx##_cfg = {\
	DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(idx)),\
	.bank_devices = gpio_xlnx_ps##idx##_banks,\
	.num_banks = ARRAY_SIZE(gpio_xlnx_ps##idx##_banks),\
	.config_func = gpio_xlnx_ps##idx##_irq_config\
};

/*
 * Macro used to generate each parent controller device's IRQ attach
 * function.
 */
#define GPIO_XLNX_PS_DEV_CONFIG_IRQ_FUNC(idx)\
static void gpio_xlnx_ps##idx##_irq_config(const struct device *dev)\
{\
	ARG_UNUSED(dev);\
	IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),\
		    gpio_xlnx_ps_isr, DEVICE_DT_INST_GET(idx), 0);\
	irq_enable(DT_INST_IRQN(idx));\
}

/* Device definition macro */
#define GPIO_XLNX_PS_DEV_DEFINE(idx)\
DEVICE_DT_INST_DEFINE(idx, gpio_xlnx_ps_init, NULL,\
	&gpio_xlnx_ps##idx##_data, &gpio_xlnx_ps##idx##_cfg,\
	PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_xlnx_ps_default_apis);

/*
 * Top-level device initialization macro, executed for each PS GPIO
 * parent device entry contained in the device tree which has status
 * "okay".
 */
#define GPIO_XLNX_PS_DEV_INITITALIZE(idx)\
GPIO_XLNX_PS_GEN_BANK_ARRAY(idx)\
GPIO_XLNX_PS_DEV_CONFIG_IRQ_FUNC(idx)\
GPIO_XLNX_PS_DEV_DATA(idx)\
GPIO_XLNX_PS_DEV_CONFIG(idx)\
GPIO_XLNX_PS_DEV_DEFINE(idx)

/*
 * Register & initialize all instances of the Processor System's MIO / EMIO GPIO
 * controller specified in the device tree.
 */
DT_INST_FOREACH_STATUS_OKAY(GPIO_XLNX_PS_DEV_INITITALIZE);
