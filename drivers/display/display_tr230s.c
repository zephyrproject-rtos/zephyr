/*
 * SPDX-FileCopyrightText: 2026 Javier Longares Abaiz
 * SPDX-FileCopyrightText: 2026 A Blue Thing In The Cloud SLU
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tdo_tr230s

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(display_tr230s, CONFIG_DISPLAY_LOG_LEVEL);

#define TR230S_COMMAND_PWM_DUTY			0x20U
#define TR230S_COMMAND_COLUMN_ADDRESS		0x2AU
#define TR230S_COMMAND_ROW_ADDRESS		0x2BU
#define TR230S_COMMAND_DISPLAY_DATA		0x2CU
#define TR230S_COMMAND_MIRROR_AND_ROTATION	0xACU

#define TR230S_MAXIMUM_PWM_DUTY_PERCENT		100U
#define TR230S_BLANKED_PWM_DUTY_PERCENT		0U
#define TR230S_READY_POLL_INTERVAL_US		1U
#define TR230S_PIXEL_SIZE_BYTES			2U
#define TR230S_ADDRESS_RANGE_SIZE_BYTES		(2U * sizeof(uint16_t))
#define TR230S_WAIT_READY_STATE			1
#define TR230S_DBI_SPI_WORD_SIZE_BITS		8U

#define TR230S_DBI_MODE(instance) \
	DT_INST_STRING_UPPER_TOKEN_OR(instance, mipi_mode, MIPI_DBI_MODE_SPI_4WIRE)


#define TR230S_ORIENTATION_PARAMETER_NORMAL		0x00U
#define TR230S_ORIENTATION_PARAMETER_90_DEGREES		0x01U
#define TR230S_ORIENTATION_PARAMETER_180_DEGREES	0x02U
#define TR230S_ORIENTATION_PARAMETER_270_DEGREES	0x03U

struct tr230s_config {
	const struct device *mipi_dbi_device;
	struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec wait_gpio;
	uint16_t width_pixels;
	uint16_t height_pixels;
	k_timeout_t ready_timeout;
	uint32_t reset_pre_delay_ms;
	uint32_t reset_pulse_ms;
	uint32_t post_reset_delay_ms;
	uint8_t initial_brightness;
};

struct tr230s_data {
	struct k_mutex access_mutex;
	k_timepoint_t reset_ready_time;
	enum display_orientation orientation;
	uint8_t brightness;
	bool display_is_blanked;
	bool blanking_state_known;
};

static int tr230s_unlock_mutex(struct k_mutex *access_mutex, int ret)
{
	int unlock_ret;

	unlock_ret = k_mutex_unlock(access_mutex);
	if ((ret == 0) && (unlock_ret != 0)) {
		return unlock_ret;
	}

	if ((ret != 0) && (unlock_ret != 0)) {
		LOG_ERR("Display mutex unlock failed after error %d (%d)",
			ret, unlock_ret);
	}

	return ret;
}

/*
 * WAIT is TR230S controller flow control, not MIPI DBI tearing-effect
 * synchronization. It must gate every new controller transaction.
 */
static int tr230s_wait_until_ready(const struct device *dev)
{
	const struct tr230s_config *config = dev->config;
	struct tr230s_data *data = dev->data;
	k_timeout_t ready_timeout = config->ready_timeout;
	k_timepoint_t timeout;
	int wait_state;

	k_sleep(sys_timepoint_timeout(data->reset_ready_time));

	if (K_TIMEOUT_EQ(ready_timeout, K_NO_WAIT)) {
		ready_timeout = K_FOREVER;
	}
	timeout = sys_timepoint_calc(ready_timeout);

	for (;;) {
		wait_state = gpio_pin_get_dt(&config->wait_gpio);
		if (wait_state < 0) {
			return wait_state;
		}

		if (wait_state == TR230S_WAIT_READY_STATE) {
			return 0;
		}

		if (sys_timepoint_expired(timeout)) {
			return -ETIMEDOUT;
		}

		(void)k_usleep(TR230S_READY_POLL_INTERVAL_US);
	}
}

/* Preserve the primary operation error if releasing a held DBI bus also fails. */
static int tr230s_complete_dbi_transaction(const struct device *dev,
					   bool transaction_was_started,
					   int ret)
{
	const struct tr230s_config *config;
	int release_ret;

	if (!transaction_was_started) {
		return ret;
	}

	config = dev->config;
	release_ret = mipi_dbi_release(config->mipi_dbi_device,
					  &config->dbi_config);

	if ((ret == 0) && (release_ret != 0)) {
		return release_ret;
	}

	if ((ret != 0) && (release_ret != 0)) {
		LOG_ERR("MIPI DBI release failed after error %d (%d)",
			ret, release_ret);
	}

	return ret;
}

static int tr230s_write_command_locked(const struct device *dev, uint8_t command,
				       const uint8_t *parameter_buffer,
				       size_t parameter_length)
{
	const struct tr230s_config *config;
	bool transaction_was_started;
	int ret;

	if ((parameter_length != 0U) && (parameter_buffer == NULL)) {
		return -EINVAL;
	}

	config = dev->config;
	transaction_was_started = false;

	ret = tr230s_wait_until_ready(dev);
	if (ret != 0) {
		return ret;
	}

	transaction_was_started = true;
	ret = mipi_dbi_command_write(
		config->mipi_dbi_device,
		&config->dbi_config,
		command, parameter_buffer, parameter_length);

	return tr230s_complete_dbi_transaction(dev, transaction_was_started,
					       ret);
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
	int ret;

	/* TR230S address-window end coordinates are inclusive. */
	x_end_position = (uint16_t)((uint32_t)x_position +
		(uint32_t)width_pixels - 1U);
	y_end_position = (uint16_t)((uint32_t)y_position +
		(uint32_t)height_pixels - 1U);

	tr230s_encode_address_range(x_position, x_end_position,
				    address_parameters);
	ret = tr230s_write_command_locked(
		dev, TR230S_COMMAND_COLUMN_ADDRESS, address_parameters,
		sizeof(address_parameters));
	if (ret != 0) {
		return ret;
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
	const struct tr230s_config *config;
	struct display_buffer_descriptor dbi_descriptor;
	bool transaction_was_started;
	size_t row_stride_bytes;
	size_t row_write_bytes;
	int ret;

	config = dev->config;
	transaction_was_started = false;
	row_stride_bytes = (size_t)descriptor->pitch * TR230S_PIXEL_SIZE_BYTES;
	row_write_bytes = (size_t)descriptor->width * TR230S_PIXEL_SIZE_BYTES;

	ret = tr230s_wait_until_ready(dev);
	if (ret != 0) {
		return ret;
	}

	transaction_was_started = true;
	ret = mipi_dbi_command_write(
		config->mipi_dbi_device,
		&config->dbi_config,
		TR230S_COMMAND_DISPLAY_DATA, NULL, 0U);

	if (ret == 0) {
		dbi_descriptor.width = descriptor->width;
		dbi_descriptor.pitch = descriptor->width;

		if (descriptor->pitch == descriptor->width) {
			dbi_descriptor.height = descriptor->height;
			dbi_descriptor.buf_size = (uint32_t)(
				row_write_bytes * (size_t)descriptor->height);
			dbi_descriptor.frame_incomplete =
				descriptor->frame_incomplete;

			ret = mipi_dbi_write_display(
				config->mipi_dbi_device,
				&config->dbi_config,
				pixel_buffer, &dbi_descriptor,
				PIXEL_FORMAT_RGB_565X);
		} else {
			dbi_descriptor.height = 1U;
			dbi_descriptor.buf_size = (uint32_t)row_write_bytes;

			for (size_t row_index = 0U;
			     (row_index < (size_t)descriptor->height) &&
			     (ret == 0);
			     ++row_index) {
				dbi_descriptor.frame_incomplete =
					((row_index + 1U) <
					 (size_t)descriptor->height) ||
					descriptor->frame_incomplete;

				ret = mipi_dbi_write_display(
					config->mipi_dbi_device,
					&config->dbi_config,
					&pixel_buffer[row_index *
						      row_stride_bytes],
					&dbi_descriptor,
					PIXEL_FORMAT_RGB_565X);
			}
		}
	}

	return tr230s_complete_dbi_transaction(dev, transaction_was_started,
					       ret);
}

static void tr230s_get_logical_resolution(const struct device *dev,
					  uint16_t *width_pixels,
					  uint16_t *height_pixels)
{
	const struct tr230s_config *config;
	const struct tr230s_data *data;

	config = dev->config;
	data = dev->data;

	if ((data->orientation == DISPLAY_ORIENTATION_ROTATED_90) ||
	    (data->orientation == DISPLAY_ORIENTATION_ROTATED_270)) {
		*width_pixels = config->height_pixels;
		*height_pixels = config->width_pixels;
	} else {
		*width_pixels = config->width_pixels;
		*height_pixels = config->height_pixels;
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
	struct tr230s_data *data;
	int ret;

	ret = tr230s_validate_write_request(
		dev, x_position, y_position, descriptor, pixel_buffer);
	if (ret != 0) {
		return ret;
	}

	data = dev->data;
	ret = k_mutex_lock(&data->access_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = tr230s_set_memory_area_locked(
		dev, x_position, y_position, descriptor->width,
		descriptor->height);
	if (ret == 0) {
		ret = tr230s_write_pixel_rows_locked(
			dev, descriptor, pixel_buffer);
	}

	return tr230s_unlock_mutex(&data->access_mutex, ret);
}

static uint8_t tr230s_brightness_to_pwm_duty(uint8_t brightness)
{
	return (uint8_t)(((uint32_t)brightness * TR230S_MAXIMUM_PWM_DUTY_PERCENT +
			  (UINT8_MAX / 2U)) /
			 UINT8_MAX);
}

/* Blanking changes PWM only; the nonblanked brightness is retained. */
static int tr230s_blanking_on(const struct device *dev)
{
	struct tr230s_data *data;
	const uint8_t blanked_pwm_duty = TR230S_BLANKED_PWM_DUTY_PERCENT;
	int ret;

	data = dev->data;
	ret = k_mutex_lock(&data->access_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (data->blanking_state_known && data->display_is_blanked) {
		ret = 0;
	} else {
		ret = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
					  &blanked_pwm_duty, sizeof(blanked_pwm_duty));
		if (ret == 0) {
			data->display_is_blanked = true;
			data->blanking_state_known = true;
		}
	}

	return tr230s_unlock_mutex(&data->access_mutex, ret);
}

static int tr230s_blanking_off(const struct device *dev)
{
	struct tr230s_data *data;
	uint8_t pwm_duty;
	int ret;

	data = dev->data;
	ret = k_mutex_lock(&data->access_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (data->blanking_state_known && !data->display_is_blanked) {
		ret = 0;
	} else {
		pwm_duty = tr230s_brightness_to_pwm_duty(data->brightness);
		ret = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
					  &pwm_duty, sizeof(pwm_duty));
		if (ret == 0) {
			data->display_is_blanked = false;
			data->blanking_state_known = true;
		}
	}

	return tr230s_unlock_mutex(&data->access_mutex, ret);
}

static int tr230s_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct tr230s_data *data;
	uint8_t pwm_duty;
	int ret;

	data = dev->data;
	ret = k_mutex_lock(&data->access_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (data->blanking_state_known && data->display_is_blanked) {
		data->brightness = brightness;
		ret = 0;
	} else {
		pwm_duty = tr230s_brightness_to_pwm_duty(brightness);
		ret = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
					  &pwm_duty, sizeof(pwm_duty));
		if (ret == 0) {
			data->brightness = brightness;
			data->display_is_blanked = false;
			data->blanking_state_known = true;
		}
	}

	return tr230s_unlock_mutex(&data->access_mutex, ret);
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
	struct tr230s_data *data;
	uint8_t orientation_parameter;
	int ret;

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

	data = dev->data;
	ret = k_mutex_lock(&data->access_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (data->orientation == orientation) {
		ret = 0;
	} else {
		ret = tr230s_write_command_locked(
			dev, TR230S_COMMAND_MIRROR_AND_ROTATION,
			&orientation_parameter, sizeof(orientation_parameter));
		if (ret == 0) {
			data->orientation = orientation;
		}
	}

	return tr230s_unlock_mutex(&data->access_mutex, ret);
}

static void tr230s_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct tr230s_data *data;
	uint16_t logical_height_pixels;
	uint16_t logical_width_pixels;

	if (capabilities == NULL) {
		return;
	}

	data = dev->data;
	tr230s_get_logical_resolution(dev, &logical_width_pixels,
				      &logical_height_pixels);

	capabilities->x_resolution = logical_width_pixels;
	capabilities->y_resolution = logical_height_pixels;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565X;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565X;
	capabilities->current_orientation = data->orientation;
}

static int tr230s_initialize(const struct device *dev)
{
	const struct tr230s_config *config;
	struct tr230s_data *data;
	int ret;

	config = dev->config;
	data = dev->data;

	if (!device_is_ready(config->mipi_dbi_device) ||
	    !gpio_is_ready_dt(&config->wait_gpio)) {
		return -ENODEV;
	}

	k_mutex_init(&data->access_mutex);
	data->orientation = DISPLAY_ORIENTATION_NORMAL;
	data->brightness = config->initial_brightness;
	data->display_is_blanked = false;
	data->blanking_state_known = false;

	ret = gpio_pin_configure_dt(
		&config->wait_gpio, GPIO_INPUT | GPIO_PULL_UP);
	if (ret != 0) {
		return ret;
	}

	if (config->reset_pre_delay_ms != 0U) {
		k_sleep(K_MSEC(config->reset_pre_delay_ms));
	}

	ret = mipi_dbi_reset(
		config->mipi_dbi_device,
		config->reset_pulse_ms);
	if (ret != 0) {
		return ret;
	}

	data->reset_ready_time =
		sys_timepoint_calc(K_MSEC(config->post_reset_delay_ms));

	LOG_INF("TR230S initialized: %ux%u, RGB565X, MIPI DBI Type C, %u Hz",
		(unsigned int)config->width_pixels,
		(unsigned int)config->height_pixels,
		(unsigned int)config->dbi_config.config.frequency);

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

#define TR230S_DEFINE(instance)                                                        \
	BUILD_ASSERT(TR230S_DBI_MODE(instance) == MIPI_DBI_MODE_SPI_4WIRE,             \
		     "TR230S requires MIPI DBI SPI 4-wire mode");                         \
	BUILD_ASSERT(DT_INST_PROP(instance, width) > 0,                                \
		     "TR230S width must be positive");                                 \
	BUILD_ASSERT(DT_INST_PROP(instance, width) <= UINT16_MAX,                       \
		     "TR230S width exceeds the display API range");                    \
	BUILD_ASSERT(DT_INST_PROP(instance, height) > 0,                               \
		     "TR230S height must be positive");                                \
	BUILD_ASSERT(DT_INST_PROP(instance, height) <= UINT16_MAX,                      \
		     "TR230S height exceeds the display API range");                   \
	BUILD_ASSERT(DT_INST_PROP(instance, reset_pulse_ms) > 0,                       \
		     "TR230S reset pulse must be positive");                           \
	BUILD_ASSERT(DT_INST_PROP(instance, initial_brightness) <= UINT8_MAX,           \
		     "TR230S initial brightness exceeds display API range");            \
	                                                                                 \
	static const struct tr230s_config tr230s_config_##instance = {                   \
		.mipi_dbi_device = DEVICE_DT_GET(DT_INST_PARENT(instance)),               \
		.dbi_config = {                                                            \
			.mode = MIPI_DBI_MODE_SPI_4WIRE,                                      \
			.config = MIPI_DBI_SPI_CONFIG_DT_INST(                               \
				instance, SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB |          \
					SPI_WORD_SET(TR230S_DBI_SPI_WORD_SIZE_BITS) |           \
					SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),                       \
		},                                                                         \
		.wait_gpio = GPIO_DT_SPEC_INST_GET(instance, wait_gpios),                 \
		.width_pixels = (uint16_t)DT_INST_PROP(instance, width),                   \
		.height_pixels = (uint16_t)DT_INST_PROP(instance, height),                 \
		.ready_timeout = K_MSEC(DT_INST_PROP(instance, ready_timeout_ms)),          \
		.reset_pre_delay_ms =                                                      \
			(uint32_t)DT_INST_PROP(instance, reset_pre_delay_ms),                \
		.reset_pulse_ms = (uint32_t)DT_INST_PROP(instance, reset_pulse_ms),        \
		.post_reset_delay_ms =                                                     \
			(uint32_t)DT_INST_PROP(instance, post_reset_delay_ms),               \
		.initial_brightness = (uint8_t)DT_INST_PROP(instance, initial_brightness), \
	};                                                                               \
	                                                                                 \
	static struct tr230s_data tr230s_data_##instance;                                \
	                                                                                 \
	DEVICE_DT_INST_DEFINE(instance, tr230s_initialize, NULL,                         \
			      &tr230s_data_##instance, &tr230s_config_##instance,           \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,                    \
			      &tr230s_display_api);

DT_INST_FOREACH_STATUS_OKAY(TR230S_DEFINE)
