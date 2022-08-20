/*
 * Xilinx Processor System MIO / EMIO GPIO controller driver
 *
 * Driver private data declarations, parent (IRQ handler) module
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_H_
#define _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_H_

/* Type definitions */

/* IRQ handler function type */
typedef void (*gpio_xlnx_ps_config_irq_t)(const struct device *dev);

/**
 * @brief Run-time modifiable device data structure.
 *
 * This struct contains all data of the PS GPIO controller parent
 * (IRQ handler) which is modifiable at run-time.
 */
struct gpio_xlnx_ps_dev_data {
	struct gpio_driver_data common;
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all data of the PS GPIO controller parent
 * which is required for proper operation (such as base memory
 * addresses, references to all associated banks etc.) which don't
 * have to and therefore cannot be modified at run-time.
 */
struct gpio_xlnx_ps_dev_cfg {
	struct gpio_driver_config common;

	uint32_t base_addr;
	const struct device *const *bank_devices;
	uint32_t num_banks;
	gpio_xlnx_ps_config_irq_t config_func;
};

#endif /* _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_H_ */
