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

/* An 8-bit data bus has exactly 8 pins, so it can never span more than 8 distinct GPIO
 * controllers (one pin per port, worst case) -- this is a physical ceiling, not an arbitrary cap.
 */
#define MIPI_DBI_BITBANG_MAX_LUT_PORTS 8

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
	/* Byte-write LUT fast path: the 8 data pins are grouped by GPIO controller at compile
	 * time. Slot i is populated (mask != 0) only when data pin i is the lowest-index pin on
	 * its controller -- an 8-bit bus spanning K distinct controllers therefore populates
	 * exactly K of these 8 slots, each pointing at its own real 256-entry table; the other
	 * (8-K) slots cost only a null pointer + zero mask, not an unused table. A single-port bus
	 * (K=1) costs exactly what the old single-port-only LUT cost: one 256-entry table, nothing
	 * more. Tables live in separate top-level `static const` arrays (see
	 * MIPI_DBI_BITBANG_GROUP_TABLES) so unused slots never emit unused table data.
	 */
	const struct device *const lut_port_dev[MIPI_DBI_BITBANG_MAX_LUT_PORTS];
	const uint32_t lut_port_mask[MIPI_DBI_BITBANG_MAX_LUT_PORTS];
	const uint32_t *const lut_port_words[MIPI_DBI_BITBANG_MAX_LUT_PORTS];
#endif
};

struct mipi_dbi_bitbang_data {
	struct k_mutex lock;
};

static inline void mipi_dbi_bitbang_set_data_gpios(const struct mipi_dbi_bitbang_config *config,
						   struct mipi_dbi_bitbang_data *data,
						   uint32_t value)
{
	ARG_UNUSED(data);
#if MIPI_DBI_8_BIT_MODE
	if (config->data_bus_width == 8) {
		for (int p = 0; p < MIPI_DBI_BITBANG_MAX_LUT_PORTS; p++) {
			if (config->lut_port_mask[p] == 0) {
				continue;
			}
			gpio_port_set_masked(config->lut_port_dev[p], config->lut_port_mask[p],
					     config->lut_port_words[p][value]);
		}
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
			mipi_dbi_bitbang_set_data_gpios(config, data, cmd);
			gpio_pin_set_dt(&config->wr, 1);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->wr, 0);
				mipi_dbi_bitbang_set_data_gpios(config, data, value);
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
			mipi_dbi_bitbang_set_data_gpios(config, data, cmd);
			gpio_pin_set_dt(&config->e, 0);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->e, 1);
				mipi_dbi_bitbang_set_data_gpios(config, data, value);
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
	const char *failed_pin = NULL;
	int ret = 0;

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

#if MIPI_DBI_8_BIT_MODE
	if (config->data_bus_width == 8) {
		LOG_DBG("LUT optimization enabled");
	}
#endif

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

/* Compile-time port-grouping machinery: partitions an 8-bit data bus's 8 pins by GPIO
 * controller, entirely at preprocessor time, so the byte-write LUT fast path (above) generalizes
 * from "all 8 pins on one port" to "however many distinct ports this instance's pins actually
 * span" without paying flash for unused capacity. See MIPI_DBI_BITBANG_INIT's LUT_PORT_FIELDS_INIT
 * for how this feeds the config struct.
 */

#define _MIPI_DBI_BB_CTLR(n, k)    DT_GPIO_CTLR_BY_IDX(DT_DRV_INST(n), data_gpios, k)
#define _MIPI_DBI_BB_SAME(n, i, j) DT_SAME_NODE(_MIPI_DBI_BB_CTLR(n, i), _MIPI_DBI_BB_CTLR(n, j))

/* IS_REP_i(n): bare 0/1 token, 1 if pin i is the first pin on its GPIO controller. Uses
 * UTIL_OR/UTIL_NOT, not plain ||/!, so the result stays usable by COND_CODE_1/IF_ENABLED.
 */
#define MIPI_DBI_BB_IS_REP_0(n) 1
#define MIPI_DBI_BB_IS_REP_1(n) UTIL_NOT(_MIPI_DBI_BB_SAME(n, 0, 1))
#define MIPI_DBI_BB_IS_REP_2(n)                                                                    \
	UTIL_NOT(UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 2), _MIPI_DBI_BB_SAME(n, 1, 2)))
#define MIPI_DBI_BB_IS_REP_3(n)                                                                    \
	UTIL_NOT(UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 3),                                               \
			 UTIL_OR(_MIPI_DBI_BB_SAME(n, 1, 3), _MIPI_DBI_BB_SAME(n, 2, 3))))
#define MIPI_DBI_BB_IS_REP_4(n)                                                                    \
	UTIL_NOT(                                                                                  \
		UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 4),                                                \
			UTIL_OR(_MIPI_DBI_BB_SAME(n, 1, 4),                                        \
				UTIL_OR(_MIPI_DBI_BB_SAME(n, 2, 4), _MIPI_DBI_BB_SAME(n, 3, 4)))))
#define MIPI_DBI_BB_IS_REP_5(n)                                                                    \
	UTIL_NOT(UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 5),                                               \
			 UTIL_OR(_MIPI_DBI_BB_SAME(n, 1, 5),                                       \
				 UTIL_OR(_MIPI_DBI_BB_SAME(n, 2, 5),                               \
					 UTIL_OR(_MIPI_DBI_BB_SAME(n, 3, 5),                       \
						 _MIPI_DBI_BB_SAME(n, 4, 5))))))
#define MIPI_DBI_BB_IS_REP_6(n)                                                                    \
	UTIL_NOT(UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 6),                                               \
			 UTIL_OR(_MIPI_DBI_BB_SAME(n, 1, 6),                                       \
				 UTIL_OR(_MIPI_DBI_BB_SAME(n, 2, 6),                               \
					 UTIL_OR(_MIPI_DBI_BB_SAME(n, 3, 6),                       \
						 UTIL_OR(_MIPI_DBI_BB_SAME(n, 4, 6),               \
							 _MIPI_DBI_BB_SAME(n, 5, 6)))))))
#define MIPI_DBI_BB_IS_REP_7(n)                                                                    \
	UTIL_NOT(UTIL_OR(_MIPI_DBI_BB_SAME(n, 0, 7),                                               \
			 UTIL_OR(_MIPI_DBI_BB_SAME(n, 1, 7),                                       \
				 UTIL_OR(_MIPI_DBI_BB_SAME(n, 2, 7),                               \
					 UTIL_OR(_MIPI_DBI_BB_SAME(n, 3, 7),                       \
						 UTIL_OR(_MIPI_DBI_BB_SAME(n, 4, 7),               \
							 UTIL_OR(_MIPI_DBI_BB_SAME(n, 5, 7),       \
								 _MIPI_DBI_BB_SAME(n, 6, 7))))))))

/* Mask of every pin that shares a GPIO controller with pin i (used for both the runtime port
 * mask and to select which bits of the byte value feed this group's LUT entries).
 */
#define _MIPI_DBI_BB_MASK_TERM(k, n, i)                                                            \
	(_MIPI_DBI_BB_SAME(n, k, i) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, k)) : 0) |
#define MIPI_DBI_BB_GROUP_MASK(n, i) (LISTIFY(8, _MIPI_DBI_BB_MASK_TERM, (), n, i) 0)

/* Repeatedly called by the outer LISTIFY(256, ...) at compile time to generate one group's
 * 256-entry LUT: for byte value `val`, OR together the bit positions of whichever pins both
 * (a) are set in `val` and (b) belong to group `i`. Hardcodes all 8 candidate bits directly
 * (matching the original LUT_GEN's own style) rather than nesting another LISTIFY call here --
 * the preprocessor won't re-expand a macro (LISTIFY) from within its own already-expanding call
 * tree, so a LISTIFY(8, ...) inside a LISTIFY(256, ...) callback silently fails to expand.
 */
#define MIPI_DBI_BB_GROUP_LUT_ENTRY(val, n, i)                                                     \
	((((val) & (1 << 0)) && _MIPI_DBI_BB_SAME(n, 0, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 0))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 1)) && _MIPI_DBI_BB_SAME(n, 1, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 1))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 2)) && _MIPI_DBI_BB_SAME(n, 2, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 2))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 3)) && _MIPI_DBI_BB_SAME(n, 3, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 3))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 4)) && _MIPI_DBI_BB_SAME(n, 4, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 4))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 5)) && _MIPI_DBI_BB_SAME(n, 5, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 5))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 6)) && _MIPI_DBI_BB_SAME(n, 6, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 6))                               \
		  : 0) |                                                                           \
	 (((val) & (1 << 7)) && _MIPI_DBI_BB_SAME(n, 7, i)                                         \
		  ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 7))                               \
		  : 0))

/* Emits a real 256-entry `static const` table for group `i` only when pin `i` is that group's
 * representative (IS_REP); otherwise emits nothing, so non-representative slots cost zero flash.
 */
#define MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, i)                                                        \
	IF_ENABLED(MIPI_DBI_BB_IS_REP_##i(n),                                                      \
		   (static const uint32_t mipi_dbi_bitbang_lut_##n##_##i[256] = {                 \
			    LISTIFY(256, MIPI_DBI_BB_GROUP_LUT_ENTRY, (,), n, i)};))

#define MIPI_DBI_BITBANG_GROUP_TABLES(n)                                                           \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 0)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 1)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 2)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 3)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 4)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 5)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 6)                                                        \
	MIPI_DBI_BB_MAYBE_GROUP_TABLE(n, 7)

#define _MIPI_DBI_BB_DEV_ENTRY(n, i)                                                               \
	COND_CODE_1(MIPI_DBI_BB_IS_REP_##i(n), (DEVICE_DT_GET(_MIPI_DBI_BB_CTLR(n, i))), (NULL)),
#define _MIPI_DBI_BB_MASK_ENTRY(n, i)                                                              \
	COND_CODE_1(MIPI_DBI_BB_IS_REP_##i(n), (MIPI_DBI_BB_GROUP_MASK(n, i)), (0)),
#define _MIPI_DBI_BB_WORDS_ENTRY(n, i)                                                             \
	COND_CODE_1(MIPI_DBI_BB_IS_REP_##i(n), (mipi_dbi_bitbang_lut_##n##_##i), (NULL)),

#define LUT_PORT_FIELDS_INIT(n)                                                                    \
	.lut_port_dev = {_MIPI_DBI_BB_DEV_ENTRY(n, 0) _MIPI_DBI_BB_DEV_ENTRY(n, 1)                 \
				 _MIPI_DBI_BB_DEV_ENTRY(n, 2) _MIPI_DBI_BB_DEV_ENTRY(n, 3)         \
					 _MIPI_DBI_BB_DEV_ENTRY(n, 4) _MIPI_DBI_BB_DEV_ENTRY(n, 5) \
						 _MIPI_DBI_BB_DEV_ENTRY(n, 6)                      \
							 _MIPI_DBI_BB_DEV_ENTRY(n, 7)},            \
	.lut_port_mask = {_MIPI_DBI_BB_MASK_ENTRY(n, 0) _MIPI_DBI_BB_MASK_ENTRY(n, 1)              \
				  _MIPI_DBI_BB_MASK_ENTRY(n, 2) _MIPI_DBI_BB_MASK_ENTRY(n, 3)      \
					  _MIPI_DBI_BB_MASK_ENTRY(n, 4)                            \
						  _MIPI_DBI_BB_MASK_ENTRY(n, 5)                    \
							  _MIPI_DBI_BB_MASK_ENTRY(n, 6)            \
								  _MIPI_DBI_BB_MASK_ENTRY(n, 7)},  \
	.lut_port_words = {_MIPI_DBI_BB_WORDS_ENTRY(n, 0) _MIPI_DBI_BB_WORDS_ENTRY(                \
		n, 1) _MIPI_DBI_BB_WORDS_ENTRY(n, 2) _MIPI_DBI_BB_WORDS_ENTRY(n, 3)                \
				   _MIPI_DBI_BB_WORDS_ENTRY(n, 4) _MIPI_DBI_BB_WORDS_ENTRY(n, 5)   \
					   _MIPI_DBI_BB_WORDS_ENTRY(n, 6)                          \
						   _MIPI_DBI_BB_WORDS_ENTRY(n, 7)},
#else
#define MIPI_DBI_BITBANG_GROUP_TABLES(n)
#define LUT_PORT_FIELDS_INIT(n)
#endif

#define MIPI_DBI_BITBANG_INIT(n)                                                                   \
	MIPI_DBI_BITBANG_GROUP_TABLES(n)                                                           \
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
		LUT_PORT_FIELDS_INIT(n)};                                                          \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, data_gpios) <= MIPI_DBI_MAX_DATA_BUS_WIDTH,               \
		     "Number of data GPIOs in DT exceeds MIPI_DBI_MAX_DATA_BUS_WIDTH");            \
	static struct mipi_dbi_bitbang_data mipi_dbi_bitbang_data_##n;                             \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_bitbang_init, NULL, &mipi_dbi_bitbang_data_##n,          \
			      &mipi_dbi_bitbang_config_##n, POST_KERNEL,                           \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_bitbang_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_BITBANG_INIT)
