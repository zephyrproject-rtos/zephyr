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
	.bitrate = DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_CLOCK_FREQUENCY,

#if DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_PCIE
	.pcie = true,
	.pcie_bdf = DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_BASE_ADDRESS,
	.pcie_id = DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_SIZE,
#endif
};

static struct i2c_dw_dev_config i2c_@NUM@_runtime = {
	.regs = (struct i2c_dw_registers *)
		DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_BASE_ADDRESS
};

DEVICE_AND_API_INIT(i2c_@NUM@, DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_LABEL,
		    &i2c_dw_initialize,
		    &i2c_@NUM@_runtime, &i2c_config_dw_@NUM@,
		    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		    &funcs);

#ifndef DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_SENSE
#define DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_SENSE 0
#endif
static void i2c_config_@NUM@(struct device *port)
{
	ARG_UNUSED(port);

#if DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_PCIE
#if DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0 == PCIE_IRQ_DETECT

	/* PCI(e) with auto IRQ detection */

	BUILD_ASSERT_MSG(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),
		"DW I2C PCI auto-IRQ needs CONFIG_DYNAMIC_INTERRUPTS");

	unsigned int irq;

	irq = pcie_wired_irq(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_BASE_ADDRESS);

	if (irq == PCIE_CONF_INTR_IRQ_NONE) {
		return;
	}

	irq_connect_dynamic(irq,
			    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_PRIORITY,
			    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
			    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_SENSE);
	pcie_irq_enable(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_BASE_ADDRESS, irq);

#else

	/* PCI(e) with fixed or MSI IRQ */

	IRQ_CONNECT(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0,
		    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_PRIORITY,
		    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
		    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_SENSE);
	pcie_irq_enable(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_BASE_ADDRESS,
			DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0);

#endif
#else

	/* not PCI(e) */

	IRQ_CONNECT(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0,
		    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_PRIORITY,
		    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
		    DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0_SENSE);
	irq_enable(DT_INST_@NUM@_SNPS_DESIGNWARE_I2C_IRQ_0);

#endif
}
