/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_mipi_dbi

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_stm32_fmc, CONFIG_MIPI_DBI_LOG_LEVEL);

#define FMC_DATA_ADDR_OFFSET 0x02

struct mipi_dbi_stm32_fmc_config {
	/* Reset GPIO */
	const struct gpio_dt_spec reset;
	/* Power GPIO */
	const struct gpio_dt_spec power;
	uint16_t *register_addr;
	uint16_t *data_addr;
};

struct mipi_dbi_stm32_fmc_data {
};

int mipi_dbi_stm32_fmc_command_write(const struct device *dev,
				     const struct mipi_dbi_config *dbi_config, uint8_t cmd,
				     const uint8_t *data_buf, size_t len)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;

	uint32_t i = 0;

	*config->register_addr = cmd;

	while (i < len) {
		uint16_t data = (uint16_t)data_buf[i];
		*config->data_addr = data;
		i += 1;
	}

	return 0;
}

static int mipi_dbi_stm32_fmc_write_display(const struct device *dev,
					    const struct mipi_dbi_config *dbi_config,
					    const uint8_t *framebuf,
					    struct display_buffer_descriptor *desc,
					    enum display_pixel_format pixfmt)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;
	uint32_t i = 0;

	while (i < desc->buf_size) {
		uint16_t data = ((uint16_t)framebuf[i + 1U] << 8U) | (uint16_t)framebuf[i];
		*config->data_addr = data;
		i += 2;
	}

	return 0;
}

static int mipi_dbi_stm32_fmc_reset(const struct device *dev, uint32_t delay)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;
	int ret;

	if (config->reset.port) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_stm32_fmc_init(const struct device *dev)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;

	if (config->reset.port) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}

	if (config->power.port) {
		if (!gpio_is_ready_dt(&config->power)) {
			LOG_ERR("Power GPIO device not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->power, GPIO_OUTPUT)) {
			LOG_ERR("Couldn't configure power pin");
			return -EIO;
		}
	}

	return 0;
}

static struct mipi_dbi_driver_api mipi_dbi_stm32_fmc_driver_api = {
	.reset = mipi_dbi_stm32_fmc_reset,
	.command_write = mipi_dbi_stm32_fmc_command_write,
	.write_display = mipi_dbi_stm32_fmc_write_display,
};

/* Offset between the address of the subbanks of the bank 1 */
#define FMC_BANK1_OFFSET 0x04000000UL

#define MIPI_DBI_FMC_GET_ADDRESS(n) \
	FMC_BANK1 + DT_REG_ADDR(DT_INST_PARENT(n)) * FMC_BANK1_OFFSET

#define MIPI_DBI_STM32_FMC_INIT(n)                                                                 \
	static const struct mipi_dbi_stm32_fmc_config mipi_dbi_stm32_fmc_config_##n = {            \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
		.power = GPIO_DT_SPEC_INST_GET_OR(n, power_gpios, {}),                             \
		.register_addr = (uint16_t *)MIPI_DBI_FMC_GET_ADDRESS(n),                          \
		.data_addr = (uint16_t *)(FMC_BANK1_1 + FMC_DATA_ADDR_OFFSET),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_stm32_fmc_init, NULL, NULL,                              \
			      &mipi_dbi_stm32_fmc_config_##n, POST_KERNEL,                         \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_stm32_fmc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_STM32_FMC_INIT)
