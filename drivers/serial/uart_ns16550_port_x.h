/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2019-2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a template for cmake and is not meant to be used directly!
 */

#ifdef DT_INST_@NUM@_NS16550

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(struct device *port);
#endif

static const struct uart_ns16550_device_config uart_ns16550_dev_cfg_@NUM@ = {
	.devconf.port = DT_INST_@NUM@_NS16550_BASE_ADDRESS,
	.devconf.sys_clk_freq = DT_INST_@NUM@_NS16550_CLOCK_FREQUENCY,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.devconf.irq_config_func = irq_config_func_@NUM@,
#endif

#ifdef DT_INST_@NUM@_NS16550_PCP
	.pcp = DT_INST_@NUM@_NS16550_PCP,
#endif

#if DT_INST_@NUM@_NS16550_PCIE
	.pcie = true,
	.pcie_bdf = DT_INST_@NUM@_NS16550_BASE_ADDRESS,
	.pcie_id = DT_INST_@NUM@_NS16550_SIZE,
#endif
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_@NUM@ = {
#ifdef DT_INST_@NUM@_NS16550_CURRENT_SPEED
	.uart_config.baudrate = DT_INST_@NUM@_NS16550_CURRENT_SPEED,
#endif
	.uart_config.parity = UART_CFG_PARITY_NONE,
	.uart_config.stop_bits = UART_CFG_STOP_BITS_1,
	.uart_config.data_bits = UART_CFG_DATA_BITS_8,
#if DT_INST_@NUM@_NS16550_HW_FLOW_CONTROL
	.uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS,
#else
	.uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
#endif
#ifdef DT_INST_@NUM@_NS16550_DLF
	.dlf = DT_INST_@NUM@_NS16550_DLF,
#endif
};

DEVICE_AND_API_INIT(uart_ns16550_@NUM@, DT_INST_@NUM@_NS16550_LABEL,
		    &uart_ns16550_init,
		    &uart_ns16550_dev_data_@NUM@, &uart_ns16550_dev_cfg_@NUM@,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_ns16550_driver_api);

#ifndef DT_INST_@NUM@_NS16550_IRQ_0_SENSE
#define DT_INST_@NUM@_NS16550_IRQ_0_SENSE 0
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_INST_@NUM@_NS16550_PCIE
#if DT_INST_@NUM@_NS16550_IRQ_0 == PCIE_IRQ_DETECT

	/* PCI(e) with auto IRQ detection */

	BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),
		     "NS16550 PCI auto-IRQ needs CONFIG_DYNAMIC_INTERRUPTS");

	unsigned int irq;

	irq = pcie_wired_irq(DT_INST_@NUM@_NS16550_BASE_ADDRESS);

	if (irq == PCIE_CONF_INTR_IRQ_NONE) {
		return;
	}

	irq_connect_dynamic(irq,
			    DT_INST_@NUM@_NS16550_IRQ_0_PRIORITY,
			    uart_ns16550_isr,
			    DEVICE_GET(uart_ns16550_@NUM@),
			    DT_INST_@NUM@_NS16550_IRQ_0_SENSE);

	pcie_irq_enable(DT_INST_@NUM@_NS16550_BASE_ADDRESS, irq);

#else

	/* PCI(e) with fixed or MSI IRQ */

	IRQ_CONNECT(DT_INST_@NUM@_NS16550_IRQ_0,
		    DT_INST_@NUM@_NS16550_IRQ_0_PRIORITY,
		    uart_ns16550_isr, DEVICE_GET(uart_ns16550_@NUM@),
		    DT_INST_@NUM@_NS16550_IRQ_0_SENSE);

	pcie_irq_enable(DT_INST_@NUM@_NS16550_BASE_ADDRESS,
			DT_INST_@NUM@_NS16550_IRQ_0);

#endif
#else

	/* not PCI(e) */

	IRQ_CONNECT(DT_INST_@NUM@_NS16550_IRQ_0,
		    DT_INST_@NUM@_NS16550_IRQ_0_PRIORITY,
		    uart_ns16550_isr, DEVICE_GET(uart_ns16550_@NUM@),
		    DT_INST_@NUM@_NS16550_IRQ_0_SENSE);

	irq_enable(DT_INST_@NUM@_NS16550_IRQ_0);

#endif
}
#endif

#endif
