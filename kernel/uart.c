/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <uart.h>
#include <device.h>

int _impl_uart_err_check(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->err_check) {
		return api->err_check(dev);
	}
	return 0;
}

int _impl_uart_poll_in(struct device *dev, unsigned char *p_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	return api->poll_in(dev, p_char);
}

void _impl_uart_poll_out(struct device *dev,
			 unsigned char out_char)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	api->poll_out(dev, out_char);
}

int _impl_uart_configure(struct device *dev,
			 const struct uart_config *cfg)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	if (api->configure) {
		return api->configure(dev, cfg);
	}

	return -ENOTSUP;
}

int _impl_uart_config_get(struct device *dev,
			  struct uart_config *cfg)
{
	const struct uart_driver_api *api =
				(const struct uart_driver_api *)dev->driver_api;

	if (api->config_get) {
		return api->config_get(dev, cfg);
	}

	return -ENOTSUP;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

void _impl_uart_irq_tx_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_enable) {
		api->irq_tx_enable(dev);
	}
}

void _impl_uart_irq_tx_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_tx_disable) {
		api->irq_tx_disable(dev);
	}
}

void _impl_uart_irq_rx_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_rx_enable) {
		api->irq_rx_enable(dev);
	}
}

void _impl_uart_irq_rx_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_rx_disable) {
		api->irq_rx_disable(dev);
	}
}

void _impl_uart_irq_err_enable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_err_enable) {
		api->irq_err_enable(dev);
	}
}

void _impl_uart_irq_err_disable(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_err_disable) {
		api->irq_err_disable(dev);
	}
}

int _impl_uart_irq_is_pending(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_is_pending)	{
		return api->irq_is_pending(dev);
	}

	return 0;
}

int _impl_uart_irq_update(struct device *dev)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->irq_update) {
		return api->irq_update(dev);
	}

	return 0;
}

#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
int _impl_uart_line_ctrl_set(struct device *dev,
			     u32_t ctrl, u32_t val)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->line_ctrl_set) {
		return api->line_ctrl_set(dev, ctrl, val);
	}

	return -ENOTSUP;
}

int _impl_uart_line_ctrl_get(struct device *dev,
					   u32_t ctrl, u32_t *val)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api && api->line_ctrl_get) {
		return api->line_ctrl_get(dev, ctrl, val);
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD

int _impl_uart_drv_cmd(struct device *dev, u32_t cmd, u32_t p)
{
	const struct uart_driver_api *api =
		(const struct uart_driver_api *)dev->driver_api;

	if (api->drv_cmd) {
		return api->drv_cmd(dev, cmd, p);
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_DRV_CMD */
