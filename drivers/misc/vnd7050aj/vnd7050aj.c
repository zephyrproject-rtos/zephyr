/*
 * Copyright (c) 2025, Eduard Iten
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_vnd7050aj

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/vnd7050aj/vnd7050aj.h>

LOG_MODULE_REGISTER(VND7050AJ, CONFIG_VND7050AJ_LOG_LEVEL);

/* Diagnostic selection modes */
enum vnd7050aj_diag_mode {
	DIAG_CURRENT_CH0,
	DIAG_CURRENT_CH1,
	DIAG_VCC,
	DIAG_TEMP,
};

struct vnd7050aj_config {
	struct gpio_dt_spec input0_gpio;
	struct gpio_dt_spec input1_gpio;
	struct gpio_dt_spec sel0_gpio;
	struct gpio_dt_spec sel1_gpio;
	struct gpio_dt_spec sen_gpio;
	struct gpio_dt_spec fault_reset_gpio;
	struct adc_dt_spec io_channels;
	uint32_t r_sense_ohms;
	uint32_t k_factor;          /* Current sense ratio */
	uint32_t k_vcc;             /* VCC sense ratio * 1000 */
	int32_t t_sense_0;          /* Temp sense reference temperature in °C */
	uint32_t v_sense_0;         /* Temp sense reference voltage in mV */
	uint32_t k_tchip;           /* Temp sense gain in °C/mV * 1000 */
};

struct vnd7050aj_data {
	struct k_mutex lock;
};

static int vnd7050aj_init(const struct device *dev)
{
	const struct vnd7050aj_config *config = dev->config;
	struct vnd7050aj_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	LOG_DBG("Initializing VND7050AJ device %s", dev->name);

	/* --- Check if all required devices are ready --- */
	if (!gpio_is_ready_dt(&config->input0_gpio)) {
		LOG_ERR("Input0 GPIO port is not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->input1_gpio)) {
		LOG_ERR("Input1 GPIO port is not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->sel0_gpio)) {
		LOG_ERR("Select0 GPIO port is not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->sel1_gpio)) {
		LOG_ERR("Select1 GPIO port is not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->sen_gpio)) {
		LOG_ERR("Sense GPIO port is not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->fault_reset_gpio)) {
		LOG_ERR("Fault Reset GPIO port is not ready");
		return -ENODEV;
	}

	if (!adc_is_ready_dt(&config->io_channels)) {
		LOG_ERR("ADC controller not ready");
		return -ENODEV;
	}

	/* --- Configure GPIOs to their initial states --- */
	err = gpio_pin_configure_dt(&config->input0_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure input0 GPIO: %d", err);
		return err;
	}

	err = gpio_pin_configure_dt(&config->input1_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure input1 GPIO: %d", err);
		return err;
	}
	err = gpio_pin_configure_dt(&config->sel0_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure select0 GPIO: %d", err);
		return err;
	}
	err = gpio_pin_configure_dt(&config->sel1_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure select1 GPIO: %d", err);
		return err;
	}
	err = gpio_pin_configure_dt(&config->sen_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure sense GPIO: %d", err);
		return err;
	}
	err = gpio_pin_configure_dt(&config->fault_reset_gpio,
				    GPIO_OUTPUT_ACTIVE); /* Active-low, so init high */
	if (err) {
		LOG_ERR("Failed to configure fault reset GPIO: %d", err);
		return err;
	}

	/* --- Configure the ADC channel --- */
	err = adc_channel_setup_dt(&config->io_channels);
	if (err) {
		LOG_ERR("Failed to setup ADC channel: %d", err);
		return err;
	}

	LOG_DBG("Device %s initialized", dev->name);
	return 0;
}

#define VND7050AJ_DEFINE(inst)                                                                     \
	static struct vnd7050aj_data vnd7050aj_data_##inst;                                        \
                                                                                                   \
	static const struct vnd7050aj_config vnd7050aj_config_##inst = {                           \
		.input0_gpio = GPIO_DT_SPEC_INST_GET(inst, input0_gpios),                          \
		.input1_gpio = GPIO_DT_SPEC_INST_GET(inst, input1_gpios),                          \
		.sel0_gpio = GPIO_DT_SPEC_INST_GET(inst, select0_gpios),                           \
		.sel1_gpio = GPIO_DT_SPEC_INST_GET(inst, select1_gpios),                           \
		.sen_gpio = GPIO_DT_SPEC_INST_GET(inst, sense_enable_gpios),                       \
		.fault_reset_gpio = GPIO_DT_SPEC_INST_GET(inst, fault_reset_gpios),                \
		.io_channels = ADC_DT_SPEC_INST_GET(inst),                                         \
		.r_sense_ohms = DT_INST_PROP(inst, r_sense_ohms),                                  \
		.k_factor = DT_INST_PROP(inst, k_factor),                                          \
		.k_vcc = DT_INST_PROP(inst, k_vcc),                                                \
		.t_sense_0 = DT_INST_PROP(inst, t_sense_0),                                        \
		.v_sense_0 = DT_INST_PROP(inst, v_sense_0),                                        \
		.k_tchip = DT_INST_PROP(inst, k_tchip),                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vnd7050aj_init, NULL, /* No PM support yet */                  \
			      &vnd7050aj_data_##inst, &vnd7050aj_config_##inst, POST_KERNEL,       \
			      CONFIG_VND7050AJ_INIT_PRIORITY,                                      \
			      NULL); /* No API struct needed for custom API */

DT_INST_FOREACH_STATUS_OKAY(VND7050AJ_DEFINE)

int vnd7050aj_set_output_state(const struct device *dev, uint8_t channel, bool state)
{
	const struct vnd7050aj_config *config = dev->config;

	if (channel != VND7050AJ_CHANNEL_0 && channel != VND7050AJ_CHANNEL_1) {
		return -EINVAL;
	}

	const struct gpio_dt_spec *gpio =
		(channel == VND7050AJ_CHANNEL_0) ? &config->input0_gpio : &config->input1_gpio;

	return gpio_pin_set_dt(gpio, (int)state);
}

static int vnd7050aj_read_sense_voltage(const struct device *dev, enum vnd7050aj_diag_mode mode,
					int32_t *voltage_mv)
{
	const struct vnd7050aj_config *config = dev->config;
	struct vnd7050aj_data *data = dev->data;
	int err = 0;

	/* Initialize the buffer to zero */
	*voltage_mv = 0;

	struct adc_sequence sequence = {
		.buffer = voltage_mv,
		.buffer_size = sizeof(*voltage_mv),
#ifdef CONFIG_ADC_CALIBRATION
		.calibrate = true,
#endif
	};
	adc_sequence_init_dt(&config->io_channels, &sequence);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Step 1: Select diagnostic mode */
	switch (mode) {
	case DIAG_CURRENT_CH0:
		gpio_pin_set_dt(&config->sel0_gpio, 0);
		gpio_pin_set_dt(&config->sel1_gpio, 0);
		gpio_pin_set_dt(&config->sen_gpio, 1);
		break;
	case DIAG_CURRENT_CH1:
		gpio_pin_set_dt(&config->sel0_gpio, 0);
		gpio_pin_set_dt(&config->sel1_gpio, 1);
		gpio_pin_set_dt(&config->sen_gpio, 1);
		break;
	case DIAG_TEMP:
		gpio_pin_set_dt(&config->sel0_gpio, 1);
		gpio_pin_set_dt(&config->sel1_gpio, 0);
		gpio_pin_set_dt(&config->sen_gpio, 1);
		break;
	case DIAG_VCC:
		gpio_pin_set_dt(&config->sel1_gpio, 1);
		gpio_pin_set_dt(&config->sel0_gpio, 1);
		gpio_pin_set_dt(&config->sen_gpio, 1);
		break;
	default:
		err = -ENOTSUP;
		goto cleanup;
	}

	/* Allow time for GPIO changes to settle and ADC input to stabilize */
	k_msleep(1);

	/* Initialize buffer before read */
	*voltage_mv = 0;
	err = adc_read_dt(&config->io_channels, &sequence);
	if (err) {
		LOG_ERR("ADC read failed: %d", err);
		goto cleanup;
	}

	LOG_DBG("ADC read completed, raw value: %d", *voltage_mv);
	err = adc_raw_to_millivolts_dt(&config->io_channels, voltage_mv);
	if (err) {
		LOG_ERR("ADC raw to millivolts conversion failed: %d", err);
		goto cleanup;
	}
	LOG_DBG("Raw reading %dmV", *voltage_mv);

cleanup:
	/* Deactivate sense enable to save power */
	gpio_pin_set_dt(&config->sen_gpio, 0);
	k_mutex_unlock(&data->lock);
	return err;
}

int vnd7050aj_read_load_current(const struct device *dev, uint8_t channel, int32_t *current_ma)
{
	const struct vnd7050aj_config *config = dev->config;
	int32_t sense_mv;
	int err;

	if (channel != VND7050AJ_CHANNEL_0 && channel != VND7050AJ_CHANNEL_1) {
		return -EINVAL;
	}

	enum vnd7050aj_diag_mode mode =
		(channel == VND7050AJ_CHANNEL_0) ? DIAG_CURRENT_CH0 : DIAG_CURRENT_CH1;

	err = vnd7050aj_read_sense_voltage(dev, mode, &sense_mv);
	if (err) {
		return err;
	}

	/* Formula according to datasheet: I_OUT = (V_SENSE / R_SENSE) * K_IL */
	/* To avoid floating point, we calculate in microamps and then convert to milliamps */
	int64_t current_ua = ((int64_t)sense_mv * 1000 * config->k_factor) / config->r_sense_ohms;
	*current_ma = (int32_t)(current_ua / 1000);

	return 0;
}

int vnd7050aj_read_chip_temp(const struct device *dev, int32_t *temp_c)
{
	const struct vnd7050aj_config *config = dev->config;
	int32_t sense_mv;
	int err;

	err = vnd7050aj_read_sense_voltage(dev, DIAG_TEMP, &sense_mv);
	if (err) {
		return err;
	}
	
	/* Calculate temperature difference in kelvin first to avoid overflow */
	int32_t voltage_diff = sense_mv - (int32_t)config->v_sense_0;
	int32_t temp_diff_kelvin = (voltage_diff * 1000) / (int32_t)config->k_tchip;
	
	*temp_c = config->t_sense_0 + temp_diff_kelvin;
	
	LOG_DBG("Voltage diff: %d mV, Temp diff: %d milli°C, Final temp: %d °C",
		voltage_diff, temp_diff_kelvin, *temp_c);

	return 0;
}

int vnd7050aj_read_supply_voltage(const struct device *dev, int32_t *voltage_mv)
{
	const struct vnd7050aj_config *config = dev->config;
	int32_t sense_mv;
	int err;

	err = vnd7050aj_read_sense_voltage(dev, DIAG_VCC, &sense_mv);
	if (err) {
		return err;
	}

	/* Formula from datasheet: VCC = V_SENSE * K_VCC */
	*voltage_mv = (sense_mv * config->k_vcc) / 1000;

	return 0;
}

int vnd7050aj_reset_fault(const struct device *dev)
{
	const struct vnd7050aj_config *config = dev->config;
	int err;

	/* Pulse the active-low fault reset pin */
	err = gpio_pin_set_dt(&config->fault_reset_gpio, 0);
	if (err) {
		return err;
	}
	k_msleep(1); /* Short pulse */
	err = gpio_pin_set_dt(&config->fault_reset_gpio, 1);

	return err;
}
