/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 */

#define DT_DRV_COMPAT zephyr_board_id_gpio

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/board_id.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(board_id_gpio, CONFIG_BOARD_ID_LOG_LEVEL);

struct board_id_gpio_config {
	const struct gpio_dt_spec *gpios;
	uint8_t num_gpios;
};

struct board_id_gpio_data {
	uint32_t id;
};

static ssize_t board_id_gpio_read(const struct device *dev, uint8_t *buffer, size_t length)
{
	const struct board_id_gpio_config *cfg = dev->config;
	struct board_id_gpio_data *data = dev->data;
	/* Identity byte count needed for the strap count; emitted big-endian. */
	uint8_t id_bytes = DIV_ROUND_UP(cfg->num_gpios, 8U);
	uint32_t be_id = sys_cpu_to_be32(data->id);
	size_t copy_len = MIN(length, id_bytes);

	memcpy(buffer, (uint8_t *)&be_id + (sizeof(be_id) - id_bytes), copy_len);

	return (ssize_t)copy_len;
}

static int board_id_gpio_init(const struct device *dev)
{
	const struct board_id_gpio_config *cfg = dev->config;
	struct board_id_gpio_data *data = dev->data;
	uint32_t id = 0;
	int ret;

	for (uint8_t i = 0; i < cfg->num_gpios; i++) {
		if (!gpio_is_ready_dt(&cfg->gpios[i])) {
			LOG_ERR("GPIO %u not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->gpios[i], GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure GPIO %u (err %d)", i, ret);
			return ret;
		}
	}

	for (uint8_t i = 0; i < cfg->num_gpios; i++) {
		ret = gpio_pin_get_dt(&cfg->gpios[i]);
		if (ret < 0) {
			LOG_ERR("Failed to read GPIO %u (err %d)", i, ret);
			return -EIO;
		}

		id |= ((uint32_t)ret & 0x1U) << i;
	}

	data->id = id;

	return 0;
}

static DEVICE_API(board_id, board_id_gpio_api) = {
	.read = board_id_gpio_read,
};

#define BOARD_ID_GPIO_INIT(inst)                                                                   \
	static const struct gpio_dt_spec board_id_gpio_pins_##inst[] = {                           \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};        \
                                                                                                   \
	static const struct board_id_gpio_config board_id_gpio_config_##inst = {                   \
		.gpios = board_id_gpio_pins_##inst,                                                \
		.num_gpios = DT_INST_PROP_LEN(inst, gpios),                                        \
	};                                                                                         \
                                                                                                   \
	static struct board_id_gpio_data board_id_gpio_data_##inst;                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, board_id_gpio_init, NULL, &board_id_gpio_data_##inst,          \
			      &board_id_gpio_config_##inst, POST_KERNEL,                           \
			      CONFIG_BOARD_ID_INIT_PRIORITY, &board_id_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(BOARD_ID_GPIO_INIT)
