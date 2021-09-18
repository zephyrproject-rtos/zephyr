/*
 * Copyright (c) 2021 Zisis Adamos <zisarono@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT samsung_ks0108

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/display.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(display_ks0108);

#define KS0108_WRITE_DATA_OR_CMD false
#define KS0108_INSTRUCTION false
#define KS0108_PIXEL_DATA true

#define KS0108_SET_RAM_ADR_CMD     ((uint8_t) 0xC0)
#define KS0108_SET_COLUMN_ADR_CMD  ((uint8_t) 0x40)
#define KS0108_SET_ROW_ADR_CMD     ((uint8_t) 0xB8)
#define KS0108_TURN_ON_CMD         ((uint8_t) 0x3F)
#define KS0108_TURN_OFF_CMD        ((uint8_t) 0x3E)

#define KS0108_BITS_IN_ROW         (8)
#define KS0108_MAX_COLS_IN_PAGE    (64)

#define KS0108_DISPLAY_DB0_INDEX        0
#define KS0108_DISPLAY_DB1_INDEX        1
#define KS0108_DISPLAY_DB2_INDEX        2
#define KS0108_DISPLAY_DB3_INDEX        3
#define KS0108_DISPLAY_DB4_INDEX        4
#define KS0108_DISPLAY_DB5_INDEX        5
#define KS0108_DISPLAY_DB6_INDEX        6
#define KS0108_DISPLAY_DB7_INDEX        7
#define KS0108_DISPLAY_RS_INDEX         8
#define KS0108_DISPLAY_RW_INDEX         9
#define KS0108_DISPLAY_EN_INDEX         10
#define KS0108_DISPLAY_CS1_INDEX        11
#define KS0108_DISPLAY_CS2_INDEX        12
#define KS0108_DISPLAY_RESET_INDEX      13

struct ks0108_gpio_data {
	const char *const name;
	gpio_dt_flags_t flags;
	gpio_pin_t pin;
};

struct ks0108_config {
	struct ks0108_gpio_data ks0108_pins[14];
	uint8_t width;
	uint8_t height;
};

struct ks0108_data {
	struct ks0108_config *configuration;
	const struct device *gpio_ports[14];
};

static void
ks0108_fill_data_pins(const struct device *dev, uint8_t byte)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;

	/* Fill register */
	for (uint8_t bit_iterator = 0; bit_iterator < 8; bit_iterator++) {
		if (0 < ((byte & (1 << bit_iterator)))) {
			gpio_pin_set(data->gpio_ports[bit_iterator],
				     config->ks0108_pins[bit_iterator].pin, 1);
		} else {
			gpio_pin_set(data->gpio_ports[bit_iterator],
				     config->ks0108_pins[bit_iterator].pin, 0);
		}
	}
}

static void
ks0108_rw_cmd_or_data(const struct device *dev, bool is_data, bool is_read_access, uint8_t byte)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;

	/* Set data mode. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_RS_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_RS_INDEX].pin, is_data);
	/* Set write mode. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_RW_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_RW_INDEX].pin, is_read_access);
	/* Wait for twl that covers tasu as well. */
	k_busy_wait(1);
	/* Bring E high */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_EN_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_EN_INDEX].pin, 1);
	/* Send data. */
	ks0108_fill_data_pins(dev, byte);
	/* Wait for twh that covers tdsu as well */
	k_busy_wait(1);
	/* Latch value. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_EN_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_EN_INDEX].pin, 0);
	/* LCD-controller busy for 12 us (max Tbusy according to datasheet) covers twl. */
	k_busy_wait(12);
}

static int ks0108_blanking_on(const struct device *dev)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;

	/* Choose left half. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 1);
	ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
			      KS0108_TURN_OFF_CMD);
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 0);

	/* Choose right half. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 1);
	ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
			      KS0108_TURN_OFF_CMD);
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 0);

	return 0;
}

static int ks0108_blanking_off(const struct device *dev)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;

	/* Choose left half. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 1);
	ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
			      KS0108_TURN_ON_CMD);
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 0);

	/* Choose right half. */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 1);
	ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
			      KS0108_TURN_ON_CMD);
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 0);

	return 0;
}

static int ks0108_read(const struct device *dev,
		       const uint16_t x,
		       const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       void *buf)
{
	return -ENOTSUP;
}

static int ks0108_write(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;
	uint8_t col_negative_offset;

	for (uint8_t row_iterator = (y / 8); row_iterator < ((y / 8) + (desc->height / 8));
	     row_iterator++) {

		for (uint8_t col_iterator = x; col_iterator < (x + desc->width); col_iterator++) {
			if (col_iterator >= KS0108_MAX_COLS_IN_PAGE) {
				/* Unselect left half */
				gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
					     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 0);
				/* Select right half */
				gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
					     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 1);
				ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION,
						      KS0108_WRITE_DATA_OR_CMD,
						      (uint8_t) KS0108_SET_ROW_ADR_CMD +
						      row_iterator);
				col_negative_offset = KS0108_MAX_COLS_IN_PAGE;
			} else {
				/* Unselect right half */
				gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
					     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 0);
				/* Select left half */
				gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
					     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 1);
				ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION,
						      KS0108_WRITE_DATA_OR_CMD,
						      (uint8_t) KS0108_SET_ROW_ADR_CMD +
						      row_iterator);
				col_negative_offset = 0;
			}
			ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
					      KS0108_SET_COLUMN_ADR_CMD +
					      (col_iterator - col_negative_offset));
			ks0108_rw_cmd_or_data(dev, KS0108_PIXEL_DATA, KS0108_WRITE_DATA_OR_CMD,
					      *((uint8_t *)buf++));
		}
	}
	return 0;
}

static void *ks0108_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int ks0108_set_brightness(const struct device *dev,
				 const uint8_t brightness)
{
	return -ENOTSUP;
}

static int ks0108_set_contrast(const struct device *dev,
			       const uint8_t contrast)
{
	return -ENOTSUP;
}

static void ks0108_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->screen_info = SCREEN_INFO_MONO_VTILED;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	capabilities->current_pixel_format = PIXEL_FORMAT_MONO01;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int ks0108_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_MONO01) {
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int ks0108_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void ks0108_write_zeroes_to_ram(const struct device *dev)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;
	uint8_t col_negative_offset;

	for (uint8_t col_iterator = 0; col_iterator < config->width; col_iterator++) {
		if (col_iterator >= KS0108_MAX_COLS_IN_PAGE) {
			/* Unselect left half */
			gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
				     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 0);
			/* Select right half */
			gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
				     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 1);
			col_negative_offset = KS0108_MAX_COLS_IN_PAGE;
		} else {
			/* Unselect right half */
			gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS2_INDEX],
				     config->ks0108_pins[KS0108_DISPLAY_CS2_INDEX].pin, 0);
			/* Select left half */
			gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_CS1_INDEX],
				     config->ks0108_pins[KS0108_DISPLAY_CS1_INDEX].pin, 1);
			col_negative_offset = 0;
		}
		for (uint8_t row_iterator = 0; row_iterator < (config->height / KS0108_BITS_IN_ROW);
		     row_iterator++) {
			/* Set page address (row) */
			ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
					      (uint8_t) KS0108_SET_ROW_ADR_CMD + row_iterator);
			ks0108_rw_cmd_or_data(dev, KS0108_INSTRUCTION, KS0108_WRITE_DATA_OR_CMD,
					      KS0108_SET_COLUMN_ADR_CMD +
					      (col_iterator - col_negative_offset));

			ks0108_rw_cmd_or_data(dev, KS0108_PIXEL_DATA, KS0108_WRITE_DATA_OR_CMD, 0);
		}
	}
}

static int ks0108_init(const struct device *dev)
{
	struct ks0108_config *config = (struct ks0108_config *)dev->config;
	struct ks0108_data *data = (struct ks0108_config *)dev->data;

	for (uint8_t gpio_iterator = 0; gpio_iterator < 14; gpio_iterator++) {
		data->gpio_ports[gpio_iterator] =
			device_get_binding(config->ks0108_pins[gpio_iterator].name);
		if (data->gpio_ports[gpio_iterator] != NULL) {
			if (0 >
			    gpio_pin_configure(data->gpio_ports[gpio_iterator],
					       config->ks0108_pins[gpio_iterator].pin,
					       GPIO_OUTPUT_LOW |
					       config->ks0108_pins[gpio_iterator].flags)) {
				LOG_ERR("Failure configure required gpio port");
				return -ENODEV;
			}
		} else {
			LOG_ERR("Coulld not bind required gpio port");
			return -ENODEV;
		}
	}
	/* Reset Display by turning low for at least 1us */
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_RESET_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_RESET_INDEX].pin, 0);
	k_busy_wait(2);
	gpio_pin_set(data->gpio_ports[KS0108_DISPLAY_RESET_INDEX],
		     config->ks0108_pins[KS0108_DISPLAY_RESET_INDEX].pin, 1);
	ks0108_write_zeroes_to_ram(dev);

	return 0;
}

static const struct display_driver_api ks0108_api = {
	.blanking_on = ks0108_blanking_on,
	.blanking_off = ks0108_blanking_off,
	.write = ks0108_write,
	.read = ks0108_read,
	.get_framebuffer = ks0108_get_framebuffer,
	.set_brightness = ks0108_set_brightness,
	.set_contrast = ks0108_set_contrast,
	.get_capabilities = ks0108_get_capabilities,
	.set_pixel_format = ks0108_set_pixel_format,
	.set_orientation = ks0108_set_orientation,
};

#define KS0108_INIT(inst)								     \
	static struct ks0108_data ks0108_data_ ## inst;					     \
											     \
	const static struct ks0108_config ks0108_config_ ## inst = {			     \
		.ks0108_pins = {							     \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db0_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db0_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db0_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db0_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db0_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db0_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db1_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db1_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db1_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db1_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db1_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db1_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db2_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db2_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db2_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db2_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db2_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db2_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db3_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db3_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db3_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db3_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db3_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db3_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db4_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db4_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db4_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db4_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db4_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db4_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db5_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db5_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db5_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db5_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db5_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db5_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db6_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db6_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db6_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db6_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db6_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db6_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db7_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, db7_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db7_gpios),			     \
				  DT_INST_GPIO_PIN(inst, db7_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, db7_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, db7_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rs_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, rs_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rs_gpios),			     \
				  DT_INST_GPIO_PIN(inst, rs_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rs_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, rs_gpios))                     }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rw_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, rw_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rw_gpios),			     \
				  DT_INST_GPIO_PIN(inst, rw_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rw_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, rw_gpios))                     }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, en_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, en_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, en_gpios),			     \
				  DT_INST_GPIO_PIN(inst, en_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, en_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, en_gpios))                     }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs1_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, cs1_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs1_gpios),			     \
				  DT_INST_GPIO_PIN(inst, cs1_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs1_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, cs1_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs2_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, cs2_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs2_gpios),			     \
				  DT_INST_GPIO_PIN(inst, cs2_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, cs2_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, cs2_gpios))                    }, \
			{ .name = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rst_gpios),			     \
				  DT_INST_GPIO_LABEL(inst, rst_gpios)),			     \
			  .pin = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rst_gpios),			     \
				  DT_INST_GPIO_PIN(inst, rst_gpios)),			     \
			  .flags = UTIL_AND(						     \
				  DT_INST_HAS_PROP(inst, rst_gpios),			     \
				  DT_INST_GPIO_FLAGS(inst, rst_gpios))                    }, \
		},									     \
		.width = UTIL_AND(							     \
			DT_INST_HAS_PROP(inst, rst_gpios),				     \
			DT_INST_PROP(inst, width)),					     \
		.height = UTIL_AND(							     \
			DT_INST_HAS_PROP(inst, rst_gpios),				     \
			DT_INST_PROP(inst, height)),					     \
	};										     \
											     \
	static struct ks0108_data ks0108_data_ ## inst = {				     \
		.configuration = &ks0108_config_ ## inst,				     \
	};										     \
	DEVICE_DT_INST_DEFINE(inst, ks0108_init, NULL,					     \
			      &ks0108_data_ ## inst, &ks0108_config_ ## inst,		     \
			      APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,		     \
			      &ks0108_api);

DT_INST_FOREACH_STATUS_OKAY(KS0108_INIT)
