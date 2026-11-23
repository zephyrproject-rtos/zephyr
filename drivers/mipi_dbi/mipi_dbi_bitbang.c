/*
 * MIPI DBI Type A and B driver using GPIO
 *
 * Copyright 2024 Stefan Gloor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mipi_dbi_bitbang

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_bitbang, CONFIG_MIPI_DBI_LOG_LEVEL);

/* The MIPI DBI spec allows 8, 9, and 16 bits */
#define MIPI_DBI_MAX_DATA_BUS_WIDTH 16

/* Compile in a data bus LUT for improved performance if at least one instance uses an 8-bit bus */
#define _8_BIT_MODE_PRESENT(n) (DT_INST_PROP_LEN(n, data_gpios) == 8) |
#define MIPI_DBI_8_BIT_MODE    DT_INST_FOREACH_STATUS_OKAY(_8_BIT_MODE_PRESENT) 0

struct mipi_dbi_bitbang_config {
	/* Parallel 8080/6800 data GPIOs */
	const struct gpio_dt_spec data[MIPI_DBI_MAX_DATA_BUS_WIDTH];
	const uint8_t data_bus_width;

	/* Read (type B) GPIO */
	const struct gpio_dt_spec rd;

	/* Write (type B) or Read/!Write (type A) GPIO */
	const struct gpio_dt_spec wr;

	/* Enable/strobe GPIO (type A) */
	const struct gpio_dt_spec e;

	/* Chip-select GPIO */
	const struct gpio_dt_spec cs;

	/* Command/Data GPIO */
	const struct gpio_dt_spec cmd_data;

	/* Reset GPIO */
	const struct gpio_dt_spec reset;

#if MIPI_DBI_8_BIT_MODE
	/* Byte-write LUT, one slot per GPIO port the 8-bit bus spans, filled from slot 0. An
	 * empty first slot selects the per-pin path.
	 */
	const struct {
		const struct device *dev;
		uint32_t mask;
		const uint32_t *words;
	} lut_port[CONFIG_MIPI_DBI_BITBANG_MAX_LUT_PORTS];
#endif
};

struct mipi_dbi_bitbang_data {
	struct k_mutex lock;
};

static inline void mipi_dbi_bitbang_set_data_gpios(const struct mipi_dbi_bitbang_config *config,
						   uint32_t value)
{
#if MIPI_DBI_8_BIT_MODE
	if (config->lut_port[0].mask != 0) {
		int p = 0;

		do {
			gpio_port_set_masked(config->lut_port[p].dev, config->lut_port[p].mask,
					     config->lut_port[p].words[value]);
			p++;
		} while (p < ARRAY_SIZE(config->lut_port) && config->lut_port[p].mask != 0);
		return;
	}
#endif
	for (int i = 0; i < config->data_bus_width; i++) {
		gpio_pin_set_dt(&config->data[i], (value & (1 << i)) != 0);
	}
}

static int mipi_dbi_bitbang_write_helper(const struct device *dev,
					 const struct mipi_dbi_config *dbi_config, bool cmd_present,
					 uint8_t cmd, const uint8_t *data_buf, size_t len)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	struct mipi_dbi_bitbang_data *data = dev->data;
	int ret = 0;
	uint8_t value;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (dbi_config->mode) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
		gpio_pin_set_dt(&config->cs, 1);
		if (cmd_present) {
			gpio_pin_set_dt(&config->wr, 0);
			gpio_pin_set_dt(&config->cmd_data, 0);
			mipi_dbi_bitbang_set_data_gpios(config, cmd);
			gpio_pin_set_dt(&config->wr, 1);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->wr, 0);
				mipi_dbi_bitbang_set_data_gpios(config, value);
				gpio_pin_set_dt(&config->wr, 1);
				len--;
			}
		}
		gpio_pin_set_dt(&config->cs, 0);
		break;

	/* Clocked E mode */
	case MIPI_DBI_MODE_6800_BUS_8_BIT:
	case MIPI_DBI_MODE_6800_BUS_9_BIT:
	case MIPI_DBI_MODE_6800_BUS_16_BIT:
		gpio_pin_set_dt(&config->cs, 1);
		gpio_pin_set_dt(&config->wr, 0);
		if (cmd_present) {
			gpio_pin_set_dt(&config->e, 1);
			gpio_pin_set_dt(&config->cmd_data, 0);
			mipi_dbi_bitbang_set_data_gpios(config, cmd);
			gpio_pin_set_dt(&config->e, 0);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->e, 1);
				mipi_dbi_bitbang_set_data_gpios(config, value);
				gpio_pin_set_dt(&config->e, 0);
				len--;
			}
		}
		gpio_pin_set_dt(&config->cs, 0);
		break;

	default:
		LOG_ERR("MIPI DBI mode %u is not supported.", dbi_config->mode);
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int mipi_dbi_bitbang_command_write(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					  const uint8_t *data_buf, size_t len)
{
	return mipi_dbi_bitbang_write_helper(dev, dbi_config, true, cmd, data_buf, len);
}

static int mipi_dbi_bitbang_write_display(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config,
					  const uint8_t *framebuf,
					  struct display_buffer_descriptor *desc,
					  enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);

	return mipi_dbi_bitbang_write_helper(dev, dbi_config, false, 0x0, framebuf, desc->buf_size);
}

static int mipi_dbi_bitbang_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	int ret;

	LOG_DBG("Performing hw reset.");

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}
	k_sleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_bitbang_init(const struct device *dev)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	struct mipi_dbi_bitbang_data *data = dev->data;
	const char *failed_pin = NULL;
	int ret = 0;

	k_mutex_init(&data->lock);

	if (gpio_is_ready_dt(&config->cmd_data)) {
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cmd_data";
			goto fail;
		}
		gpio_pin_set_dt(&config->cmd_data, 0);
	}
	if (gpio_is_ready_dt(&config->rd)) {
		gpio_pin_configure_dt(&config->rd, GPIO_OUTPUT_ACTIVE);
		/* Don't emit an error because this pin is unused in type A */
		gpio_pin_set_dt(&config->rd, 1);
	}
	if (gpio_is_ready_dt(&config->wr)) {
		ret = gpio_pin_configure_dt(&config->wr, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "wr";
			goto fail;
		}
		gpio_pin_set_dt(&config->wr, 1);
	}
	if (gpio_is_ready_dt(&config->e)) {
		gpio_pin_configure_dt(&config->e, GPIO_OUTPUT_ACTIVE);
		/* Don't emit an error because this pin is unused in type B */
		gpio_pin_set_dt(&config->e, 0);
	}
	if (gpio_is_ready_dt(&config->cs)) {
		ret = gpio_pin_configure_dt(&config->cs, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cs";
			goto fail;
		}
		gpio_pin_set_dt(&config->cs, 0);
	}
	if (gpio_is_ready_dt(&config->reset)) {
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "reset";
			goto fail;
		}
		gpio_pin_set_dt(&config->reset, 0);
	}
	for (int i = 0; i < config->data_bus_width; i++) {
		if (gpio_is_ready_dt(&config->data[i])) {
			ret = gpio_pin_configure_dt(&config->data[i], GPIO_OUTPUT_ACTIVE);
			if (ret < 0) {
				failed_pin = "data";
				goto fail;
			}
			gpio_pin_set_dt(&config->data[i], 0);
		}
	}

	return ret;
fail:
	LOG_ERR("Failed to configure %s GPIO pin.", failed_pin);
	return ret;
}

static DEVICE_API(mipi_dbi, mipi_dbi_bitbang_driver_api) = {
	.reset = mipi_dbi_bitbang_reset,
	.command_write = mipi_dbi_bitbang_command_write,
	.write_display = mipi_dbi_bitbang_write_display
};

#if MIPI_DBI_8_BIT_MODE

/* The 8 data pins are grouped by GPIO controller. Each group gets a table (MIPI_DBI_BB_LUT_TABLES)
 * and a config slot (MIPI_DBI_BB_LUT_SLOTS). Neither is emitted for a bus that is not 8 bits wide
 * or that has more groups than CONFIG_MIPI_DBI_BITBANG_MAX_LUT_PORTS.
 */

#define MIPI_DBI_BB_CTLR(n, k)    DT_GPIO_CTLR_BY_IDX(DT_DRV_INST(n), data_gpios, k)
#define MIPI_DBI_BB_SAME(n, i, j) DT_SAME_NODE(MIPI_DBI_BB_CTLR(n, i), MIPI_DBI_BB_CTLR(n, j))
#define MIPI_DBI_BB_EACH_PIN(fn, n)                                                                \
	fn(n, 0) fn(n, 1) fn(n, 2) fn(n, 3) fn(n, 4) fn(n, 5) fn(n, 6) fn(n, 7)

/* 1 if no earlier pin is on the controller of pin i, i.e. pin i starts its group */
#define MIPI_DBI_BB_EARLIER(j, n, i) IF_ENABLED(MIPI_DBI_BB_SAME(n, j, i), (1,))
#define MIPI_DBI_BB_IS_FIRST(n, i)                                                                 \
	IS_EQ(NUM_VA_ARGS_LESS_1(LISTIFY(i, MIPI_DBI_BB_EARLIER, (), n, i) _), 0)

#define MIPI_DBI_BB_COUNT_FIRST(n, i) IF_ENABLED(MIPI_DBI_BB_IS_FIRST(n, i), (1,))
#define MIPI_DBI_BB_NUM_GROUPS(n)                                                                  \
	NUM_VA_ARGS_LESS_1(MIPI_DBI_BB_EACH_PIN(MIPI_DBI_BB_COUNT_FIRST, n) _)

/* 1 if k groups fit: the first CONFIG_MIPI_DBI_BITBANG_MAX_LUT_PORTS entries are 1 */
#define MIPI_DBI_BB_ONE(i, _) 1
#define MIPI_DBI_BB_FITS(k)                                                                        \
	GET_ARG_N(k, LISTIFY(CONFIG_MIPI_DBI_BITBANG_MAX_LUT_PORTS, MIPI_DBI_BB_ONE, (,)),         \
		  0, 0, 0, 0, 0, 0, 0, 0)

#define MIPI_DBI_BB_USE_LUT(n)                                                                     \
	COND_CODE_1(IS_EQ(DT_INST_PROP_LEN(n, data_gpios), 8),                                     \
		    (MIPI_DBI_BB_FITS(MIPI_DBI_BB_NUM_GROUPS(n))), (0))

/* Bit that pin k contributes to the port mask of group i, or 0 if it is on another controller */
#define MIPI_DBI_BB_PIN_BIT(k, n, i)                                                               \
	(MIPI_DBI_BB_SAME(n, k, i) ? (1U << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, k)) : 0)
#define MIPI_DBI_BB_GROUP_MASK(n, i) (LISTIFY(8, MIPI_DBI_BB_PIN_BIT, (|), n, i))
#define MIPI_DBI_BB_GROUP_BITS(n, i) LISTIFY(8, MIPI_DBI_BB_PIN_BIT, (,), n, i)

/* LUT entry for byte value val: the port bits of the set bits of val. The group's bits arrive
 * evaluated once as arguments, so the 256 entries need no devicetree lookups.
 */
#define MIPI_DBI_BB_LUT_ENTRY(val, b0, b1, b2, b3, b4, b5, b6, b7)                                 \
	((((val) & BIT(0)) ? (b0) : 0) | (((val) & BIT(1)) ? (b1) : 0) |                           \
	 (((val) & BIT(2)) ? (b2) : 0) | (((val) & BIT(3)) ? (b3) : 0) |                           \
	 (((val) & BIT(4)) ? (b4) : 0) | (((val) & BIT(5)) ? (b5) : 0) |                           \
	 (((val) & BIT(6)) ? (b6) : 0) | (((val) & BIT(7)) ? (b7) : 0))

/* Selected by token pasting so that only the chosen variant is expanded */
#define MIPI_DBI_BB_TABLE_0(n, i)
#define MIPI_DBI_BB_TABLE_1(n, i)                                                                  \
	static const uint32_t mipi_dbi_bitbang_lut_##n##_##i[256] = {                              \
		LISTIFY(256, MIPI_DBI_BB_LUT_ENTRY, (,), MIPI_DBI_BB_GROUP_BITS(n, i))};
#define MIPI_DBI_BB_TABLE(n, i) UTIL_CAT(MIPI_DBI_BB_TABLE_, MIPI_DBI_BB_IS_FIRST(n, i))(n, i)

#define MIPI_DBI_BB_SLOT_0(n, i)
#define MIPI_DBI_BB_SLOT_1(n, i)                                                                   \
	{.dev = DEVICE_DT_GET(MIPI_DBI_BB_CTLR(n, i)),                                             \
	 .mask = MIPI_DBI_BB_GROUP_MASK(n, i),                                                     \
	 .words = mipi_dbi_bitbang_lut_##n##_##i},
#define MIPI_DBI_BB_SLOT(n, i) UTIL_CAT(MIPI_DBI_BB_SLOT_, MIPI_DBI_BB_IS_FIRST(n, i))(n, i)

#define MIPI_DBI_BB_LUT_TABLES_0(n)
#define MIPI_DBI_BB_LUT_TABLES_1(n) MIPI_DBI_BB_EACH_PIN(MIPI_DBI_BB_TABLE, n)
#define MIPI_DBI_BB_LUT_TABLES(n) UTIL_CAT(MIPI_DBI_BB_LUT_TABLES_, MIPI_DBI_BB_USE_LUT(n))(n)

#define MIPI_DBI_BB_LUT_SLOTS_0(n)
#define MIPI_DBI_BB_LUT_SLOTS_1(n) .lut_port = {MIPI_DBI_BB_EACH_PIN(MIPI_DBI_BB_SLOT, n)},
#define MIPI_DBI_BB_LUT_SLOTS(n) UTIL_CAT(MIPI_DBI_BB_LUT_SLOTS_, MIPI_DBI_BB_USE_LUT(n))(n)
#else
#define MIPI_DBI_BB_LUT_TABLES(n)
#define MIPI_DBI_BB_LUT_SLOTS(n)
#endif

#define MIPI_DBI_BITBANG_INIT(n)                                                                   \
	MIPI_DBI_BB_LUT_TABLES(n)                                                                  \
	static const struct mipi_dbi_bitbang_config mipi_dbi_bitbang_config_##n = {                \
		.data = {GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 0, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 1, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 2, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 3, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 4, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 5, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 6, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 7, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 8, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 9, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 10, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 11, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 12, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 13, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 14, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 15, {0})},                 \
		.data_bus_width = DT_INST_PROP_LEN(n, data_gpios),                                 \
		.rd = GPIO_DT_SPEC_INST_GET_OR(n, rd_gpios, {}),                                   \
		.wr = GPIO_DT_SPEC_INST_GET_OR(n, wr_gpios, {}),                                   \
		.e = GPIO_DT_SPEC_INST_GET_OR(n, e_gpios, {}),                                     \
		.cs = GPIO_DT_SPEC_INST_GET_OR(n, cs_gpios, {}),                                   \
		.cmd_data = GPIO_DT_SPEC_INST_GET_OR(n, dc_gpios, {}),                             \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
		MIPI_DBI_BB_LUT_SLOTS(n)};                                                         \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, data_gpios) <= MIPI_DBI_MAX_DATA_BUS_WIDTH,               \
		     "Number of data GPIOs in DT exceeds MIPI_DBI_MAX_DATA_BUS_WIDTH");            \
	static struct mipi_dbi_bitbang_data mipi_dbi_bitbang_data_##n;                             \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_bitbang_init, NULL, &mipi_dbi_bitbang_data_##n,          \
			      &mipi_dbi_bitbang_config_##n, POST_KERNEL,                           \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_bitbang_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_BITBANG_INIT)
