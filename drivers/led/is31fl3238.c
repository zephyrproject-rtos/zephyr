/*
 * Copyright (c) 2026 Moritz Gericke
 *
 * @see https://www.lumissil.com/assets/pdf/core/IS31FL3238_DS.pdf
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3238

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3238, CONFIG_LED_LOG_LEVEL);

/* IS31FL3238 register definitions */
#define CONTROL_REG         0x00 /* OSC freq / PWM bit res / software shutdown */
#define PWM_BASE_REG        0x01 /* Per channel PWM (4 bytes each) */
#define UPDATE_REG          0x49 /* PWM update */
#define SCALE_BASE_REG      0x4A /* Per channel output scaling (2 bytes each) */
#define GLOBAL_CURRENT_REG  0x6E /* Global output scaling */
#define SPREAD_SPECTRUM_REG 0x78 /* DC or PWM / spread spectrum*/
#define RESET_REG           0x7F /* Reset all registers to POR state (0x00)*/

#define CONTROL_NORMAL_OPERATION BIT(0)

#define PWM_STRIDE 4

#define MAX_LEDS 18

#define BUF_NOT_INC 0
#define BUF_INC     1

/* Extended led_info */
struct is31fl3238_led_info {
	struct led_info info;
	/* Mapping of the LED current limits */
	const uint16_t *current_limit_mapping;
	/* Starting channel of LED */
	const uint8_t start_channel;
};

struct is31fl3238_cfg {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec sdb;
	uint8_t current_limit;
	uint8_t spread_spectrum;
	uint8_t control;
	uint8_t num_leds;
	const struct is31fl3238_led_info *led_infos;
};

struct is31fl3238_data {
	/*
	 * Scratch buffer, used for bulk writes
	 * plus additional register byte
	 */
	uint8_t scratch_buf[1 + (MAX_LEDS * PWM_STRIDE)];
};

static int is31fl3238_led_write_channels_raw(const struct device *dev, uint32_t start_channel,
					     uint32_t num_channels, const uint8_t *buf,
					     uint32_t buf_inc)
{
	const struct is31fl3238_cfg *config = dev->config;
	struct is31fl3238_data *data = dev->data;
	int ret = 0U;
	uint8_t *pwm_start;

	if ((start_channel + num_channels) > MAX_LEDS) {
		return -EINVAL;
	}

	pwm_start = data->scratch_buf + (start_channel * PWM_STRIDE);
	/* Set PWM and LED target registers as first byte of each transfer */
	*pwm_start = PWM_BASE_REG + (start_channel * PWM_STRIDE);

	memset((pwm_start + 1), 0, (num_channels * PWM_STRIDE));

	for (uint32_t i = 0, j = 0; i < num_channels; i++, j += buf_inc) {
		/*
		 * Always write both PWMA and PWMB for maximum brightness.
		 * Use scaling and current limiting to configure.
		 */
		pwm_start[1 + (i * PWM_STRIDE)] = buf[j];
		pwm_start[1 + 2 + (i * PWM_STRIDE)] = buf[j];
	}
	LOG_HEXDUMP_DBG(pwm_start, 1 + (num_channels * PWM_STRIDE), "PWM states");

	ret = i2c_write_dt(&config->i2c, pwm_start, 1 + (num_channels * PWM_STRIDE));
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->i2c, UPDATE_REG, 0);
}

static int is31fl3238_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3238_cfg *config = dev->config;
	const struct is31fl3238_led_info *ext_info = &config->led_infos[led];
	uint8_t led_brightness = (uint8_t)(((uint32_t)value * 255) / LED_BRIGHTNESS_MAX);

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	return is31fl3238_led_write_channels_raw(dev, ext_info->start_channel,
						 ext_info->info.num_colors, &led_brightness,
						 BUF_NOT_INC);
}

int is31fl3238_led_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct is31fl3238_cfg *config = dev->config;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	*info = &config->led_infos[led].info;
	return 0;
}

int is31fl3238_led_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			     const uint8_t *color)
{
	const struct is31fl3238_cfg *config = dev->config;
	const struct is31fl3238_led_info *ext_info = &config->led_infos[led];

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	if (num_colors != ext_info->info.num_colors) {
		return -EINVAL;
	}

	return is31fl3238_led_write_channels_raw(dev, ext_info->start_channel, num_colors, color,
						 BUF_INC);
}

static int is31fl3238_led_write_channels(const struct device *dev, uint32_t start_channel,
					 uint32_t num_channels, const uint8_t *buf)
{
	return is31fl3238_led_write_channels_raw(dev, start_channel, num_channels, buf, BUF_INC);
}

static int is31fl3238_init(const struct device *dev)
{
	const struct is31fl3238_cfg *config = dev->config;
	struct is31fl3238_data *data = dev->data;
	int ret = 0U;
	int idx;

	LOG_DBG("Initializing @0x%x...", config->i2c.addr);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	if (config->sdb.port != NULL) {
		if (!gpio_is_ready_dt(&config->sdb)) {
			LOG_ERR("GPIO SDB pin not ready");
			return -ENODEV;
		}
		/* Set SDB pin high to exit hardware shutdown */
		ret = gpio_pin_configure_dt(&config->sdb, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * write reset reg to reset all registers to POR state,
	 * in case we are booting from a warm reset.
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, RESET_REG, 0x00);
	if (ret < 0) {
		return ret;
	}

	/*
	 * for each LED's color: apply individual output scaling
	 */
	data->scratch_buf[0] = SCALE_BASE_REG;
	idx = 1;
	for (uint8_t i = 0; i < config->num_leds; i++) {
		for (uint8_t j = 0; j < config->led_infos[i].info.num_colors; j++) {
			/* I_OUT is proportional to (SLA + SLB)  */
			data->scratch_buf[idx++] = config->led_infos[i].current_limit_mapping[j];
			data->scratch_buf[idx++] =
				(config->led_infos[i].current_limit_mapping[j] >> 8) * 0xFF;
		}
	}
	ret = i2c_write_dt(&config->i2c, data->scratch_buf, idx);
	if (ret < 0) {
		return ret;
	}

	/*
	 * apply global output scaling
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, GLOBAL_CURRENT_REG, config->current_limit);
	if (ret < 0) {
		return ret;
	}

	/*
	 * apply spread spectrum configuration
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, SPREAD_SPECTRUM_REG, config->spread_spectrum);
	if (ret < 0) {
		return ret;
	}

	/*
	 * set configured oscillator frequency and enter normal operation mode
	 */
	return i2c_reg_write_byte_dt(&config->i2c, CONTROL_REG,
				     (config->control | CONTROL_NORMAL_OPERATION));
}

static DEVICE_API(led, is31fl3238_led_api) = {.set_brightness = is31fl3238_led_set_brightness,
					      .get_info = is31fl3238_led_get_info,
					      .set_color = is31fl3238_led_set_color,
					      .write_channels = is31fl3238_led_write_channels};

#define SPREAD_SPECTRUM(id)                                                                        \
	((DT_INST_PROP(id, spread_spectrum_enable) << 4) |                                         \
	 (DT_ENUM_IDX(DT_DRV_INST(id), spread_spectrum_range_percent) << 2) |                      \
	 (DT_ENUM_IDX(DT_DRV_INST(id), spread_spectrum_cycle_time_us)))

#define CONTROL(id) (DT_ENUM_IDX(DT_DRV_INST(id), pwm_frequency_hz) << 4)

#define COLOR_MAPPING(led_node_id)                                                                 \
	static const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define COLOR_MAPPING_SIZE(led_node_id) DT_PROP_LEN(led_node_id, color_mapping)

#define CURRENT_LIMIT_MAPPING(led_node_id)                                                         \
	static const uint16_t current_limit_mapping_##led_node_id[] =                              \
		DT_PROP(led_node_id, current_limit_mapping);

#define BUILD_ASSERT_MAPPING_SIZE(led_node_id)                                                     \
	BUILD_ASSERT(ARRAY_SIZE(color_mapping_##led_node_id) ==                                    \
			     ARRAY_SIZE(current_limit_mapping_##led_node_id),                      \
		     "Color mapping and current limit mapping mismatch for " #led_node_id);

#define IS31FL3238_LED_INFO(led_node_id)                                                           \
	{                                                                                          \
		.info =                                                                            \
			{                                                                          \
				.label = DT_PROP(led_node_id, label),                              \
				.num_colors = DT_PROP_LEN(led_node_id, color_mapping),             \
				.color_mapping = color_mapping_##led_node_id,                      \
			},                                                                         \
		.current_limit_mapping = current_limit_mapping_##led_node_id,                      \
		.start_channel = DT_PROP(led_node_id, start_channel),                              \
	},

#define IS31FL3238_INIT(id)                                                                        \
                                                                                                   \
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)                                                   \
                                                                                                   \
	BUILD_ASSERT(DT_INST_FOREACH_CHILD_SEP(id, COLOR_MAPPING_SIZE, (+)) <= MAX_LEDS,           \
		     "Exceeding max number of LEDs for " #id);                                     \
                                                                                                   \
	DT_INST_FOREACH_CHILD(id, CURRENT_LIMIT_MAPPING)                                           \
                                                                                                   \
	DT_INST_FOREACH_CHILD(id, BUILD_ASSERT_MAPPING_SIZE)                                       \
                                                                                                   \
	static const struct is31fl3238_led_info is31fl3238_leds_##id[] = {                         \
		DT_INST_FOREACH_CHILD(id, IS31FL3238_LED_INFO)};                                   \
	BUILD_ASSERT(ARRAY_SIZE(is31fl3238_leds_##id) > 0, "No LEDs defined for " #id);            \
                                                                                                   \
	static const struct is31fl3238_cfg is31fl3238_##id##_cfg = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(id),                                                   \
		.sdb = GPIO_DT_SPEC_INST_GET_OR(id, sdb_gpios, {}),                                \
		.current_limit = DT_INST_PROP(id, current_limit),                                  \
		.spread_spectrum = SPREAD_SPECTRUM(id),                                            \
		.control = CONTROL(id),                                                            \
		.num_leds = ARRAY_SIZE(is31fl3238_leds_##id),                                      \
		.led_infos = is31fl3238_leds_##id,                                                 \
	};                                                                                         \
                                                                                                   \
	static struct is31fl3238_data is31fl3238_##id##_data = {0};                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &is31fl3238_init, NULL, &is31fl3238_##id##_data,                 \
			      &is31fl3238_##id##_cfg, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &is31fl3238_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3238_INIT)
