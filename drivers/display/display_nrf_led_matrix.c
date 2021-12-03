/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/display.h>
#include <devicetree.h>
#include <dt-bindings/gpio/gpio.h>
#include <hal/nrf_timer.h>
#ifdef PWM_PRESENT
#include <hal/nrf_pwm.h>
#endif
#include <nrfx_gpiote.h>
#ifdef PPI_PRESENT
#include <nrfx_ppi.h>
#endif

#define MATRIX_NODE  DT_INST(0, nordic_nrf_led_matrix)
#define TIMER_NODE   DT_PHANDLE(MATRIX_NODE, timer)
#define USE_PWM      DT_NODE_HAS_PROP(MATRIX_NODE, pwm)
#define ROW_COUNT    DT_PROP_LEN(MATRIX_NODE, row_gpios)
#define COL_COUNT    DT_PROP_LEN(MATRIX_NODE, col_gpios)

#define X_PIXELS     DT_PROP(MATRIX_NODE, width)
#define Y_PIXELS     DT_PROP(MATRIX_NODE, height)
#define PIXEL_COUNT  DT_PROP_LEN(MATRIX_NODE, pixel_mapping)
BUILD_ASSERT(PIXEL_COUNT == (X_PIXELS * Y_PIXELS),
	     "Invalid length of pixel-mapping");

#define PIXEL_MAPPING(idx)  DT_PROP_BY_IDX(MATRIX_NODE, pixel_mapping, idx)
#define CHECK_PIXEL(node_id, pha, idx) \
	BUILD_ASSERT((PIXEL_MAPPING(idx) >> 4) < ROW_COUNT, \
		     "Invalid row index in pixel-mapping["#idx"]"); \
	BUILD_ASSERT((PIXEL_MAPPING(idx) & 0xF) < COL_COUNT, \
		     "Invalid column index in pixel-mapping["#idx"]");
DT_FOREACH_PROP_ELEM(MATRIX_NODE, pixel_mapping, CHECK_PIXEL)

#define REFRESH_FREQUENCY  DT_PROP(MATRIX_NODE, refresh_frequency)
#define BASE_FREQUENCY     16000000
#define TIMER_CLK_CONFIG   NRF_TIMER_FREQ_16MHz
#define PWM_CLK_CONFIG     NRF_PWM_CLK_16MHz
#define BRIGHTNESS_MAX     255

#define QUANTUM  (BASE_FREQUENCY / (REFRESH_FREQUENCY * PIXEL_COUNT * \
				    BRIGHTNESS_MAX))
#define PIXEL_PERIOD  (BRIGHTNESS_MAX * QUANTUM)
BUILD_ASSERT(PIXEL_PERIOD <= BIT_MASK(16));
#if USE_PWM
BUILD_ASSERT(PIXEL_PERIOD <= PWM_COUNTERTOP_COUNTERTOP_Msk);
#endif

#define ACTIVE_LOW_MASK  0x80
#define PSEL_MASK        0x7F

struct display_drv_config {
	NRF_TIMER_Type *timer;
#if USE_PWM
	NRF_PWM_Type *pwm;
#endif
	uint8_t rows[ROW_COUNT];
	uint8_t cols[COL_COUNT];
	uint8_t pixel_mapping[PIXEL_COUNT];
};

struct display_drv_data {
#if USE_PWM
	uint16_t seq;
#else
	uint8_t gpiote_ch;
#endif
	uint8_t pixel_idx;
	uint8_t framebuf[PIXEL_COUNT];
	uint8_t brightness;
	bool    blanking;
};

static void set_pin(uint8_t pin_info, bool active)
{
	uint32_t value = active ? 1 : 0;

	if (pin_info & ACTIVE_LOW_MASK) {
		value = !value;
	}
	nrf_gpio_pin_write(pin_info & PSEL_MASK, value);
}

static int api_blanking_on(const struct device *dev)
{
	struct display_drv_data *dev_data = dev->data;
	const struct display_drv_config *dev_config = dev->config;

	if (!dev_data->blanking) {
		nrf_timer_task_trigger(dev_config->timer, NRF_TIMER_TASK_STOP);
		for (uint8_t i = 0; i < ROW_COUNT; ++i) {
			set_pin(dev_config->rows[i], false);
		}
		for (uint8_t i = 0; i < COL_COUNT; ++i) {
			set_pin(dev_config->cols[i], false);
		}

		dev_data->blanking = true;
	}

	return 0;
}

static int api_blanking_off(const struct device *dev)
{
	struct display_drv_data *dev_data = dev->data;
	const struct display_drv_config *dev_config = dev->config;

	if (dev_data->blanking) {
		dev_data->pixel_idx = PIXEL_COUNT - 1;

		nrf_timer_task_trigger(dev_config->timer, NRF_TIMER_TASK_CLEAR);
		nrf_timer_task_trigger(dev_config->timer, NRF_TIMER_TASK_START);

		dev_data->blanking = false;
	}

	return 0;
}

static void *api_get_framebuffer(const struct device *dev)
{
	struct display_drv_data *dev_data = dev->data;

	return dev_data->framebuf;
}

static int api_set_brightness(const struct device *dev,
			      const uint8_t brightness)
{
	struct display_drv_data *dev_data = dev->data;
	uint8_t new_brightness = CLAMP(brightness, 1, BRIGHTNESS_MAX);
	int16_t delta = (int16_t)new_brightness - dev_data->brightness;

	dev_data->brightness = new_brightness;

	for (uint8_t i = 0; i < PIXEL_COUNT; ++i) {
		uint8_t old_val = dev_data->framebuf[i];

		if (old_val) {
			int16_t new_val = old_val + delta;

			dev_data->framebuf[i] =
				(uint8_t)CLAMP(new_val, 1, BRIGHTNESS_MAX);
		}
	}

	return 0;
}

static int api_set_contrast(const struct device *dev,
			    const uint8_t contrast)
{
	return -ENOTSUP;
}

static int api_set_pixel_format(const struct device *dev,
				const enum display_pixel_format format)
{
	switch (format) {
	case PIXEL_FORMAT_MONO01:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int api_set_orientation(const struct device *dev,
			       const enum display_orientation orientation)
{
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void api_get_capabilities(const struct device *dev,
				 struct display_capabilities *caps)
{
	caps->x_resolution = X_PIXELS;
	caps->y_resolution = Y_PIXELS;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static inline void move_to_next_pixel(uint8_t *mask, uint8_t *data,
				      const uint8_t **byte_buf)
{
	*mask <<= 1;
	if (!*mask) {
		*mask = 0x01;
		*data = *(*byte_buf)++;
	}
}

static int api_write(const struct device *dev,
		     const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc,
		     const void *buf)
{
	struct display_drv_data *dev_data = dev->data;
	const uint8_t *byte_buf = buf;
	uint16_t end_x = x + desc->width;
	uint16_t end_y = y + desc->height;

	if (x >= X_PIXELS || end_x > X_PIXELS ||
	    y >= Y_PIXELS || end_y > Y_PIXELS) {
		return -EINVAL;
	}

	if (desc->pitch < desc->width) {
		return -EINVAL;
	}

	uint16_t to_skip = desc->pitch - desc->width;
	uint8_t mask = 0;
	uint8_t data = 0;

	for (uint16_t py = y; py < end_y; ++py) {
		for (uint16_t px = x; px < end_x; ++px) {
			move_to_next_pixel(&mask, &data, &byte_buf);
			dev_data->framebuf[px + (py * X_PIXELS)] =
				(data & mask) ? dev_data->brightness : 0;
		}

		if (to_skip) {
			uint16_t cnt = to_skip;

			do {
				move_to_next_pixel(&mask, &data, &byte_buf);
			} while (--cnt);
		}
	}

	return 0;
}

static int api_read(const struct device *dev,
		    const uint16_t x, const uint16_t y,
		    const struct display_buffer_descriptor *desc,
		    void *buf)
{
	return -ENOTSUP;
}

const struct display_driver_api driver_api = {
	.blanking_on = api_blanking_on,
	.blanking_off = api_blanking_off,
	.write = api_write,
	.read = api_read,
	.get_framebuffer = api_get_framebuffer,
	.set_brightness = api_set_brightness,
	.set_contrast = api_set_contrast,
	.get_capabilities = api_get_capabilities,
	.set_pixel_format = api_set_pixel_format,
	.set_orientation = api_set_orientation,
};

static void timer_irq_handler(void *arg)
{
	const struct device *dev = arg;
	struct display_drv_data *dev_data = dev->data;
	const struct display_drv_config *dev_config = dev->config;
	uint8_t prev_row_idx, pixel_mapping, row_pin_info, col_pin_info;
	uint16_t pulse;

	/* The timer is automagically stopped and cleared by shortcuts
	 * on the same event (COMPARE0) that generates this interrupt.
	 * But the event itself needs to be cleared here.
	 */
	nrf_timer_event_clear(dev_config->timer, NRF_TIMER_EVENT_COMPARE0);

	/* Disable the row that contains the previously handled pixel. */
	prev_row_idx = dev_config->pixel_mapping[dev_data->pixel_idx] >> 4;
	set_pin(dev_config->rows[prev_row_idx], false);
	/* Disconnect that pixel column pin from the peripheral driving it. */
#if USE_PWM
	nrf_pwm_disable(dev_config->pwm);
#else
	NRF_GPIOTE->CONFIG[dev_data->gpiote_ch] = 0;
#endif

	/* Switch to the next pixel. */
	++dev_data->pixel_idx;
	if (dev_data->pixel_idx >= PIXEL_COUNT) {
		dev_data->pixel_idx = 0;
	}
	pixel_mapping = dev_config->pixel_mapping[dev_data->pixel_idx];
	row_pin_info = dev_config->rows[pixel_mapping >> 4];
	col_pin_info = dev_config->cols[pixel_mapping & 0xF];

	/* Prepare the low pulse on the column pin for the current pixel. */
	pulse = dev_data->framebuf[dev_data->pixel_idx] * QUANTUM;
#if USE_PWM
	dev_config->pwm->PSEL.OUT[0] = col_pin_info & PSEL_MASK;
	dev_data->seq = pulse
		      | ((col_pin_info & ACTIVE_LOW_MASK) ? 0 : BIT(15));
	nrf_pwm_enable(dev_config->pwm);
	nrf_pwm_task_trigger(dev_config->pwm, NRF_PWM_TASK_SEQSTART0);
#else
	uint32_t gpiote_cfg = GPIOTE_CONFIG_MODE_Task
		| ((col_pin_info & PSEL_MASK) << GPIOTE_CONFIG_PSEL_Pos);

	if (col_pin_info & ACTIVE_LOW_MASK) {
		gpiote_cfg |= (GPIOTE_CONFIG_POLARITY_LoToHi
			       << GPIOTE_CONFIG_POLARITY_Pos)
			      /* If there should be no pulse at all for a given
			       * pixel, its column GPIO needs to be configured
			       * as initially inactive.
			       */
			   |  ((pulse == 0 ? GPIOTE_CONFIG_OUTINIT_High
					   : GPIOTE_CONFIG_OUTINIT_Low)
			       << GPIOTE_CONFIG_OUTINIT_Pos);
	} else {
		gpiote_cfg |= (GPIOTE_CONFIG_POLARITY_HiToLo
			       << GPIOTE_CONFIG_POLARITY_Pos)
			   |  ((pulse == 0 ? GPIOTE_CONFIG_OUTINIT_Low
					   : GPIOTE_CONFIG_OUTINIT_High)
			       << GPIOTE_CONFIG_OUTINIT_Pos);
	}
	nrf_timer_cc_set(dev_config->timer, 1, pulse);
	NRF_GPIOTE->CONFIG[dev_data->gpiote_ch] = gpiote_cfg;
#endif

	/* Enable the row drive for the current pixel and restart the timer. */
	set_pin(row_pin_info, true);
	nrf_timer_task_trigger(dev_config->timer, NRF_TIMER_TASK_START);
}

static int instance_init(const struct device *dev)
{
	struct display_drv_data *dev_data = dev->data;
	const struct display_drv_config *dev_config = dev->config;

#if USE_PWM
	uint32_t out_psels[NRF_PWM_CHANNEL_COUNT] = {
		NRF_PWM_PIN_NOT_CONNECTED,
		NRF_PWM_PIN_NOT_CONNECTED,
		NRF_PWM_PIN_NOT_CONNECTED,
		NRF_PWM_PIN_NOT_CONNECTED,
	};
	nrf_pwm_sequence_t sequence = {
		.values.p_raw = &dev_data->seq,
		.length = 1,
	};

	nrf_pwm_pins_set(dev_config->pwm, out_psels);
	nrf_pwm_configure(dev_config->pwm,
		PWM_CLK_CONFIG, NRF_PWM_MODE_UP, PIXEL_PERIOD);
	nrf_pwm_decoder_set(dev_config->pwm,
		NRF_PWM_LOAD_COMMON, NRF_PWM_STEP_TRIGGERED);
	nrf_pwm_sequence_set(dev_config->pwm, 0, &sequence);
	nrf_pwm_loop_set(dev_config->pwm, 0);
	nrf_pwm_shorts_set(dev_config->pwm, NRF_PWM_SHORT_SEQEND0_STOP_MASK);
#else
	nrfx_err_t err;
	nrf_ppi_channel_t ppi_ch;

	err = nrfx_ppi_channel_alloc(&ppi_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	err = nrfx_gpiote_channel_alloc(&dev_data->gpiote_ch);
	if (err != NRFX_SUCCESS) {
		nrfx_ppi_channel_free(ppi_ch);
		return -ENOMEM;
	}

	nrf_ppi_channel_endpoint_setup(NRF_PPI, ppi_ch,
		nrf_timer_event_address_get(dev_config->timer,
			nrf_timer_compare_event_get(1)),
		nrf_gpiote_event_address_get(NRF_GPIOTE,
			nrf_gpiote_out_task_get(dev_data->gpiote_ch)));
	nrf_ppi_channel_enable(NRF_PPI, ppi_ch);
#endif /* USE_PWM */

	for (uint8_t i = 0; i < ROW_COUNT; ++i) {
		uint8_t row_pin_info = dev_config->rows[i];

		set_pin(row_pin_info, false);
		nrf_gpio_cfg(row_pin_info & PSEL_MASK,
			     NRF_GPIO_PIN_DIR_OUTPUT,
			     NRF_GPIO_PIN_INPUT_DISCONNECT,
			     NRF_GPIO_PIN_NOPULL,
			     NRF_GPIO_PIN_S0S1,
			     NRF_GPIO_PIN_NOSENSE);
	}

	for (uint8_t i = 0; i < COL_COUNT; ++i) {
		uint8_t col_pin_info = dev_config->cols[i];

		set_pin(col_pin_info, false);
		nrf_gpio_cfg(col_pin_info & PSEL_MASK,
			     NRF_GPIO_PIN_DIR_OUTPUT,
			     NRF_GPIO_PIN_INPUT_DISCONNECT,
			     NRF_GPIO_PIN_NOPULL,
			     NRF_GPIO_PIN_S0S1,
			     NRF_GPIO_PIN_NOSENSE);
	}

	nrf_timer_bit_width_set(dev_config->timer, NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_frequency_set(dev_config->timer, TIMER_CLK_CONFIG);
	nrf_timer_cc_set(dev_config->timer, 0, PIXEL_PERIOD);
	nrf_timer_shorts_set(dev_config->timer,
			     NRF_TIMER_SHORT_COMPARE0_STOP_MASK |
			     NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(dev_config->timer, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(dev_config->timer, NRF_TIMER_INT_COMPARE0_MASK);

	IRQ_CONNECT(DT_IRQN(TIMER_NODE), DT_IRQ(TIMER_NODE, priority),
		    timer_irq_handler, DEVICE_DT_GET(MATRIX_NODE), 0);
	irq_enable(DT_IRQN(TIMER_NODE));

	return 0;
}

static struct display_drv_data instance_data = {
	.brightness = 0xFF,
	.blanking   = true,
};

#define GET_PIN_INFO(node_id, pha, idx) \
	(DT_GPIO_PIN_BY_IDX(node_id, pha, idx) | \
	 (DT_PROP_BY_PHANDLE_IDX(node_id, pha, idx, port) << 5) | \
	 ((DT_GPIO_FLAGS_BY_IDX(node_id, pha, idx) & GPIO_ACTIVE_LOW) ? \
		ACTIVE_LOW_MASK : 0)),

static const struct display_drv_config instance_config = {
	.timer = (NRF_TIMER_Type *)DT_REG_ADDR(TIMER_NODE),
#if USE_PWM
	.pwm = (NRF_PWM_Type *)DT_REG_ADDR(DT_PHANDLE(MATRIX_NODE, pwm)),
#endif
	.rows = { DT_FOREACH_PROP_ELEM(MATRIX_NODE, row_gpios, GET_PIN_INFO) },
	.cols = { DT_FOREACH_PROP_ELEM(MATRIX_NODE, col_gpios, GET_PIN_INFO) },
	.pixel_mapping = DT_PROP(MATRIX_NODE, pixel_mapping),
};

DEVICE_DT_DEFINE(MATRIX_NODE,
		 instance_init, NULL,
		 &instance_data, &instance_config,
		 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &driver_api);
