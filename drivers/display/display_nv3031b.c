/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT newvision_nv3031b

#include <zephyr/display/mipi_display.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define NV3031B_SPI_WRITE 0x02U
#define NV3031B_CMD2      0xFDU
#define NV3031B_RGB565    0x55U

#define NV3031B_MADCTL_MY  BIT(7)
#define NV3031B_MADCTL_MX  BIT(6)
#define NV3031B_MADCTL_MV  BIT(5)
#define NV3031B_MADCTL_BGR BIT(3)

#define NV3031B_SLEEP_DELAY_MS 120U

struct nv3031b_register {
	uint8_t command;
	uint8_t length;
	uint8_t values[8];
};

struct nv3031b_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec supply;
	struct gpio_dt_spec reset;
	const struct led_dt_spec *backlights;
	size_t num_backlights;
	const struct nv3031b_register *registers;
	size_t num_registers;
	uint16_t width;
	uint16_t height;
	uint8_t rotation;
	uint8_t brightness;
	bool color_invert;
	bool red_blue_swap;
};

struct nv3031b_data {
	struct k_mutex lock;
	k_timepoint_t ready_time;
	enum display_orientation orientation;
	uint8_t brightness;
	bool blanked;
	bool suspended;
};

static int nv3031b_command(const struct device *dev, uint8_t command, const void *values,
			  size_t length)
{
	const struct nv3031b_config *config = dev->config;
	struct nv3031b_data *data = dev->data;
	uint8_t header[] = {NV3031B_SPI_WRITE, 0U, command, 0U};
	struct spi_buf buffers[] = {
		{.buf = header, .len = sizeof(header)},
		{.buf = (void *)values, .len = length},
	};
	const struct spi_buf_set tx = {
		.buffers = buffers,
		.count = length == 0U ? 1U : ARRAY_SIZE(buffers),
	};
	int ret;

	while (!sys_timepoint_expired(data->ready_time)) {
		k_sleep(sys_timepoint_timeout(data->ready_time));
	}
	ret = spi_write_dt(&config->spi, &tx);
	if (ret == 0 && (command == MIPI_DCS_ENTER_SLEEP_MODE ||
			command == MIPI_DCS_EXIT_SLEEP_MODE)) {
		data->ready_time = sys_timepoint_calc(K_MSEC(NV3031B_SLEEP_DELAY_MS));
	}
	return ret;
}

static void nv3031b_resolution(const struct device *dev, uint16_t *width, uint16_t *height)
{
	const struct nv3031b_config *config = dev->config;
	const struct nv3031b_data *data = dev->data;
	bool landscape = data->orientation == DISPLAY_ORIENTATION_ROTATED_90 ||
			 data->orientation == DISPLAY_ORIENTATION_ROTATED_270;

	*width = landscape ? config->height : config->width;
	*height = landscape ? config->width : config->height;
}

static int nv3031b_window(const struct device *dev, uint16_t x, uint16_t y,
			 uint16_t width, uint16_t height)
{
	uint8_t address[4];
	int ret;

	sys_put_be16(x, address);
	sys_put_be16(x + width - 1U, &address[2]);
	ret = nv3031b_command(dev, MIPI_DCS_SET_COLUMN_ADDRESS, address, sizeof(address));
	if (ret != 0) {
		return ret;
	}

	sys_put_be16(y, address);
	sys_put_be16(y + height - 1U, &address[2]);

	return nv3031b_command(dev, MIPI_DCS_SET_PAGE_ADDRESS, address, sizeof(address));
}

static int nv3031b_write(const struct device *dev, uint16_t x, uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buffer)
{
	struct nv3031b_data *data = dev->data;
	const uint8_t *pixels = buffer;
	uint16_t width;
	uint16_t height;
	int ret;

	if (desc == NULL || buffer == NULL || desc->width == 0U || desc->height == 0U ||
	    desc->pitch < desc->width) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->suspended) {
		ret = -EHOSTDOWN;
		goto out;
	}
	nv3031b_resolution(dev, &width, &height);
	if (x >= width || y >= height || desc->width > width - x || desc->height > height - y ||
	    desc->buf_size < ((size_t)(desc->height - 1U) * desc->pitch + desc->width) * 2U) {
		ret = -EINVAL;
		goto out;
	}

	if (desc->pitch == desc->width) {
		ret = nv3031b_window(dev, x, y, desc->width, desc->height);
		if (ret == 0) {
			ret = nv3031b_command(dev, MIPI_DCS_WRITE_MEMORY_START, pixels,
					      (size_t)desc->width * desc->height * 2U);
		}
	} else {
		for (uint16_t row = 0U; row < desc->height; row++) {
			ret = nv3031b_window(dev, x, y + row, desc->width, 1U);
			if (ret != 0) {
				goto out;
			}
			ret = nv3031b_command(dev, MIPI_DCS_WRITE_MEMORY_START,
					      pixels + (size_t)row * desc->pitch * 2U,
					      desc->width * 2U);
			if (ret != 0) {
				goto out;
			}
		}
	}
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int nv3031b_backlight(const struct device *dev, uint8_t brightness)
{
	const struct nv3031b_config *config = dev->config;

	for (size_t i = 0U; i < config->num_backlights; i++) {
		int ret = led_set_brightness_dt(&config->backlights[i],
					       DIV_ROUND_CLOSEST(brightness * 100U, 255U));

		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

static int nv3031b_set_blanking(const struct device *dev, bool blanked)
{
	struct nv3031b_data *data = dev->data;
	uint8_t zero = 0U;
	int ret;

	if (blanked) {
		ret = nv3031b_backlight(dev, 0U);
		if (ret == 0) {
			ret = nv3031b_command(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0U);
		}
	} else {
		/* Display-on requires a dummy parameter byte on NV3031B. */
		ret = nv3031b_command(dev, MIPI_DCS_SET_DISPLAY_ON, &zero, sizeof(zero));
		if (ret == 0) {
			ret = nv3031b_backlight(dev, data->brightness);
		}
	}
	return ret;
}

static int nv3031b_blanking(const struct device *dev, bool blanked)
{
	struct nv3031b_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = data->suspended ? -EHOSTDOWN : nv3031b_set_blanking(dev, blanked);
	if (ret == 0) {
		data->blanked = blanked;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int nv3031b_blanking_on(const struct device *dev)
{
	return nv3031b_blanking(dev, true);
}

static int nv3031b_blanking_off(const struct device *dev)
{
	return nv3031b_blanking(dev, false);
}

static int nv3031b_set_brightness(const struct device *dev, uint8_t brightness)
{
	const struct nv3031b_config *config = dev->config;
	struct nv3031b_data *data = dev->data;
	int ret = 0;

	if (config->num_backlights == 0U) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->suspended) {
		ret = -EHOSTDOWN;
	} else if (!data->blanked) {
		ret = nv3031b_backlight(dev, brightness);
	}
	if (ret == 0) {
		data->brightness = brightness;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int nv3031b_set_orientation(const struct device *dev, enum display_orientation orientation)
{
	const struct nv3031b_config *config = dev->config;
	struct nv3031b_data *data = dev->data;
	static const uint8_t madctl[] = {
		[DISPLAY_ORIENTATION_NORMAL] = NV3031B_MADCTL_MY | NV3031B_MADCTL_MX,
		[DISPLAY_ORIENTATION_ROTATED_90] = NV3031B_MADCTL_MY | NV3031B_MADCTL_MV,
		[DISPLAY_ORIENTATION_ROTATED_180] = 0U,
		[DISPLAY_ORIENTATION_ROTATED_270] = NV3031B_MADCTL_MX | NV3031B_MADCTL_MV,
	};
	uint8_t value;
	int ret;

	if ((unsigned int)orientation >= ARRAY_SIZE(madctl)) {
		return -ENOTSUP;
	}
	value = madctl[orientation] | (config->red_blue_swap ? NV3031B_MADCTL_BGR : 0U);
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = data->suspended ? -EHOSTDOWN :
		nv3031b_command(dev, MIPI_DCS_SET_ADDRESS_MODE, &value, sizeof(value));
	if (ret == 0) {
		data->orientation = orientation;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static void nv3031b_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	struct nv3031b_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	nv3031b_resolution(dev, &caps->x_resolution, &caps->y_resolution);
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565X;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565X;
	caps->current_orientation = data->orientation;
	k_mutex_unlock(&data->lock);
}

static int nv3031b_set_pixel_format(const struct device *dev, enum display_pixel_format format)
{
	ARG_UNUSED(dev);

	return format == PIXEL_FORMAT_RGB_565X ? 0 : -ENOTSUP;
}

static int nv3031b_init(const struct device *dev)
{
	const struct nv3031b_config *config = dev->config;
	struct nv3031b_data *data = dev->data;
	const uint8_t unlock[] = {0x06U, 0x08U};
	const uint8_t lock[] = {0xFAU, 0xFCU};
	const uint8_t format = NV3031B_RGB565;
	int ret;

	k_mutex_init(&data->lock);
	data->ready_time = sys_timepoint_calc(K_NO_WAIT);
	data->brightness = config->brightness;
	data->blanked = true;
	if (!spi_is_ready_dt(&config->spi) || !gpio_is_ready_dt(&config->reset)) {
		return -ENODEV;
	}
	for (size_t i = 0U; i < config->num_backlights; i++) {
		if (!led_is_ready_dt(&config->backlights[i])) {
			return -ENODEV;
		}
	}
	ret = nv3031b_backlight(dev, 0U);
	if (ret != 0) {
		return ret;
	}
	if (config->supply.port != NULL) {
		if (!gpio_is_ready_dt(&config->supply)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->supply, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			return ret;
		}
		k_sleep(K_MSEC(50));
	}
	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		return ret;
	}
	k_sleep(K_MSEC(10));
	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret != 0) {
		return ret;
	}
	k_sleep(K_MSEC(500));
	ret = nv3031b_command(dev, MIPI_DCS_NOP, NULL, 0U);
	if (ret != 0) {
		return ret;
	}
	ret = nv3031b_command(dev, NV3031B_CMD2, unlock, sizeof(unlock));
	if (ret != 0) {
		return ret;
	}
	for (size_t i = 0U; i < config->num_registers; i++) {
		const struct nv3031b_register *reg = &config->registers[i];

		ret = nv3031b_command(dev, reg->command, reg->values, reg->length);
		if (ret != 0) {
			return ret;
		}
	}
	ret = nv3031b_command(dev, NV3031B_CMD2, lock, sizeof(lock));
	if (ret != 0) {
		return ret;
	}
	ret = nv3031b_command(dev, MIPI_DCS_SET_PIXEL_FORMAT, &format, sizeof(format));
	if (ret != 0) {
		return ret;
	}
	ret = nv3031b_set_orientation(dev, config->rotation);
	if (ret != 0) {
		return ret;
	}
	ret = nv3031b_command(dev, config->color_invert ? MIPI_DCS_ENTER_INVERT_MODE :
			      MIPI_DCS_EXIT_INVERT_MODE, NULL, 0U);
	if (ret != 0) {
		return ret;
	}
	return nv3031b_command(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0U);
}

#ifdef CONFIG_PM_DEVICE
static int nv3031b_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct nv3031b_data *data = dev->data;
	int ret;

	if (action != PM_DEVICE_ACTION_SUSPEND && action != PM_DEVICE_ACTION_RESUME) {
		return -ENOTSUP;
	}
	if (!k_can_yield()) {
		/* Mutex acquisition and synchronous bus transfers may block. */
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (action == PM_DEVICE_ACTION_SUSPEND) {
		ret = nv3031b_set_blanking(dev, true);
		if (ret == 0) {
			ret = nv3031b_command(dev, MIPI_DCS_ENTER_SLEEP_MODE, NULL, 0U);
		}
		if (ret == 0) {
			data->suspended = true;
		} else {
			/* Restore the active display after an unsuccessful suspend. */
			(void)nv3031b_set_blanking(dev, data->blanked);
		}
	} else {
		ret = nv3031b_command(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0U);
		if (ret == 0) {
			ret = nv3031b_set_blanking(dev, data->blanked);
		}
		if (ret == 0) {
			data->suspended = false;
		}
	}
	k_mutex_unlock(&data->lock);
	return ret;
}
#endif

static DEVICE_API(display, nv3031b_api) = {
	.blanking_on = nv3031b_blanking_on,
	.blanking_off = nv3031b_blanking_off,
	.write = nv3031b_write,
	.set_brightness = nv3031b_set_brightness,
	.get_capabilities = nv3031b_get_capabilities,
	.set_pixel_format = nv3031b_set_pixel_format,
	.set_orientation = nv3031b_set_orientation,
};

#define NV3031B_REGISTERS(fn, inst)                                                            \
	fn(inst, source_timing, 0x60U, 1U)                                                     \
	fn(inst, gate_timing, 0x61U, 2U)                                                       \
	fn(inst, inversion_control, 0xB4U, 1U)                                                 \
	fn(inst, frame_rate, 0xB1U, 3U)                                                        \
	fn(inst, porch_control, 0xB5U, 4U)                                                     \
	fn(inst, display_function, 0xB6U, 5U)                                                  \
	fn(inst, bias_trim, 0xDFU, 1U)                                                         \
	fn(inst, bias_current, 0x67U, 1U)                                                      \
	fn(inst, power_control, 0x68U, 4U)                                                     \
	fn(inst, gamma_positive_voltage, 0xE1U, 2U)                                            \
	fn(inst, gamma_negative_voltage, 0xE4U, 2U)                                            \
	fn(inst, gamma_positive_mid, 0xE2U, 6U)                                                \
	fn(inst, gamma_negative_mid, 0xE5U, 6U)                                                \
	fn(inst, gamma_positive, 0xE0U, 8U)                                                    \
	fn(inst, gamma_negative, 0xE3U, 8U)                                                    \
	fn(inst, power_slope, 0xE6U, 2U)                                                       \
	fn(inst, source_equalization, 0xE7U, 6U)                                               \
	fn(inst, source_level, 0xE8U, 3U)                                                      \
	fn(inst, gate_equalization, 0xECU, 1U)

#define NV3031B_REG_CONFIG(inst, prop, cmd, len)                                                 \
	{.command = cmd, .length = len, .values = DT_INST_PROP(inst, prop)},

#define NV3031B_BACKLIGHT(node, prop, idx) LED_DT_SPEC_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),

#define NV3031B_DEFINE(inst)                                                                     \
	static const struct nv3031b_register nv3031b_registers_##inst[] = {                      \
		NV3031B_REGISTERS(NV3031B_REG_CONFIG, inst)                                      \
	};                                                                                       \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, backlight_leds),                                  \
		(static const struct led_dt_spec nv3031b_backlights_##inst[] = {                 \
			DT_INST_FOREACH_PROP_ELEM(inst, backlight_leds, NV3031B_BACKLIGHT)       \
		};))                                                                             \
	static struct nv3031b_data nv3031b_data_##inst;                                          \
	static const struct nv3031b_config nv3031b_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                              \
		.supply = GPIO_DT_SPEC_INST_GET_OR(inst, supply_gpios, {0}),                     \
		.reset = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                               \
		.backlights = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, backlight_leds),           \
					 (nv3031b_backlights_##inst), (NULL)),                   \
		.num_backlights = DT_INST_PROP_LEN_OR(inst, backlight_leds, 0),                  \
		.registers = nv3031b_registers_##inst,                                           \
		.num_registers = ARRAY_SIZE(nv3031b_registers_##inst),                           \
		.width = DT_INST_PROP(inst, width),                                             \
		.height = DT_INST_PROP(inst, height),                                           \
		.rotation = DT_INST_ENUM_IDX(inst, rotation),                                    \
		.brightness = DT_INST_PROP(inst, brightness),                                    \
		.color_invert = DT_INST_PROP(inst, color_invert),                                \
		.red_blue_swap = DT_INST_PROP(inst, red_blue_swap),                              \
	};                                                                                       \
	PM_DEVICE_DT_INST_DEFINE(inst, nv3031b_pm_action);                                       \
	DEVICE_DT_INST_DEFINE(inst, nv3031b_init, PM_DEVICE_DT_INST_GET(inst),                   \
			      &nv3031b_data_##inst, \
			      &nv3031b_config_##inst, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, \
			      &nv3031b_api);

DT_INST_FOREACH_STATUS_OKAY(NV3031B_DEFINE)
