/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/syscall_handler.h>

#define UART_SIMPLE(op_) \
	static inline int z_vrfy_uart_##op_(const struct device *dev) \
	{							\
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		return z_impl_uart_ ## op_(dev); \
	}

#define UART_SIMPLE_VOID(op_) \
	static inline void z_vrfy_uart_##op_(const struct device *dev) \
	{							 \
		Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, op_)); \
		z_impl_uart_ ## op_(dev); \
	}

UART_SIMPLE(err_check)
#include <syscalls/uart_err_check_mrsh.c>

static inline int z_vrfy_uart_poll_in(const struct device *dev,
				      unsigned char *p_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_in));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(p_char, sizeof(unsigned char)));
	return z_impl_uart_poll_in(dev, p_char);
}
#include <syscalls/uart_poll_in_mrsh.c>

static inline int z_vrfy_uart_poll_in_u16(const struct device *dev,
					  uint16_t *p_u16)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_in));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(p_u16, sizeof(uint16_t)));
	return z_impl_uart_poll_in_u16(dev, p_u16);
}
#include <syscalls/uart_poll_in_u16_mrsh.c>

static inline void z_vrfy_uart_poll_out(const struct device *dev,
					unsigned char out_char)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_out));
	z_impl_uart_poll_out((const struct device *)dev, out_char);
}
#include <syscalls/uart_poll_out_mrsh.c>

static inline void z_vrfy_uart_poll_out_u16(const struct device *dev,
					    uint16_t out_u16)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, poll_out));
	z_impl_uart_poll_out_u16((const struct device *)dev, out_u16);
}
#include <syscalls/uart_poll_out_u16_mrsh.c>

static inline int z_vrfy_uart_config_get(const struct device *dev,
					 struct uart_config *cfg)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, config_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cfg, sizeof(struct uart_config)));

	return z_impl_uart_config_get(dev, cfg);
}
#include <syscalls/uart_config_get_mrsh.c>

static inline int z_vrfy_uart_configure(const struct device *dev,
					const struct uart_config *cfg)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, config_get));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(cfg, sizeof(struct uart_config)));

	return z_impl_uart_configure(dev, cfg);
}
#include <syscalls/uart_configure_mrsh.c>

#ifdef CONFIG_UART_ASYNC_API
/* callback_set() excluded as we don't allow ISR callback installation from
 * user mode
 *
 * rx_buf_rsp() excluded as it's designed to be called from ISR callbacks
 */

static inline int z_vrfy_uart_tx(const struct device *dev, const uint8_t *buf,
				 size_t len, int32_t timeout)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, tx));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, len));
	return z_impl_uart_tx(dev, buf, len, timeout);
}
#include <syscalls/uart_tx_mrsh.c>

#ifdef CONFIG_UART_WIDE_DATA
static inline int z_vrfy_uart_tx_u16(const struct device *dev,
				     const uint16_t *buf,
				     size_t len, int32_t timeout)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, tx));
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(buf, len, sizeof(uint16_t)));
	return z_impl_uart_tx_u16(dev, buf, len, timeout);
}
#include <syscalls/uart_tx_u16_mrsh.c>
#endif

UART_SIMPLE(tx_abort);
#include <syscalls/uart_tx_abort_mrsh.c>

static inline int z_vrfy_uart_rx_enable(const struct device *dev,
					uint8_t *buf,
					size_t len, int32_t timeout)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, rx_enable));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buf, len));
	return z_impl_uart_rx_enable(dev, buf, len, timeout);
}
#include <syscalls/uart_rx_enable_mrsh.c>

#ifdef CONFIG_UART_WIDE_DATA
static inline int z_vrfy_uart_rx_enable_u16(const struct device *dev,
					    uint16_t *buf,
					    size_t len, int32_t timeout)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, rx_enable));
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_WRITE(buf, len, sizeof(uint16_t)));
	return z_impl_uart_rx_enable_u16(dev, buf, len, timeout);
}
#include <syscalls/uart_rx_enable_u16_mrsh.c>
#endif

UART_SIMPLE(rx_disable);
#include <syscalls/uart_rx_disable_mrsh.c>
#endif /* CONFIG_UART_ASYNC_API */

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
static inline int z_vrfy_uart_line_ctrl_set(const struct device *dev,
					    uint32_t ctrl, uint32_t val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_set));
	return z_impl_uart_line_ctrl_set((const struct device *)dev, ctrl,
					 val);
}
#include <syscalls/uart_line_ctrl_set_mrsh.c>

static inline int z_vrfy_uart_line_ctrl_get(const struct device *dev,
					    uint32_t ctrl, uint32_t *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, line_ctrl_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(uint32_t)));
	return z_impl_uart_line_ctrl_get((const struct device *)dev, ctrl,
					 (uint32_t *)val);
}
#include <syscalls/uart_line_ctrl_get_mrsh.c>
#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
static inline int z_vrfy_uart_drv_cmd(const struct device *dev, uint32_t cmd,
				      uint32_t p)
{
	Z_OOPS(Z_SYSCALL_DRIVER_UART(dev, drv_cmd));
	return z_impl_uart_drv_cmd((const struct device *)dev, cmd, p);
}
#include <syscalls/uart_drv_cmd_mrsh.c>
#endif /* CONFIG_UART_DRV_CMD */
