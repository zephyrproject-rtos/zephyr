/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2019-2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a template for cmake and is not meant to be used directly!
 */

#if DT_NODE_HAS_STATUS(DT_DRV_INST(@NUM@), okay)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(const struct device *port);
#endif

static const struct uart_digi_ns16550_device_config uart_digi_ns16550_dev_cfg_@NUM@ = {
#ifdef UART_DIGI_NS16550_ACCESS_IOPORT
	.port = DT_INST_REG_ADDR(@NUM@),
#elif !DT_INST_PROP(@NUM@, pcie)
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(@NUM@)),
#endif
	.sys_clk_freq = DT_INST_PROP(@NUM@, clock_frequency),

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_@NUM@,
#endif

};

static struct uart_digi_ns16550_dev_data_t uart_digi_ns16550_dev_data_@NUM@ = {
#if DT_INST_NODE_HAS_PROP(@NUM@, current_speed)
	.uart_config.baudrate = DT_INST_PROP(@NUM@, current_speed),
#endif
	.uart_config.parity = UART_CFG_PARITY_NONE,
	.uart_config.stop_bits = UART_CFG_STOP_BITS_1,
	.uart_config.data_bits = UART_CFG_DATA_BITS_8,
#if DT_INST_PROP(@NUM@, hw_flow_control)
	.uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS,
#else
	.uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
#endif
};

DEVICE_DT_INST_DEFINE(@NUM@,
		    &uart_digi_ns16550_init,
		    device_pm_control_nop,
		    &uart_digi_ns16550_dev_data_@NUM@, &uart_digi_ns16550_dev_cfg_@NUM@,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_digi_ns16550_driver_api);

#define INST_@NUM@_IRQ_FLAGS 0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_@NUM@(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(@NUM@),
		    //DT_INST_IRQ(@NUM@, priority),
            1,  //XXX HARDCODING FOR NOW!! FIXME
		    uart_digi_ns16550_isr,
		    DEVICE_DT_INST_GET(@NUM@),
		    INST_@NUM@_IRQ_FLAGS);

	irq_enable(DT_INST_IRQN(@NUM@));

}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#else
#error " @NUM@ NOT OK"
#endif //#if DT_NODE_HAS_STATUS(DT_DRV_INST(@NUM@), okay)
