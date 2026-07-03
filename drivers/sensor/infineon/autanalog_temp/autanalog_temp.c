/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_autanalog_die_temp

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>
#include <cy_autanalog_sar.h>
#include <cy_autanalog_ac.h>

LOG_MODULE_REGISTER(autanalog_die_temp, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Zephyr channel IDs at or above this offset map to AutAnalog SAR MUX channels.
 * MUX channel index = channel_id - IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET.
 * Matches the offset used by the AutAnalog SAR ADC driver.
 */
#define IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET 8U

/* The AutAnalog SAR ADC only supports a fixed 12-bit sequence resolution */
#define IFX_AUTANALOG_SAR_SEQ_RESOLUTION 12U

struct autanalog_die_temp_config {
	const struct device *adc;
	const struct device *mfd;
	struct adc_channel_cfg ch_cfg;
	uint8_t channel_id;
	uint8_t sar_index;
	uint8_t sar_sequencer;
	uint8_t sar_mux_channel;
	bool low_power;
};

struct autanalog_die_temp_data {
	uint32_t raw;
	struct adc_sequence seq;
	int16_t temp_degc;
};

static int autanalog_die_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct autanalog_die_temp_config *config = dev->config;
	struct autanalog_die_temp_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/*
	 * The Autonomous Controller must be running to drive the conversions
	 * consumed by adc_read().  The parent autanalog node defers the AC start
	 * (ac-app-managed, enforced at build time), so it is not started by
	 * the MFD at boot. This lets the application to configure all of its
	 * ADC channels before the AC begins sampling.
	 */
	cy_stc_autanalog_state_t ac_state;

	Cy_AutAnalog_GetControllerState(&ac_state);
	if (ac_state.ac.state == 0u) {
		ret = ifx_autanalog_start_autonomous_control(config->mfd);
		if (ret) {
			LOG_ERR("Failed to start Autonomous Controller (%d)", ret);
			return ret;
		}
	}

	ret = adc_read(config->adc, &data->seq);
	if (ret) {
		LOG_ERR("ADC read failed (%d)", ret);
		return ret;
	}

	/*
	 * Convert the raw accumulated SAR counts to degrees Celsius.  VBGR must
	 * be used as the SAR reference for accurate die temperature conversion
	 * (enforced by the channel configuration in devicetree).
	 */
	data->temp_degc = Cy_AutAnalog_SAR_CountsTo_degreeC(
		config->sar_index, config->low_power, config->sar_sequencer,
		config->sar_mux_channel, (int32_t)data->raw);

	return 0;
}

static int autanalog_die_temp_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	const struct autanalog_die_temp_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->temp_degc;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, autanalog_die_temp_api) = {
	.sample_fetch = autanalog_die_temp_sample_fetch,
	.channel_get = autanalog_die_temp_channel_get,
};

static int autanalog_die_temp_init(const struct device *dev)
{
	const struct autanalog_die_temp_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device %s not ready", config->adc->name);
		return -ENODEV;
	}

	ret = adc_channel_setup(config->adc, &config->ch_cfg);
	if (ret) {
		LOG_ERR("ADC channel setup failed (%d)", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_INFINEON_AUTANALOG_TEMP_VREF_TRIM_WORKAROUND)) {
		INFRA->AREF.VREF_TRIM1 = 0U;
		INFRA->AREF.VREF_TRIM2 = 0U;
	}
	return 0;
}

/* clang-format off */

#define AUTANALOG_DIE_TEMP_INIT(inst)                                                          \
	BUILD_ASSERT(DT_PROP(DT_PARENT(DT_INST_IO_CHANNELS_CTLR(inst)), ac_app_managed),           \
		     "infineon,autanalog-die-temp requires the parent autanalog node to set "      \
		     "ac-app-managed; the sensor starts the Autonomous Controller itself.");       \
                                                                                               \
	static struct autanalog_die_temp_data autanalog_die_temp_data_##inst;                      \
                                                                                               \
	static const struct autanalog_die_temp_config autanalog_die_temp_config_##inst = {         \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.mfd = DEVICE_DT_GET(DT_PARENT(DT_INST_IO_CHANNELS_CTLR(inst))),                   \
		.ch_cfg = ADC_CHANNEL_CFG_DT(                                                      \
			DT_CHILD(DT_INST_IO_CHANNELS_CTLR(inst),                                   \
				 UTIL_CAT(channel_, DT_INST_IO_CHANNELS_INPUT(inst)))),            \
		.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                                     \
		.sar_index = DT_INST_PROP(inst, sar_index),                                        \
		.sar_sequencer = DT_INST_PROP(inst, sar_sequencer),                                \
		.sar_mux_channel =                                                                 \
			DT_INST_IO_CHANNELS_INPUT(inst) - IFX_AUTANALOG_SAR_MUX_CHANNEL_OFFSET,    \
		.low_power = DT_PROP(DT_INST_IO_CHANNELS_CTLR(inst), lp_mode),                     \
	};                                                                                         \
                                                                                               \
	static int autanalog_die_temp_init_##inst(const struct device *dev)                        \
	{                                                                                          \
		struct autanalog_die_temp_data *data = dev->data;                                  \
                                                                                               \
		data->seq.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst));                         \
		data->seq.buffer = &data->raw;                                                     \
		data->seq.buffer_size = sizeof(data->raw);                                         \
		data->seq.resolution = IFX_AUTANALOG_SAR_SEQ_RESOLUTION;                           \
                                                                                               \
		return autanalog_die_temp_init(dev);                                               \
	}                                                                                          \
                                                                                               \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, autanalog_die_temp_init_##inst, NULL,                   \
				     &autanalog_die_temp_data_##inst,                              \
				     &autanalog_die_temp_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &autanalog_die_temp_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(AUTANALOG_DIE_TEMP_INIT)
