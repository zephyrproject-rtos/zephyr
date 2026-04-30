/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#define DT_DRV_COMPAT newhaven_nhd_k3z

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_nhd_k3z, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define NHD_K3Z_CMD_CLEAR_DISPLAY   0x01
#define NHD_K3Z_CMD_RETURN_HOME     0x02
#define NHD_K3Z_CMD_ENTRY_MODE      0x04
#define NHD_K3Z_CMD_DISPLAY_CONTROL 0x08
#define NHD_K3Z_CMD_CURSOR_SHIFT    0x10
#define NHD_K3Z_CMD_FUNCTION_SET    0x20
#define NHD_K3Z_CMD_SET_CGRAM_ADDR  0x40
#define NHD_K3Z_CMD_SET_DDRAM_ADDR  0x80
#define NHD_K3Z_CMD_SET_BACKLIGHT   0x53

#define NHD_K3Z_BACKLIGHT_MIN 1U
#define NHD_K3Z_BACKLIGHT_MAX 8U

#define NHD_K3Z_ENTRY_INCREMENT     BIT(1)
#define NHD_K3Z_ENTRY_SHIFT_DISPLAY BIT(0)

#define NHD_K3Z_DISPLAY_ON BIT(2)
#define NHD_K3Z_CURSOR_ON  BIT(1)
#define NHD_K3Z_BLINK_ON   BIT(0)

#define NHD_K3Z_SHIFT_DISPLAY BIT(3)
#define NHD_K3Z_SHIFT_RIGHT   BIT(2)

#define NHD_K3Z_CONTROL_COMMAND 0x00
#define NHD_K3Z_CONTROL_DATA    0x40

#define NHD_K3Z_UART_COMMAND_PREFIX 0xFE

#define NHD_K3Z_CUSTOM_CHARACTERS       8
#define NHD_K3Z_CUSTOM_CHARACTER_WIDTH  5
#define NHD_K3Z_CUSTOM_CHARACTER_HEIGHT 8

#define NHD_K3Z_COMMAND_DELAY_US 600U
#define NHD_K3Z_CLEAR_DELAY_US   2000U

#define NHD_K3Z_SPI_OPERATION (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

enum auxdisplay_nhd_k3z_bus {
	AUXDISPLAY_NHD_K3Z_BUS_I2C,
	AUXDISPLAY_NHD_K3Z_BUS_SPI,
	AUXDISPLAY_NHD_K3Z_BUS_UART,
};

struct auxdisplay_nhd_k3z_config {
	struct auxdisplay_capabilities capabilities;
	enum auxdisplay_nhd_k3z_bus bus_type;
	union {
		struct i2c_dt_spec i2c;
		struct spi_dt_spec spi;
		const struct device *uart;
	} bus;
};

struct auxdisplay_nhd_k3z_data {
	bool display_on;
	bool cursor_on;
	bool blink_on;
	uint8_t entry_mode;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

static int auxdisplay_nhd_k3z_write_raw(const struct device *dev, uint8_t prefix,
					const uint8_t *data, size_t len)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;

	if (config->bus_type == AUXDISPLAY_NHD_K3Z_BUS_I2C) {
		uint8_t buffer[16];
		size_t offset = 0;

		while (offset < len) {
			size_t chunk = MIN(len - offset, sizeof(buffer) - 1);

			buffer[0] = prefix;
			memcpy(&buffer[1], &data[offset], chunk);
			if (i2c_write_dt(&config->bus.i2c, buffer, chunk + 1) < 0) {
				return -EIO;
			}

			offset += chunk;
		}

		return 0;
	}

	if (config->bus_type == AUXDISPLAY_NHD_K3Z_BUS_SPI) {
		uint8_t buffer[16];
		size_t offset = 0;

		while (offset < len) {
			size_t chunk = MIN(len - offset, sizeof(buffer) - 1);
			struct spi_buf tx_buf = {
				.buf = buffer,
				.len = chunk + 1,
			};
			struct spi_buf_set tx = {
				.buffers = &tx_buf,
				.count = 1,
			};

			buffer[0] = prefix;
			memcpy(&buffer[1], &data[offset], chunk);
			if (spi_write_dt(&config->bus.spi, &tx) < 0) {
				return -EIO;
			}

			offset += chunk;
		}

		return 0;
	}

	if (config->bus_type == AUXDISPLAY_NHD_K3Z_BUS_UART) {
		uint8_t buffer[16];
		size_t offset = 0;

		while (offset < len) {
			size_t chunk = MIN(len - offset, sizeof(buffer) - 1);

			buffer[0] = prefix;
			memcpy(&buffer[1], &data[offset], chunk);
			for (size_t i = 0; i < chunk + 1; i++) {
				uart_poll_out(config->bus.uart, buffer[i]);
			}

			offset += chunk;
		}

		return 0;
	}

	return -ENOTSUP;
}

static int auxdisplay_nhd_k3z_command(const struct device *dev, uint8_t command)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;
	const uint8_t payload = command;
	int rc;

	if (config->bus_type == AUXDISPLAY_NHD_K3Z_BUS_UART) {
		rc = auxdisplay_nhd_k3z_write_raw(dev, NHD_K3Z_UART_COMMAND_PREFIX, &payload,
						  sizeof(payload));
	} else {
		rc = auxdisplay_nhd_k3z_write_raw(dev, NHD_K3Z_CONTROL_COMMAND, &payload,
						  sizeof(payload));
	}

	k_sleep(K_USEC(NHD_K3Z_COMMAND_DELAY_US));
	return rc;
}

static int auxdisplay_nhd_k3z_write_data(const struct device *dev, const uint8_t *data,
					 uint16_t len)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;

	if (len == 0U) {
		return 0;
	}

	if (config->bus_type == AUXDISPLAY_NHD_K3Z_BUS_UART) {
		for (uint16_t i = 0; i < len; i++) {
			uart_poll_out(config->bus.uart, data[i]);
		}

		return 0;
	}

	return auxdisplay_nhd_k3z_write_raw(dev, NHD_K3Z_CONTROL_DATA, data, len);
}

static uint8_t auxdisplay_nhd_k3z_row_offset(uint16_t row)
{
	static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

	if (row < ARRAY_SIZE(row_offsets)) {
		return row_offsets[row];
	}

	return row * 0x20;
}

static int auxdisplay_nhd_k3z_display_state_commit(const struct device *dev)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;
	uint8_t command = NHD_K3Z_CMD_DISPLAY_CONTROL;

	if (data->display_on) {
		command |= NHD_K3Z_DISPLAY_ON;
	}

	if (data->cursor_on) {
		command |= NHD_K3Z_CURSOR_ON;
	}

	if (data->blink_on) {
		command |= NHD_K3Z_BLINK_ON;
	}

	return auxdisplay_nhd_k3z_command(dev, command);
}

static int auxdisplay_nhd_k3z_display_on(const struct device *dev)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	data->display_on = true;
	return auxdisplay_nhd_k3z_display_state_commit(dev);
}

static int auxdisplay_nhd_k3z_display_off(const struct device *dev)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	data->display_on = false;
	return auxdisplay_nhd_k3z_display_state_commit(dev);
}

static int auxdisplay_nhd_k3z_cursor_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	data->cursor_on = enable;
	return auxdisplay_nhd_k3z_display_state_commit(dev);
}

static int auxdisplay_nhd_k3z_position_blinking_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	data->blink_on = enable;
	return auxdisplay_nhd_k3z_display_state_commit(dev);
}

static int auxdisplay_nhd_k3z_backlight_set(const struct device *dev, uint8_t brightness)
{
	int rc;

	if (brightness < NHD_K3Z_BACKLIGHT_MIN || brightness > NHD_K3Z_BACKLIGHT_MAX) {
		return -EINVAL;
	}

	rc = auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_SET_BACKLIGHT);
	if (rc < 0) {
		return rc;
	}

	return auxdisplay_nhd_k3z_write_data(dev, &brightness, sizeof(brightness));
}

static int auxdisplay_nhd_k3z_cursor_shift_set(const struct device *dev, uint8_t direction,
					       bool display_shift)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	if (direction >= AUXDISPLAY_DIRECTION_COUNT) {
		return -EINVAL;
	}

	data->entry_mode = NHD_K3Z_ENTRY_INCREMENT;
	if (direction == AUXDISPLAY_DIRECTION_LEFT) {
		data->entry_mode = 0;
	}
	if (display_shift) {
		data->entry_mode |= NHD_K3Z_ENTRY_SHIFT_DISPLAY;
	}

	return auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_ENTRY_MODE | data->entry_mode);
}

static int auxdisplay_nhd_k3z_cursor_position_set(const struct device *dev,
						  enum auxdisplay_position type, int16_t x,
						  int16_t y)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;
	struct auxdisplay_nhd_k3z_data *data = dev->data;
	uint16_t xpos;
	uint16_t ypos;

	if (type != AUXDISPLAY_POSITION_ABSOLUTE) {
		return -ENOTSUP;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	}

	xpos = (uint16_t)x;
	ypos = (uint16_t)y;

	if (xpos >= config->capabilities.columns || ypos >= config->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = xpos;
	data->cursor_y = ypos;

	return auxdisplay_nhd_k3z_command(
		dev, NHD_K3Z_CMD_SET_DDRAM_ADDR | (xpos + auxdisplay_nhd_k3z_row_offset(ypos)));
}

static int auxdisplay_nhd_k3z_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;

	return 0;
}

static int auxdisplay_nhd_k3z_display_position_set(const struct device *dev,
						   enum auxdisplay_position type, int16_t x,
						   int16_t y)
{
	uint8_t command = NHD_K3Z_CMD_CURSOR_SHIFT;

	if (type != AUXDISPLAY_POSITION_RELATIVE || y != 0 || (x != -1 && x != 1)) {
		return -ENOTSUP;
	}

	command |= NHD_K3Z_SHIFT_DISPLAY;
	if (x > 0) {
		command |= NHD_K3Z_SHIFT_RIGHT;
	}

	return auxdisplay_nhd_k3z_command(dev, command);
}

static int auxdisplay_nhd_k3z_display_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);

	return -ENOTSUP;
}

static int auxdisplay_nhd_k3z_capabilities_get(const struct device *dev,
					       struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;

	*capabilities = config->capabilities;
	return 0;
}

static int auxdisplay_nhd_k3z_clear(const struct device *dev)
{
	struct auxdisplay_nhd_k3z_data *data = dev->data;
	int rc;

	rc = auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_CLEAR_DISPLAY);
	k_sleep(K_USEC(NHD_K3Z_CLEAR_DELAY_US));

	data->cursor_x = 0;
	data->cursor_y = 0;

	return rc;
}

static int auxdisplay_nhd_k3z_custom_character_set(const struct device *dev,
						   struct auxdisplay_character *character)
{
	uint8_t character_data[NHD_K3Z_CUSTOM_CHARACTER_HEIGHT];
	uint8_t address;

	if (character->index >= NHD_K3Z_CUSTOM_CHARACTERS) {
		return -EINVAL;
	}

	for (uint8_t row = 0; row < NHD_K3Z_CUSTOM_CHARACTER_HEIGHT; row++) {
		character_data[row] = 0;
		for (uint8_t col = 0; col < NHD_K3Z_CUSTOM_CHARACTER_WIDTH; col++) {
			if (character->data[(row * NHD_K3Z_CUSTOM_CHARACTER_WIDTH) + col] > 0) {
				character_data[row] |=
					BIT(NHD_K3Z_CUSTOM_CHARACTER_WIDTH - col - 1);
			}
		}
	}

	address = character->index << 3;
	character->character_code = character->index;

	if (auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_SET_CGRAM_ADDR | address) < 0) {
		return -EIO;
	}

	return auxdisplay_nhd_k3z_write_data(dev, character_data, sizeof(character_data));
}

static int auxdisplay_nhd_k3z_write(const struct device *dev, const uint8_t *data, uint16_t len)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;
	struct auxdisplay_nhd_k3z_data *driver_data = dev->data;
	int rc;

	rc = auxdisplay_nhd_k3z_write_data(dev, data, len);
	if (rc < 0) {
		return rc;
	}

	if (len > 0U) {
		driver_data->cursor_x += len;
		driver_data->cursor_y += driver_data->cursor_x / config->capabilities.columns;
		driver_data->cursor_x %= config->capabilities.columns;
		driver_data->cursor_y %= config->capabilities.rows;
	}

	return 0;
}

static int auxdisplay_nhd_k3z_init(const struct device *dev)
{
	const struct auxdisplay_nhd_k3z_config *config = dev->config;
	struct auxdisplay_nhd_k3z_data *data = dev->data;
	int rc;

	switch (config->bus_type) {
	case AUXDISPLAY_NHD_K3Z_BUS_I2C:
		if (!device_is_ready(config->bus.i2c.bus)) {
			return -ENODEV;
		}
		break;
	case AUXDISPLAY_NHD_K3Z_BUS_SPI:
		if (!spi_is_ready_dt(&config->bus.spi)) {
			return -ENODEV;
		}
		break;
	case AUXDISPLAY_NHD_K3Z_BUS_UART:
		if (!device_is_ready(config->bus.uart)) {
			return -ENODEV;
		}
		break;
	default:
		return -ENOTSUP;
	}

	data->display_on = true;
	data->cursor_on = false;
	data->blink_on = false;
	data->entry_mode = NHD_K3Z_ENTRY_INCREMENT;
	data->cursor_x = 0;
	data->cursor_y = 0;

	rc = auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_FUNCTION_SET | BIT(3));
	if (rc < 0) {
		return rc;
	}

	rc = auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_ENTRY_MODE | data->entry_mode);
	if (rc < 0) {
		return rc;
	}

	rc = auxdisplay_nhd_k3z_display_state_commit(dev);
	if (rc < 0) {
		return rc;
	}

	rc = auxdisplay_nhd_k3z_clear(dev);
	if (rc < 0) {
		return rc;
	}

	return auxdisplay_nhd_k3z_command(dev, NHD_K3Z_CMD_RETURN_HOME);
}

static DEVICE_API(auxdisplay, auxdisplay_nhd_k3z_api) = {
	.capabilities_get = auxdisplay_nhd_k3z_capabilities_get,
	.write = auxdisplay_nhd_k3z_write,
	.clear = auxdisplay_nhd_k3z_clear,
	.display_on = auxdisplay_nhd_k3z_display_on,
	.display_off = auxdisplay_nhd_k3z_display_off,
	.cursor_set_enabled = auxdisplay_nhd_k3z_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_nhd_k3z_position_blinking_set_enabled,
	.backlight_set = auxdisplay_nhd_k3z_backlight_set,
	.cursor_shift_set = auxdisplay_nhd_k3z_cursor_shift_set,
	.cursor_position_set = auxdisplay_nhd_k3z_cursor_position_set,
	.cursor_position_get = auxdisplay_nhd_k3z_cursor_position_get,
	.display_position_set = auxdisplay_nhd_k3z_display_position_set,
	.display_position_get = auxdisplay_nhd_k3z_display_position_get,
	.custom_character_set = auxdisplay_nhd_k3z_custom_character_set,
};

#define AUXDISPLAY_NHD_K3Z_CONFIG_I2C(inst)                                                        \
	.bus_type = AUXDISPLAY_NHD_K3Z_BUS_I2C, .bus.i2c = I2C_DT_SPEC_INST_GET(inst)

#define AUXDISPLAY_NHD_K3Z_CONFIG_SPI(inst)                                                        \
	.bus_type = AUXDISPLAY_NHD_K3Z_BUS_SPI,                                                    \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, NHD_K3Z_SPI_OPERATION)

#define AUXDISPLAY_NHD_K3Z_CONFIG_UART(inst)                                                       \
	.bus_type = AUXDISPLAY_NHD_K3Z_BUS_UART, .bus.uart = DEVICE_DT_GET(DT_INST_BUS(inst))

#define AUXDISPLAY_NHD_K3Z_CONFIG_BUS(inst)                                                        \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                                     \
		    (AUXDISPLAY_NHD_K3Z_CONFIG_I2C(inst)),                                         \
		    (COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                        \
				 (AUXDISPLAY_NHD_K3Z_CONFIG_SPI(inst)),                            \
				 (AUXDISPLAY_NHD_K3Z_CONFIG_UART(inst)))))

#define AUXDISPLAY_NHD_K3Z_INIT(inst)                                                              \
	static struct auxdisplay_nhd_k3z_data auxdisplay_nhd_k3z_data_##inst;                      \
                                                                                                   \
	static const struct auxdisplay_nhd_k3z_config auxdisplay_nhd_k3z_config_##inst = {         \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(inst, columns),                            \
				.rows = DT_INST_PROP(inst, rows),                                  \
				.mode = 0,                                                         \
				.brightness =                                                      \
					{                                                          \
						.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,         \
						.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,         \
					},                                                         \
				.backlight =                                                       \
					{                                                          \
						.minimum = NHD_K3Z_BACKLIGHT_MIN,                  \
						.maximum = NHD_K3Z_BACKLIGHT_MAX,                  \
					},                                                         \
				.custom_characters = NHD_K3Z_CUSTOM_CHARACTERS,                    \
				.custom_character_width = NHD_K3Z_CUSTOM_CHARACTER_WIDTH,          \
				.custom_character_height = NHD_K3Z_CUSTOM_CHARACTER_HEIGHT,        \
			},                                                                         \
		AUXDISPLAY_NHD_K3Z_CONFIG_BUS(inst),                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, auxdisplay_nhd_k3z_init, NULL,                                 \
			      &auxdisplay_nhd_k3z_data_##inst, &auxdisplay_nhd_k3z_config_##inst,  \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &auxdisplay_nhd_k3z_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_NHD_K3Z_INIT)
