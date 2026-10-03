/*
 * Copyright (c) 2026 Jasper Jonker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT newhaven_us2066_spi

#include "auxdisplay_us2066.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(auxdisplay_us2066, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* The serial write path cannot read the busy flag, so every byte is paced. */
#define US2066_SHORT_DELAY         K_MSEC(1)
#define US2066_CLEAR_HOME_DELAY    K_MSEC(2)
#define US2066_I2C_CONTROL_COMMAND 0x00
#define US2066_I2C_CONTROL_DATA    0x40
#define US2066_SPI_CONTROL_COMMAND 0xf8
#define US2066_SPI_CONTROL_DATA    0xfa

struct auxdisplay_us2066_bus {
	int (*init)(const struct device *dev);
	int (*write)(const struct device *dev, bool data, uint8_t value);
};

struct auxdisplay_us2066_data {
	bool power;
	bool cursor;
	bool blinking;
	uint8_t brightness;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct auxdisplay_us2066_config {
	const struct auxdisplay_us2066_bus *bus;
	struct gpio_dt_spec reset_gpio;
	struct i2c_dt_spec i2c;
	struct spi_dt_spec spi;
	struct auxdisplay_capabilities capabilities;
	uint16_t io_voltage_mv;
	uint8_t contrast;
	uint8_t clock_divider;
	uint8_t clock_freq;
	uint8_t phase1_period;
	uint8_t phase2_period;
	uint8_t vcomh_level;
	uint8_t func_n_bit;          /* Pre-calculated N bit for line configuration */
	uint8_t func_nw_bit;         /* Pre-calculated NW bit for line configuration */
	uint8_t font_width;          /* Pre-calculated font width bit (5 or 6 dot) */
	uint8_t cursor_invert;       /* Pre-calculated cursor invert bit */
	uint8_t entry_mode;          /* Pre-calculated entry mode flags (I/D and S bits) */
	uint8_t display_orientation; /* Pre-calculated BDC/BDS orientation flags */
};

/* Get DDRAM row addresses based on row count */
static const uint8_t *get_row_addresses(uint8_t rows)
{
	static const uint8_t addr_1_line[]  = {0x00};
	static const uint8_t addr_2_lines[] = {0x00, 0x20};
	static const uint8_t addr_3_lines[] = {0x00, 0x10, 0x20};
	static const uint8_t addr_4_lines[] = {0x00, 0x20, 0x40, 0x60};

	switch (rows) {
	case 1:
		return addr_1_line;
	case 2:
		return addr_2_lines;
	case 3:
		return addr_3_lines;
	case 4:
	default:
		return addr_4_lines;
	}
}

#if DT_HAS_COMPAT_STATUS_OKAY(newhaven_us2066_i2c)
static int auxdisplay_us2066_write_i2c(const struct device *dev, bool data, uint8_t value)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	uint8_t tx_data[] = {
		data ? US2066_I2C_CONTROL_DATA : US2066_I2C_CONTROL_COMMAND,
		value,
	};
	int rc;

	rc = i2c_write_dt(&cfg->i2c, tx_data, sizeof(tx_data));
	if (rc < 0) {
		LOG_ERR("Failed to send %s byte 0x%02x over I2C [%d]", data ? "data" : "command",
			value, rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_init_i2c(const struct device *dev)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct auxdisplay_us2066_bus auxdisplay_us2066_i2c_bus = {
	.init = auxdisplay_us2066_init_i2c,
	.write = auxdisplay_us2066_write_i2c,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(newhaven_us2066_i2c) */

#if DT_HAS_COMPAT_STATUS_OKAY(newhaven_us2066_spi)
/* Reverse the bit order within one nibble for the US2066 serial transfer format. */
static uint8_t auxdisplay_us2066_reverse_nibble(uint8_t value)
{
	value &= 0x0f;

	return ((value & BIT(0)) << 3) | ((value & BIT(1)) << 1) |
	       ((value & BIT(2)) >> 1) | ((value & BIT(3)) >> 3);
}

/*
 * Write one command or data byte using the US2066 SPI-compatible serial format.
 * The first byte is the start/control byte, followed by the low and high nibbles
 * with each nibble bit-reversed and shifted into the upper half of the byte.
 */
static int auxdisplay_us2066_write_spi(const struct device *dev, bool data, uint8_t value)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	uint8_t tx_data[] = {
		data ? US2066_SPI_CONTROL_DATA : US2066_SPI_CONTROL_COMMAND,
		auxdisplay_us2066_reverse_nibble(value) << 4,
		auxdisplay_us2066_reverse_nibble(value >> 4) << 4,
	};
	const struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = sizeof(tx_data),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	int rc;

	rc = spi_write_dt(&cfg->spi, &tx);
	if (rc < 0) {
		LOG_ERR("Failed to send %s byte 0x%02x over SPI [%d]",
			data ? "data" : "command", value, rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_init_spi(const struct device *dev)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct auxdisplay_us2066_bus auxdisplay_us2066_spi_bus = {
	.init = auxdisplay_us2066_init_spi,
	.write = auxdisplay_us2066_write_spi,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(newhaven_us2066_i2c) */

static int auxdisplay_us2066_write_bus(const struct device *dev, bool data, uint8_t value)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;

	return cfg->bus->write(dev, data, value);
}

static int auxdisplay_us2066_command(const struct device *dev, uint8_t cmd)
{
	int rc;

	rc = auxdisplay_us2066_write_bus(dev, false, cmd);
	if (rc < 0) {
		return rc;
	}

	if (cmd == US2066_CMD_CLEAR_DISPLAY || cmd == US2066_CMD_RETURN_HOME) {
		k_sleep(US2066_CLEAR_HOME_DELAY);
	} else {
		k_sleep(US2066_SHORT_DELAY);
	}

	return 0;
}

static int auxdisplay_us2066_data(const struct device *dev, uint8_t data)
{
	int rc;

	rc = auxdisplay_us2066_write_bus(dev, true, data);
	if (rc < 0) {
		return rc;
	}

	k_sleep(US2066_SHORT_DELAY);
	return 0;
}

static int auxdisplay_us2066_apply_display_control(const struct device *dev)
{
	struct auxdisplay_us2066_data *data = dev->data;
	uint8_t cmd;

	cmd = (data->power ? US2066_DISPLAY_ON : 0) |
	      (data->cursor ? US2066_DISPLAY_CURSOR_ON : 0) |
	      (data->blinking ? US2066_DISPLAY_BLINK_ON : 0);

	return auxdisplay_us2066_command(dev, US2066_CMD_DISPLAY_CTRL | cmd);
}

static int auxdisplay_us2066_display_on(const struct device *dev)
{
	struct auxdisplay_us2066_data *data = dev->data;

	data->power = true;

	return auxdisplay_us2066_apply_display_control(dev);
}

static int auxdisplay_us2066_display_off(const struct device *dev)
{
	struct auxdisplay_us2066_data *data = dev->data;

	data->power = false;

	return auxdisplay_us2066_apply_display_control(dev);
}

static int auxdisplay_us2066_cursor_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_us2066_data *data = dev->data;

	data->cursor = enable;

	return auxdisplay_us2066_apply_display_control(dev);
}

static int auxdisplay_us2066_position_blinking_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_us2066_data *data = dev->data;

	data->blinking = enable;

	return auxdisplay_us2066_apply_display_control(dev);
}

static int auxdisplay_us2066_cursor_position_set(const struct device *dev,
						 enum auxdisplay_position type, int16_t x,
						 int16_t y)
{
	const struct auxdisplay_us2066_config *config = dev->config;
	struct auxdisplay_us2066_data *data = dev->data;
	const uint8_t *row_addr = get_row_addresses(config->capabilities.rows);
	uint8_t addr;
	int rc;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -EINVAL;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	} else if (x >= config->capabilities.columns || y >= config->capabilities.rows) {
		return -EINVAL;
	}

	/* Send hardware command first, only update tracking if successful */
	addr = US2066_CMD_SET_DDRAM_ADDR | (row_addr[y] + x);
	rc = auxdisplay_us2066_command(dev, addr);
	if (rc) {
		return rc;
	}

	/* Update software cursor tracking after successful hardware update */
	data->cursor_x = (uint16_t)x;
	data->cursor_y = (uint16_t)y;

	return 0;
}

static int auxdisplay_us2066_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_us2066_data *data = dev->data;

	*x = (int16_t)data->cursor_x;
	*y = (int16_t)data->cursor_y;

	return 0;
}

static int auxdisplay_us2066_capabilities_get(const struct device *dev,
					      struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_us2066_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_us2066_brightness_get(const struct device *dev, uint8_t *brightness)
{
	struct auxdisplay_us2066_data *data = dev->data;

	*brightness = data->brightness;

	return 0;
}

static int auxdisplay_us2066_brightness_set(const struct device *dev, uint8_t brightness)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	struct auxdisplay_us2066_data *data = dev->data;
	int rc;

	/* Enter extended mode (RE=1) before using OLED commands */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode [%d]", rc);
		return rc;
	}

	/* Enter OLED command set */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to enable OLED command set [%d]", rc);
		return rc;
	}

	/* Set contrast */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_CONTRAST);
	if (rc < 0) {
		LOG_ERR("Failed to send set contrast command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, brightness);
	if (rc < 0) {
		LOG_ERR("Failed to send brightness value [%d]", rc);
		return rc;
	}

	/* Exit OLED command set */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to disable OLED command set [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	data->brightness = brightness;

	return 0;
}

/* Write text */
static int auxdisplay_us2066_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct auxdisplay_us2066_config *config = dev->config;
	struct auxdisplay_us2066_data *data = dev->data;
	const uint8_t *row_addr = get_row_addresses(config->capabilities.rows);
	int rc;

	for (uint16_t i = 0; i < len; i++) {
		/* Check if we need to wrap to next line before writing */
		if (data->cursor_x >= config->capabilities.columns) {
			data->cursor_x = 0;
			data->cursor_y++;

			if (data->cursor_y >= config->capabilities.rows) {
				data->cursor_y = 0;
			}

			rc = auxdisplay_us2066_cursor_position_set(
				dev, AUXDISPLAY_POSITION_ABSOLUTE, data->cursor_x, data->cursor_y);
			if (rc < 0) {
				LOG_ERR("Failed to set cursor position [%d]", rc);
				return rc;
			}
		}

		/* Write the character */
		rc = auxdisplay_us2066_data(dev, text[i]);
		if (rc < 0) {
			LOG_ERR("Failed to write character 0x%02x [%d]", text[i], rc);
			return rc;
		}

		/* Update cursor position tracking */
		data->cursor_x++;

		/* After writing to last column, explicitly set hardware cursor for next row
		 * This prevents hardware DDRAM auto-increment from going to invalid addresses
		 */
		if (data->cursor_x >= config->capabilities.columns) {
			uint16_t next_x = 0;
			uint16_t next_y = data->cursor_y + 1;
			uint8_t addr;

			if (next_y >= config->capabilities.rows) {
				next_y = 0;
			}

			/* Update hardware cursor position to start of next line */
			addr = US2066_CMD_SET_DDRAM_ADDR | (row_addr[next_y] + next_x);
			rc = auxdisplay_us2066_command(dev, addr);
			if (rc < 0) {
				LOG_ERR("Failed to set DDRAM address [%d]", rc);
				return rc;
			}

			/* Update software tracking */
			data->cursor_x = next_x;
			data->cursor_y = next_y;
		}
	}

	return 0;
}

static int auxdisplay_us2066_clear(const struct device *dev)
{
	struct auxdisplay_us2066_data *data = dev->data;
	int rc;

	data->cursor_x = 0;
	data->cursor_y = 0;

	/* Clear Display */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_CLEAR_DISPLAY);
	if (rc < 0) {
		LOG_ERR("Failed to clear display [%d]", rc);
		return rc;
	}

	/* Return Home */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_RETURN_HOME);
	if (rc < 0) {
		LOG_ERR("Failed to return home [%d]", rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_set_scroll_enable(const struct device *dev, uint8_t lines)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	int rc;

	/* Select IS=1 while RE=0, then RE=1 to access shift/scroll enable commands. */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_IS_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to select special registers for scroll [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode for scroll [%d]", rc);
		return rc;
	}

	/* Set extended function set: 5-dot font, scroll enable (DH'=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET_EXTENDED | cfg->font_width |
						    cfg->func_nw_bit | US2066_DH_DOT_SHIFT);
	if (rc < 0) {
		LOG_ERR("Failed to enable scroll mode [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_CMD_SCROLL_ENABLE | (lines & 0x0f));
	if (rc < 0) {
		LOG_ERR("Failed to set scroll lines [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0, IS=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_set_scroll_quantity(const struct device *dev, uint8_t quantity)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	int rc;

	if (quantity > US2066_SCROLL_QUANTITY_MAX) {
		LOG_ERR("Invalid scroll quantity: %d", quantity);
		return -EINVAL;
	}

	/* Enter extended mode (RE=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode [%d]", rc);
		return rc;
	}

	/* Set scroll quantity */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_SET_SCROLL_QTY | (quantity & 0x3F));
	if (rc < 0) {
		LOG_ERR("Failed to set scroll quantity [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_set_shift_enable(const struct device *dev, uint8_t lines)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	int rc;

	/* Select IS=1 while RE=0, then RE=1 to access shift/scroll enable commands. */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_IS_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to select special registers for shift [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode [%d]", rc);
		return rc;
	}

	/* Set extended function set to enable shift mode (DH'=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET_EXTENDED | cfg->font_width |
						    cfg->func_nw_bit | US2066_DH_DISPLAY_SHIFT);
	if (rc < 0) {
		LOG_ERR("Failed to enable shift mode [%d]", rc);
		return rc;
	}

	/* Enable shift for specified lines */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_SHIFT_ENABLE | (lines & 0x0F));
	if (rc < 0) {
		LOG_ERR("Failed to set shift lines [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_display_shift(const struct device *dev, bool right)
{
	/* Use cursor/display shift command in fundamental mode (RE=0) */
	return auxdisplay_us2066_command(dev,
					 US2066_CMD_CURSOR_SHIFT | US2066_SHIFT_DISPLAY |
						 (right ? US2066_SHIFT_RIGHT : US2066_SHIFT_LEFT));
}

static int auxdisplay_us2066_set_fade_blink(const struct device *dev, uint8_t mode,
					    uint8_t interval)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	int rc;

	/* Validate mode */
	if (mode != US2066_FADE_BLINK_DISABLE && mode != US2066_FADE_OUT_ENABLE &&
	    mode != US2066_BLINK_ENABLE) {
		LOG_ERR("Invalid fade/blink mode: %d", mode);
		return -EINVAL;
	}

	/* Validate interval */
	if (interval > US2066_FADE_INTERVAL_MAX) {
		LOG_ERR("Invalid fade/blink interval: %d", interval);
		return -EINVAL;
	}

	/* Enter extended mode (RE=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to enable OLED command set [%d]", rc);
		return rc;
	}

	/* Set fade/blink: OLED command followed by its command-parameter byte. */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_FADE_BLINK);
	if (rc < 0) {
		LOG_ERR("Failed to send fade/blink command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, mode | US2066_FADE_INTERVAL(interval));
	if (rc < 0) {
		LOG_ERR("Failed to set fade/blink mode [%d]", rc);
		return rc;
	}

	/* Disable OLED command set */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to disable OLED command set [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	/* Small delay to ensure command is processed */
	k_usleep(100);

	return 0;
}

static int auxdisplay_us2066_set_double_height(const struct device *dev, uint8_t upper_lines,
					       uint8_t lower_lines)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	int rc;

	/* Enable double height in function set (RE=0, DH=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit |
						    US2066_FUNC_DOUBLE_HEIGHT_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to enable double height [%d]", rc);
		return rc;
	}

	/* Enter extended mode (RE=1) to set which lines show upper/lower parts */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to enter extended mode [%d]", rc);
		return rc;
	}

	/* Set which line pairs show upper/lower halves */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_DH_DOT_SHIFT | upper_lines | lower_lines);
	if (rc < 0) {
		LOG_ERR("Failed to set double height lines [%d]", rc);
		return rc;
	}

	/* Return to fundamental mode (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit |
						    US2066_FUNC_DOUBLE_HEIGHT_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to return to fundamental mode [%d]", rc);
		return rc;
	}

	return 0;
}

static int auxdisplay_us2066_clear_double_height(const struct device *dev)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;

	/* Disable double height in function set (RE=0, DH=0) and return to normal height */
	return auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
}

static int auxdisplay_us2066_custom_command(const struct device *dev,
					    struct auxdisplay_custom_data *command)
{
	if (command == NULL) {
		return -EINVAL;
	}

	switch (command->options) {
	case AUXDISPLAY_US2066_CUSTOM_SET_SCROLL_ENABLE:
		if (command->data == NULL || command->len != 1) {
			return -EINVAL;
		}

		return auxdisplay_us2066_set_scroll_enable(dev, command->data[0]);

	case AUXDISPLAY_US2066_CUSTOM_SET_SCROLL_QUANTITY:
		if (command->data == NULL || command->len != 1) {
			return -EINVAL;
		}

		return auxdisplay_us2066_set_scroll_quantity(dev, command->data[0]);

	case AUXDISPLAY_US2066_CUSTOM_SET_SHIFT_ENABLE:
		if (command->data == NULL || command->len != 1) {
			return -EINVAL;
		}

		return auxdisplay_us2066_set_shift_enable(dev, command->data[0]);

	case AUXDISPLAY_US2066_CUSTOM_DISPLAY_SHIFT:
		if (command->data == NULL || command->len != 1 ||
		    command->data[0] > AUXDISPLAY_US2066_DISPLAY_SHIFT_RIGHT) {
			return -EINVAL;
		}

		return auxdisplay_us2066_display_shift(
			dev, command->data[0] == AUXDISPLAY_US2066_DISPLAY_SHIFT_RIGHT);

	case AUXDISPLAY_US2066_CUSTOM_SET_FADE_BLINK:
		if (command->data == NULL || command->len != 2) {
			return -EINVAL;
		}

		return auxdisplay_us2066_set_fade_blink(dev, command->data[0], command->data[1]);

	case AUXDISPLAY_US2066_CUSTOM_SET_DOUBLE_HEIGHT:
		if (command->data == NULL || command->len != 2) {
			return -EINVAL;
		}

		return auxdisplay_us2066_set_double_height(dev, command->data[0], command->data[1]);

	case AUXDISPLAY_US2066_CUSTOM_CLEAR_DOUBLE_HEIGHT:
		if (command->len != 0) {
			return -EINVAL;
		}

		return auxdisplay_us2066_clear_double_height(dev);

	default:
		return -EINVAL;
	}
}

static int auxdisplay_us2066_init(const struct device *dev)
{
	const struct auxdisplay_us2066_config *cfg = dev->config;
	struct auxdisplay_us2066_data *data = dev->data;
	int rc;

	rc = cfg->bus->init(dev);
	if (rc < 0) {
		return rc;
	}

	if (cfg->reset_gpio.port) {
		rc = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("Failed to configure reset GPIO [%d]", rc);
			return rc;
		}
		k_msleep(1);

		/* Assert reset */
		rc = gpio_pin_set_dt(&cfg->reset_gpio, 1);
		if (rc < 0) {
			LOG_ERR("Failed to assert reset [%d]", rc);
			return rc;
		}
		k_msleep(20);

		/* Release reset */
		rc = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (rc < 0) {
			LOG_ERR("Failed to release reset [%d]", rc);
			return rc;
		}
		k_msleep(150);
	}

	k_msleep(10);

	/* Convert VCOMH level to register value */
	const uint8_t vcomh_values[] = {US2066_VCOMH_0_65_VCC, US2066_VCOMH_0_71_VCC,
					US2066_VCOMH_0_77_VCC, US2066_VCOMH_0_83_VCC,
					US2066_VCOMH_1_00_VCC};
	uint8_t vcomh_level = cfg->vcomh_level;
	uint8_t vcomh;

	if (vcomh_level >= ARRAY_SIZE(vcomh_values)) {
		/* Clamp to the highest supported VCOMH level */
		vcomh_level = ARRAY_SIZE(vcomh_values) - 1;
	}
	vcomh = vcomh_values[vcomh_level];

	/* Initialization sequence per datasheet (Table 11) */

	/* Function set: extended command set (RE=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit |
						    US2066_FUNC_RE_EXTENDED);
	if (rc < 0) {
		LOG_ERR("Failed to set extended function set [%d]", rc);
		return rc;
	}

	/* Function selection A: Set VDD regulator based on I/O voltage */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNC_SELECTION_A);
	if (rc < 0) {
		LOG_ERR("Failed to send function selection A command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_data(dev, (cfg->io_voltage_mv == 5000) ? US2066_FSA_VDD_5V
								      : US2066_FSA_VDD_3V3);
	if (rc < 0) {
		LOG_ERR("Failed to set VDD regulator [%d]", rc);
		return rc;
	}

	/* Function set: fundamental command set (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to set fundamental function set [%d]", rc);
		return rc;
	}

	/* Display off */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_DISPLAY_CTRL);
	if (rc < 0) {
		LOG_ERR("Failed to turn display off [%d]", rc);
		return rc;
	}

	/* Function set: extended command set (RE=1) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit |
						    US2066_FUNC_RE_EXTENDED);
	if (rc < 0) {
		LOG_ERR("Failed to set extended function set [%d]", rc);
		return rc;
	}

	/* OLED command set enabled */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to enable OLED command set [%d]", rc);
		return rc;
	}

	/* Set display clock divide ratio/oscillator frequency */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_DISPLAY_CLK);
	if (rc < 0) {
		LOG_ERR("Failed to set display clock command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_CLK_DIVIDER(cfg->clock_divider) |
						    US2066_CLK_FREQ(cfg->clock_freq));
	if (rc < 0) {
		LOG_ERR("Failed to set clock divider/frequency [%d]", rc);
		return rc;
	}

	/* OLED command set disabled */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to disable OLED command set [%d]", rc);
		return rc;
	}

	/* Extended function set: 5-dot font */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET_EXTENDED | cfg->font_width |
						    cfg->cursor_invert | cfg->func_nw_bit);
	if (rc < 0) {
		LOG_ERR("Failed to set extended function set [%d]", rc);
		return rc;
	}

	/* COM-SEG direction */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_ENTRY_MODE_SET | cfg->display_orientation);
	if (rc < 0) {
		LOG_ERR("Failed to set COM-SEG direction [%d]", rc);
		return rc;
	}

	/* Function selection B: ROM selection */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNC_SELECTION_B);
	if (rc < 0) {
		LOG_ERR("Failed to send function selection B command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_data(
		dev,
		US2066_FSB_ROM_A | US2066_FSB_CGRAM_8_A); /* ROM CGRAM selection per datasheet */
	if (rc < 0) {
		LOG_ERR("Failed to set ROM selection [%d]", rc);
		return rc;
	}

	/* Switch to fundamental mode (RE=0) for cursor/shift entry mode */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to set fundamental function set [%d]", rc);
		return rc;
	}

	/* Entry mode: cursor direction and shift (I/D, S bits in RE=0 mode) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_ENTRY_MODE_SET | cfg->entry_mode);
	if (rc < 0) {
		LOG_ERR("Failed to set entry mode [%d]", rc);
		return rc;
	}

	/* Function set: extended command set (RE=1) for OLED config */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | US2066_FUNC_RE_EXTENDED |
						    cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to set extended function set [%d]", rc);
		return rc;
	}

	/* OLED command set enabled */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_ENABLE);
	if (rc < 0) {
		LOG_ERR("Failed to enable OLED command set [%d]", rc);
		return rc;
	}

	/* Set SEG pins hardware configuration */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_SEG_PINS);
	if (rc < 0) {
		LOG_ERR("Failed to send set SEG pins command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_SEG_PINS_ALTERNATIVE);
	if (rc < 0) {
		LOG_ERR("Failed to set SEG pins alternative [%d]", rc);
		return rc;
	}

	/* Function selection C: Internal VSL, GPIO input disable */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_FUNC_SELECTION_C);
	if (rc < 0) {
		LOG_ERR("Failed to send function selection C command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev,
				       US2066_FSC_VSL_INTERNAL | US2066_FSC_GPIO_HIZ_INPUT_OFF);
	if (rc < 0) {
		LOG_ERR("Failed to set VSL and GPIO config [%d]", rc);
		return rc;
	}

	/* Set contrast control */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_CONTRAST);
	if (rc < 0) {
		LOG_ERR("Failed to send set contrast command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, cfg->contrast);
	if (rc < 0) {
		LOG_ERR("Failed to set contrast value [%d]", rc);
		return rc;
	}

	/* Set phase length */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_PHASE_LENGTH);
	if (rc < 0) {
		LOG_ERR("Failed to send set phase length command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, US2066_PHASE1(cfg->phase1_period) |
						    US2066_PHASE2(cfg->phase2_period));
	if (rc < 0) {
		LOG_ERR("Failed to set phase periods [%d]", rc);
		return rc;
	}

	/* Set VCOMH deselect level */
	rc = auxdisplay_us2066_command(dev, US2066_OLED_SET_VCOMH);
	if (rc < 0) {
		LOG_ERR("Failed to send set VCOMH command [%d]", rc);
		return rc;
	}

	rc = auxdisplay_us2066_command(dev, vcomh);
	if (rc < 0) {
		LOG_ERR("Failed to set VCOMH level [%d]", rc);
		return rc;
	}

	/* OLED command set disabled */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_OLED_DISABLE);
	if (rc < 0) {
		LOG_ERR("Failed to disable OLED command set [%d]", rc);
		return rc;
	}

	/* Function set: fundamental command set (RE=0) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_FUNCTION_SET | cfg->func_n_bit);
	if (rc < 0) {
		LOG_ERR("Failed to set fundamental function set [%d]", rc);
		return rc;
	}

	/* Clear display */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_CLEAR_DISPLAY);
	if (rc < 0) {
		LOG_ERR("Failed to clear display [%d]", rc);
		return rc;
	}
	k_msleep(2); /* Required pause after clear display */

	/* Set DDRAM address 0x00 (cursor home) */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_SET_DDRAM_ADDR);
	if (rc < 0) {
		LOG_ERR("Failed to set DDRAM address [%d]", rc);
		return rc;
	}

	/* Display ON, cursor off, blink off */
	rc = auxdisplay_us2066_command(dev, US2066_CMD_DISPLAY_CTRL | US2066_DISPLAY_ON);
	if (rc < 0) {
		LOG_ERR("Failed to turn display on [%d]", rc);
		return rc;
	}

	data->power = true;
	data->cursor = false;
	data->blinking = false;
	data->brightness = cfg->contrast;

	k_msleep(100); /* Wait for stabilization after display on */

	return 0;
}

/* API struct */
static DEVICE_API(auxdisplay, auxdisplay_us2066_auxdisplay_api) = {
	.display_on = auxdisplay_us2066_display_on,
	.display_off = auxdisplay_us2066_display_off,
	.cursor_set_enabled = auxdisplay_us2066_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_us2066_position_blinking_set_enabled,
	.cursor_position_set = auxdisplay_us2066_cursor_position_set,
	.cursor_position_get = auxdisplay_us2066_cursor_position_get,
	.capabilities_get = auxdisplay_us2066_capabilities_get,
	.clear = auxdisplay_us2066_clear,
	.brightness_get = auxdisplay_us2066_brightness_get,
	.brightness_set = auxdisplay_us2066_brightness_set,
	.write = auxdisplay_us2066_write,
	.custom_command = auxdisplay_us2066_custom_command,
};

/* Device instantiation macros */
#define AUXDISPLAY_US2066_COMMON_CONFIG(n)                                             \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                   \
	.io_voltage_mv = DT_INST_PROP(n, io_voltage_mv),                               \
	.contrast = DT_INST_PROP(n, contrast),                                         \
	.clock_divider = DT_INST_PROP(n, clock_divider),                               \
	.clock_freq = DT_INST_PROP(n, clock_freq),                                     \
	.phase1_period = DT_INST_PROP(n, phase1_period),                               \
	.phase2_period = DT_INST_PROP(n, phase2_period),                               \
	.vcomh_level = DT_INST_PROP(n, vcomh_level),                                   \
	.func_n_bit = ((DT_INST_PROP(n, rows) == 2 || DT_INST_PROP(n, rows) == 4)      \
			       ? US2066_FUNC_2_4_LINES                                     \
			       : US2066_FUNC_1_3_LINES),                                   \
	.func_nw_bit = ((DT_INST_PROP(n, rows) == 3 || DT_INST_PROP(n, rows) == 4)     \
				? US2066_FUNC_EXT_LINES_3_4                                \
				: US2066_FUNC_EXT_LINES_1_2),                              \
	.font_width = (DT_INST_PROP(n, font_width) == 6)                               \
			      ? US2066_FUNC_EXT_FONT_WIDTH_6                               \
			      : US2066_FUNC_EXT_FONT_WIDTH_5,                              \
	.cursor_invert = DT_INST_PROP_OR(n, cursor_invert, false)                      \
				 ? US2066_FUNC_EXT_INVERT_ENABLE                       \
				 : US2066_FUNC_EXT_INVERT_DISABLE,                     \
	.entry_mode = ((DT_INST_PROP_OR(n, cursor_direction_left, false)               \
				? US2066_ENTRY_DEC                                       \
				: US2066_ENTRY_INC) |                                    \
		       (DT_INST_PROP_OR(n, display_shift_on_write, false)                  \
				? US2066_ENTRY_SHIFT                                     \
				: US2066_ENTRY_NO_SHIFT)),                               \
	.display_orientation = ((DT_INST_PROP_OR(n, display_rotate_180, false)         \
					 ? US2066_ENTRY_BDS                            \
					 : US2066_ENTRY_BDC) |                         \
				(DT_INST_PROP_OR(n, display_mirror_horizontal, false)     \
					 ? (US2066_ENTRY_BDC | US2066_ENTRY_BDS)       \
					 : 0)),                                        \
	.capabilities =                                                                \
		{                                                                      \
			.columns = DT_INST_PROP(n, columns),                           \
			.rows = DT_INST_PROP(n, rows),                                 \
			.mode = 0,                                                     \
			.brightness.minimum = 0,                                       \
			.brightness.maximum = 255,                                     \
			.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,           \
			.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,           \
			.custom_characters = 0,                                        \
			.custom_character_width = 0,                                   \
			.custom_character_height = 0,                                  \
		}

#define AUXDISPLAY_US2066_DATA(name)                                                   \
	static struct auxdisplay_us2066_data auxdisplay_us2066_data_##name = {         \
		.power = true,                                                         \
		.cursor = false,                                                       \
		.blinking = false,                                                     \
		.brightness = 0,                                                       \
		.cursor_x = 0,                                                         \
		.cursor_y = 0,                                                         \
	}

#define AUXDISPLAY_US2066_I2C_INST(n)                                                  \
	static const struct auxdisplay_us2066_config auxdisplay_us2066_i2c_config_##n = {  \
		.bus = &auxdisplay_us2066_i2c_bus,                                         \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                            \
		AUXDISPLAY_US2066_COMMON_CONFIG(n),                                        \
	};                                                                                 \
	AUXDISPLAY_US2066_DATA(i2c_##n);                                                   \
	DEVICE_DT_INST_DEFINE(n, &auxdisplay_us2066_init, NULL,                           \
			      &auxdisplay_us2066_data_i2c_##n,                             \
			      &auxdisplay_us2066_i2c_config_##n, POST_KERNEL,              \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_us2066_auxdisplay_api);

#define AUXDISPLAY_US2066_SPI_INST(n)                                                  \
	static const struct auxdisplay_us2066_config auxdisplay_us2066_spi_config_##n = {  \
		.bus = &auxdisplay_us2066_spi_bus,                                         \
		.spi = SPI_DT_SPEC_INST_GET(                                               \
			n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)),       \
		AUXDISPLAY_US2066_COMMON_CONFIG(n),                                        \
	};                                                                                 \
	AUXDISPLAY_US2066_DATA(spi_##n);                                                   \
	DEVICE_DT_INST_DEFINE(n, &auxdisplay_us2066_init, NULL,                           \
			      &auxdisplay_us2066_data_spi_##n,                             \
			      &auxdisplay_us2066_spi_config_##n, POST_KERNEL,              \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_us2066_auxdisplay_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT newhaven_us2066_i2c
DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_US2066_I2C_INST)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT newhaven_us2066_spi
DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_US2066_SPI_INST)
