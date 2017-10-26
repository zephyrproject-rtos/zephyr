/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <uart.h>
#include <syscall_handler.h>

#define UART_SIMPLE(name_) \
	_SYSCALL_HANDLER1_SIMPLE(name_, K_OBJ_DRIVER_UART, struct device *)

#define UART_SIMPLE_VOID(name_) \
	_SYSCALL_HANDLER1_SIMPLE_VOID(name_, K_OBJ_DRIVER_UART, \
				      struct device *)

UART_SIMPLE(uart_err_check);

_SYSCALL_HANDLER(uart_poll_in, dev, p_char)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_UART);
	_SYSCALL_MEMORY_WRITE(p_char, sizeof(unsigned char));
	return _impl_uart_poll_in((struct device *)dev,
				  (unsigned char *)p_char);
}

_SYSCALL_HANDLER(uart_poll_out, dev, out_char)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_UART);
	return _impl_uart_poll_out((struct device *)dev, out_char);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
UART_SIMPLE_VOID(uart_irq_tx_enable);
UART_SIMPLE_VOID(uart_irq_tx_disable);
UART_SIMPLE_VOID(uart_irq_rx_enable);
UART_SIMPLE_VOID(uart_irq_rx_disable);
UART_SIMPLE_VOID(uart_irq_err_enable);
UART_SIMPLE_VOID(uart_irq_err_disable);
UART_SIMPLE(uart_irq_is_pending);
UART_SIMPLE(uart_irq_update);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
_SYSCALL_HANDLER(uart_line_ctrl_set, dev, ctrl, val)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_UART);
	return _impl_uart_line_ctrl_set((struct device *)dev, ctrl, val);
}

_SYSCALL_HANDLER(uart_line_ctrl_get, dev, ctrl, val);
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_UART);
	_SYSCALL_MEMORY_WRITE(val, sizeof(u32_t));
	return _impl_uart_line_ctrl_get((struct device *)dev, ctrl,
					(u32_t *)val);
}
#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
_SYSCALL_HANDLER(uart_drv_cmd, dev, cmd, p)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_UART);
	return _impl_uart_drv_cmd((struct device *)dev, cmd, p);
}
#endif /* CONFIG_UART_DRV_CMD */
