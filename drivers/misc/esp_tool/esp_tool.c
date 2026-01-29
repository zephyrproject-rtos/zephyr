/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_loader_io.h"
#define DT_DRV_COMPAT espressif_esp_tool

#include <zephyr/kernel.h>
#include <sys/errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

#include <esp_loader.h>
#include <loader_port.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_tool, CONFIG_LOG_DEFAULT_LEVEL);

enum esp_tool_transport {
	ESP_LOADER_TRANSPORT_UART,
	ESP_LOADER_TRANSPORT_SPI,
};

struct esp_tool_config {
	enum esp_tool_transport transport;
	esp_loader_connect_args_t connect;

	const struct device *uart;
	const struct device *spi;

	struct gpio_dt_spec cs_gpio;   /* SPI only */
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec boot_gpio;

	uint32_t initial_baudrate;
	uint32_t higher_baudrate;
};

struct esp_tool_data {
	struct k_mutex lock;
	uint32_t current_baudrate;
	uint32_t flash_size;
	bool connected;
};

/* Only boot addresses vary by chip
 * DO NOT specify array size - let compiler determine it from initializers
 */
static const uint32_t boot_offset[] = {
	[ESP8266_CHIP] = 0x0,
	[ESP32_CHIP]   = 0x1000,
	[ESP32S2_CHIP] = 0x1000,
	[ESP32C3_CHIP] = 0x0,
	[ESP32S3_CHIP] = 0x0,
	[ESP32C2_CHIP] = 0x0,
	[ESP32C5_CHIP] = 0x2000,
	[ESP32H2_CHIP] = 0x0,
	[ESP32C6_CHIP] = 0x0,
	[ESP32P4_CHIP] = 0x2000
};

static char* target_name[] = {
	[ESP8266_CHIP] = "ESP8266",
	[ESP32_CHIP]   = "ESP32",
	[ESP32S2_CHIP] = "ESP32-S2",
	[ESP32C3_CHIP] = "ESP32-C3",
	[ESP32S3_CHIP] = "ESP32-S3",
	[ESP32C2_CHIP] = "ESP32-C2",
	[ESP32C5_CHIP] = "ESP32-C5",
	[ESP32H2_CHIP] = "ESP32-H2",
	[ESP32C6_CHIP] = "ESP32-C6",
	[ESP32P4_CHIP] = "ESP32-P4",
	[ESP_UNKNOWN_CHIP] = "Unknown"
};

/* If someone adds a new chip but forgets to update the array, compilation FAILS */
_Static_assert(sizeof(boot_offset) / sizeof(boot_offset[0]) == ESP_MAX_CHIP,
               "boot_offset array size mismatch! "
               "If you added a new chip to target_chip_t, you MUST add its address to bootloader_addresses[]");

static const char *get_error_string(const esp_loader_error_t error)
{
	const char *mapping[ESP_LOADER_ERROR_INVALID_RESPONSE + 1] = {
		"NONE", "UNKNOWN", "TIMEOUT", "IMAGE SIZE", "INVALID MD5",
		"INVALID PARAMETER", "INVALID TARGET", "UNSUPPORTED CHIP",
		"UNSUPPORTED FUNCTION", "INVALID RESPONSE"
	};

	return mapping[error];
}

int esp_tool_get_target(const struct device *dev, uint32_t *id)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (id) {
		*id = esp_loader_get_target();
	} else {
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_detect_size(const struct device *dev, uint32_t *size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_detect_size(size) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash size detection unsupported");
		err = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_start(const struct device *dev, uint32_t offset,
			 size_t image_size, size_t block_size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_start(offset, image_size, block_size) !=
				   ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash start fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_write(const struct device *dev, const void *payload,
			 size_t len)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_write((void *)payload, len) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash write fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_finish(const struct device *dev, bool reboot)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_finish(reboot) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash finish fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_binary(const struct device *dev, const void *image,
			  size_t size, uint32_t offset)
{
	esp_loader_error_t err;
	static uint8_t payload[1024];
	const uint8_t *bin_addr = image;

	printk("Erasing flash (this may take a while)...\n");
	err = esp_loader_flash_start(offset, size, sizeof(payload));

	if (err != ESP_LOADER_SUCCESS) {
		printk("Erasing flash failed with error: %s.\n", get_error_string(err));

		if (err == ESP_LOADER_ERROR_INVALID_PARAM) {
			printk("If using Secure Download Mode, double check"
			"that the specified target flash size is correct.\n");
		}
		return err;
	}
	printk("Start programming\n");

	size_t binary_size = size;
	size_t written = 0;

	while (size > 0) {
		size_t to_read = MIN(size, sizeof(payload));
		memcpy(payload, bin_addr, to_read);

		err = esp_loader_flash_write(payload, to_read);

		if (err != ESP_LOADER_SUCCESS) {
			printk("\nPacket could not be written! Error %s.\n",
				get_error_string(err));
			return err;
		}

		size -= to_read;
		bin_addr += to_read;
		written += to_read;

		int progress = (int)(((float)written / binary_size) * 100);
		ets_printf("\rProgress: %d %%", progress);
	};

	printk("\nFinished programming\n");

#if MD5_ENABLED
	err = esp_loader_flash_verify();
	if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
		printk("ESP8266 does not support flash verify command.");
		return err;
	} else if (err != ESP_LOADER_SUCCESS) {
		printk("MD5 does not match. Error: %s\n", get_error_string(err));
		return err;
	}
	printk("Flash verified\n");
#endif
	return ESP_LOADER_SUCCESS;
}

int esp_tool_mem_start(const struct device *dev, uint32_t offset, uint32_t size,
		       size_t block_size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_mem_start(offset, size, block_size) !=
		ESP_LOADER_SUCCESS) {
		LOG_ERR("Memory start fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_mem_write(const struct device *dev, const void *ptr, size_t size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_mem_write(ptr, size) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Memory write fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_mem_finish(const struct device *dev, uint32_t entry_point)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_mem_finish(entry_point) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Memory finish fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_mac_read(const struct device *dev, uint8_t mac[6])
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_read_mac(mac) != ESP_LOADER_SUCCESS) {
		LOG_ERR("MAC read fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_read(const struct device *dev, uint32_t offset, uint8_t *buf,
			size_t len)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_read(buf, offset, len) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash read fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_erase_region(const struct device *dev, uint32_t offset,
				size_t len)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_erase_region(offset, len) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash region erase fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_flash_erase(const struct device *dev)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_erase() != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash erase fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_register_write(const struct device *dev, uint32_t reg,
				uint32_t value)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_write_register(reg, value) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Register write fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_register_read(const struct device *dev, uint32_t reg,
				uint32_t *value)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	if (!data->connected) {
		return -ENOTCONN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_read_register(reg, value) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Register read fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_change_transmission_rate(const struct device *dev,
					uint32_t baudrate)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;

	if (!data->connected) {
		return -ENOTCONN;
	}

	struct uart_config uc;
	uart_config_get(cfg->uart, &uc);
	uc.baudrate = baudrate;
	uart_configure(cfg->uart, &uc);
	data->current_baudrate = baudrate;

	return 0;
}

static void gpios_engage(const struct device *dev)
{
	const struct esp_tool_config *cfg = dev->config;

//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_ACTIVE);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
}

static void gpios_disengage(const struct device *dev)
{
	const struct esp_tool_config *cfg = dev->config;

//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_INPUT);
//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_LOW
	//gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_INPUT);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_LOW);
//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_DISCONNECTED);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_DISCONNECTED);
}

int esp_tool_reset_target(const struct device *dev)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	esp_loader_reset_target();
	gpios_disengage(dev);

	data->connected = false;
	data->current_baudrate = cfg->initial_baudrate;
	esp_loader_change_transmission_rate(cfg->initial_baudrate);

	LOG_DBG("ESP device reset done");

	k_mutex_unlock(&data->lock);

	return 0;
}

int esp_tool_connect(const struct device *dev, bool hs)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	gpios_engage(dev);

	if (esp_loader_connect((esp_loader_connect_args_t *) &cfg->connect) !=
				ESP_LOADER_SUCCESS) {
		gpios_disengage(dev);
		err = -ETIMEDOUT;
		LOG_ERR("Target connection failed");
	} else {
		data->connected = true;
		LOG_DBG("Connected target ROM with chip id %d",
			esp_loader_get_target());
	}

	if (hs && data->current_baudrate != cfg->higher_baudrate) {
		if (esp_loader_change_transmission_rate(cfg->higher_baudrate)) {
			LOG_ERR("Failed to change baudrate");
			err = -EIO;
		} else {
			if (loader_port_change_transmission_rate(
					cfg->higher_baudrate)) {
				LOG_ERR("Unable to change baudrate");
				err = -EIO;
			} else {
				data->current_baudrate = cfg->higher_baudrate;
				LOG_INF("Transmission rate changed");
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_connect_stub(const struct device *dev, bool hs)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	gpios_engage(dev);

	if (esp_loader_connect_with_stub(&cfg->connect) != ESP_LOADER_SUCCESS) {
		gpios_disengage(dev);
		err = -ETIMEDOUT;
		LOG_ERR("Target stub connection failed");
	} else {
		data->connected = true;
		LOG_DBG("Connected target STUB with chip id %d",
			 esp_loader_get_target());
	}

	if (hs && data->current_baudrate != cfg->higher_baudrate) {
		if (esp_loader_change_transmission_rate_stub(
				data->current_baudrate,
				cfg->higher_baudrate)) {
			LOG_ERR("Failed to change baudrate");
			err = -EIO;
		} else {
			if (loader_port_change_transmission_rate(
					cfg->higher_baudrate)) {
				LOG_ERR("Unable to change baudrate");
				err = -EIO;
			} else {
				data->current_baudrate = cfg->higher_baudrate;
				LOG_INF("Transmission rate changed");
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_get_boot_offset(const struct device *dev, uint32_t *offset)
{
	struct esp_tool_data *data = dev->data;
	target_chip_t chip;

	if (!offset) {
		return -EINVAL;
	}
	if (!data->connected) {
		return -ENOTCONN;
	}
	if ((chip = esp_loader_get_target()) == ESP_UNKNOWN_CHIP) {
		return -EIO;
	}

	*offset = boot_offset[chip];

	return 0;
}

int esp_tool_get_target_name(const struct device *dev, char **name)
{
	struct esp_tool_data *data = dev->data;
	target_chip_t chip;

	if (!name) {
		return -EINVAL;
	}
	if (!data->connected) {
		return -ENOTCONN;
	}
	if ((chip = esp_loader_get_target()) == ESP_UNKNOWN_CHIP) {
		return -EIO;
	}

	*name = target_name[chip];

	return 0;
}

uint32_t esp_tool_get_current_baudrate(const struct device *dev)
{
	struct esp_tool_data *data = dev->data;

	return data->current_baudrate;
}

bool esp_tool_is_connected(const struct device *dev)
{
	struct esp_tool_data *data = dev->data;

	return data->connected;
}

static int esp_tool_init(const struct device *dev)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;

	if (!device_is_ready(cfg->uart)) {
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->boot_gpio)) {
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	data->current_baudrate = cfg->initial_baudrate;
	data->connected = false;
	data->flash_size = 0;

	/* ESF Zephyr port interface */
	const loader_zephyr_config_t config = {
		.uart_dev = cfg->uart,
		.reset_spec = cfg->reset_gpio,
		.boot_spec = cfg->boot_gpio
	};
	loader_port_zephyr_init(&config);

	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);

	LOG_DBG("Serial interface name: %s", config.uart_dev->name);
	LOG_DBG("Enable/Reset gpio: %s.%d", config.reset_spec.port->name,
						  config.reset_spec.pin);
	LOG_DBG("Boot gpio: %s.%d", config.boot_spec.port->name,
				    config.boot_spec.pin);
	return 0;
}

#define ESP_TOOL_INIT(inst)							\
	static struct esp_tool_data esp_tool_data_##inst;			\
										\
	static const struct esp_tool_config esp_tool_config_##inst = {		\
		.transport = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, uart),	\
					(ESP_LOADER_TRANSPORT_UART),		\
					(ESP_LOADER_TRANSPORT_SPI)),		\
		.uart = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, uart),		\
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart))), (NULL)),	\
		.spi = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, spi),		\
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, spi))), (NULL)),	\
		.cs_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, cs_gpios, {0}),	\
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),		\
		.boot_gpio  = GPIO_DT_SPEC_INST_GET(inst, boot_gpios),		\
		.initial_baudrate =						\
		    DT_INST_PROP_OR(inst, initial_baudrate, 115200),		\
		.higher_baudrate =						\
		    DT_INST_PROP_OR(inst, higher_baudrate, 230400),		\
		.connect.sync_timeout = DT_INST_PROP(inst, sync_timeout_ms),	\
		.connect.trials = DT_INST_PROP(inst, num_trials),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, esp_tool_init, NULL, &esp_tool_data_##inst,	\
				&esp_tool_config_##inst, POST_KERNEL,		\
				/*CONFIG_KERNEL_INIT_PRIORITY_DEVICE*/90, NULL);

DT_INST_FOREACH_STATUS_OKAY(ESP_TOOL_INIT)
