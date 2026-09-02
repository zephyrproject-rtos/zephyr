/*
 * SPDX-FileCopyrightText: Copyright 2026 Javier Longares Abaiz
 * SPDX-FileCopyrightText: Copyright 2026 A Blue Thing In The Cloud SLU
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tdo_tr230s

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(display_tr230s, CONFIG_DISPLAY_LOG_LEVEL);

#define TR230S_COMMAND_PWM_DUTY 0x20U
#define TR230S_COMMAND_COLUMN_ADDRESS 0x2AU
#define TR230S_COMMAND_ROW_ADDRESS 0x2BU
#define TR230S_COMMAND_DISPLAY_DATA 0x2CU
#define TR230S_COMMAND_MIRROR_AND_ROTATION 0xACU

#define TR230S_MAXIMUM_BRIGHTNESS_PERCENT 100U
#define TR230S_BLANKED_BRIGHTNESS_PERCENT 0U
#define TR230S_READY_POLL_INTERVAL_US 1U
#define TR230S_READY_TIMEOUT_FOREVER_MS 0U
#define TR230S_PIXEL_SIZE_BYTES 2U
#define TR230S_ADDRESS_RANGE_SIZE_BYTES (2U * sizeof(uint16_t))
#define TR230S_WAIT_READY_STATE 1
#define TR230S_DBI_SPI_WORD_SIZE_BITS 8U

#define TR230S_ORIENTATION_PARAMETER_NORMAL 0x00U
#define TR230S_ORIENTATION_PARAMETER_90_DEGREES 0x01U
#define TR230S_ORIENTATION_PARAMETER_180_DEGREES 0x02U
#define TR230S_ORIENTATION_PARAMETER_270_DEGREES 0x03U

struct tr230s_configuration {
	const struct device *mipi_dbi_device;
	struct mipi_dbi_config dbi_configuration;
	struct gpio_dt_spec wait_gpio;
	uint16_t width_pixels;
	uint16_t height_pixels;
	uint32_t ready_timeout_milliseconds;
	uint32_t reset_pre_delay_milliseconds;
	uint32_t reset_pulse_milliseconds;
	uint32_t post_reset_delay_milliseconds;
	uint8_t initial_brightness_percent;
};

struct tr230s_runtime_data {
	struct k_mutex access_mutex;
	enum display_orientation orientation;
	uint8_t brightness_percent;
	bool display_is_blanked;
};

static int tr230s_unlock_mutex(struct k_mutex *access_mutex, int operation_result)
{
	int unlock_result;

	unlock_result = k_mutex_unlock(access_mutex);
	if ((operation_result == 0) && (unlock_result != 0)) {
		return unlock_result;
	}

	if ((operation_result != 0) && (unlock_result != 0)) {
		LOG_ERR("Display mutex unlock failed after error %d (%d)",
			operation_result, unlock_result);
	}

	return operation_result;
}

/*
 * WAIT is TR230S controller flow control, not MIPI DBI tearing-effect
 * synchronization. It must gate every new controller transaction.
 */
static int tr230s_wait_until_ready(const struct device *dev)
{
	const struct tr230s_configuration *configuration;
	int64_t timeout_deadline;
	int wait_state;

	configuration = dev->config;

	if (configuration->ready_timeout_milliseconds ==
	    TR230S_READY_TIMEOUT_FOREVER_MS) {
		for (;;) {
			wait_state = gpio_pin_get_dt(&configuration->wait_gpio);
			if (wait_state < 0) {
				return wait_state;
			}

			if (wait_state == TR230S_WAIT_READY_STATE) {
				return 0;
			}

			k_busy_wait(TR230S_READY_POLL_INTERVAL_US);
		}
	}

	timeout_deadline = k_uptime_get() +
		(int64_t)configuration->ready_timeout_milliseconds;

	do {
		wait_state = gpio_pin_get_dt(&configuration->wait_gpio);
		if (wait_state < 0) {
			return wait_state;
		}

		if (wait_state == TR230S_WAIT_READY_STATE) {
			return 0;
		}

		k_busy_wait(TR230S_READY_POLL_INTERVAL_US);
	} while (k_uptime_get() < timeout_deadline);

	return -ETIMEDOUT;
}

/* Preserve the primary operation error if releasing a held DBI bus also fails. */
static int tr230s_complete_dbi_transaction(const struct device *dev,
					   bool transaction_was_started,
					   int operation_result)
{
	const struct tr230s_configuration *configuration;
	int release_result;

	if (!transaction_was_started) {
		return operation_result;
	}

	configuration = dev->config;
	release_result = mipi_dbi_release(configuration->mipi_dbi_device,
					  &configuration->dbi_configuration);

	if ((operation_result == 0) && (release_result != 0)) {
		return release_result;
	}

	if ((operation_result != 0) && (release_result != 0)) {
		LOG_ERR("MIPI DBI release failed after error %d (%d)",
			operation_result, release_result);
	}

	return operation_result;
}

static int tr230s_write_command_locked(const struct device *dev, uint8_t command,
				       const uint8_t *parameter_buffer,
				       size_t parameter_length)
{
	const struct tr230s_configuration *configuration;
	bool transaction_was_started;
	int operation_result;

	if ((parameter_length != 0U) && (parameter_buffer == NULL)) {
		return -EINVAL;
	}

	configuration = dev->config;
	transaction_was_started = false;

	operation_result = tr230s_wait_until_ready(dev);
	if (operation_result != 0) {
		return operation_result;
	}

	transaction_was_started = true;
	operation_result = mipi_dbi_command_write(
		configuration->mipi_dbi_device,
		&configuration->dbi_configuration,
		command, parameter_buffer, parameter_length);

	return tr230s_complete_dbi_transaction(dev, transaction_was_started,
					       operation_result);
}

static void tr230s_encode_address_range(
	uint16_t start_position, uint16_t end_position,
	uint8_t encoded_range[TR230S_ADDRESS_RANGE_SIZE_BYTES])
{
	sys_put_be16(start_position, encoded_range);
	sys_put_be16(end_position, &encoded_range[sizeof(start_position)]);
}

static int tr230s_set_memory_area_locked(const struct device *dev,
					 uint16_t x_position,
					 uint16_t y_position,
					 uint16_t width_pixels,
					 uint16_t height_pixels)
{
	uint16_t x_end_position;
	uint16_t y_end_position;
	uint8_t address_parameters[TR230S_ADDRESS_RANGE_SIZE_BYTES];
	int operation_result;

	/* TR230S address-window end coordinates are inclusive. */
	x_end_position = (uint16_t)((uint32_t)x_position +
		(uint32_t)width_pixels - 1U);
	y_end_position = (uint16_t)((uint32_t)y_position +
		(uint32_t)height_pixels - 1U);

	tr230s_encode_address_range(x_position, x_end_position,
				    address_parameters);
	operation_result = tr230s_write_command_locked(
		dev, TR230S_COMMAND_COLUMN_ADDRESS, address_parameters,
		sizeof(address_parameters));
	if (operation_result != 0) {
		return operation_result;
	}

	tr230s_encode_address_range(y_position, y_end_position,
				    address_parameters);

	return tr230s_write_command_locked(
		dev, TR230S_COMMAND_ROW_ADDRESS, address_parameters,
		sizeof(address_parameters));
}

/*
 * MIPI DBI requires pitch == width. A source buffer with row padding is
 * therefore transferred one visible row at a time. CS remains held from the
 * 0x2C command through the complete pixel payload and is released afterwards.
 */
static int tr230s_write_pixel_rows_locked(
	const struct device *dev,
	const struct display_buffer_descriptor *descriptor,
	const uint8_t *pixel_buffer)
{
	const struct tr230s_configuration *configuration;
	struct display_buffer_descriptor dbi_descriptor;
	bool transaction_was_started;
	size_t row_stride_bytes;
	size_t row_write_bytes;
	int operation_result;

	configuration = dev->config;
	transaction_was_started = false;
	row_stride_bytes = (size_t)descriptor->pitch * TR230S_PIXEL_SIZE_BYTES;
	row_write_bytes = (size_t)descriptor->width * TR230S_PIXEL_SIZE_BYTES;

	operation_result = tr230s_wait_until_ready(dev);
	if (operation_result != 0) {
		return operation_result;
	}

	transaction_was_started = true;
	operation_result = mipi_dbi_command_write(
		configuration->mipi_dbi_device,
		&configuration->dbi_configuration,
		TR230S_COMMAND_DISPLAY_DATA, NULL, 0U);

	if (operation_result == 0) {
		dbi_descriptor.width = descriptor->width;
		dbi_descriptor.pitch = descriptor->width;

		if (descriptor->pitch == descriptor->width) {
			dbi_descriptor.height = descriptor->height;
			dbi_descriptor.buf_size = (uint32_t)(
				row_write_bytes * (size_t)descriptor->height);
			dbi_descriptor.frame_incomplete =
				descriptor->frame_incomplete;

			operation_result = mipi_dbi_write_display(
				configuration->mipi_dbi_device,
				&configuration->dbi_configuration,
				pixel_buffer, &dbi_descriptor,
				PIXEL_FORMAT_RGB_565X);
		} else {
			dbi_descriptor.height = 1U;
			dbi_descriptor.buf_size = (uint32_t)row_write_bytes;

			for (size_t row_index = 0U;
			     (row_index < (size_t)descriptor->height) &&
			     (operation_result == 0);
			     ++row_index) {
				dbi_descriptor.frame_incomplete =
					((row_index + 1U) <
					 (size_t)descriptor->height) ||
					descriptor->frame_incomplete;

				operation_result = mipi_dbi_write_display(
					configuration->mipi_dbi_device,
					&configuration->dbi_configuration,
					&pixel_buffer[row_index *
						      row_stride_bytes],
					&dbi_descriptor,
					PIXEL_FORMAT_RGB_565X);
			}
		}
	}

	return tr230s_complete_dbi_transaction(dev, transaction_was_started,
					       operation_result);
}

static void tr230s_get_logical_resolution(const struct device *dev,
					  uint16_t *width_pixels,
					  uint16_t *height_pixels)
{
	const struct tr230s_configuration *configuration;
	const struct tr230s_runtime_data *runtime_data;

	configuration = dev->config;
	runtime_data = dev->data;

	if ((runtime_data->orientation == DISPLAY_ORIENTATION_ROTATED_90) ||
	    (runtime_data->orientation == DISPLAY_ORIENTATION_ROTATED_270)) {
		*width_pixels = configuration->height_pixels;
		*height_pixels = configuration->width_pixels;
	} else {
		*width_pixels = configuration->width_pixels;
		*height_pixels = configuration->height_pixels;
	}
}

static int tr230s_validate_write_request(
	const struct device *dev, uint16_t x_position, uint16_t y_position,
	const struct display_buffer_descriptor *descriptor,
	const void *pixel_buffer)
{
	uint16_t logical_height_pixels;
	uint16_t logical_width_pixels;
	uint64_t required_buffer_size;
	uint64_t row_stride_bytes;
	uint64_t row_write_bytes;

	if ((descriptor == NULL) || (pixel_buffer == NULL)) {
		return -EINVAL;
	}

	if ((descriptor->width == 0U) || (descriptor->height == 0U) ||
	    (descriptor->pitch < descriptor->width)) {
		return -EINVAL;
	}

	tr230s_get_logical_resolution(dev, &logical_width_pixels,
				      &logical_height_pixels);

	if (((uint32_t)x_position + (uint32_t)descriptor->width >
	     (uint32_t)logical_width_pixels) ||
	    ((uint32_t)y_position + (uint32_t)descriptor->height >
	     (uint32_t)logical_height_pixels)) {
		return -EINVAL;
	}

	row_stride_bytes =
		(uint64_t)descriptor->pitch * TR230S_PIXEL_SIZE_BYTES;
	row_write_bytes =
		(uint64_t)descriptor->width * TR230S_PIXEL_SIZE_BYTES;
	required_buffer_size =
		((uint64_t)descriptor->height - 1U) * row_stride_bytes +
		row_write_bytes;

	if ((required_buffer_size > (uint64_t)descriptor->buf_size) ||
	    (required_buffer_size > (uint64_t)SIZE_MAX)) {
		return -EINVAL;
	}

	return 0;
}

static int tr230s_write(const struct device *dev, uint16_t x_position,
			uint16_t y_position,
			const struct display_buffer_descriptor *descriptor,
			const void *pixel_buffer)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	operation_result = tr230s_validate_write_request(
		dev, x_position, y_position, descriptor, pixel_buffer);
	if (operation_result != 0) {
		return operation_result;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_set_memory_area_locked(
		dev, x_position, y_position, descriptor->width,
		descriptor->height);
	if (operation_result == 0) {
		operation_result = tr230s_write_pixel_rows_locked(
			dev, descriptor, pixel_buffer);
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/* Blanking changes PWM only; the nonblanked brightness is retained. */
static int tr230s_blanking_on(const struct device *dev)
{
	struct tr230s_runtime_data *runtime_data;
	const uint8_t blanked_brightness_percent =
		TR230S_BLANKED_BRIGHTNESS_PERCENT;
	int operation_result;

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_write_command_locked(
		dev, TR230S_COMMAND_PWM_DUTY, &blanked_brightness_percent,
		sizeof(blanked_brightness_percent));
	if (operation_result == 0) {
		runtime_data->display_is_blanked = true;
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

static int tr230s_blanking_off(const struct device *dev)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_write_command_locked(
		dev, TR230S_COMMAND_PWM_DUTY,
		&runtime_data->brightness_percent,
		sizeof(runtime_data->brightness_percent));
	if (operation_result == 0) {
		runtime_data->display_is_blanked = false;
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

static int tr230s_set_brightness(const struct device *dev,
				 uint8_t brightness_percent)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	if (brightness_percent > TR230S_MAXIMUM_BRIGHTNESS_PERCENT) {
		return -EINVAL;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	/* Preserve blanking while updating the brightness restored by blanking_off. */
	if (runtime_data->display_is_blanked) {
		runtime_data->brightness_percent = brightness_percent;
		operation_result = 0;
	} else {
		operation_result = tr230s_write_command_locked(
			dev, TR230S_COMMAND_PWM_DUTY, &brightness_percent,
			sizeof(brightness_percent));
		if (operation_result == 0) {
			runtime_data->brightness_percent = brightness_percent;
		}
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

static int tr230s_set_pixel_format(const struct device *dev,
				   enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	if (pixel_format != PIXEL_FORMAT_RGB_565X) {
		return -ENOTSUP;
	}

	return 0;
}

static int tr230s_set_orientation(const struct device *dev,
				  enum display_orientation orientation)
{
	struct tr230s_runtime_data *runtime_data;
	uint8_t orientation_parameter;
	int operation_result;

	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_NORMAL;
		break;
	case DISPLAY_ORIENTATION_ROTATED_90:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_90_DEGREES;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_180_DEGREES;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_270_DEGREES;
		break;
	default:
		return -EINVAL;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	if (runtime_data->orientation == orientation) {
		operation_result = 0;
	} else {
		operation_result = tr230s_write_command_locked(
			dev, TR230S_COMMAND_MIRROR_AND_ROTATION,
			&orientation_parameter, sizeof(orientation_parameter));
		if (operation_result == 0) {
			runtime_data->orientation = orientation;
		}
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

static void tr230s_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct tr230s_runtime_data *runtime_data;
	uint16_t logical_height_pixels;
	uint16_t logical_width_pixels;

	if (capabilities == NULL) {
		return;
	}

	runtime_data = dev->data;
	tr230s_get_logical_resolution(dev, &logical_width_pixels,
				      &logical_height_pixels);

	(void)memset(capabilities, 0, sizeof(*capabilities));
	capabilities->x_resolution = logical_width_pixels;
	capabilities->y_resolution = logical_height_pixels;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565X;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565X;
	capabilities->current_orientation = runtime_data->orientation;
}

static int tr230s_initialize(const struct device *dev)
{
	const struct tr230s_configuration *configuration;
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	configuration = dev->config;
	runtime_data = dev->data;

	if (!device_is_ready(configuration->mipi_dbi_device) ||
	    !gpio_is_ready_dt(&configuration->wait_gpio)) {
		return -ENODEV;
	}

	if (configuration->dbi_configuration.mode !=
	    MIPI_DBI_MODE_SPI_4WIRE) {
		return -ENOTSUP;
	}

	if ((configuration->width_pixels == 0U) ||
	    (configuration->height_pixels == 0U) ||
	    (configuration->reset_pulse_milliseconds == 0U) ||
	    (configuration->initial_brightness_percent >
	     TR230S_MAXIMUM_BRIGHTNESS_PERCENT)) {
		return -EINVAL;
	}

	k_mutex_init(&runtime_data->access_mutex);
	runtime_data->orientation = DISPLAY_ORIENTATION_NORMAL;
	runtime_data->brightness_percent =
		configuration->initial_brightness_percent;
	runtime_data->display_is_blanked = false;

	operation_result = gpio_pin_configure_dt(
		&configuration->wait_gpio, GPIO_INPUT | GPIO_PULL_UP);
	if (operation_result != 0) {
		return operation_result;
	}

	if (configuration->reset_pre_delay_milliseconds != 0U) {
		k_sleep(K_MSEC(configuration->reset_pre_delay_milliseconds));
	}

	operation_result = mipi_dbi_reset(
		configuration->mipi_dbi_device,
		configuration->reset_pulse_milliseconds);
	if (operation_result != 0) {
		return operation_result;
	}

	if (configuration->post_reset_delay_milliseconds != 0U) {
		k_sleep(K_MSEC(configuration->post_reset_delay_milliseconds));
	}

	LOG_INF("TR230S initialized: %ux%u, RGB565X, MIPI DBI Type C, %u Hz",
		(unsigned int)configuration->width_pixels,
		(unsigned int)configuration->height_pixels,
		(unsigned int)configuration->dbi_configuration.config.frequency);

	return 0;
}

static DEVICE_API(display, tr230s_display_api) = {
	.blanking_on = tr230s_blanking_on,
	.blanking_off = tr230s_blanking_off,
	.write = tr230s_write,
	.set_brightness = tr230s_set_brightness,
	.get_capabilities = tr230s_get_capabilities,
	.set_pixel_format = tr230s_set_pixel_format,
	.set_orientation = tr230s_set_orientation,
};

#define TR230S_DEFINE(instance)                                                   \
	BUILD_ASSERT(DT_INST_PROP(instance, width) > 0,                           \
		     "TR230S width must be positive");                            \
	BUILD_ASSERT(DT_INST_PROP(instance, width) <= UINT16_MAX,                  \
		     "TR230S width exceeds the display API range");               \
	BUILD_ASSERT(DT_INST_PROP(instance, height) > 0,                          \
		     "TR230S height must be positive");                           \
	BUILD_ASSERT(DT_INST_PROP(instance, height) <= UINT16_MAX,                 \
		     "TR230S height exceeds the display API range");              \
	BUILD_ASSERT(DT_INST_PROP(instance, reset_pulse_ms) > 0,                  \
		     "TR230S reset pulse must be positive");                      \
	BUILD_ASSERT(DT_INST_PROP(instance, initial_brightness) <=                 \
			     TR230S_MAXIMUM_BRIGHTNESS_PERCENT,                       \
		     "TR230S initial brightness exceeds 100 percent");            \
	                                                                            \
	static const struct tr230s_configuration tr230s_configuration_##instance = { \
		.mipi_dbi_device = DEVICE_DT_GET(DT_INST_PARENT(instance)),          \
		.dbi_configuration = MIPI_DBI_CONFIG_DT_INST(                       \
			instance, SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB |         \
				SPI_WORD_SET(TR230S_DBI_SPI_WORD_SIZE_BITS) |          \
				SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),                      \
		.wait_gpio = GPIO_DT_SPEC_INST_GET(instance, wait_gpios),            \
		.width_pixels = (uint16_t)DT_INST_PROP(instance, width),              \
		.height_pixels = (uint16_t)DT_INST_PROP(instance, height),            \
		.ready_timeout_milliseconds =                                         \
			(uint32_t)DT_INST_PROP(instance, ready_timeout_ms),             \
		.reset_pre_delay_milliseconds =                                       \
			(uint32_t)DT_INST_PROP(instance, reset_pre_delay_ms),           \
		.reset_pulse_milliseconds =                                           \
			(uint32_t)DT_INST_PROP(instance, reset_pulse_ms),               \
		.post_reset_delay_milliseconds =                                      \
			(uint32_t)DT_INST_PROP(instance, post_reset_delay_ms),          \
		.initial_brightness_percent =                                         \
			(uint8_t)DT_INST_PROP(instance, initial_brightness),            \
	};                                                                          \
	                                                                            \
	static struct tr230s_runtime_data tr230s_runtime_data_##instance;           \
	                                                                            \
	DEVICE_DT_INST_DEFINE(instance, tr230s_initialize, NULL,                    \
			      &tr230s_runtime_data_##instance,                        \
			      &tr230s_configuration_##instance, POST_KERNEL,          \
			      CONFIG_DISPLAY_INIT_PRIORITY, &tr230s_display_api);

DT_INST_FOREACH_STATUS_OKAY(TR230S_DEFINE)
