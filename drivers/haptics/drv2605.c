/*
 * Copyright 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DRV2605 Datasheet: https://www.ti.com/lit/gpn/drv2605
 */

#define DT_DRV_COMPAT ti_drv2605

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/drv2605.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(DRV2605, CONFIG_HAPTICS_LOG_LEVEL);

#define DRV2605_REG_STATUS         0x0
#define DRV2605_DEVICE_ID          GENMASK(7, 5)
#define DRV2605_DEVICE_ID_DRV2605  0x3
#define DRV2605_DEVICE_ID_DRV2605L 0x7
#define DRV2605_DIAG_RESULT        BIT(3)
#define DRV2605_FB_STS             BIT(2)
#define DRV2605_OVER_TEMP          BIT(1)
#define DRV2605_OC_DETECT          BIT(0)

#define DRV2605_REG_MODE  0x1
#define DRV2605_DEV_RESET BIT(7)
#define DRV2605_STANDBY   BIT(6)
#define DRV2605_MODE      GENMASK(2, 0)

#define DRV2605_REG_RT_PLAYBACK_INPUT 0x2

#define DRV2605_REG_UNNAMED 0x3
#define DRV2605_HI_Z_OUTPUT BIT(4)
#define DRV2605_LIBRARY_SEL GENMASK(2, 0)

#define DRV2605_REG_WAVEFORM_SEQUENCER 0x4
#define DRV2605_WAIT                   BIT(7)
#define DRV2605_WAV_FRM_SEQ            GENMASK(6, 0)

#define DRV2605_REG_GO 0xc
#define DRV2605_GO     BIT(0)

#define DRV2605_REG_OVERDRIVE_TIME_OFFSET 0xd

#define DRV2605_REG_SUSTAIN_TIME_OFFSET_POS 0xe

#define DRV2605_REG_SUSTAIN_TIME_OFFSET_NEG 0xf

#define DRV2605_REG_BRAKE_TIME_OFFSET 0x10

#define DRV2605_TIME_STEP_MS 5

#define DRV2605_REG_AUDIO_TO_VIBE_CONTROL 0x11
#define DRV2605_ATH_PEAK_TIME             GENMASK(3, 2)
#define DRV2605_ATH_FILTER                GENMASK(1, 0)

#define DRV2605_REG_AUDIO_TO_VIBE_MIN_INPUT_LEVEL 0x12

#define DRV2605_REG_AUDIO_TO_VIBE_MAX_INPUT_LEVEL 0x13

#define DRV2605_ATH_INPUT_STEP_UV (1800000 / 255)

#define DRV2605_REG_AUDIO_TO_VIBE_MIN_OUTPUT_DRIVE 0x14

#define DRV2605_REG_AUDIO_TO_VIBE_MAX_OUTPUT_DRIVE 0x15

#define DRV2605_ATH_OUTPUT_DRIVE_PCT (100 * 255)

#define DRV2605_REG_RATED_VOLTAGE 0x16

#define DRV2605_REG_OVERDRIVE_CLAMP_VOLTAGE 0x17

#define DRV2605_REG_AUTO_CAL_COMP_RESULT 0x18

#define DRV2605_REG_AUTO_CAL_BACK_EMF_RESULT 0x19

#define DRV2605_REG_FEEDBACK_CONTROL 0x1a
#define DRV2605_N_ERM_LRA            BIT(7)
#define DRV2605_FB_BRAKE_FACTOR      GENMASK(6, 4)
#define DRV2605_LOOP_GAIN            GENMASK(3, 2)
#define DRV2605_BEMF_GAIN            GENMASK(1, 0)

#define DRV2605_ACTUATOR_MODE_ERM 0
#define DRV2605_ACTUATOR_MODE_LRA 1

#define DRV2605_REG_CONTROL1  0x1b
#define DRV2605_STARTUP_BOOST BIT(7)
#define DRV2605_AC_COUPLE     BIT(5)
#define DRV2605_DRIVE_TIME    GENMASK(4, 0)

#define DRV2605_REG_CONTROL2     0x1c
#define DRV2605_BIDIR_INPUT      BIT(7)
#define DRV2605_BRAKE_STABILIZER BIT(6)
#define DRV2605_SAMPLE_TIME      GENMASK(5, 4)
#define DRV2605_BLANKING_TIME    GENMASK(3, 2)
#define DRV2605_IDISS_TIME       GENMASK(1, 0)

#define DRV2605_REG_CONTROL3    0x1d
#define DRV2605_NG_THRESH       GENMASK(7, 6)
#define DRV2605_ERM_OPEN_LOOP   BIT(5)
#define DRV2605_SUPPLY_COMP_DIS BIT(4)
#define DRV2605_DATA_FORMAT_RTP BIT(3)
#define DRV2605_LRA_DRIVE_MODE  BIT(2)
#define DRV2605_N_PWM_ANALOG    BIT(1)
#define DRV2605_LRA_OPEN_LOOP   BIT(0)

#define DRV2605_REG_CONTROL4       0x1e
#define DRV2605_ZERO_CROSSING_TIME GENMASK(7, 6)
#define DRV2605_AUTO_CAL_TIME      GENMASK(5, 4)
#define DRV2605_OTP_STATUS         BIT(2)
#define DRV2605_OTP_PROGRAM        BIT(0)

#define DRV2605_REG_BATT_VOLTAGE_MONITOR 0x21
#define DRV2605_VBAT_STEP_UV             (5600000 / 255)

#define DRV2605_REG_LRA_RESONANCE_PERIOD 0x22

#define DRV2605_POWER_UP_DELAY_US 250

#define DRV2605_VOLTAGE_SCALE_FACTOR_MV 5600

#define DRV2605_CALCULATE_VOLTAGE(_volt) ((_volt * 255) / DRV2605_VOLTAGE_SCALE_FACTOR_MV)

struct drv2605_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec en_gpio;
	struct gpio_dt_spec in_trig_gpio;
	uint8_t feedback_brake_factor;
	uint8_t loop_gain;
	uint8_t rated_voltage;
	uint8_t overdrive_clamp_voltage;
	uint8_t auto_cal_time;
	uint8_t drive_time;
	bool actuator_mode;
};

struct drv2605_data {
	const struct device *dev;
	struct k_work rtp_work;
	const struct drv2605_rtp_data *rtp_data;
	enum drv2605_mode mode;
};

static inline int drv2605_haptic_config_audio(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_CONTROL3, DRV2605_N_PWM_ANALOG,
				     DRV2605_N_PWM_ANALOG);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_CONTROL1, DRV2605_AC_COUPLE,
				     DRV2605_AC_COUPLE);
	if (ret < 0) {
		return ret;
	}

	data->mode = DRV2605_MODE_AUDIO_TO_VIBE;

	return 0;
}

static inline int drv2605_haptic_config_pwm_analog(const struct device *dev, const bool analog)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;
	uint8_t value = 0;
	int ret;

	if (analog) {
		value = DRV2605_N_PWM_ANALOG;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_CONTROL3, DRV2605_N_PWM_ANALOG,
				     value);
	if (ret < 0) {
		return ret;
	}

	data->mode = DRV2605_MODE_PWM_ANALOG_INPUT;

	return 0;
}

static void drv2605_rtp_work_handler(struct k_work *work)
{
	struct drv2605_data *data = CONTAINER_OF(work, struct drv2605_data, rtp_work);
	const struct drv2605_rtp_data *rtp_data = data->rtp_data;
	const struct drv2605_config *config = data->dev->config;
	int i;

	for (i = 0; i < rtp_data->size; i++) {
		i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_RT_PLAYBACK_INPUT,
				      rtp_data->rtp_input[i]);
		k_usleep(rtp_data->rtp_hold_us[i]);
	}
}

static inline int drv2605_haptic_config_rtp(const struct device *dev,
					    const struct drv2605_rtp_data *rtp_data)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;
	int ret;

	data->rtp_data = rtp_data;

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_RT_PLAYBACK_INPUT, 0);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_MODE,
				     (uint8_t)DRV2605_MODE_RTP);
	if (ret < 0) {
		return ret;
	}

	data->mode = DRV2605_MODE_RTP;

	return 0;
}

static inline int drv2605_haptic_config_rom(const struct device *dev,
					    const struct drv2605_rom_data *rom_data)
{
	const struct drv2605_config *config = dev->config;
	uint8_t reg_addr = DRV2605_REG_WAVEFORM_SEQUENCER;
	struct drv2605_data *data = dev->data;
	int i, ret;

	switch (rom_data->trigger) {
	case DRV2605_MODE_INTERNAL_TRIGGER:
	case DRV2605_MODE_EXTERNAL_EDGE_TRIGGER:
	case DRV2605_MODE_EXTERNAL_LEVEL_TRIGGER:
		ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_MODE,
					     (uint8_t)rom_data->trigger);
		if (ret < 0) {
			return ret;
		}

		data->mode = rom_data->trigger;
		break;
	default:
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_UNNAMED, DRV2605_LIBRARY_SEL,
				     (uint8_t)rom_data->library);
	if (ret < 0) {
		return ret;
	}

	for (i = 0; i < DRV2605_WAVEFORM_SEQUENCER_MAX; i++) {
		ret = i2c_reg_write_byte_dt(&config->i2c, reg_addr, rom_data->seq_regs[i]);
		if (ret < 0) {
			return ret;
		}

		reg_addr++;

		if (rom_data->seq_regs[i] == 0U) {
			break;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_OVERDRIVE_TIME_OFFSET,
				    rom_data->overdrive_time);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_SUSTAIN_TIME_OFFSET_POS,
				    rom_data->sustain_pos_time);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_SUSTAIN_TIME_OFFSET_NEG,
				    rom_data->sustain_neg_time);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_BRAKE_TIME_OFFSET,
				    rom_data->brake_time);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int drv2605_haptic_config(const struct device *dev, enum drv2605_haptics_source source,
			  const union drv2605_config_data *config_data)
{
	switch (source) {
	case DRV2605_HAPTICS_SOURCE_ROM:
		return drv2605_haptic_config_rom(dev, config_data->rom_data);
	case DRV2605_HAPTICS_SOURCE_RTP:
		return drv2605_haptic_config_rtp(dev, config_data->rtp_data);
	case DRV2605_HAPTICS_SOURCE_AUDIO:
		return drv2605_haptic_config_audio(dev);
	case DRV2605_HAPTICS_SOURCE_PWM:
		return drv2605_haptic_config_pwm_analog(dev, false);
	case DRV2605_HAPTICS_SOURCE_ANALOG:
		return drv2605_haptic_config_pwm_analog(dev, true);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int drv2605_edge_mode_event(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->in_trig_gpio, 1);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_set_dt(&config->in_trig_gpio, 0);
}

static int drv2605_stop_output(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;
	uint8_t value;
	int ret;

	switch (data->mode) {
	case DRV2605_MODE_DIAGNOSTICS:
	case DRV2605_MODE_AUTO_CAL:
		ret = i2c_reg_read_byte_dt(&config->i2c, DRV2605_REG_GO, &value);
		if (ret < 0) {
			return ret;
		}

		if (FIELD_GET(DRV2605_GO, value)) {
			LOG_DBG("Playback mode: %d is uninterruptible", data->mode);
			return -EBUSY;
		}

		break;
	case DRV2605_MODE_INTERNAL_TRIGGER:
		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_GO, DRV2605_GO, 0);
	case DRV2605_MODE_EXTERNAL_EDGE_TRIGGER:
		return drv2605_edge_mode_event(dev);
	case DRV2605_MODE_EXTERNAL_LEVEL_TRIGGER:
		return gpio_pin_set_dt(&config->in_trig_gpio, 0);
	case DRV2605_MODE_PWM_ANALOG_INPUT:
	case DRV2605_MODE_AUDIO_TO_VIBE:
		ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_MODE,
					     (uint8_t)DRV2605_MODE_INTERNAL_TRIGGER);
		if (ret < 0) {
			return ret;
		}

		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_GO, DRV2605_GO, 0);
	case DRV2605_MODE_RTP:
		return k_work_cancel(&data->rtp_work);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int drv2605_start_output(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;

	switch (data->mode) {
	case DRV2605_MODE_DIAGNOSTICS:
	case DRV2605_MODE_AUTO_CAL:
	case DRV2605_MODE_INTERNAL_TRIGGER:
		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_GO, DRV2605_GO, DRV2605_GO);
	case DRV2605_MODE_EXTERNAL_EDGE_TRIGGER:
		return drv2605_edge_mode_event(dev);
	case DRV2605_MODE_EXTERNAL_LEVEL_TRIGGER:
		return gpio_pin_set_dt(&config->in_trig_gpio, 1);
	case DRV2605_MODE_AUDIO_TO_VIBE:
	case DRV2605_MODE_PWM_ANALOG_INPUT:
		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_MODE,
					      (uint8_t)data->mode);
	case DRV2605_MODE_RTP:
		return k_work_submit(&data->rtp_work);
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int drv2605_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct drv2605_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_STANDBY, 0);
	case PM_DEVICE_ACTION_SUSPEND:
		return i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_STANDBY,
					      DRV2605_STANDBY);
	case PM_DEVICE_ACTION_TURN_OFF:
		if (config->en_gpio.port != NULL) {
			gpio_pin_set_dt(&config->en_gpio, 0);
		}

		break;
	case PM_DEVICE_ACTION_TURN_ON:
		if (config->en_gpio.port != NULL) {
			gpio_pin_set_dt(&config->en_gpio, 1);
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static int drv2605_hw_config(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	uint8_t mask, value;
	int ret;

	value = FIELD_PREP(DRV2605_N_ERM_LRA, config->actuator_mode) |
		FIELD_PREP(DRV2605_FB_BRAKE_FACTOR, config->feedback_brake_factor) |
		FIELD_PREP(DRV2605_LOOP_GAIN, config->loop_gain);

	mask = DRV2605_N_ERM_LRA | DRV2605_FB_BRAKE_FACTOR | DRV2605_LOOP_GAIN;

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_FEEDBACK_CONTROL, mask, value);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_RATED_VOLTAGE, config->rated_voltage);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, DRV2605_REG_OVERDRIVE_CLAMP_VOLTAGE,
				    config->overdrive_clamp_voltage);
	if (ret < 0) {
		return ret;
	}

	if (config->actuator_mode == DRV2605_ACTUATOR_MODE_LRA) {
		ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_CONTROL3,
					     DRV2605_LRA_OPEN_LOOP, DRV2605_LRA_OPEN_LOOP);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int drv2605_reset(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	int retries = 5, ret;
	uint8_t value;

	i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_STANDBY, 0);

	ret = i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_DEV_RESET,
				     DRV2605_DEV_RESET);
	if (ret < 0) {
		return ret;
	}

	k_msleep(100);

	while (retries > 0) {
		i2c_reg_read_byte_dt(&config->i2c, DRV2605_REG_MODE, &value);

		if ((value & DRV2605_DEV_RESET) == 0U) {
			i2c_reg_update_byte_dt(&config->i2c, DRV2605_REG_MODE, DRV2605_STANDBY, 0);
			return 0;
		}

		retries--;

		k_usleep(10000);
	}

	return -ETIMEDOUT;
}

static int drv2605_check_devid(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	uint8_t value;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, DRV2605_REG_STATUS, &value);
	if (ret < 0) {
		return ret;
	}

	value = FIELD_GET(DRV2605_DEVICE_ID, value);

	switch (value) {
	case DRV2605_DEVICE_ID_DRV2605:
	case DRV2605_DEVICE_ID_DRV2605L:
		break;
	default:
		LOG_ERR("Invalid device ID found");
		return -ENOTSUP;
	}

	LOG_DBG("Found DRV2605, DEVID: 0x%x", value);

	return 0;
}

static int drv2605_gpio_config(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	int ret;

	if (config->en_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->en_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->in_trig_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->in_trig_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->in_trig_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int drv2605_init(const struct device *dev)
{
	const struct drv2605_config *config = dev->config;
	struct drv2605_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_usleep(DRV2605_POWER_UP_DELAY_US);

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	k_work_init(&data->rtp_work, drv2605_rtp_work_handler);

	ret = drv2605_gpio_config(dev);
	if (ret < 0) {
		LOG_DBG("Failed to allocate GPIOs: %d", ret);
		return ret;
	}

	ret = drv2605_check_devid(dev);
	if (ret < 0) {
		return ret;
	}

	ret = drv2605_reset(dev);
	if (ret < 0) {
		LOG_DBG("Failed to reset device: %d", ret);
		return ret;
	}

	ret = drv2605_hw_config(dev);
	if (ret < 0) {
		LOG_DBG("Failed to configure device: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(haptics, drv2605_driver_api) = {
	.start_output = &drv2605_start_output,
	.stop_output = &drv2605_stop_output,
};

#define HAPTICS_DRV2605_DEFINE(inst)                                                               \
                                                                                                   \
	static const struct drv2605_config drv2605_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.en_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {}),                           \
		.in_trig_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, in_trig_gpios, {}),                 \
		.feedback_brake_factor = DT_INST_ENUM_IDX(inst, feedback_brake_factor),            \
		.loop_gain = DT_INST_ENUM_IDX(inst, loop_gain),                                    \
		.actuator_mode = DT_INST_ENUM_IDX(inst, actuator_mode),                            \
		.rated_voltage = DRV2605_CALCULATE_VOLTAGE(DT_INST_PROP(inst, vib_rated_mv)),      \
		.overdrive_clamp_voltage =                                                         \
			DRV2605_CALCULATE_VOLTAGE(DT_INST_PROP(inst, vib_overdrive_mv)),           \
	};                                                                                         \
                                                                                                   \
	static struct drv2605_data drv2605_data_##inst = {                                         \
		.mode = DRV2605_MODE_INTERNAL_TRIGGER,                                             \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, drv2605_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, drv2605_init, PM_DEVICE_DT_INST_GET(inst),                     \
			      &drv2605_data_##inst, &drv2605_config_##inst, POST_KERNEL,           \
			      CONFIG_HAPTICS_INIT_PRIORITY, &drv2605_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HAPTICS_DRV2605_DEFINE)
