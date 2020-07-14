/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a template for cmake and is not meant to be used directly!
 */

static void i2c_config_@NUM@(const struct device *port);

static const struct i2c_dw_rom_config i2c_config_dw_@NUM@ = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(@NUM@)),
	.config_func = i2c_config_@NUM@,
	.bitrate = DT_INST_PROP(@NUM@, clock_frequency),

#if DT_INST_PROP(@NUM@, pcie)
	.pcie = true,
	.pcie_bdf = DT_INST_REG_ADDR(@NUM@),
	.pcie_id = DT_INST_REG_SIZE(@NUM@),
#endif
};

static struct i2c_dw_dev_config i2c_@NUM@_runtime;

DEVICE_AND_API_INIT(i2c_@NUM@, DT_INST_LABEL(@NUM@),
		    &i2c_dw_initialize,
		    &i2c_@NUM@_runtime, &i2c_config_dw_@NUM@,
		    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		    &funcs);

#if DT_INST_IRQ_HAS_CELL(@NUM@, sense)
#define INST_@NUM@_IRQ_FLAGS  DT_INST_IRQ(@NUM@, sense)
#else
#define INST_@NUM@_IRQ_FLAGS  0
#endif
static void i2c_config_@NUM@(const struct device *port)
{
	ARG_UNUSED(port);

#if DT_INST_PROP(@NUM@, pcie)
#if DT_INST_IRQN(@NUM@) == PCIE_IRQ_DETECT

	/* PCI(e) with auto IRQ detection */

	BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),
		     "DW I2C PCI auto-IRQ needs CONFIG_DYNAMIC_INTERRUPTS");

	unsigned int irq;

	irq = pcie_wired_irq(DT_INST_REG_ADDR(@NUM@));

	if (irq == PCIE_CONF_INTR_IRQ_NONE) {
		return;
	}

	irq_connect_dynamic(irq,
			    DT_INST_IRQ(@NUM@, priority),
			    (void (*)(const void *))i2c_dw_isr,
			    DEVICE_GET(i2c_@NUM@), INST_@NUM@_IRQ_FLAGS);
	pcie_irq_enable(DT_INST_REG_ADDR(@NUM@), irq);

#else

	/* PCI(e) with fixed or MSI IRQ */

	IRQ_CONNECT(DT_INST_IRQN(@NUM@),
		    DT_INST_IRQ(@NUM@, priority),
		    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
		    INST_@NUM@_IRQ_FLAGS);
	pcie_irq_enable(DT_INST_REG_ADDR(@NUM@),
			DT_INST_IRQN(@NUM@));

#endif
#else

	/* not PCI(e) */

	IRQ_CONNECT(DT_INST_IRQN(@NUM@),
		    DT_INST_IRQ(@NUM@, priority),
		    i2c_dw_isr, DEVICE_GET(i2c_@NUM@),
		    INST_@NUM@_IRQ_FLAGS);
	irq_enable(DT_INST_IRQN(@NUM@));

#endif
}
