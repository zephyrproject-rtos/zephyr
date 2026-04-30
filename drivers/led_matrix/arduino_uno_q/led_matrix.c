/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Shankar Sridhar
 *
 * drivers/led_matrix/led_matrix.c
 *
 * Generic Charlieplex LED matrix driver.
 *
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led_matrix.h>

LOG_MODULE_REGISTER(led_matrix, CONFIG_LED_MATRIX_UNO_Q_LOG_LEVEL);

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * Rebuild the active-list from the current framebuffer.
 *
 * Called from thread context (inside irq_lock) whenever the framebuffer
 * changes.  Walks all pin_map_len entries and records the indices of LEDs
 * that are ON into data->active_list[].
 *
 * The ISR then cycles through only those entries, ensuring every lit LED
 * receives equal on-time regardless of total LED count.
 */
static void led_matrix_rebuild_active_list(const struct led_matrix_config *config,
					   struct led_matrix_data *data)
{
	uint8_t count = 0;

	for (uint8_t i = 0; i < config->pin_map_len; i++) {
		if (data->framebuffer[i]) {
			data->active_list[count++] = i;
		}
	}

	data->active_count = count;

	/*
	 * Clamp current_slot so the ISR never reads past the end of the
	 * (possibly shorter) new active list.
	 */
	if (data->current_slot >= count && count > 0) {
		data->current_slot = 0;
	}
}

/* ------------------------------------------------------------------ */
/* ISR                                                                 */
/* ------------------------------------------------------------------ */

/*
 * Multiplexing timer callback
 * Runs at LED_MATRIX_UNO_Q_PIXEL_PERIOD_US intervals.
 *
 * Lights exactly one LED per tick by advancing through active_list[].
 *
 */
static void led_matrix_isr(struct k_timer *timer)
{
    struct led_matrix_data *data = 
        CONTAINER_OF(timer, struct led_matrix_data, timer);
    const struct device *dev = data->dev;
	const struct led_matrix_config *config = dev->config;	

	/* Turn off LEDs first */
	config->hal->led_matrix_all_pins_highz();

	if (data->blanked || data->active_count == 0) {
		return;
	}

	data->current_slot = (data->current_slot + 1) % data->active_count;

	const struct led_matrix_pin_pair *pair =
		&config->pin_map[data->active_list[data->current_slot]];

	config->hal->led_matrix_set_pin(pair->anode_pin, 1);
	config->hal->led_matrix_set_pin(pair->cathode_pin, 0);
}

/* ------------------------------------------------------------------ */
/* display_driver_api callbacks                                        */
/* ------------------------------------------------------------------ */

/*
 * display_blanking_on — freeze the display with all LEDs off.
 *
 * The framebuffer and active list are preserved.  Calling blanking_off
 * resumes multiplexing from the current state.
 */
static int led_matrix_blanking_on(const struct device *dev)
{
	struct led_matrix_data *data = dev->data;
	const struct led_matrix_config *config = dev->config;

	data->blanked = true;
	config->hal->led_matrix_all_pins_highz();
	return 0;
}

/*
 * display_blanking_off — resume normal multiplexing.
 */
static int led_matrix_blanking_off(const struct device *dev)
{
	struct led_matrix_data *data = dev->data;

	data->blanked = false;
	return 0;
}

/*
 * display_write — write a rectangular region of pixels to the framebuffer.
 *
 * Accepts PIXEL_FORMAT_MONO01:
 *   - desc->pitch is pixels per source row (may exceed desc->width for
 *     aligned/padded buffers)
 *   - bit 0 (LSB) of each byte is the leftmost pixel in that byte
 *
 */
static int led_matrix_write(const struct device *dev,
			    const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc,
			    const void *buf)
{
	struct led_matrix_data *data = dev->data;
	const struct led_matrix_config *config = dev->config;
	const uint8_t *src = buf;

	if (x >= config->cols || y >= config->rows) {
		return -EINVAL;
	}
	if ((x + desc->width) > config->cols ||
	    (y + desc->height) > config->rows) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	/*
	 * Unpack MONO01 bits into the flat framebuffer.
	 *
	 * Source bit index for pixel (col, row) within the descriptor:
	 *   bit_idx = row * desc->pitch + col
	 *
	 * Destination index in the flat framebuffer:
	 *   (y + row) * config->cols + (x + col)
	 */
	for (uint16_t row = 0; row < desc->height; row++) {
		for (uint16_t col = 0; col < desc->width; col++) {
			uint32_t bit_idx = (uint32_t)row * desc->pitch + col;
			uint8_t pixel = (src[bit_idx / 8] >> (bit_idx % 8)) & 1u;

			data->framebuffer[(y + row) * config->cols + (x + col)] = pixel;
		}
	}

	led_matrix_rebuild_active_list(config, data);

	irq_unlock(key);
	return 0;
}

/*
 * display_read — read back a rectangular region from the framebuffer.
 *
 * Packs the stored binary pixels back into MONO01 format.
 * Useful for display shell introspection and test harnesses.
 */
static int led_matrix_read(const struct device *dev,
			   const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc,
			   void *buf)
{
	struct led_matrix_data *data = dev->data;
	const struct led_matrix_config *config = dev->config;
	uint8_t *dst = buf;

	if (x >= config->cols || y >= config->rows) {
		return -EINVAL;
	}
	if ((x + desc->width) > config->cols ||
	    (y + desc->height) > config->rows) {
		return -EINVAL;
	}

	memset(dst, 0, desc->buf_size);

	unsigned int key = irq_lock();

	for (uint16_t row = 0; row < desc->height; row++) {
		for (uint16_t col = 0; col < desc->width; col++) {
			uint8_t pixel = data->framebuffer[
				(y + row) * config->cols + (x + col)];
			uint32_t bit_idx = (uint32_t)row * desc->pitch + col;

			if (pixel) {
				dst[bit_idx / 8] |= (1u << (bit_idx % 8));
			}
		}
	}

	irq_unlock(key);
	return 0;
}

/*
 * display_set_brightness — store the brightness value.
 *
 * The value is saved to data->brightness but is not yet wired to ISR-level
 * on-time scaling.  That will be added when BAM grayscale is implemented.
 */
static int led_matrix_set_brightness(const struct device *dev,
				     const uint8_t brightness)
{
	struct led_matrix_data *data = dev->data;

	data->brightness = brightness;
	return 0;
}

/*
 * display_get_capabilities — report matrix geometry and pixel format.
 */
static void led_matrix_get_capabilities(const struct device *dev,
					 struct display_capabilities *caps)
{
	const struct led_matrix_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution            = config->cols;    /* 13 */
	caps->y_resolution            = config->rows;    /* 8  */
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format    = PIXEL_FORMAT_MONO01;
	caps->current_orientation     = DISPLAY_ORIENTATION_NORMAL;
	/*
	 * screen_info = 0:
	 *   No SCREEN_INFO_MONO_MSB_FIRST — LSB is the leftmost pixel
	 *   No SCREEN_INFO_MONO_VTILED   — bytes run left-to-right
	 */
}

const struct display_driver_api led_matrix_display_api = {
	.blanking_on      = led_matrix_blanking_on,
	.blanking_off     = led_matrix_blanking_off,
	.write            = led_matrix_write,
	.read             = led_matrix_read,
	.set_brightness   = led_matrix_set_brightness,
	.get_capabilities = led_matrix_get_capabilities,
	/* .set_pixel_format and .set_orientation: not supported */
};

/* ------------------------------------------------------------------ */
/* Generic init — called by the board-specific HAL via                 */
/* DEVICE_DT_INST_DEFINE                                               */
/* ------------------------------------------------------------------ */

int led_matrix_init(const struct device *dev)
{
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;
	int ret;

	LOG_INF("Initializing LED matrix (%dx%d, %u us/pixel)",
		config->rows, config->cols, CONFIG_LED_MATRIX_UNO_Q_PIXEL_PERIOD_US);

	ret = config->hal->led_matrix_init_pins();
	if (ret != 0) {
		LOG_ERR("led_matrix_init_pins() failed: %d", ret);
		return ret;
	}

	memset(data->framebuffer, 0, sizeof(data->framebuffer));
	data->active_count = 0;
	data->current_slot = 0;
	data->brightness   = 255;
	data->blanked      = false;

    data->dev = dev;

	k_timer_init(&data->timer, led_matrix_isr, NULL);
	k_timer_start(&data->timer,
		      K_USEC(CONFIG_LED_MATRIX_UNO_Q_PIXEL_PERIOD_US),
		      K_USEC(CONFIG_LED_MATRIX_UNO_Q_PIXEL_PERIOD_US));

	LOG_INF("LED matrix ready");
	return 0;
}
