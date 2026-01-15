/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#include "zephyr/drivers/misc/esp_tool/esp_tool.h"
#define DT_DRV_COMPAT espressif_esp_tool

#include "zephyr/kernel.h"
#include <sys/errno.h>

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
//#include <zephyr/console/tty.h>

#include <zephyr_port.h>
#include <esp_loader.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(esp_tool, CONFIG_UART_LOG_LEVEL);

#if 0
/*
 * DTS usage
 */
esp_tool0: esp_tool {
    compatible = "espressif,esp_tool";
    uart = <&uart2>;
    reset-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
    boot-gpios  = <&gpio0 11 GPIO_ACTIVE_HIGH>;
    initial-baudrate = <115200>;
};
esp_tool0: esp_tool {
    compatible = "espressif,esp_tool";
    spi = <&spi1>;
    cs-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;
    reset-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
    boot-gpios  = <&gpio0 11 GPIO_ACTIVE_HIGH>;
};
#endif

enum esp_tool_transport {
	ESP_LOADER_TRANSPORT_UART,
	ESP_LOADER_TRANSPORT_SPI,
};

struct esp_tool_config {
	enum esp_tool_transport transport;

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
	bool connected;
	uint32_t current_baudrate;
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

/* If someone adds a new chip but forgets to update the array, compilation FAILS */
_Static_assert(sizeof(boot_offset) / sizeof(boot_offset[0]) == ESP_MAX_CHIP,
               "boot_offset array size mismatch! "
               "If you added a new chip to target_chip_t, you MUST add its address to bootloader_addresses[]");

#if 0
static int esp_tool_tx(const struct device *dev,
                      const void *buf, size_t len)
{
    const struct esp_tool_config *cfg = dev->config;

    switch (cfg->transport) {
    case ESP_LOADER_TRANSPORT_UART:
        return uart_tx(cfg->uart, buf, len, SYS_FOREVER_MS);

    case ESP_LOADER_TRANSPORT_SPI:
        return spi_write(cfg->spi, &spi_cfg, &tx_bufs);

    default:
        return -ENOTSUP;
    }
}

#endif

int esp_tool_get_target(const struct device *dev, uint32_t *id)
{
	target_chip_t chip_id = esp_loader_get_target();

	if (id) {
		*id = (uint32_t) chip_id;
		return 0;
	} else {
		return -EINVAL;
	}

	return -ENODATA;
}

int esp_tool_flash_detect_size(const struct device *dev, uint32_t *size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

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

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_finish(reboot) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash finish fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_mem_start(const struct device *dev, uint32_t offset, uint32_t size,
		       size_t block_size)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

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

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_mem_finish(entry_point) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Memory finish fail");
		err = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_read_mac(const struct device *dev, uint8_t mac[6])
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

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

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_flash_read(buf, offset, len) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Flash read fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_write_register(const struct device *dev, uint32_t reg,
				uint32_t value)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_write_register(reg, value) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Register write fail");
		err = -EIO;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_read_register(const struct device *dev, uint32_t reg,
				uint32_t *value)
{
	struct esp_tool_data *data = dev->data;
	int err = 0;

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
//	const struct esp_tool_config *cfg = dev->config;
//	struct esp_tool_data *data = dev->data;
//
//	struct uart_config uc;
//	uart_config_get(cfg->uart, &uc);
//	uc.baudrate = baudrate;
//	uart_configure(cfg->uart, &uc);
//
//	data->current_baudrate = baudrate;
	return 0;
}

int esp_tool_reset_target(const struct device *dev)
{
	struct esp_tool_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	esp_loader_reset_target();
	LOG_DBG("ESP device reset done");
	k_mutex_unlock(&data->lock);

	return 0;
}

int esp_tool_connect(const struct device *dev)
{
	const struct esp_tool_config *cfg = dev->config;
	struct esp_tool_data *data = dev->data;
	esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
	int err = 0;

	//ets_printf("### %s ===\n", __func__);

	k_mutex_lock(&data->lock, K_FOREVER);

	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_INACTIVE);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);

	if (esp_loader_connect(&config) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Target connection failed");
		printk("timeouted connection.\n");
		err = -ETIMEDOUT;
	} else {
		data->connected = true;
		printk("connection OK!\n");


	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_connect_with_stub(const struct device *dev)
{
	struct esp_tool_data *data = dev->data;
	esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (esp_loader_connect(&config) != ESP_LOADER_SUCCESS) {
		LOG_ERR("Target stub connection failed");
		err = -ETIMEDOUT;
	} else {
		data->connected = true;
	}

	k_mutex_unlock(&data->lock);

	return err;
}

int esp_tool_get_boot_offset(const struct device *dev, int chip, uint32_t *offset)
{
	if (offset == NULL || chip > ESP_MAX_CHIP) {
		return -EINVAL;
	}

	*offset = boot_offset[chip];

	return 0;
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

	const loader_zephyr_config_t config = {
		.uart_dev = cfg->uart,
		.enable_spec = cfg->reset_gpio,
		.boot_spec = cfg->boot_gpio
	};

	k_mutex_init(&data->lock);
	data->connected = false;
	data->current_baudrate = cfg->initial_baudrate;

	loader_port_zephyr_init(&config);

	printk("    %s \n", config.uart_dev->name);
	printk("    %s .%d EN\n", config.enable_spec.port->name, config.enable_spec.pin);
	printk("    %s .%d BOOT\n", config.boot_spec.port->name, config.boot_spec.pin);

//	gpio_pin_configure_dt(&cfg->boot_gpio, GPIO_OUTPUT_ACTIVE);
//	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);

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
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, esp_tool_init, NULL, &esp_tool_data_##inst,	\
				&esp_tool_config_##inst, POST_KERNEL,		\
				/*CONFIG_KERNEL_INIT_PRIORITY_DEVICE*/90, NULL);

DT_INST_FOREACH_STATUS_OKAY(ESP_TOOL_INIT)
