/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zephyr_modem_uart, CONFIG_MODEM_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_modem_uart

struct driver_data {
	const struct device *dev;
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_ZEPHYR_MODEM_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_ZEPHYR_MODEM_UART_BUFFER_SIZES];
};

struct driver_config {
	const struct device *uart;
	struct gpio_dt_spec reset_pin;
	struct gpio_dt_spec power_pin;
	struct modem_pipelink *pipelink;
	uint16_t power_pulse_ms;
	uint16_t reset_pulse_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;
	bool autostarts;
};

static bool driver_has_pin(const struct gpio_dt_spec *pin)
{
	return pin->port != NULL;
}

static int driver_pm_action_suspend(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	int ret;

	modem_pipelink_notify_disconnected(dev_config->pipelink);

	ret = modem_pipe_close(dev_data->uart_pipe, K_SECONDS(1));
	if (ret) {
		LOG_ERR("%s failed to close uart pipe", dev->name);
		return ret;
	}

	if (driver_has_pin(&dev_config->power_pin)) {
		gpio_pin_set_dt(&dev_config->power_pin, 1);
		k_msleep(dev_config->power_pulse_ms);
		gpio_pin_set_dt(&dev_config->power_pin, 0);
		k_msleep(dev_config->shutdown_time_ms);
	}

	if (driver_has_pin(&dev_config->reset_pin)) {
		gpio_pin_set_dt(&dev_config->reset_pin, 1);
		k_msleep(dev_config->reset_pulse_ms);
	}

	return pm_device_runtime_put(dev_config->uart);
}

static int driver_pm_action_resume(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	struct driver_data *dev_data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev_config->uart);
	if (ret) {
		LOG_ERR("%s failed to resume uart %s", dev->name, dev_config->uart->name);
		return ret;
	}

	if (driver_has_pin(&dev_config->reset_pin)) {
		gpio_pin_set_dt(&dev_config->reset_pin, 0);
	}

	if (driver_has_pin(&dev_config->power_pin) && !dev_config->autostarts) {
		gpio_pin_set_dt(&dev_config->power_pin, 1);
		k_msleep(dev_config->power_pulse_ms);
		gpio_pin_set_dt(&dev_config->power_pin, 0);
	}

	k_msleep(dev_config->startup_time_ms);

	ret = modem_pipe_open(dev_data->uart_pipe, K_SECONDS(1));
	if (ret) {
		LOG_ERR("%s failed to open uart pipe", dev->name);
		pm_device_runtime_put(dev_config->uart);
		return ret;
	}

	modem_pipelink_notify_connected(dev_config->pipelink);
	return ret;
}

static int driver_pm_action_turn_off(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	if (driver_has_pin(&dev_config->reset_pin)) {
		gpio_pin_configure_dt(&dev_config->reset_pin, GPIO_INPUT);
	}

	if (driver_has_pin(&dev_config->power_pin)) {
		gpio_pin_configure_dt(&dev_config->power_pin, GPIO_INPUT);
	}

	return 0;
}

static int driver_pm_action_turn_on(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	if (driver_has_pin(&dev_config->reset_pin)) {
		gpio_pin_configure_dt(&dev_config->reset_pin, GPIO_OUTPUT_ACTIVE);
	}

	if (driver_has_pin(&dev_config->power_pin)) {
		gpio_pin_configure_dt(&dev_config->power_pin, GPIO_OUTPUT_INACTIVE);
	}

	return 0;
}

static int driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return driver_pm_action_suspend(dev);

	case PM_DEVICE_ACTION_RESUME:
		return driver_pm_action_resume(dev);

	case PM_DEVICE_ACTION_TURN_OFF:
		return driver_pm_action_turn_off(dev);

	case PM_DEVICE_ACTION_TURN_ON:
		return driver_pm_action_turn_on(dev);

	default:
		break;
	}

	return -ENOTSUP;
}

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	const struct modem_backend_uart_config uart_backend_config = {
		.uart = dev_config->uart,
		.receive_buf = dev_data->uart_backend_receive_buf,
		.receive_buf_size = ARRAY_SIZE(dev_data->uart_backend_receive_buf),
		.transmit_buf = dev_data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(dev_data->uart_backend_transmit_buf),
	};

	dev_data->uart_pipe = modem_backend_uart_init(&dev_data->uart_backend, &uart_backend_config);
	modem_pipelink_init(dev_config->pipelink, dev_data->uart_pipe);
	return pm_device_driver_init(dev, driver_pm_action);
}

#define DRIVER_NAMESPACE(inst, name) \
	_CONCAT(name, inst)

#define DRIVER_DEFINE(inst)									\
	MODEM_PIPELINK_DT_INST_DEFINE(inst, user_pipe_0);					\
												\
	static struct driver_data DRIVER_NAMESPACE(inst, data);					\
	static const struct driver_config DRIVER_NAMESPACE(inst, config) = {			\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.reset_pin = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),		\
		.power_pin = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),		\
		.pipelink = MODEM_PIPELINK_DT_INST_GET(inst, user_pipe_0),			\
		.power_pulse_ms = DT_INST_PROP_OR(inst, mdm_power_pulse_ms, 0),			\
		.reset_pulse_ms = DT_INST_PROP_OR(inst, mdm_reset_pulse_ms, 0),			\
		.startup_time_ms = DT_INST_PROP_OR(inst, mdm_startup_time_ms, 0),		\
		.shutdown_time_ms = DT_INST_PROP_OR(inst, mdm_shutdown_time_ms, 0),		\
		.autostarts = DT_INST_PROP(inst, mdm_autostarts),				\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_action);					\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&DRIVER_NAMESPACE(inst, data),							\
		&DRIVER_NAMESPACE(inst, config),						\
		POST_KERNEL,									\
		CONFIG_ZEPHYR_MODEM_UART_INIT_PRIORITY,						\
		NULL										\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
