/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a template for cmake and is not meant to be used directly!
 */

static void i2c_config_@NUM@(struct device *port);

static const struct i2c_dw_rom_config i2c_config_dw_@NUM@ = {
	.config_func = i2c_config_@NUM@,

#ifdef CONFIG_I2C_DW_@NUM@_IRQ_SHARED
	.shared_irq_dev_name = DT_I2C_DW_@NUM@_IRQ_SHARED_NAME,
#endif
	.bitrate = DT_SNPS_DESIGNWARE_I2C_@NUM@_CLOCK_FREQUENCY,
};

static struct i2c_dw_dev_config i2c_@NUM@_runtime = {
	.base_address = DT_SNPS_DESIGNWARE_I2C_@NUM@_BASE_ADDRESS,
#if CONFIG_PCI
	.pci_dev.class_type = I2C_DW_@NUM@_PCI_CLASS,
	.pci_dev.bus = I2C_DW_@NUM@_PCI_BUS,
	.pci_dev.dev = I2C_DW_@NUM@_PCI_DEV,
	.pci_dev.vendor_id = I2C_DW_@NUM@_PCI_VENDOR_ID,
	.pci_dev.device_id = I2C_DW_@NUM@_PCI_DEVICE_ID,
	.pci_dev.function = I2C_DW_@NUM@_PCI_FUNCTION,
	.pci_dev.bar = I2C_DW_@NUM@_PCI_BAR,
#endif
};

DEVICE_AND_API_INIT(i2c_@NUM@, DT_SNPS_DESIGNWARE_I2C_@NUM@_LABEL,
		    &i2c_dw_initialize,
		    &i2c_@NUM@_runtime, &i2c_config_dw_@NUM@,
		    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		    &funcs);

#ifndef DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0_SENSE
#define DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0_SENSE 0
#endif
static void i2c_config_@NUM@(struct device *port)
{
#if defined(CONFIG_I2C_DW_@NUM@_IRQ_SHARED)
	const struct i2c_dw_rom_config * const config =
		port->config->config_info;
	struct device *shared_irq_dev;

	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	shared_irq_isr_register(shared_irq_dev, (isr_t)i2c_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#else
	IRQ_CONNECT(DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0,
		    DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0_PRIORITY,
		    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
		    DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0_SENSE);
	irq_enable(DT_SNPS_DESIGNWARE_I2C_@NUM@_IRQ_0);
#endif
}
