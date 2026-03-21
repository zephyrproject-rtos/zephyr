/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino UNO Q LED Matrix - Zephyr Driver
 *
 * Implements the Zephyr display_driver_api for the 8×13 Charlieplexed LED
 * matrix on the Arduino UNO Q (STM32U585AIQ + QRB2210).
 *
 * display_api pixel format: PIXEL_FORMAT_MONO01
 *   - 1 bit per pixel
 *   - LSB of each byte = leftmost pixel in that byte
 *   - Bytes packed left-to-right along the x axis
 *
 * The existing uno_q_ledmatrix_* API functions are kept alongside
 * display_api so existing application code continues to compile unchanged.
 */

#define DT_DRV_COMPAT arduino_uno_q_ledmatrix

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include "uno_q_matrix_hal.h"
#include <zephyr/drivers/uno_q_ledmatrix.h>

LOG_MODULE_REGISTER(uno_q_ledmatrix, CONFIG_CONFIG_LED_MATRIX_UNO_Q_LOG_LEVEL);

/* ------------------------------------------------------------------ */
/* Driver structs                                                      */
/* ------------------------------------------------------------------ */

struct uno_q_ledmatrix_config {
	uint8_t rows;
	uint8_t columns;
	uint8_t default_brightness;
};

struct uno_q_ledmatrix_data {
	/*
	 * Flat framebuffer: fb[row * cols + col], one byte per LED.
	 * 0 = off, 1 = on.  Kept in sync with the HAL framebuffer via
	 * matrixWriteBuffer() on every write.
	 */
	uint8_t framebuffer[UNO_Q_MATRIX_PIXELS];
	uint8_t brightness;
	bool    initialized;
	struct  k_mutex lock;
};

/* ------------------------------------------------------------------ */
/* display_driver_api callbacks                                        */
/* ------------------------------------------------------------------ */

/*
 * display_blanking_on — stop the ISR, drive all pins to HIGH-Z.
 * The framebuffer is preserved; unblanking resumes from current state.
 */
static int uno_q_display_blanking_on(const struct device *dev)
{
	const struct uno_q_ledmatrix_data *data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}

	matrixBlanking(true);
	return 0;
}

/*
 * display_blanking_off — restart the ISR; display resumes immediately.
 */
static int uno_q_display_blanking_off(const struct device *dev)
{
	const struct uno_q_ledmatrix_data *data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}

	matrixBlanking(false);
	return 0;
}

/*
 * display_write — write a sub-region of pixels to the framebuffer.
 *
 * Accepts PIXEL_FORMAT_MONO01 data:
 *   - Each byte holds 8 horizontally adjacent pixels
 *   - Bit 0 (LSB) = leftmost pixel in that byte
 *   - desc->pitch is the number of pixels per source row (may be > width
 *     when the caller provides a padded/aligned buffer)
 *
 * The sub-region starts at (x, y) and is (desc->width × desc->height) pixels.
 */
static int uno_q_display_write(const struct device *dev,
				const uint16_t x, const uint16_t y,
				const struct display_buffer_descriptor *desc,
				const void *buf)
{
	struct uno_q_ledmatrix_data *data = dev->data;
	const struct uno_q_ledmatrix_config *config = dev->config;
	const uint8_t *src = buf;

	if (!data->initialized) {
		return -ENODEV;
	}

	/* Bounds check — reject writes that would overflow the matrix */
	if (x >= config->columns || y >= config->rows) {
		return -EINVAL;
	}
	if ((x + desc->width) > config->columns ||
	    (y + desc->height) > config->rows) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * Unpack MONO01 bits into the flat framebuffer.
	 *
	 * Source bit index for pixel (col, row) within the descriptor:
	 *   bit_idx = row * desc->pitch + col
	 *   byte    = bit_idx / 8
	 *   bit     = bit_idx % 8
	 *
	 * Destination index in our flat framebuffer:
	 *   (y + row) * config->columns + (x + col)
	 */
	for (uint16_t row = 0; row < desc->height; row++) {
		for (uint16_t col = 0; col < desc->width; col++) {
			uint32_t bit_idx = (uint32_t)row * desc->pitch + col;
			uint8_t  pixel   = (src[bit_idx / 8] >> (bit_idx % 8)) & 1u;

			data->framebuffer[(y + row) * config->columns + (x + col)] = pixel;
		}
	}

	matrixWriteBuffer(data->framebuffer, UNO_Q_MATRIX_PIXELS);

	k_mutex_unlock(&data->lock);
	return 0;
}

/*
 * display_read — read back pixel data from the framebuffer.
 *
 * Packs the framebuffer back into MONO01 format for the requested region.
 * Useful for diagnostics and test harnesses.
 */
static int uno_q_display_read(const struct device *dev,
			       const uint16_t x, const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       void *buf)
{
	struct uno_q_ledmatrix_data *data = dev->data;
	const struct uno_q_ledmatrix_config *config = dev->config;
	uint8_t *dst = buf;

	if (!data->initialized) {
		return -ENODEV;
	}

	if (x >= config->columns || y >= config->rows) {
		return -EINVAL;
	}
	if ((x + desc->width) > config->columns ||
	    (y + desc->height) > config->rows) {
		return -EINVAL;
	}

	/* Zero destination first — we only set bits, never clear them */
	memset(dst, 0, desc->buf_size);

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint16_t row = 0; row < desc->height; row++) {
		for (uint16_t col = 0; col < desc->width; col++) {
			uint8_t  pixel   = data->framebuffer[
						(y + row) * config->columns + (x + col)];
			uint32_t bit_idx = (uint32_t)row * desc->pitch + col;

			if (pixel) {
				dst[bit_idx / 8] |= (1u << (bit_idx % 8));
			}
		}
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

/*
 * display_get_capabilities — report matrix geometry and pixel format.
 */
static void uno_q_display_get_capabilities(
	const struct device *dev,
	struct display_capabilities *caps)
{
	const struct uno_q_ledmatrix_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution           = config->columns;   /* 13 */
	caps->y_resolution           = config->rows;       /* 8  */
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format   = PIXEL_FORMAT_MONO01;
	caps->current_orientation    = DISPLAY_ORIENTATION_NORMAL;
	/*
	 * screen_info = 0:
	 *   No SCREEN_INFO_MONO_MSB_FIRST — LSB is leftmost pixel (MONO01 default)
	 *   No SCREEN_INFO_MONO_VTILED   — bytes run left-to-right, not top-to-bottom
	 */
}

static const struct display_driver_api uno_q_display_api = {
	.blanking_on      = uno_q_display_blanking_on,
	.blanking_off     = uno_q_display_blanking_off,
	.write            = uno_q_display_write,
	.read             = uno_q_display_read,
	.get_capabilities = uno_q_display_get_capabilities,
	/* .set_pixel_format and .set_orientation intentionally NULL — not supported */
};

/* ------------------------------------------------------------------ */
/* Custom API (kept for backward compatibility with existing app code) */
/* ------------------------------------------------------------------ */

int uno_q_ledmatrix_set_pixel(const struct device *dev,
			       uint8_t row, uint8_t col, uint8_t value)
{
	struct uno_q_ledmatrix_data *data = dev->data;
	const struct uno_q_ledmatrix_config *config = dev->config;

	if (row >= config->rows || col >= config->columns) {
		return -EINVAL;
	}
	if (!data->initialized) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->framebuffer[row * config->columns + col] = value ? 1u : 0u;
	matrixWriteBuffer(data->framebuffer, UNO_Q_MATRIX_PIXELS);
	k_mutex_unlock(&data->lock);

	return 0;
}

int uno_q_ledmatrix_clear(const struct device *dev)
{
	struct uno_q_ledmatrix_data *data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	memset(data->framebuffer, 0, UNO_Q_MATRIX_PIXELS);
	matrixWriteBuffer(data->framebuffer, UNO_Q_MATRIX_PIXELS);
	k_mutex_unlock(&data->lock);

	return 0;
}

int uno_q_ledmatrix_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct uno_q_ledmatrix_data *data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->brightness = brightness;
	/*
	 * brightness is stored but not yet wired to on-time scaling in the HAL.
	 * This will be connected when BAM/PWM grayscale is added (Step 5).
	 */
	k_mutex_unlock(&data->lock);

	return 0;
}

/* ------------------------------------------------------------------ */
/* Driver init                                                         */
/* ------------------------------------------------------------------ */

static int uno_q_ledmatrix_init(const struct device *dev)
{
	struct uno_q_ledmatrix_data *data = dev->data;
	const struct uno_q_ledmatrix_config *config = dev->config;
	int ret;

	LOG_INF("Initializing UNO Q LED Matrix (%dx%d)",
		config->rows, config->columns);

	k_mutex_init(&data->lock);

	ret = matrixBegin();
	if (ret != 0) {
		LOG_ERR("matrixBegin() failed: %d", ret);
		return ret;
	}

	memset(data->framebuffer, 0, UNO_Q_MATRIX_PIXELS);
	data->brightness   = config->default_brightness;
	data->initialized  = true;

	uno_q_ledmatrix_clear(dev);

	LOG_INF("LED Matrix initialized successfully");
	return 0;
}

/* ------------------------------------------------------------------ */
/* Device instantiation                                                */
/* ------------------------------------------------------------------ */

#define UNO_Q_LEDMATRIX_DEVICE(inst)						\
	static const struct uno_q_ledmatrix_config				\
		uno_q_ledmatrix_config_##inst = {				\
		.rows             = DT_INST_PROP(inst, rows),			\
		.columns          = DT_INST_PROP(inst, columns),		\
		.default_brightness = DT_INST_PROP_OR(inst, brightness, 255),	\
	};									\
										\
	static struct uno_q_ledmatrix_data uno_q_ledmatrix_data_##inst;		\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			      uno_q_ledmatrix_init,				\
			      NULL,						\
			      &uno_q_ledmatrix_data_##inst,			\
			      &uno_q_ledmatrix_config_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_LED_MATRIX_UNO_Q_INIT_PRIORITY,			\
			      &uno_q_display_api);

DT_INST_FOREACH_STATUS_OKAY(UNO_Q_LEDMATRIX_DEVICE)