/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT apa_apa102

#include <errno.h>
#include <drivers/led_strip.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/util.h>

struct apa102_data {
	const struct device *spi;
	struct spi_config cfg;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
};

static int apa102_update(const struct device *dev, void *buf, size_t size)
{
	struct apa102_data *data = dev->data;
	static const uint8_t zeros[] = {0, 0, 0, 0};
	static const uint8_t ones[] = {0xFF, 0xFF, 0xFF, 0xFF};
	const struct spi_buf tx_bufs[] = {
		{
			/* Start frame: at least 32 zeros */
			.buf = (uint8_t *)zeros,
			.len = sizeof(zeros),
		},
		{
			/* LED data itself */
			.buf = buf,
			.len = size,
		},
		{
			/* End frame: at least 32 ones to clock the
			 * remaining bits to the LEDs at the end of
			 * the strip.
			 */
			.buf = (uint8_t *)ones,
			.len = sizeof(ones),
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	return spi_write(data->spi, &data->cfg, &tx);
}

static int apa102_update_rgb(const struct device *dev, struct led_rgb *pixels,
			     size_t count)
{
	uint8_t *p = (uint8_t *)pixels;
	size_t i;
	/* SOF (3 bits) followed by the 0 to 31 global dimming level */
	uint8_t prefix = 0xE0 | 31;

	/* Rewrite to the on-wire format */
	for (i = 0; i < count; i++) {
		uint8_t r = pixels[i].r;
		uint8_t g = pixels[i].g;
		uint8_t b = pixels[i].b;

		*p++ = prefix;
		*p++ = b;
		*p++ = g;
		*p++ = r;
	}

	BUILD_ASSERT(sizeof(struct led_rgb) == 4);
	return apa102_update(dev, pixels, sizeof(struct led_rgb) * count);
}

static int apa102_update_channels(const struct device *dev, uint8_t *channels,
				  size_t num_channels)
{
	/* Not implemented */
	return -EINVAL;
}

static int apa102_init(const struct device *dev)
{
	struct apa102_data *data = dev->data;

	data->spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!data->spi) {
		return -ENODEV;
	}

	data->cfg.slave = DT_INST_REG_ADDR(0);
	data->cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	data->cfg.operation =
		SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->cs_ctl.gpio_dev =
		device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!data->cs_ctl.gpio_dev) {
		return -ENODEV;
	}
	data->cs_ctl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	data->cs_ctl.delay = 0;

	data->cfg.cs = &data->cs_ctl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	return 0;
}

static struct apa102_data apa102_data_0;

static const struct led_strip_driver_api apa102_api = {
	.update_rgb = apa102_update_rgb,
	.update_channels = apa102_update_channels,
};

DEVICE_DT_INST_DEFINE(0, apa102_init, NULL,
		    &apa102_data_0, NULL, POST_KERNEL,
		    CONFIG_LED_STRIP_INIT_PRIORITY, &apa102_api);
