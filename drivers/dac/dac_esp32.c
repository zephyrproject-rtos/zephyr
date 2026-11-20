/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_dac

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/clock_control.h>
#include <hal/dac_ll.h>
#include <hal/dac_types.h>
#if defined(CONFIG_SOC_SERIES_ESP32S31)
#include <hal/dac_periph.h>
#include <hal/dac_types_private.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_ll.h>
#include <hal/rtc_io_periph.h>
#else
#include <hal/rtc_io_types.h>
#include <hal/rtc_io_hal.h>
#include <hal/rtc_io_ll.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_dac, CONFIG_DAC_LOG_LEVEL);

struct dac_esp32_config {
	int irq_source;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

#if defined(CONFIG_SOC_SERIES_ESP32S31)
static void dac_esp32_release_pad(uint8_t gpio_num)
{
	int rtcio_num;

	gpio_ll_input_disable(&GPIO, gpio_num);
	gpio_ll_output_disable(&GPIO, gpio_num);
	gpio_ll_pullup_dis(&GPIO, gpio_num);
	gpio_ll_pulldown_dis(&GPIO, gpio_num);
	gpio_ll_func_sel(&GPIO, gpio_num, PIN_FUNC_GPIO);

	rtcio_num = rtc_io_num_map[gpio_num];
	if (rtcio_num >= 0) {
		rtcio_ll_function_select(rtcio_num, RTCIO_LL_FUNC_DIGITAL);
		rtcio_ll_output_disable(rtcio_num);
		rtcio_ll_input_disable(rtcio_num);
		rtcio_ll_pullup_disable(rtcio_num);
		rtcio_ll_pulldown_disable(rtcio_num);
	}
}
#endif

static int dac_esp32_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_SOC_SERIES_ESP32S31)
	if (value >= (1U << SOC_DAC_RESOLUTION)) {
		LOG_ERR("Value %u is out of range", value);
		return -EINVAL;
	}

	dac_ll_pad_set_output_code((dac_channel_t)channel, (uint16_t)value);
#else
	dac_ll_update_output_value(channel, (uint8_t)value);
#endif

	return 0;
}

static int dac_esp32_channel_setup(const struct device *dev,
		   const struct dac_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id >= SOC_DAC_CHAN_NUM) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

#if defined(CONFIG_SOC_SERIES_ESP32S31)
	if (channel_cfg->resolution != SOC_DAC_RESOLUTION) {
		LOG_ERR("Resolution %u is not supported", channel_cfg->resolution);
		return -ENOTSUP;
	}

	dac_esp32_release_pad(dac_periph_signal.dac_channel_io_num[channel_cfg->channel_id]);
	dac_ll_pad_set_data_source((dac_channel_t)channel_cfg->channel_id, DAC_DATA_SOURCE_DIRECT);
	dac_ll_pad_enable_buffer((dac_channel_t)channel_cfg->channel_id, channel_cfg->buffered);
	dac_ll_pad_set_buffer_code_range((dac_channel_t)channel_cfg->channel_id, false);
	dac_ll_pad_power_on((dac_channel_t)channel_cfg->channel_id);
	dac_ll_cw_enable_dc();
#else
	dac_ll_power_on(channel_cfg->channel_id);
#endif

	return 0;
}

static int dac_esp32_init(const struct device *dev)
{
	const struct dac_esp32_config *cfg = dev->config;

	if (!cfg->clock_dev) {
		LOG_ERR("Clock device missing");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	if (clock_control_on(cfg->clock_dev, (clock_control_subsys_t)cfg->clock_subsys) != 0) {
		LOG_ERR("DAC clock setup failed (%d)", -EIO);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(dac, dac_esp32_driver_api) = {
	.channel_setup = dac_esp32_channel_setup,
	.write_value = dac_esp32_write_value
};

#define ESP32_DAC_INIT(id)									\
												\
	static const struct dac_esp32_config dac_esp32_config_##id = {				\
		.irq_source = DT_INST_IRQN(id),							\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),				\
		.clock_subsys =	(clock_control_subsys_t) DT_INST_CLOCKS_CELL(id, offset),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id,								\
		&dac_esp32_init,								\
		NULL,										\
		NULL,										\
		&dac_esp32_config_##id,								\
		POST_KERNEL,									\
		CONFIG_DAC_INIT_PRIORITY,							\
		&dac_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_DAC_INIT);
