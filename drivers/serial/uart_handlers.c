/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/uart.h>
#include <syscall_handler.h>

#define UART_SIMPLE(op_) \
	static inline int z_vrfy_uart_##op_(struct device *dev) \
	{							\
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		return z_impl_uart_ ## op_(dev); \
	}

#define UART_SIMPLE_VOID(op_) \
	static inline void z_vrfy_uart_##op_(struct device *dev) \
	{							 \
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		z_impl_uart_ ## op_(dev); \
	}

UART_SIMPLE(err_check)
#include <syscalls/uart_err_check_mrsh.c>

static inline int z_vrfy_uart_poll_in(struct device *dev,
				      unsigned char *p_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_in));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(p_char, sizeof(unsigned char)));
	return z_impl_uart_poll_in(dev, p_char);
}
#include <syscalls/uart_poll_in_mrsh.c>

static inline void z_vrfy_uart_poll_out(struct device *dev,
					unsigned char out_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_out));
	z_impl_uart_poll_out((struct device *)dev, out_char);
}
#include <syscalls/uart_poll_out_mrsh.c>


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
UART_SIMPLE_VOID(irq_tx_enable)
UART_SIMPLE_VOID(irq_tx_disable)
UART_SIMPLE_VOID(irq_rx_enable)
UART_SIMPLE_VOID(irq_rx_disable)
UART_SIMPLE_VOID(irq_err_enable)
UART_SIMPLE_VOID(irq_err_disable)
UART_SIMPLE(irq_is_pending)
UART_SIMPLE(irq_update)
#include <syscalls/uart_irq_tx_enable_mrsh.c>
#include <syscalls/uart_irq_tx_disable_mrsh.c>
#include <syscalls/uart_irq_rx_enable_mrsh.c>
#include <syscalls/uart_irq_rx_disable_mrsh.c>
#include <syscalls/uart_irq_err_enable_mrsh.c>
#include <syscalls/uart_irq_err_disable_mrsh.c>
#include <syscalls/uart_irq_is_pending_mrsh.c>
#include <syscalls/uart_irq_update_mrsh.c>
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
static inline int z_vrfy_uart_line_ctrl_set(struct device *dev,
					    u32_t ctrl, u32_t val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_set));
	return z_impl_uart_line_ctrl_set((struct device *)dev, ctrl, val);
}
#include <syscalls/uart_line_ctrl_set_mrsh.c>

static inline int z_vrfy_uart_line_ctrl_get(struct device *dev,
					    u32_t ctrl, u32_t *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(u32_t)));
	return z_impl_uart_line_ctrl_get((struct device *)dev, ctrl,
					(u32_t *)val);
}
#include <syscalls/uart_line_ctrl_get_mrsh.c>
#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
static inline int z_vrfy_uart_drv_cmd(struct device *dev, u32_t cmd, u32_t p)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, drv_cmd));
	return z_impl_uart_drv_cmd((struct device *)dev, cmd, p);
}
#include <syscalls/uart_drv_cmd_mrsh.c>
#endif /* CONFIG_UART_DRV_CMD */
