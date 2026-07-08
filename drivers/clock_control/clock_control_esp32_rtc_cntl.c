/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Clock implementation for SoCs whose slow and CPU clocks are managed
 * through the RTC_CNTL peripheral: ESP32, ESP32-S2, ESP32-S3, ESP32-C2
 * and ESP32-C3.
 */

#include "clock_control_esp32_priv.h"

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static void esp32_enable_ext_slow_clk(uint8_t slow_clk)
{
#if defined(CONFIG_SOC_SERIES_ESP32C2)
	ARG_UNUSED(slow_clk);
	rtc_clk_32k_enable_external();
#else
	if (slow_clk == ESP32_RTC_SLOW_CLK_SRC_XTAL32K) {
		rtc_clk_32k_enable(true);
	} else if (slow_clk == ESP32_RTC_SLOW_CLK_32K_EXT_OSC) {
		rtc_clk_32k_enable_external();
	}
#endif
}

int esp32_select_rtc_slow_clk(uint8_t slow_clk)
{
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk & RTC_CNTL_ANA_CLK_RTC_SEL_V;
	const uint32_t cal_cycles = CONFIG_CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES;
	uint32_t cal_val = 0;
	/* number of times to repeat 32k XTAL calibration
	 * before giving up and switching to the internal RC
	 */
	int retry_32k_xtal = 3;

	/*
	 * RTC slow clock calibration uses TIMG0 to count XTAL cycles.
	 * Ensure TIMG0 bus clock is enabled before calibration.
	 */
	_timg_ll_enable_bus_clock(0, true);
	_timg_ll_reset_register(0);
#if defined(CONFIG_SOC_SERIES_ESP32C2)
#define ESP32_RTC_EXT_SLOW_CLK_SRC ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW
#define ESP32_RTC_EXT_CLK_CAL_SRC  CLK_CAL_32K_OSC_SLOW
#else
#define ESP32_RTC_EXT_SLOW_CLK_SRC ESP32_RTC_SLOW_CLK_SRC_XTAL32K
#define ESP32_RTC_EXT_CLK_CAL_SRC  CLK_CAL_32K_XTAL
#endif

	do {
		if (rtc_slow_clk_src == ESP32_RTC_EXT_SLOW_CLK_SRC) {
			/* External slow clock needs to be enabled and running before it
			 * can be used. Hardware doesn't have a direct way of checking if
			 * it is running, so rtc_clk_cal is used to count cycles over a
			 * given window. If the clock has not started up, calibration
			 * times out and returns 0.
			 */
			LOG_DBG("waiting for external slow clock to start up");
			esp32_enable_ext_slow_clk(slow_clk);
			/* When CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES is set to 0, clock
			 * calibration will not be performed at startup.
			 */
			if (cal_cycles > 0) {
				cal_val = rtc_clk_cal(ESP32_RTC_EXT_CLK_CAL_SRC, cal_cycles);
				if (cal_val != 0) {
					/* calibration succeeded */
				} else if (retry_32k_xtal > 0) {
					retry_32k_xtal--;
					continue;
				} else {
					LOG_ERR("32 kHz XTAL not found");
					return -ENODEV;
				}
			}
		} else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256) {
			rtc_clk_8m_enable(true, true);
		} else {
			/* no action for other slow clock sources */
		}
		rtc_clk_slow_src_set(rtc_slow_clk_src);

		if (cal_cycles > 0) {
			cal_val = rtc_clk_cal(CLK_CAL_RTC_SLOW, cal_cycles);
		} else {
			const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;

			cal_val = (uint32_t)(cal_dividend / rtc_clk_slow_freq_get_hz());
		}
	} while (cal_val == 0);

	LOG_DBG("RTC_SLOW_CLK calibration value: %d", cal_val);

	esp_clk_slowclk_cal_set(cal_val);

	return 0;
}

#undef ESP32_RTC_EXT_SLOW_CLK_SRC
#undef ESP32_RTC_EXT_CLK_CAL_SRC

static bool clock_init_done;

static void esp32_cpu_clock_init(const struct esp32_cpu_clock_config *cpu_cfg)
{
	rtc_clk_config_t rtc_clk_cfg = RTC_CLK_CONFIG_DEFAULT();

	rtc_clk_cfg.xtal_freq = cpu_cfg->xtal_freq;
	rtc_clk_cfg.cpu_freq_mhz = cpu_cfg->cpu_freq;

	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DFREQ, rtc_clk_cfg.clk_8m_dfreq);

#if defined(CONFIG_SOC_SERIES_ESP32)
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, rtc_clk_cfg.clk_8m_div - 1);
#else
	rtc_clk_divider_set(rtc_clk_cfg.clk_rtc_clk_div);
	rtc_clk_8m_divider_set(rtc_clk_cfg.clk_8m_clk_div);
#endif

	regi2c_ctrl_ll_i2c_reset();
	regi2c_ctrl_ll_i2c_bbpll_enable();

#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32)
	regi2c_ctrl_ll_i2c_apll_enable();
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32S2)
	rtc_clk_xtal_freq_update(rtc_clk_cfg.xtal_freq);
#endif
	rtc_clk_apb_freq_update(rtc_clk_cfg.xtal_freq * MHZ(1));
}

int esp32_cpu_clock_configure(const struct esp32_cpu_clock_config *cpu_cfg)
{
	rtc_cpu_freq_config_t old_config;
	rtc_cpu_freq_config_t new_config;
	bool ret;

	if (!clock_init_done) {
		esp32_cpu_clock_init(cpu_cfg);
		clock_init_done = true;
	}

	esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);

#if defined(CONFIG_SOC_SERIES_ESP32C2)
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_XTAL) {
		uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_XTAL);
		uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
				     CONFIG_ESP_CONSOLE_UART_BAUDRATE, cpu_cfg->xtal_freq * MHZ(1));
		esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
	}
#endif

	rtc_clk_cpu_freq_get_config(&old_config);

	ret = rtc_clk_cpu_freq_mhz_to_config(cpu_cfg->cpu_freq, &new_config);
	if (!ret || (new_config.source != cpu_cfg->clk_src)) {
		LOG_ERR("invalid CPU frequency value");
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	rtc_clk_cpu_freq_set_config(&new_config);

	esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * cpu_cfg->cpu_freq /
				old_config.freq_mhz);
	irq_unlock(key);

#if defined(CONFIG_ESP_CONSOLE_UART)
#if defined(CONFIG_SOC_SERIES_ESP32C2)
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_PLL) {
		uint32_t uart_sclk_freq;

		esp_clk_tree_src_get_freq_hz(
			UART_SCLK_DEFAULT, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &uart_sclk_freq);
		uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_DEFAULT);
		uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
				     CONFIG_ESP_CONSOLE_UART_BAUDRATE, uart_sclk_freq);
	}
#else
#if defined(CONFIG_MCUBOOT) && defined(ESP_ROM_UART_CLK_IS_XTAL)
	uint32_t uart_clock_src_hz = (uint32_t)rtc_clk_xtal_freq_get() * MHZ(1);
#else
	uint32_t uart_clock_src_hz = esp_clk_apb_freq();
#endif
	uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
			     CONFIG_ESP_CONSOLE_UART_BAUDRATE, uart_clock_src_hz);
#endif
#endif
	return 0;
}
