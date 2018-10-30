/*
 * Copyright (c) 2018 Miras Absar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <spi.h>
#include <gpio.h>
#include <display/segdl.h>
#include <drivers/display/ls013b7dh05.h>

#define SYS_LOG_DOMAIN "LS013B7DH05"
#define SYS_LOG_LEVEL 4
#include <logging/sys_log.h>

/**
 * @see segdl_draw_fh_ln_t
 */
static int ls013b7dh05_draw_fh_ln(struct device *dev, struct segdl_ln *ln)
{
	SYS_LOG_DBG("Drawing a full width, horizontal line.");

	uint16_t buf_len = 22;
	SYS_LOG_DBG("Allocating %d bytes.", buf_len);
	uint8_t *buf = k_calloc(buf_len, sizeof(uint8_t));
	if (buf == NULL) {
		SYS_LOG_ERR("Couldn't allocate %d bytes.", buf_len);
		return -ENOMEM;
	}

	buf[0] = LS013B7DH05_UPDATE_MODE;
	buf[1] = ((ln->y + 1) * 0x0202020202ULL & 0x010884422010ULL) % 1023;
	/*
	 * Sharp Memory LCDs start the y axis at 1 and represent y coordinates
	 * in least significant bit first (LSB), so ln-> y is incremented by 1
	 * and converted to LSB.
	 */

	/*
	 * Sharp Memory LCDs represent background as 1 and foreground as 0, so
	 * ln->colors is inverted.
	 */
	for (uint8_t color_i = 0; color_i < 18; color_i++) {
		buf[color_i + 2] = ~((uint8_t *)ln->colors)[color_i];
	}

	int return_0 = ls013b7dh05_write_buf(dev, buf, buf_len);
	SYS_LOG_DBG("Freeing %d bytes.", buf_len);
	k_free(buf);
	return return_0;
}

/**
 * @see segdl_draw_fh_lns_t
 */
static int ls013b7dh05_draw_fh_lns(struct device *dev, struct segdl_ln *lns,
				   uint16_t num_lns)
{
	SYS_LOG_DBG("Drawing %d full width, horizontal lines.", num_lns);

	uint16_t buf_len = 2 + (20 * num_lns);
	SYS_LOG_DBG("Allocating %d bytes.", buf_len);
	uint8_t *buf = k_calloc(buf_len, sizeof(uint8_t));
	if (buf == NULL) {
		SYS_LOG_ERR("Couldn't allocate %d bytes.", buf_len);
		return -ENOMEM;
	}

	buf[0] = LS013B7DH05_UPDATE_MODE;
	for (uint16_t ln_i = 0; ln_i < num_lns; ln_i++) {
		struct segdl_ln *ln = lns + ln_i;
		uint16_t buf_i = 1 + (20 * ln_i);

		buf[buf_i] = ((ln->y + 1) * 0x0202020202ULL & 0x010884422010ULL)
			% 1023;
		for (uint8_t color_i = 0; color_i < 18; color_i++) {
			buf[buf_i + color_i + 1] =
				~((uint8_t *)ln->colors)[color_i];
		}
	}

	int return_0 = ls013b7dh05_write_buf(dev, buf, buf_len);
	SYS_LOG_DBG("Freeing %d bytes.", buf_len);
	k_free(buf);
	return return_0;
}

/**
 * @see segdl_draw_full_frame_t
 */
static int ls013b7dh05_draw_full_frame(struct device *dev,
				       struct segdl_frame *frame)
{
	SYS_LOG_DBG("Drawing a full frame.");

	uint16_t buf_len = 3362;
	SYS_LOG_DBG("Allocating %d bytes.", buf_len);
	uint8_t *buf = k_calloc(buf_len, sizeof(uint8_t));
	if (buf == NULL) {
		SYS_LOG_ERR("Couldn't allocate %d bytes.", buf_len);
		return -ENOMEM;
	}

	buf[0] = LS013B7DH05_UPDATE_MODE;
	for (uint16_t y = 0; y < 168; y++) {
		uint8_t *colors = frame->colors + 18 * y;
		uint16_t buf_i = 1 + (20 * y);

		buf[buf_i] = ((y + 1) * 0x0202020202ULL & 0x010884422010ULL)
			% 1023;
		for (uint8_t color_i = 0; color_i < 18; color_i++) {
			buf[buf_i + color_i + 1] = ~colors[color_i];
		}
	}

	int return_0 = ls013b7dh05_write_buf(dev, buf, buf_len);
	SYS_LOG_DBG("Freeing %d bytes.", buf_len);
	k_free(buf);
	return return_0;
}

/**
 * @see ls013b7dh05_clear_t
 */
static int ls013b7dh05_clear_i(struct device *dev)
{
	SYS_LOG_DBG("Clearing the display.");

	uint8_t buf[] = {
		LS013B7DH05_UPDATE_MODE | LS013B7DH05_CLEAR_MODE,
		0b00000000,
		0b00000000
	};

	return ls013b7dh05_write_buf(dev, &buf, 3);
}

/**
 * @see ls013b7dh05_write_buf_t
 */
static int ls013b7dh05_write_buf_i(struct device *dev, uint8_t *buf,
				   size_t buf_len)
{
	struct ls013b7dh05_data *data = dev->driver_data;
	struct device *scs_dev = data->scs_dev;

	struct spi_buf spi_bufs[] = {
		{
			.buf = buf,
			.len = buf_len
		}
	};

	struct spi_buf_set spi_buf_set_0 = {
		.buffers = spi_bufs,
		.count = ARRAY_SIZE(spi_bufs)
	};

	SYS_LOG_DBG("Writing 1 to the chip select GPIO device.");
	int return_0 = gpio_pin_write(scs_dev,
		CONFIG_LS013B7DH05_SCS_GPIO_PIN_NUM, 1);
	if (return_0 != 0) {
		SYS_LOG_ERR("Couldn't write 1 to the chip select GPIO device.");
		return return_0;
	}

	SYS_LOG_DBG("Writing %d bytes to the display SPI device.", buf_len);
	int return_1 = spi_write(data->spi_dev, &data->spi_conf,
		&spi_buf_set_0);
	if (return_1 != 0) {
		SYS_LOG_ERR("Couldn't write %d bytes to the display SPI device.", buf_len);
		return return_1;
	}

	SYS_LOG_DBG("Writing 0 to chip select GPIO device.");
	int return_2 = gpio_pin_write(scs_dev,
		CONFIG_LS013B7DH05_SCS_GPIO_PIN_NUM, 0);
	if (return_2 != 0) {
		SYS_LOG_ERR("Couldn't write 0 to the chip select GPIO device.");
		return return_2;
	}

	return 0;
}

/**
 * @see ls013b7dh05_extra_api
 */
static struct ls013b7dh05_extra_api ls013b7dh05_extra_api_i = {
	.clear = ls013b7dh05_clear_i,
	.write_buf = ls013b7dh05_write_buf_i
};

/**
 * @see segdl_api
 */
static struct segdl_api ls013b7dh05_api = {
	.width = 144,
	.height = 168,
	.color_space = segdl_color_space_1,
	.max_brightness = 0,

	.supports_draw_px = false,
	.supports_draw_pxs = false,

	.supports_draw_fh_ln = true,
	.supports_draw_ph_ln = false,
	.supports_draw_fv_ln = false,
	.supports_draw_pv_ln = false,

	.supports_draw_fh_lns = true,
	.supports_draw_ph_lns = false,
	.supports_draw_fv_lns = false,
	.supports_draw_pv_lns = false,

	.supports_draw_partial_frame = false,
	.supports_draw_partial_frames = false,
	.supports_draw_full_frame = true,

	.supports_set_brightness = false,
	.supports_sleep_wake = false,

	.has_extra_api = true,

	.draw_px = NULL,
	.draw_pxs = NULL,

	.draw_fh_ln = ls013b7dh05_draw_fh_ln,
	.draw_ph_ln = NULL,
	.draw_fv_ln = NULL,
	.draw_pv_ln = NULL,

	.draw_fh_lns = ls013b7dh05_draw_fh_lns,
	.draw_ph_lns = NULL,
	.draw_fv_lns = NULL,
	.draw_pv_lns = NULL,

	.draw_partial_frame = NULL,
	.draw_partial_frames = NULL,
	.draw_full_frame = ls013b7dh05_draw_full_frame,

	.set_brightness = NULL,
	.sleep = NULL,
	.wake = NULL,

	.extra_api = &ls013b7dh05_extra_api_i
};

static int ls013b7dh05_init(struct device *dev)
{
	SYS_LOG_DBG("Initializing the display.");

	struct ls013b7dh05_data *data = dev->driver_data;

	SYS_LOG_DBG("Initializing the display SPI device.");

	data->spi_dev = device_get_binding(CONFIG_LS013B7DH05_SPI_DEV_NAME);
	if (!data->spi_dev) {
		SYS_LOG_ERR("Couldn't get the display SPI device.");
		return -ENODEV;
	}

	data->spi_conf.frequency = CONFIG_LS013B7DH05_SPI_DEV_FREQ;
	data->spi_conf.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_conf.slave = CONFIG_LS013B7DH05_SPI_DEV_NUM;

	SYS_LOG_DBG("Initializing the chip select GPIO device.");

	data->scs_dev = device_get_binding(
		CONFIG_LS013B7DH05_SCS_GPIO_PORT_NAME);
	if (!data->scs_dev) {
		SYS_LOG_ERR("Couldn't get the chip select GPIO device.");
		return -ENODEV;
	}

	int return_0 = gpio_pin_configure(data->scs_dev,
		CONFIG_LS013B7DH05_SCS_GPIO_PIN_NUM, GPIO_DIR_OUT);
	if (return_0 != 0) {
		SYS_LOG_ERR("Couldn't configure the chip select GPIO device.");
		return return_0;
	}

	/* LS013B7DH05 inits with a noisy image, so it's cleared after init. */
	return ls013b7dh05_clear(dev);
}

static struct ls013b7dh05_data ls013b7dh05_data_i;

DEVICE_AND_API_INIT(LS013B7DH05, CONFIG_LS013B7DH05_DEV_NAME,
		    ls013b7dh05_init, &ls013b7dh05_data_i, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ls013b7dh05_api);
