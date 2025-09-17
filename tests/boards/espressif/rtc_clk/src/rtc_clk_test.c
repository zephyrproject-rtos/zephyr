/*
 * Copyright (c) 2024-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>
#include <zephyr/drivers/clock_control.h>

#if defined(CONFIG_SOC_SERIES_ESP32)
#define DT_CPU_COMPAT espressif_xtensa_lx6
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#elif defined(CONFIG_RISCV)
#define DT_CPU_COMPAT espressif_riscv
#endif

static const struct device *const clk_dev = DEVICE_DT_GET_ONE(espressif_esp32_clock);

static void *rtc_clk_setup(void)
{
	zassert_true(device_is_ready(clk_dev), "CLK device is not ready");
	uint32_t rate = 0;
	int ret = 0;

	ret = clock_control_get_rate(clk_dev,
				     (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_CPU, &rate);
	zassert_false(ret, "Failed to get CPU clock rate");
	TC_PRINT("CPU frequency: %d\n", rate);

	ret = clock_control_get_rate(
		clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST, &rate);
	zassert_false(ret, "Failed to get RTC_FAST clock rate");
	TC_PRINT("RTC_FAST frequency: %d\n", rate);

	ret = clock_control_get_rate(
		clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW, &rate);
	zassert_false(ret, "Failed to get RTC_SLOW clock rate");
	TC_PRINT("RTC_SLOW frequency: %d\n", rate);

	return NULL;
}

ZTEST(rtc_clk, test_cpu_xtal_src)
{
	struct esp32_clock_config clk_cfg = {0};
	int ret = 0;
	uint32_t cpu_rate = 0;

	clk_cfg.cpu.clk_src = ESP32_CPU_CLK_SRC_XTAL;
	clk_cfg.cpu.xtal_freq = (DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / MHZ(1));

	for (uint8_t i = 0; i < 4; i++) {
		clk_cfg.cpu.cpu_freq = clk_cfg.cpu.xtal_freq >> i;

		TC_PRINT("Testing CPU frequency: %d MHz\n", clk_cfg.cpu.cpu_freq);

		ret = clock_control_configure(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_CPU, &clk_cfg);
		zassert_false(ret, "Failed to set CPU clock source");

		ret = clock_control_get_rate(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_CPU, &cpu_rate);
		zassert_false(ret, "Failed to get CPU clock rate");
		zassert_equal(cpu_rate, clk_cfg.cpu.cpu_freq * MHZ(1),
			      "CPU clock rate is not equal to XTAL frequency (%d != %d)", cpu_rate,
			      clk_cfg.cpu.cpu_freq);
	}
}

uint32_t rtc_pll_src_freq_mhz[] = {
#if defined(ESP32_CLK_CPU_PLL_48M)
	ESP32_CLK_CPU_PLL_48M,
#endif
#if defined(ESP32_CLK_CPU_PLL_80M)
	ESP32_CLK_CPU_PLL_80M,
#endif
#if defined(ESP32_CLK_CPU_PLL_96M)
	ESP32_CLK_CPU_PLL_96M,
#endif
#if defined(ESP32_CLK_CPU_PLL_120M)
	ESP32_CLK_CPU_PLL_120M,
#endif
#if defined(ESP32_CLK_CPU_PLL_160M)
	ESP32_CLK_CPU_PLL_160M,
#endif
#if defined(ESP32_CLK_CPU_PLL_240M)
	ESP32_CLK_CPU_PLL_240M,
#endif
};

ZTEST(rtc_clk, test_cpu_pll_src)
{
	struct esp32_clock_config clk_cfg = {0};
	int ret = 0;
	uint32_t cpu_rate = 0;

	clk_cfg.cpu.clk_src = ESP32_CPU_CLK_SRC_PLL;
	clk_cfg.cpu.xtal_freq = (DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / MHZ(1));

	for (uint8_t i = 0; i < ARRAY_SIZE(rtc_pll_src_freq_mhz); i++) {
		clk_cfg.cpu.cpu_freq = rtc_pll_src_freq_mhz[i] / MHZ(1);

		TC_PRINT("Testing CPU frequency: %d MHz\n", clk_cfg.cpu.cpu_freq);

		ret = clock_control_configure(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_CPU, &clk_cfg);
		zassert_false(ret, "Failed to set CPU clock source");

		ret = clock_control_get_rate(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_CPU, &cpu_rate);
		zassert_false(ret, "Failed to get CPU clock rate");
		zassert_equal(cpu_rate, clk_cfg.cpu.cpu_freq * MHZ(1),
			      "CPU clock rate is not equal to configured frequency (%d != %d)",
			      cpu_rate, clk_cfg.cpu.cpu_freq);
	}
}

uint32_t rtc_rtc_fast_clk_src[] = {
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32S2)
	ESP32_RTC_FAST_CLK_SRC_XTAL_D4,
#else
	ESP32_RTC_FAST_CLK_SRC_XTAL_D2,
#endif
	ESP32_RTC_FAST_CLK_SRC_RC_FAST};

uint32_t rtc_rtc_fast_clk_src_freq_mhz[] = {
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32S2)
	DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / 4,
#else
	DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / 2,
#endif
	ESP32_CLK_CPU_RC_FAST_FREQ
};

ZTEST(rtc_clk, test_rtc_fast_src)
{
	struct esp32_clock_config clk_cfg = {0};
	int ret = 0;
	uint32_t cpu_rate = 0;

	clk_cfg.cpu.xtal_freq = (DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / MHZ(1));

	for (uint8_t i = 0; i < ARRAY_SIZE(rtc_rtc_fast_clk_src); i++) {
		clk_cfg.rtc.rtc_fast_clock_src = rtc_rtc_fast_clk_src[i];

		TC_PRINT("Testing RTC FAST CLK freq: %d MHz\n", rtc_rtc_fast_clk_src_freq_mhz[i]);

		ret = clock_control_configure(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST,
			&clk_cfg);
		zassert_false(ret, "Failed to set CPU clock source");

		ret = clock_control_get_rate(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_FAST,
			&cpu_rate);
		zassert_false(ret, "Failed to get RTC_FAST clock rate");
		zassert_equal(cpu_rate, rtc_rtc_fast_clk_src_freq_mhz[i],
			      "CPU clock rate is not equal to configured frequency (%d != %d)",
			      cpu_rate, rtc_rtc_fast_clk_src_freq_mhz[i]);
	}
}

uint32_t rtc_rtc_slow_clk_src[] = {
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC_SLOW)
	ESP32_RTC_SLOW_CLK_SRC_RC_SLOW,
#endif
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC32K)
	ESP32_RTC_SLOW_CLK_SRC_RC32K,
#endif
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256)
	ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256,
#endif
#if CONFIG_FIXTURE_XTAL
	ESP32_RTC_SLOW_CLK_SRC_XTAL32K,
#endif
};

uint32_t rtc_rtc_slow_clk_src_freq[] = {
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ)
	ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ,
#endif
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC32K_FREQ)
	ESP32_RTC_SLOW_CLK_SRC_RC32K_FREQ,
#endif
#if defined(ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ)
	ESP32_RTC_SLOW_CLK_SRC_RC_FAST_D256_FREQ,
#endif
#if CONFIG_FIXTURE_XTAL
	ESP32_RTC_SLOW_CLK_SRC_XTAL32K_FREQ,
#endif
};

ZTEST(rtc_clk, test_rtc_slow_src)
{
	struct esp32_clock_config clk_cfg = {0};
	int ret = 0;
	uint32_t cpu_rate = 0;

	clk_cfg.cpu.xtal_freq = (DT_PROP(DT_INST(0, DT_CPU_COMPAT), xtal_freq) / MHZ(1));

	for (uint8_t i = 0; i < ARRAY_SIZE(rtc_rtc_slow_clk_src); i++) {
		clk_cfg.rtc.rtc_slow_clock_src = rtc_rtc_slow_clk_src[i];

		TC_PRINT("Testing RTC SLOW CLK freq: %d MHz\n", rtc_rtc_slow_clk_src_freq[i]);

		ret = clock_control_configure(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
			&clk_cfg);
		zassert_false(ret, "Failed to set CPU clock source");

		ret = clock_control_get_rate(
			clk_dev, (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
			&cpu_rate);
		zassert_false(ret, "Failed to get RTC_SLOW clock rate");
		zassert_equal(cpu_rate, rtc_rtc_slow_clk_src_freq[i],
			      "CPU clock rate is not equal to configured frequency (%d != %d)",
			      cpu_rate, rtc_rtc_slow_clk_src_freq[i]);
	}
}

ZTEST_SUITE(rtc_clk, NULL, rtc_clk_setup, NULL, NULL, NULL);
