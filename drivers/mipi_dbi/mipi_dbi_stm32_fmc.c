/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_fmc_mipi_dbi

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_stm32_fmc, CONFIG_MIPI_DBI_LOG_LEVEL);

struct mipi_dbi_stm32_fmc_config {
	/* Reset GPIO */
	const struct gpio_dt_spec reset;
	/* Power GPIO */
	const struct gpio_dt_spec power;
	mem_addr_t register_addr;
	mem_addr_t data_addr;
	uint32_t fmc_address_setup_time;
	uint32_t fmc_data_setup_time;
	uint32_t fmc_memory_width;
};

struct mipi_dbi_stm32_fmc_data {
	const struct mipi_dbi_config *dbi_config;
};

int mipi_dbi_stm32_fmc_check_config(const struct device *dev,
				    const struct mipi_dbi_config *dbi_config)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;
	struct mipi_dbi_stm32_fmc_data *data = dev->data;
	uint32_t fmc_write_cycles;

	if (data->dbi_config == dbi_config) {
		return 0;
	}

	if (dbi_config->mode != MIPI_DBI_MODE_8080_BUS_16_BIT) {
		LOG_ERR("Only support Intel 8080 16-bits");
		return -ENOTSUP;
	}

	if (config->fmc_memory_width != FMC_NORSRAM_MEM_BUS_WIDTH_16) {
		LOG_ERR("Only supports 16-bit bus width");
		return -EINVAL;
	}

	uint32_t hclk_freq =
		STM32_AHB_PRESCALER * DT_PROP(STM32_CLOCK_CONTROL_NODE, clock_frequency);

	/* According to the FMC documentation*/
	fmc_write_cycles =
		((config->fmc_address_setup_time + 1) + (config->fmc_data_setup_time + 1)) * 1;

	if (hclk_freq / fmc_write_cycles > dbi_config->config.frequency) {
		LOG_ERR("Frequency is too high for the display controller");
		return -EINVAL;
	}

	data->dbi_config = dbi_config;
	return 0;
}

int mipi_dbi_stm32_fmc_command_write(const struct device *dev,
				     const struct mipi_dbi_config *dbi_config, uint8_t cmd,
				     const uint8_t *data_buf, size_t len)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;
	int ret;
	size_t i;

	ret = mipi_dbi_stm32_fmc_check_config(dev, dbi_config);
	if (ret < 0) {
		return ret;
	}

	sys_write16(cmd, config->register_addr);
	if (IS_ENABLED(CONFIG_MIPI_DBI_STM32_FMC_MEM_BARRIER)) {
		barrier_dsync_fence_full();
	}

	for (i = 0U; i < len; i++) {
		sys_write16((uint16_t)data_buf[i], config->data_addr);
		if (IS_ENABLED(CONFIG_MIPI_DBI_STM32_FMC_MEM_BARRIER)) {
			barrier_dsync_fence_full();
		}
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
	size_t i;
	int ret;

	ret = mipi_dbi_stm32_fmc_check_config(dev, dbi_config);
	if (ret < 0) {
		return ret;
	}

	for (i = 0U; i < desc->buf_size; i += 2) {
		sys_write16(sys_get_le16(&framebuf[i]), config->data_addr);
		if (IS_ENABLED(CONFIG_MIPI_DBI_STM32_FMC_MEM_BARRIER)) {
			barrier_dsync_fence_full();
		}
	}

	return 0;
}

static int mipi_dbi_stm32_fmc_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_stm32_fmc_config *config = dev->config;
	int ret;

	if (config->reset.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}

	k_sleep(delay);

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

#define MIPI_DBI_FMC_GET_ADDRESS(n) _CONCAT(FMC_BANK1_, UTIL_INC(DT_REG_ADDR(DT_INST_PARENT(n))))

#define MIPI_DBI_FMC_GET_DATA_ADDRESS(n)                                                           \
	MIPI_DBI_FMC_GET_ADDRESS(n) + (1 << (DT_INST_PROP(n, register_select_pin) + 1))

#define MIPI_DBI_STM32_FMC_INIT(n)                                                                 \
	static const struct mipi_dbi_stm32_fmc_config mipi_dbi_stm32_fmc_config_##n = {            \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
		.power = GPIO_DT_SPEC_INST_GET_OR(n, power_gpios, {}),                             \
		.register_addr = MIPI_DBI_FMC_GET_ADDRESS(n),                                      \
		.data_addr = MIPI_DBI_FMC_GET_DATA_ADDRESS(n),                                     \
		.fmc_address_setup_time = DT_PROP_BY_IDX(DT_INST_PARENT(n), st_timing, 0),         \
		.fmc_data_setup_time = DT_PROP_BY_IDX(DT_INST_PARENT(n), st_timing, 2),            \
		.fmc_memory_width = DT_PROP_BY_IDX(DT_INST_PARENT(n), st_control, 2),              \
	};                                                                                         \
                                                                                                   \
	static struct mipi_dbi_stm32_fmc_data mipi_dbi_stm32_fmc_data_##n;                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_stm32_fmc_init, NULL, &mipi_dbi_stm32_fmc_data_##n,      \
			      &mipi_dbi_stm32_fmc_config_##n, POST_KERNEL,                         \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_stm32_fmc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_STM32_FMC_INIT)
