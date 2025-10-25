/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * UART DTR Wrapper Driver
 *
 * This driver wraps an existing UART device and adds DTR (Data Terminal Ready)
 * functionality for runtime power management. When the UART is powered off, DTR is deasserted.
 * When the UART is powered on, DTR is asserted.
 * This allows remote end to shut down the UART when DTR is deasserted.
 */

#define DT_DRV_COMPAT zephyr_uart_dtr

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_dtr, CONFIG_UART_LOG_LEVEL);

/* Device configuration structure */
struct uart_dtr_config {
	struct gpio_dt_spec dtr_gpio;
	const struct device *uart_dev; /* Parent UART device */
};

/* Device data structure */
struct uart_dtr_data {
	bool uart_powered;
	uart_callback_t user_callback;
	void *user_data;
	const struct uart_dtr_config *config;
};

/* Forward declarations */
static int uart_dtr_configure(const struct device *dev, const struct uart_config *cfg);
static int uart_dtr_config_get(const struct device *dev, struct uart_config *cfg);

static int uart_dtr_poll_in(const struct device *dev, unsigned char *c);
static void uart_dtr_poll_out(const struct device *dev, unsigned char c);

static int uart_dtr_err_check(const struct device *dev);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_dtr_configure(const struct device *dev, const struct uart_config *cfg);
#endif

#ifdef CONFIG_UART_ASYNC_API
static int uart_dtr_callback_set(const struct device *dev, uart_callback_t callback,
				 void *user_data);
static int uart_dtr_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout);
static int uart_dtr_tx_abort(const struct device *dev);
static int uart_dtr_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout);
static int uart_dtr_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len);
static int uart_dtr_rx_disable(const struct device *dev);
#endif

static int power_on_uart(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;
	int ret;

	/* Power on the UART */
	ret = pm_device_runtime_get(config->uart_dev);
	if (ret < 0) {
		LOG_ERR("Failed to power on UART: %d", ret);
		return ret;
	}

	data->uart_powered = true;
	return 0;
}

static int power_off_uart(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;
	int ret;

	ret = pm_device_runtime_put_async(config->uart_dev, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("Failed to power off UART: %d", ret);
		return ret;
	}

	data->uart_powered = false;
	return 0;
}

static bool uart_dtr_is_powered(const struct device *dev)
{
	struct uart_dtr_data *data = dev->data;

	return data->uart_powered;
}

/* UART API implementations */
static int uart_dtr_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -ENODATA;
	}

	return uart_poll_in(config->uart_dev, c);
}

static void uart_dtr_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}

	uart_poll_out(config->uart_dev, c);
}

static int uart_dtr_err_check(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}

	return uart_err_check(config->uart_dev);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* UART interrupt API wrappers */
static int uart_dtr_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}

	return uart_fifo_fill(config->uart_dev, tx_data, len);
}

static int uart_dtr_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}

	return uart_fifo_read(config->uart_dev, rx_data, size);
}
static void uart_dtr_irq_tx_enable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_tx_enable(config->uart_dev);
}

static void uart_dtr_irq_tx_disable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_tx_disable(config->uart_dev);
}

static void uart_dtr_irq_rx_enable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_rx_enable(config->uart_dev);
}

static void uart_dtr_irq_rx_disable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_rx_disable(config->uart_dev);
}

static int uart_dtr_irq_tx_ready(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}
	return uart_irq_tx_ready(config->uart_dev);
}

static int uart_dtr_irq_rx_ready(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}
	return uart_irq_rx_ready(config->uart_dev);
}

static void uart_dtr_irq_err_enable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_err_enable(config->uart_dev);
}

static void uart_dtr_irq_err_disable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_err_disable(config->uart_dev);
}

static int uart_dtr_irq_is_pending(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}
	return uart_irq_is_pending(config->uart_dev);
}

static int uart_dtr_irq_update(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return 0;
	}
	return uart_irq_update(config->uart_dev);
}

static void uart_dtr_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *user_data)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return;
	}
	uart_irq_callback_set(config->uart_dev, cb, user_data);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_dtr_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -ENODEV;
	}

	return uart_configure(config->uart_dev, cfg);
}

static int uart_dtr_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -ENODEV;
	}

	return uart_config_get(config->uart_dev, cfg);
}
#endif

#ifdef CONFIG_UART_ASYNC_API
static void uart_dtr_async_callback_wrapper(const struct device *dev, struct uart_event *evt,
					    void *user_data)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = user_data;

	if (!data->user_callback) {
		return;
	}

	data->user_callback(config->uart_dev, evt, data->user_data);
}

static int uart_dtr_callback_set(const struct device *dev, uart_callback_t callback,
				 void *user_data)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;

	data->user_callback = callback;
	data->user_data = user_data;

	return uart_callback_set(config->uart_dev, uart_dtr_async_callback_wrapper, data);
}

static int uart_dtr_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -EBUSY;
	}

	return uart_tx(config->uart_dev, buf, len, timeout);
}

static int uart_dtr_tx_abort(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -EBUSY;
	}

	return uart_tx_abort(config->uart_dev);
}

static int uart_dtr_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -EBUSY;
	}

	return uart_rx_enable(config->uart_dev, buf, len, timeout);
}

static int uart_dtr_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -EACCES;
	}

	return uart_rx_buf_rsp(config->uart_dev, buf, len);
}

static int uart_dtr_rx_disable(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -EFAULT;
	}

	return uart_rx_disable(config->uart_dev);
}
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_LINE_CTRL
static int uart_dtr_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	const struct uart_dtr_config *config = dev->config;

	if (ctrl == UART_LINE_CTRL_DTR) {
		return gpio_pin_set_dt(&config->dtr_gpio, val ? 1 : 0);
	}

	return uart_line_ctrl_set(config->uart_dev, ctrl, val);
}
#endif

/* Power management */
static int uart_dtr_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_dtr_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("PM SUSPEND - Deasserting DTR");
		ret = gpio_pin_set_dt(&config->dtr_gpio, false);
		if (ret < 0) {
			LOG_ERR("Failed to set DTR GPIO: %d", ret);
			return ret;
		}
		return power_off_uart(dev);
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("PM RESUME - Asserting DTR");
		ret = gpio_pin_set_dt(&config->dtr_gpio, true);
		if (ret < 0) {
			LOG_ERR("Failed to set DTR GPIO: %d", ret);
			return ret;
		}
		return power_on_uart(dev);
	default:
		return -ENOTSUP;
	}
}

/* Initialization */
static int uart_dtr_init(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;
	int ret;

	if (!config->uart_dev) {
		LOG_ERR("Parent UART device not found");
		return -ENODEV;
	}

	if (!device_is_ready(config->uart_dev)) {
		LOG_ERR("Parent UART device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->dtr_gpio)) {
		LOG_ERR("DTR GPIO not ready");
		return -ENODEV;
	}

	/* Configure DTR GPIO as output, initially inactive */
	ret = gpio_pin_configure_dt(&config->dtr_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DTR GPIO: %d", ret);
		return ret;
	}

	/* Initialize data */
	*data = (struct uart_dtr_data){
		.config = config,
	};

	pm_device_init_suspended(dev);
	return 0;
}

/* UART driver API */
static const struct uart_driver_api uart_dtr_driver_api = {
	.poll_in = uart_dtr_poll_in,
	.poll_out = uart_dtr_poll_out,
	.err_check = uart_dtr_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_dtr_configure,
	.config_get = uart_dtr_config_get,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_dtr_callback_set,
	.tx = uart_dtr_tx,
	.tx_abort = uart_dtr_tx_abort,
	.rx_enable = uart_dtr_rx_enable,
	.rx_buf_rsp = uart_dtr_rx_buf_rsp,
	.rx_disable = uart_dtr_rx_disable,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_dtr_fifo_fill,
	.fifo_read = uart_dtr_fifo_read,
	.irq_tx_enable = uart_dtr_irq_tx_enable,
	.irq_tx_disable = uart_dtr_irq_tx_disable,
	.irq_rx_enable = uart_dtr_irq_rx_enable,
	.irq_rx_disable = uart_dtr_irq_rx_disable,
	.irq_tx_ready = uart_dtr_irq_tx_ready,
	.irq_rx_ready = uart_dtr_irq_rx_ready,
	.irq_err_enable = uart_dtr_irq_err_enable,
	.irq_err_disable = uart_dtr_irq_err_disable,
	.irq_is_pending = uart_dtr_irq_is_pending,
	.irq_update = uart_dtr_irq_update,
	.irq_callback_set = uart_dtr_irq_callback_set,
#endif
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = uart_dtr_line_ctrl_set,
#endif
};

/* Device macro */
#define UART_DTR_INIT(n)                                                                           \
	static const struct uart_dtr_config uart_dtr_config_##n = {                                \
		.dtr_gpio = GPIO_DT_SPEC_INST_GET(n, dtr_gpios),                                   \
		.uart_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                              \
	};                                                                                         \
                                                                                                   \
	static struct uart_dtr_data uart_dtr_data_##n;                                             \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, uart_dtr_pm_action);                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_dtr_init, PM_DEVICE_DT_INST_GET(n), &uart_dtr_data_##n,      \
			      &uart_dtr_config_##n, POST_KERNEL, CONFIG_UART_DTR_INIT_PRIORITY,    \
			      &uart_dtr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_DTR_INIT)
