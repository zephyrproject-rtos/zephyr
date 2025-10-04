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
	bool dtr_asserted;
	bool uart_powered;
	uart_callback_t user_callback;
	void *user_data;
	const struct uart_dtr_config *config;
	struct k_work dtr_work;
#ifdef CONFIG_UART_ASYNC_API
	bool app_rx_enable;
	bool rx_active;
	int32_t rx_timeout;
	bool rx_suspended; /* RX disabled due to DTR deassert */
#endif
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

static void power_on_uart(struct uart_dtr_data *data)
{
	int ret;
	const struct uart_dtr_config *config = data->config;

	/* Power on the UART */
	if (pm_device_runtime_is_enabled(config->uart_dev)) {
		ret = pm_device_runtime_get(config->uart_dev);
	} else {
		ret = pm_device_action_run(config->uart_dev, PM_DEVICE_ACTION_RESUME);
	}
	if (ret == -ENOSYS) {
		LOG_ERR("Runtime PM not supported");
		ret = 0; /* Device does not support runtime PM */
	}
	if (ret == -EALREADY) {
		LOG_ERR("Device already active");
		ret = 0; /* Device already active */
	}

	if (ret == 0) {
		LOG_DBG("UART powered on");
		data->uart_powered = true;
#ifdef CONFIG_UART_ASYNC_API
		if (data->app_rx_enable) {
			/* Request a buffer from application to restart RX */
			data->rx_suspended = false;
			if (data->user_callback) {
				LOG_DBG("Requesting RX buffer after DTR assert");
				struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

				data->user_callback(config->uart_dev, &evt, data->user_data);
			}
		}
#endif
	} else {
		LOG_ERR("Failed to power on UART: %d", ret);
	}
}

static void power_off_uart(struct uart_dtr_data *data)
{
	int ret;
	const struct uart_dtr_config *config = data->config;

#ifdef CONFIG_UART_ASYNC_API
	if (data->rx_active) {
		ret = uart_rx_disable(config->uart_dev);
		if (ret) {
			LOG_ERR("Failed to disable RX: %d", ret);
		}
		data->rx_active = false;
		data->rx_suspended = true;
	}
#endif
	/* Power off the UART */
	if (pm_device_runtime_is_enabled(config->uart_dev)) {
		ret = pm_device_runtime_put_async(config->uart_dev, K_MSEC(10));
	} else {
		ret = pm_device_action_run(config->uart_dev, PM_DEVICE_ACTION_SUSPEND);
	}

	if (ret == 0) {
		data->uart_powered = false;
		LOG_DBG("UART powered off");
	} else {
		LOG_ERR("Failed to power off UART: %d", ret);
	}
}

/* DTR control functions */
static void uart_dtr_work_handler(struct k_work *work)
{
	struct uart_dtr_data *data = CONTAINER_OF(work, struct uart_dtr_data, dtr_work);

	if (data->dtr_asserted == data->uart_powered) {
		/* No change in state */
		return;
	}

	if (data->dtr_asserted) {
		power_on_uart(data);
	} else {
		power_off_uart(data);
	}
}

static int uart_dtr_set_dtr(const struct device *dev, bool assert)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->dtr_gpio)) {
		return -ENODEV;
	}

	/* Use GPIO_ACTIVE/INACTIVE - the GPIO subsystem handles polarity */
	ret = gpio_pin_set_dt(&config->dtr_gpio, assert ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Failed to set DTR GPIO: %d", ret);
		return ret;
	}

	data->dtr_asserted = assert;

	if (assert) {
		/* Handle wake-up immediately */
		uart_dtr_work_handler(&data->dtr_work);
	} else {
		/* Schedule work to handle power state change */
		k_work_submit(&data->dtr_work);
	}

	LOG_DBG("DTR %s", assert ? "asserted" : "deasserted");

	return 0;
}

static bool uart_dtr_is_powered(const struct device *dev)
{
	struct uart_dtr_data *data = dev->data;
	bool powered;

	powered = data->uart_powered;

	return powered;
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

	if (evt->type == UART_RX_DISABLED && (data->rx_suspended)) {
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
		return -ENODEV;
	}

	return uart_tx(config->uart_dev, buf, len, timeout);
}

static int uart_dtr_tx_abort(const struct device *dev)
{
	const struct uart_dtr_config *config = dev->config;

	if (!uart_dtr_is_powered(dev)) {
		return -ENODEV;
	}

	return uart_tx_abort(config->uart_dev);
}

static int uart_dtr_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;

	data->app_rx_enable = true;
	data->rx_timeout = timeout;

	if (!uart_dtr_is_powered(dev)) {
		/* Release buffer immediately; will request again on power up */
		if (data->user_callback && buf) {
			struct uart_event evt = {
				.type = UART_RX_BUF_RELEASED,
				.data.rx_buf.buf = buf,
			};
			data->user_callback(config->uart_dev, &evt, data->user_data);
		}
		LOG_DBG("DTR not asserted, releasing buffer");
		return 0;
	}

	data->rx_active = true;
	return uart_rx_enable(config->uart_dev, buf, len, timeout);
}

static int uart_dtr_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct uart_dtr_config *config = dev->config;
	struct uart_dtr_data *data = dev->data;
	int err = 0;

	if (!data->app_rx_enable) {
		goto release;
	}

	if (!uart_dtr_is_powered(dev) || data->rx_suspended) {
		goto release;
	}

	if (!data->rx_active) {
		data->rx_active = true;
		err = uart_rx_enable(config->uart_dev, buf, len, data->rx_timeout);

		if (err == -EBUSY) {
			LOG_ERR("uart_rx_enable() busy");
			err = 0;
			goto release;
		}
		if (err < 0) {
			data->rx_active = false;
			LOG_ERR("uart_rx_enable() failed: %d", err);
			goto release;
		}
		LOG_DBG("RX was not active, enabled RX (ret %d)", err);
		return 0;
	}

	return uart_rx_buf_rsp(config->uart_dev, buf, len);
release:
	LOG_DBG("Releasing buffer, RX not enabled");
	if (data->user_callback && buf) {
		struct uart_event evt = {.type = UART_RX_BUF_RELEASED, .data.rx_buf.buf = buf};

		data->user_callback(config->uart_dev, &evt, data->user_data);
	}
	return err;
}

static int rx_disable(struct uart_dtr_data *data)
{
	const struct uart_dtr_config *config = data->config;

	if (!data->rx_active) {
		return 0;
	}
	data->rx_active = false;
	if (!uart_dtr_is_powered(config->uart_dev)) {
		return 0;
	}
	int err = uart_rx_disable(config->uart_dev);

	LOG_DBG("uart_rx_disable() %d", err);
	return err;
}

static int uart_dtr_rx_disable(const struct device *dev)
{
	struct uart_dtr_data *data = dev->data;

	data->app_rx_enable = false;
	LOG_DBG("uart_dtr_rx_disable");

	return rx_disable(data);
}
#endif /* CONFIG_UART_ASYNC_API */

/* Power management */
#ifdef CONFIG_PM_DEVICE
static int uart_dtr_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("PM SUSPEND - Deasserting DTR");
		uart_dtr_set_dtr(dev, false);
		break;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("PM RESUME - Asserting DTR");
		uart_dtr_set_dtr(dev, true);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

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
	data->dtr_asserted = false;
	data->uart_powered = false;
	data->user_callback = NULL;
	data->user_data = NULL;
	data->config = config;
#ifdef CONFIG_UART_ASYNC_API
	data->app_rx_enable = false;
	data->rx_active = false;
	data->rx_timeout = SYS_FOREVER_MS;
	data->rx_suspended = false;
#endif

	k_work_init(&data->dtr_work, uart_dtr_work_handler);

	/* Enable runtime PM for the parent UART device */
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(config->uart_dev);
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
