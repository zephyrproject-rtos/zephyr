/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <esp_rom_sys.h>
#include <soc/rtc.h>
#include <soc/dport_access.h>
#if !defined(CONFIG_SOC_SERIES_ESP32C6) && !defined(CONFIG_SOC_SERIES_ESP32H2)
#include <soc/rtc_cntl_reg.h>
#endif
#include <hal/clk_gate_ll.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/esp_clk.h>
#include <hal/clk_tree_hal.h>
#include <esp_private/esp_clk_tree_common.h>

#include <zephyr/logging/log.h>

#include "clock_control_esp32_common.h"

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_ESP32)
#define DT_CPU_COMPAT espressif_xtensa_lx6
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#else
#define DT_CPU_COMPAT espressif_riscv
#endif

static bool reset_reason_is_cpu_reset(void)
{
	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	if ((rst_reason == RESET_REASON_CPU0_MWDT0) || (rst_reason == RESET_REASON_CPU0_SW) ||
	    (rst_reason == RESET_REASON_CPU0_RTC_WDT)
#if !defined(CONFIG_SOC_SERIES_ESP32) && !defined(CONFIG_SOC_SERIES_ESP32C2)
	    || (rst_reason == RESET_REASON_CPU0_MWDT1)
#endif
	) {
		return true;
	}
	return false;
}

static enum clock_control_status clock_control_esp32_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t clk_en_reg = periph_ll_get_clk_en_reg((periph_module_t)sys);
	uint32_t clk_en_mask = periph_ll_get_clk_en_mask((periph_module_t)sys);

	if (DPORT_GET_PERI_REG_MASK(clk_en_reg, clk_en_mask)) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_esp32_on(const struct device *dev, clock_control_subsys_t sys)
{
	enum clock_control_status status = clock_control_esp32_get_status(dev, sys);

	if (status == CLOCK_CONTROL_STATUS_ON && !reset_reason_is_cpu_reset()) {
		return -EALREADY;
	}

	periph_module_enable((periph_module_t)sys);

	return 0;
}

static int clock_control_esp32_off(const struct device *dev, clock_control_subsys_t sys)
{
	enum clock_control_status status = clock_control_esp32_get_status(dev, sys);

	if (status == CLOCK_CONTROL_STATUS_ON) {
		periph_module_disable((periph_module_t)sys);
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

static void esp32_rtc_clk_slow_src_enable(uint8_t slow_clk, soc_rtc_slow_clk_src_t rtc_slow_clk_src)
{
#if defined(CONFIG_SOC_SERIES_ESP32C2)
	if (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW) {
		rtc_clk_32k_enable_external();
	}
#else
	if (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_XTAL32K) {
		if (slow_clk == ESP32_RTC_SLOW_CLK_SRC_XTAL32K) {
			rtc_clk_32k_enable(true);
		} else {
			rtc_clk_32k_enable_external();
		}
	}
#endif
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32H2)
	else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K) {
		rtc_clk_rc32k_enable(true);
	}
#else
	else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256) {
		rtc_clk_8m_enable(true, true);
	}
#endif
	else {
		/* SOC_RTC_SLOW_CLK_SRC_RC_SLOW: nothing to do */
	}
}

static int esp32_calibrate_rtc_xtal(uint32_t cal_clk, int *retry)
{
	uint32_t cal_val;

	if (CONFIG_RTC_CLK_CAL_CYCLES == 0) {
		return 0;
	}

	cal_val = rtc_clk_cal(cal_clk, CONFIG_RTC_CLK_CAL_CYCLES);
	if (cal_val != 0) {
		return 0;
	}

	if ((*retry)-- > 0) {
		return -EAGAIN;
	}

	LOG_ERR("32 kHz XTAL not found");
	return -ENODEV;
}

static int esp32_select_rtc_slow_clk(uint8_t slow_clk)
{
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32H2)
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk;
#else
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk & RTC_CNTL_ANA_CLK_RTC_SEL_V;
#endif
	uint32_t cal_val = 0;
	int retry_32k_xtal = 3;
	int ret;
#if defined(CONFIG_SOC_SERIES_ESP32C2)
	bool is_xtal32k = (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW);
	uint32_t cal_clk = RTC_CAL_32K_OSC_SLOW;
#else
	bool is_xtal32k = (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_XTAL32K);
	uint32_t cal_clk = RTC_CAL_32K_XTAL;
#endif

	do {
		esp32_rtc_clk_slow_src_enable(slow_clk, rtc_slow_clk_src);

		if (is_xtal32k) {
			LOG_DBG("waiting for 32k oscillator to start up");
			ret = esp32_calibrate_rtc_xtal(cal_clk, &retry_32k_xtal);
			if (ret == -EAGAIN) {
				continue;
			}
			if (ret != 0) {
				return ret;
			}
		}

		rtc_clk_slow_src_set(rtc_slow_clk_src);

		if (CONFIG_RTC_CLK_CAL_CYCLES > 0) {
			cal_val = rtc_clk_cal(RTC_CAL_RTC_MUX, CONFIG_RTC_CLK_CAL_CYCLES);
		} else {
			const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;

			cal_val = (uint32_t)(cal_dividend / rtc_clk_slow_freq_get_hz());
		}
	} while (cal_val == 0);

	LOG_DBG("RTC_SLOW_CLK calibration value: %d", cal_val);
	esp_clk_slowclk_cal_set(cal_val);

	return 0;
}

static int clock_control_esp32_configure(const struct device *dev, clock_control_subsys_t sys,
					 void *data)
{
	struct esp32_clock_config *new_cfg = data;
	int ret = 0;

	ARG_UNUSED(dev);

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
	const struct esp32_clock_config *cfg = dev->config;
	int ret;

	ret = esp32_clock_early_init();
	if (ret) {
		LOG_ERR("Failed early clock init");
		return ret;
	}

	ret = esp32_cpu_clock_configure(&cfg->cpu);
	if (ret) {
		LOG_ERR("Failed to configure CPU clock");
		return ret;
	}

#if !defined(CONFIG_SOC_ESP32_APPCPU) && !defined(CONFIG_SOC_ESP32S3_APPCPU)
	rtc_clk_fast_src_set(cfg->rtc.rtc_fast_clock_src);

	ret = esp32_select_rtc_slow_clk(cfg->rtc.rtc_slow_clock_src);
	if (ret) {
		LOG_ERR("Failed to configure RTC clock");
		return ret;
	}

	esp32_clock_peripheral_init();
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

static const struct esp32_clock_config esp32_clock_config0 = {.cpu = esp32_cpu_clock_config0,
							      .rtc = esp32_rtc_clock_config0};

DEVICE_DT_DEFINE(DT_NODELABEL(clock), clock_control_esp32_init, NULL, NULL, &esp32_clock_config0,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_esp32_api);
