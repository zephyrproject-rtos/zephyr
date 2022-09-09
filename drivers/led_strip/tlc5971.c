/*
 * Copyright (c) 2022 Esco Medical ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc5971

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/led_strip/tlc5971.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlc5971, CONFIG_LED_STRIP_LOG_LEVEL);

struct tlc5971_config {
	struct spi_dt_spec bus;
	const uint8_t *color_mapping;
	uint8_t num_pixels;
	uint8_t num_colors;
};

struct tlc5971_data {
	uint8_t *data_buffer;
	uint8_t gbc_color_1;
	uint8_t gbc_color_2;
	uint8_t gbc_color_3;
	uint8_t control_data;
};

/** SPI operation word constant, SPI mode 0, CPOL = 0, CPHA = 0 */
#define TLC5971_SPI_OPERATION (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

/** Number of supported colors */
#define TLC5971_NUMBER_OF_COLORS 3

/**
 * @brief Number of RGB pixels per TLC5791 device
 *
 * The TLC5971 has 4x RGB outputs per device, where each RGB group constitues a pixel from this
 * drivers point of view.
 */
#define TLC5971_PIXELS_PER_DEVICE 4

/** Length in bytes of data packet per TLC5791 device */
#define TLC5971_PACKET_LEN 28

/** write command for writing control data and GS data to internal registers */
#define TLC5971_WRITE_COMMAND 0x25

/** GS reference clock select bit in FC data (0 = internal oscillator clock, 1 = SCKI clock). */
#define TLC5971_BYTE27_CTRL_BIT_EXTGCK BIT(0)

/** GS reference clock edge select bit for OUTXn on-off timing control in FC data */
#define TLC5971_BYTE27_CTRL_BIT_OUTTMG BIT(1)

/** Constant-current output enable bit in FC data (0 = output control enabled, 1 = blank). */
#define TLC5971_BYTE26_CTRL_BIT_BLANK BIT(5)

/** Auto display repeat mode enable bit in FC data (0 = disabled, 1 = enabled). */
#define TLC5971_BYTE26_CTRL_BIT_DSPRPT BIT(6)

/** Display timing reset mode enable bit in FC data (0 = disabled, 1 = enabled). */
#define TLC5971_BYTE26_CTRL_BIT_TMGRST BIT(7)

/** Bit mask for write cmd in data byte 27 */
#define TLC5971_BYTE27_WRITE_CMD_MASK GENMASK(7, 2)

/** Bit mask for control bits in data byte 27 */
#define TLC5971_BYTE27_CTRL_MASK GENMASK(1, 0)

/** Bit mask for control bits in data byte 26 */
#define TLC5971_BYTE26_CTRL_MASK GENMASK(7, 5)

/** Bit mask for global brightness control for color 1 in data byte 26, upper 5 bits of GBC */
#define TLC5971_BYTE26_GBC1_MASK GENMASK(4, 0)

/** Bit mask for global brightness control for color 1 in data byte 25, lower 2 bits of GBC */
#define TLC5971_BYTE25_GBC1_MASK GENMASK(7, 6)

/** Bit mask for global brightness control for color 2 in data byte 25, upper 6 bits of GBC */
#define TLC5971_BYTE25_GBC2_MASK GENMASK(5, 0)

/** Bit mask for global brightness control for color 2 in data byte 24, lower 1 bits of GBC */
#define TLC5971_BYTE24_GBC2_MASK BIT(7)

/** Bit mask for global brightness control for color 3 in data byte 24, all 7 bits of GBC */
#define TLC5971_BYTE24_GBC3_MASK GENMASK(6, 0)

/**
 * @brief create data byte 27 from control data
 *
 * @param control_data control bits
 * @return uint8_t the serialized data byte 27
 */
static inline uint8_t tlc5971_data_byte27(uint8_t control_data)
{
	return FIELD_PREP(TLC5971_BYTE27_WRITE_CMD_MASK, TLC5971_WRITE_COMMAND) |
	       FIELD_PREP(TLC5971_BYTE27_CTRL_MASK, control_data);
}

/**
 * @brief create data byte 26 from control data and color 1 GBC
 *
 * @param control_data control bits
 * @param gbc_color_1 global brightness control for color 1 LEDs
 * @return uint8_t the serialized data byte 26
 */
static inline uint8_t tlc5971_data_byte26(uint8_t control_data, uint8_t gbc_color_1)
{
	return FIELD_PREP(TLC5971_BYTE26_CTRL_MASK, control_data) |
	       FIELD_PREP(TLC5971_BYTE26_GBC1_MASK, gbc_color_1 >> 2);
}

/**
 * @brief create data byte 25 from color 1 and 2 GBC
 *
 * @param gbc_color_1 global brightness control for color 1 LEDs
 * @param gbc_color_2 global brightness control for color 2 LEDs
 * @return uint8_t the serialized data byte 25
 */
static inline uint8_t tlc5971_data_byte25(uint8_t gbc_color_1, uint8_t gbc_color_2)
{
	return FIELD_PREP(TLC5971_BYTE25_GBC1_MASK, gbc_color_1 << 6) |
	       FIELD_PREP(TLC5971_BYTE25_GBC2_MASK, gbc_color_2 >> 1);
}

/**
 * @brief create data byte 24 from color 2 and 3 GBC
 *
 * @param gbc_color_2 global brightness control for color 2 LEDs
 * @param gbc_color_3 global brightness control for color 3 LEDs
 * @return uint8_t the serialized data byte 24
 */
static inline uint8_t tlc5971_data_byte24(uint8_t gbc_color_2, uint8_t gbc_color_3)
{
	return FIELD_PREP(TLC5971_BYTE24_GBC2_MASK, gbc_color_2 << 7) |
	       FIELD_PREP(TLC5971_BYTE24_GBC3_MASK, gbc_color_3);
}

/**
 * @brief map user colors to tlc5971 color order
 *
 * @param color_id color id from color mapping
 * @param pixel_data rgb data to be mapped
 * @return the mapped color value
 */
static uint8_t tlc5971_map_color(int color_id, const struct led_rgb *pixel_data)
{
	uint8_t temp = 0;

	switch (color_id) {
	case LED_COLOR_ID_RED:
		temp = pixel_data->r;
		break;
	case LED_COLOR_ID_GREEN:
		temp = pixel_data->g;
		break;
	case LED_COLOR_ID_BLUE:
		temp = pixel_data->b;
		break;
	default:
		temp = 0;
		break;
	}

	return temp;
}

/**
 * @brief serialize control data and pixel data for device daisy chain
 *
 * the serializer only supports "full" devices, meaning each device is expected
 * to be mounted with all 4 LEDs.
 *
 * @param dev device pointer
 * @param pixels pixel RGB data for daisy chain
 * @param num_pixels number of pixels in daisy chain
 */
static void tlc5971_fill_data_buffer(const struct device *dev, struct led_rgb *pixels,
				     size_t num_pixels)
{
	const struct tlc5971_config *cfg = dev->config;
	struct tlc5971_data *data = dev->data;
	uint8_t *data_buffer = data->data_buffer;
	int count = 0;

	/*
	 * tlc5971 device order is reversed as the rgb data for the last device in the daisy chain
	 * should be transmitted first.
	 */
	for (int device = (num_pixels / TLC5971_PIXELS_PER_DEVICE) - 1; device >= 0; device--) {
		/*
		 * The SPI frame format expects a BGR color order for the global brightness control
		 * values, but since the led_strip API allows custom color mappings, we simply use
		 * color_x terms to keep things generic.
		 */
		data_buffer[count++] = tlc5971_data_byte27(data->control_data);
		data_buffer[count++] = tlc5971_data_byte26(data->control_data, data->gbc_color_1);
		data_buffer[count++] = tlc5971_data_byte25(data->gbc_color_1, data->gbc_color_2);
		data_buffer[count++] = tlc5971_data_byte24(data->gbc_color_2, data->gbc_color_3);

		for (int pixel = (TLC5971_PIXELS_PER_DEVICE - 1); pixel >= 0; pixel--) {
			/* data is "reversed" so RGB0 comes last, i.e at byte 0 */
			const struct led_rgb *pixel_data =
				&pixels[(device * TLC5971_PIXELS_PER_DEVICE) + pixel];

			/*
			 * Convert pixel data into SPI frames, mapping user colors to tlc5971
			 * data frame color order (BGR).
			 */
			for (int color = 0; color < cfg->num_colors; color++) {
				uint8_t temp =
					tlc5971_map_color(cfg->color_mapping[color], pixel_data);

				/*
				 * The tlc5971 rgb values are 16 bit but zephyr's rgb values are
				 * 8 bit. Simply upscale to 16 bit by using the 8 bit value for both
				 * LSB and MSB of the 16 bit word.
				 */
				data_buffer[count++] = temp;
				data_buffer[count++] = temp;
			}
		}
	}
}

static int tlc5971_transmit_data(const struct device *dev, size_t num_pixels)
{
	const struct tlc5971_config *cfg = dev->config;
	struct tlc5971_data *data = dev->data;

	struct spi_buf buf = {
		.buf = data->data_buffer,
		.len = (num_pixels / TLC5971_PIXELS_PER_DEVICE) * TLC5971_PACKET_LEN,
	};

	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1,
	};

	return spi_write_dt(&cfg->bus, &tx);
}

static int tlc5971_update_rgb(const struct device *dev, struct led_rgb *pixels, size_t num_pixels)
{
	const struct tlc5971_config *cfg = dev->config;

	if (num_pixels > cfg->num_pixels) {
		LOG_ERR("invalid number of pixels, %zu vs actual %i", num_pixels, cfg->num_pixels);
		return -EINVAL;
	}

	tlc5971_fill_data_buffer(dev, pixels, num_pixels);

	return tlc5971_transmit_data(dev, num_pixels);
}

static int tlc5971_update_channels(const struct device *dev, uint8_t *channels, size_t num_channels)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channels);
	ARG_UNUSED(num_channels);

	return -ENOTSUP;
}

int tlc5971_set_global_brightness(const struct device *dev, struct led_rgb pixel)
{
	const struct tlc5971_config *cfg = dev->config;
	struct tlc5971_data *data = dev->data;
	int res = -EINVAL;

	if ((pixel.r <= TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX) &&
	    (pixel.g <= TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX) &&
	    (pixel.b <= TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX)) {
		data->gbc_color_1 = tlc5971_map_color(cfg->color_mapping[0], &pixel);
		data->gbc_color_2 = tlc5971_map_color(cfg->color_mapping[1], &pixel);
		data->gbc_color_3 = tlc5971_map_color(cfg->color_mapping[2], &pixel);
		res = 0;
	}

	return res;
}

static int tlc5971_init(const struct device *dev)
{
	const struct tlc5971_config *cfg = dev->config;
	struct tlc5971_data *data = dev->data;

	if (!spi_is_ready(&cfg->bus)) {
		LOG_ERR("%s: SPI device %s not ready", dev->name, cfg->bus.bus->name);
		return -ENODEV;
	}

	if ((cfg->num_pixels % TLC5971_PIXELS_PER_DEVICE) != 0) {
		LOG_ERR("%s: chain length must be multiple of 4", dev->name);
		return -EINVAL;
	}

	if (cfg->num_colors != TLC5971_NUMBER_OF_COLORS) {
		LOG_ERR("%s: the tlc5971 only supports %i colors", dev->name,
			TLC5971_NUMBER_OF_COLORS);
		return -EINVAL;
	}

	for (int i = 0; i < cfg->num_colors; i++) {
		switch (cfg->color_mapping[i]) {
		case LED_COLOR_ID_RED:
		case LED_COLOR_ID_GREEN:
		case LED_COLOR_ID_BLUE:
			break;
		default:
			LOG_ERR("%s: invalid color mapping", dev->name);
			return -EINVAL;
		}
	}

	/*
	 * set up sane defaults for control data.
	 * unblanks leds, enables auto display repeat, enables timing resetm uses internal clock for
	 * PWM generation and sets the GS reference clock edge select to rising edge
	 */
	data->control_data = TLC5971_BYTE27_CTRL_BIT_OUTTMG | TLC5971_BYTE26_CTRL_BIT_DSPRPT |
			     TLC5971_BYTE26_CTRL_BIT_TMGRST;

	return 0;
}

static const struct led_strip_driver_api tlc5971_api = {
	.update_rgb = tlc5971_update_rgb,
	.update_channels = tlc5971_update_channels,
};

#define TLC5971_DATA_BUFFER_LENGTH(inst)                                                           \
	(DT_INST_PROP(inst, chain_length) / TLC5971_PIXELS_PER_DEVICE) * TLC5971_PACKET_LEN

#define TLC5971_DEVICE(inst)                                                                       \
	static const uint8_t tlc5971_##inst##_color_mapping[] = DT_INST_PROP(inst, color_mapping); \
	static const struct tlc5971_config tlc5971_##inst##_config = {                             \
		.bus = SPI_DT_SPEC_INST_GET(inst, TLC5971_SPI_OPERATION, 0),                       \
		.num_pixels = DT_INST_PROP(inst, chain_length),                                    \
		.num_colors = DT_INST_PROP_LEN(inst, color_mapping),                               \
		.color_mapping = tlc5971_##inst##_color_mapping,                                   \
	};                                                                                         \
	static uint8_t tlc5971_##inst##_data_buffer[TLC5971_DATA_BUFFER_LENGTH(inst)];             \
	static struct tlc5971_data tlc5971_##inst##_data = {                                       \
		.data_buffer = tlc5971_##inst##_data_buffer,                                       \
		.gbc_color_1 = TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX,                              \
		.gbc_color_2 = TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX,                              \
		.gbc_color_3 = TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX,                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tlc5971_init, NULL, &tlc5971_##inst##_data,                    \
			      &tlc5971_##inst##_config, POST_KERNEL,                               \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &tlc5971_api);

DT_INST_FOREACH_STATUS_OKAY(TLC5971_DEVICE)
