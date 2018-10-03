/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <uart.h>
#include <syscall_handler.h>

#define UART_SIMPLE(op_) \
	Z_SYSCALL_HANDLER(uart_ ## op_, dev) { \
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		return _impl_uart_ ## op_((struct device *)dev); \
	}

#define UART_SIMPLE_VOID(op_) \
	Z_SYSCALL_HANDLER(uart_ ## op_, dev) { \
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		_impl_uart_ ## op_((struct device *)dev); \
		return 0; \
	}

UART_SIMPLE(err_check)

Z_SYSCALL_HANDLER(uart_poll_in, dev, p_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_in));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(p_char, sizeof(unsigned char)));
	return _impl_uart_poll_in((struct device *)dev,
				  (unsigned char *)p_char);
}

Z_SYSCALL_HANDLER(uart_poll_out, dev, out_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_out));
	return _impl_uart_poll_out((struct device *)dev, out_char);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
UART_SIMPLE_VOID(irq_tx_enable)
UART_SIMPLE_VOID(irq_tx_disable)
UART_SIMPLE_VOID(irq_rx_enable)
UART_SIMPLE_VOID(irq_rx_disable)
UART_SIMPLE_VOID(irq_err_enable)
UART_SIMPLE_VOID(irq_err_disable)
UART_SIMPLE(irq_is_pending)
UART_SIMPLE(irq_update)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
Z_SYSCALL_HANDLER(uart_line_ctrl_set, dev, ctrl, val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_set));
	return _impl_uart_line_ctrl_set((struct device *)dev, ctrl, val);
}

Z_SYSCALL_HANDLER(uart_line_ctrl_get, dev, ctrl, val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(u32_t)));
	return _impl_uart_line_ctrl_get((struct device *)dev, ctrl,
					(u32_t *)val);
}
#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
Z_SYSCALL_HANDLER(uart_drv_cmd, dev, cmd, p)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, drv_cmd));
	return _impl_uart_drv_cmd((struct device *)dev, cmd, p);
}
#endif /* CONFIG_UART_DRV_CMD */
