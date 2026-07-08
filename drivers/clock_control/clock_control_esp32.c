/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_clock

#include "clock_control_esp32_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static enum clock_control_status clock_control_esp32_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	int module_id = (int)sys;

	if (module_id >= ESP32_MODULE_MAX) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

#if defined(CONFIG_SOC_SERIES_ESP32P4)
	return CLOCK_CONTROL_STATUS_UNKNOWN;
#else
	uint32_t clk_en_reg = periph_ll_get_clk_en_reg((shared_periph_module_t)module_id);
	uint32_t clk_en_mask = periph_ll_get_clk_en_mask((shared_periph_module_t)module_id);

	if (clk_en_reg == 0) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (DPORT_GET_PERI_REG_MASK(clk_en_reg, clk_en_mask)) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
#endif
}

static int clock_control_esp32_on(const struct device *dev, clock_control_subsys_t sys)
{
	enum clock_control_status status;

	status = clock_control_esp32_get_status(dev, sys);
	if (status == CLOCK_CONTROL_STATUS_ON) {
		return 0;
	}

	int module_id = (int)sys;

	if (module_id < ESP32_MODULE_MAX) {
#if !defined(CONFIG_SOC_SERIES_ESP32P4)
		periph_module_enable((shared_periph_module_t)module_id);
#endif
	} else {
		non_shared_periph_module_enable(module_id);
	}

	return 0;
}

static int clock_control_esp32_off(const struct device *dev, clock_control_subsys_t sys)
{
	if (clock_control_esp32_get_status(dev, sys) == CLOCK_CONTROL_STATUS_OFF) {
		return 0;
	}

	int module_id = (int)sys;

	if (module_id < ESP32_MODULE_MAX) {
#if !defined(CONFIG_SOC_SERIES_ESP32P4)
		periph_module_disable((shared_periph_module_t)module_id);
#endif
	} else {
		non_shared_periph_module_disable(module_id);
	}

	return 0;
}

static int clock_control_esp32_get_rate(const struct device *dev, clock_control_subsys_t sys,
					uint32_t *rate)
{
	ARG_UNUSED(dev);

	switch ((int)sys) {
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST:
		*rate = esp_clk_tree_lp_fast_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW:
		*rate = esp_clk_tree_lp_slow_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST_NOMINAL:
		*rate = esp_clk_tree_lp_fast_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW_NOMINAL:
		*rate = esp_clk_tree_lp_slow_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX);
		break;
	default:
		*rate = clk_hal_cpu_get_freq_hz();
	}

	return 0;
}

static int clock_control_esp32_configure(const struct device *dev, clock_control_subsys_t sys,
					 void *data)
{
	struct esp32_clock_config *new_cfg = data;
	int ret = 0;

	switch ((int)sys) {
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST:
		rtc_clk_fast_src_set(new_cfg->rtc.rtc_fast_clock_src);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW:
		ret = esp32_select_rtc_slow_clk(new_cfg->rtc.rtc_slow_clock_src);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_CPU:
		/* Normalize frequency */
		new_cfg->cpu.xtal_freq = new_cfg->cpu.xtal_freq > MHZ(1)
						 ? new_cfg->cpu.xtal_freq / MHZ(1)
						 : new_cfg->cpu.xtal_freq;
		new_cfg->cpu.cpu_freq = new_cfg->cpu.cpu_freq > MHZ(1)
						? new_cfg->cpu.cpu_freq / MHZ(1)
						: new_cfg->cpu.cpu_freq;
		ret = esp32_cpu_clock_configure(&new_cfg->cpu);
		break;
	default:
		LOG_ERR("Unsupported subsystem %d", (int)sys);
		return -EINVAL;
	}
	return ret;
}

static int clock_control_esp32_init(const struct device *dev)
{
#if !defined(CONFIG_SOC_ESP32_APPCPU) && !defined(CONFIG_SOC_ESP32S3_APPCPU)
	const struct esp32_clock_config *cfg = dev->config;
	int ret;

	esp_rtc_init();

	ret = esp32_cpu_clock_configure(&cfg->cpu);
	if (ret) {
		LOG_ERR("Failed to configure CPU clock");
		return ret;
	}

	rtc_clk_fast_src_set(cfg->rtc.rtc_fast_clock_src);

	ret = esp32_select_rtc_slow_clk(cfg->rtc.rtc_slow_clock_src);
	if (ret) {
		LOG_ERR("Failed to configure RTC clock");
		return ret;
	}

	esp_perip_clk_init();

	/*
	 * esp_perip_clk_init() gates TIMG0 bus clock. Re-enable it
	 * for the RTC calibration circuit used by rtc_clk_cal().
	 */
	_timg_ll_enable_bus_clock(0, true);
	_timg_ll_reset_register(0);
#endif

	return 0;
}

static DEVICE_API(clock_control, clock_control_esp32_api) = {
	.on = clock_control_esp32_on,
	.off = clock_control_esp32_off,
	.get_rate = clock_control_esp32_get_rate,
	.get_status = clock_control_esp32_get_status,
	.configure = clock_control_esp32_configure,
};

static const struct esp32_cpu_clock_config esp32_cpu_clock_config0 = {
	.clk_src = DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_source),
	.cpu_freq = (DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_frequency) / MHZ(1)),
	.xtal_freq = ((DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq)) / MHZ(1)),
};

static const struct esp32_rtc_clock_config esp32_rtc_clock_config0 = {
	.rtc_fast_clock_src = DT_PROP(DT_INST(0, espressif_esp32_clock), fast_clk_src),
	.rtc_slow_clock_src = DT_PROP(DT_INST(0, espressif_esp32_clock), slow_clk_src),
};

static const struct esp32_clock_config esp32_clock_config0 = {
	.cpu = esp32_cpu_clock_config0,
	.rtc = esp32_rtc_clock_config0
};

DEVICE_DT_DEFINE(DT_NODELABEL(clock),
		 clock_control_esp32_init,
		 NULL,
		 NULL,
		 &esp32_clock_config0,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_esp32_api);
