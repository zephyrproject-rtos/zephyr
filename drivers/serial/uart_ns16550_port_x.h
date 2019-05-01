/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a template for cmake and is not meant to be used directly!
 */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(struct device *port);
#endif

static const struct uart_ns16550_device_config uart_ns16550_dev_cfg_@NUM@ = {
	.sys_clk_freq = DT_UART_NS16550_PORT_@NUM@_CLK_FREQ,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_@NUM@,
#endif

#ifdef DT_UART_NS16550_PORT_@NUM@_PCP
	.pcp = DT_UART_NS16550_PORT_@NUM@_PCP,
#endif

#if DT_UART_NS16550_PORT_@NUM@_PCIE
	.pcie = true,
	.pcie_bdf = DT_UART_NS16550_PORT_@NUM@_BASE_ADDR,
	.pcie_id = DT_UART_NS16550_PORT_@NUM@_SIZE,
#endif
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_@NUM@ = {
	.port = DT_UART_NS16550_PORT_@NUM@_BASE_ADDR,
	.baud_rate = DT_UART_NS16550_PORT_@NUM@_BAUD_RATE,
	.options = CONFIG_UART_NS16550_PORT_@NUM@_OPTIONS,

#ifdef DT_UART_NS16550_PORT_@NUM@_DLF
	.dlf = DT_UART_NS16550_PORT_@NUM@_DLF,
#endif
};

DEVICE_AND_API_INIT(uart_ns16550_@NUM@, DT_UART_NS16550_PORT_@NUM@_NAME,
		    &uart_ns16550_init,
		    &uart_ns16550_dev_data_@NUM@, &uart_ns16550_dev_cfg_@NUM@,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_ns16550_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_UART_NS16550_PORT_@NUM@_IRQ,
		    DT_UART_NS16550_PORT_@NUM@_IRQ_PRI,
		    uart_ns16550_isr, DEVICE_GET(uart_ns16550_@NUM@),
		    DT_UART_NS16550_PORT_@NUM@_IRQ_FLAGS);

#ifdef UART_NS16550_PCIE_ENABLED
	if (DEV_CFG(dev)->pcie) {
		pcie_irq_enable(DT_UART_NS16550_PORT_@NUM@_BASE_ADDR,
				DT_UART_NS16550_PORT_@NUM@_IRQ);
	} else {
		irq_enable(DT_UART_NS16550_PORT_@NUM@_IRQ);
	}
#else
	irq_enable(DT_UART_NS16550_PORT_@NUM@_IRQ);
#endif
}
#endif
