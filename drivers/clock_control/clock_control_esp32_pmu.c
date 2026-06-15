/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Clock implementation for SoCs whose slow and CPU clocks are managed
 * through the PMU and LP_CLKRST peripherals: ESP32-C5, ESP32-C6,
 * ESP32-H2 and ESP32-P4.
 */

#include "clock_control_esp32_priv.h"

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

int esp32_select_rtc_slow_clk(uint8_t slow_clk)
{
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk;
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
	do {
		if (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_XTAL32K) {
			/* 32k XTAL oscillator needs to be enabled and running before it can
			 * be used. Hardware doesn't have a direct way of checking if the
			 * oscillator is running. Here we use rtc_clk_cal function to count
			 * the number of main XTAL cycles in the given number of 32k XTAL
			 * oscillator cycles. If the 32k XTAL has not started up, calibration
			 * will time out, returning 0.
			 */
			LOG_DBG("waiting for 32k oscillator to start up");
			if (slow_clk == ESP32_RTC_SLOW_CLK_SRC_XTAL32K) {
				rtc_clk_32k_enable(true);
			}
#if !defined(CONFIG_SOC_SERIES_ESP32P4)
			else if (slow_clk == ESP32_RTC_SLOW_CLK_32K_EXT_OSC) {
				rtc_clk_32k_enable_external();
			} else {
			}
#endif
			/* When CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES is set to 0, clock
			 * calibration will not be performed at startup.
			 */
			if (CONFIG_CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES > 0) {
				cal_val =
					rtc_clk_cal(CLK_CAL_32K_XTAL,
						    CONFIG_CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES);
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
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32H2) ||                    \
	defined(CONFIG_SOC_SERIES_ESP32P4)
		} else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K) {
			rtc_clk_rc32k_enable(true);
		} else {
			/* no action for other slow clock sources */
		}
#else
		}
#endif
		rtc_clk_slow_src_set(rtc_slow_clk_src);

		/*
		 * The source enums differ per SoC (ESP32-C5 has no RC32K,
		 * ESP32-P4 has no OSC_SLOW), so each term is computed under the
		 * SoC guard where its enum exists.
		 */
		bool xpd_xtal32k = (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_XTAL32K);
		bool xpd_rc32k = false;

#if !defined(CONFIG_SOC_SERIES_ESP32P4)
		xpd_xtal32k |= (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_OSC_SLOW);
#endif
#if !defined(CONFIG_SOC_SERIES_ESP32C5)
		xpd_rc32k = (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K);
#endif

		pmu_lp_power_t lp_clk_power = {
			.xpd_xtal32k = xpd_xtal32k,
			.xpd_rc32k = xpd_rc32k,
			.xpd_fosc = 1,
			.pd_osc = 0,
		};
		pmu_ll_lp_set_clk_power(&PMU, PMU_MODE_LP_ACTIVE, lp_clk_power.val);

		if (CONFIG_CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES > 0) {
			cal_val = rtc_clk_cal(CLK_CAL_RTC_SLOW,
					      CONFIG_CLOCK_CONTROL_ESP32_RTC_CLK_CAL_CYCLES);
		} else {
			const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;

			cal_val = (uint32_t)(cal_dividend / rtc_clk_slow_freq_get_hz());
		}
	} while (cal_val == 0);

	LOG_DBG("RTC_SLOW_CLK calibration value: %d", cal_val);

	esp_clk_slowclk_cal_set(cal_val);

	return 0;
}

static bool clock_init_done;

static void esp32_cpu_clock_init(const struct esp32_cpu_clock_config *cpu_cfg)
{
	rtc_clk_config_t rtc_clk_cfg = RTC_CLK_CONFIG_DEFAULT();

	rtc_clk_cfg.xtal_freq = cpu_cfg->xtal_freq;
	rtc_clk_cfg.cpu_freq_mhz = cpu_cfg->cpu_freq;

	_regi2c_ctrl_ll_master_enable_clock(true);

	/* clang-format off */
#if defined(CONFIG_SOC_SERIES_ESP32C5) || defined(CONFIG_SOC_SERIES_ESP32C6)
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_ENIF_RTC_DREG, 1);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_ENIF_DIG_DREG, 1);
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_FORCE_RTC_DREG, 1);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_FORCE_DIG_DREG, 1);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_XPD_RTC_REG, 0);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_XPD_DIG_REG, 0);
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_OC_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_EN_I2C_RTC_DREG, 0);
	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_EN_I2C_DIG_DREG, 0);
#endif
	/* clang-format on */

	REG_SET_FIELD(LP_CLKRST_FOSC_CNTL_REG, LP_CLKRST_FOSC_DFREQ, rtc_clk_cfg.clk_8m_dfreq);
#if !defined(CONFIG_SOC_SERIES_ESP32C5)
	REG_SET_FIELD(LP_CLKRST_RC32K_CNTL_REG, LP_CLKRST_RC32K_DFREQ, rtc_clk_cfg.rc32k_dfreq);
#endif

	/*
	 * When PVT is enabled, esp_rtc_init() -> pmu_init() already
	 * hands DBIAS control to PVT (DBIAS_SEL=0) and enables dynamic
	 * tracking. Taking it back to PMU here stops PVT from updating
	 * the RO HP/LP DBIAS_VOL fields, causing pvt_func_enable(false)
	 * at sleep entry to read stale values (0) and write an invalid
	 * regulator bias which brown-outs the core.
	 */
#if !defined(CONFIG_ESP_ENABLE_PVT)
	uint32_t hp_cali_dbias = get_act_hp_dbias();
	uint32_t lp_cali_dbias = get_act_lp_dbias();

#if !defined(CONFIG_SOC_SERIES_ESP32H2)
	SET_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_REGULATOR0_DBIAS_SEL);
#endif

	SET_PERI_REG_BITS(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS,
			  hp_cali_dbias, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_S);
	SET_PERI_REG_BITS(PMU_HP_MODEM_HP_REGULATOR0_REG, PMU_HP_MODEM_HP_REGULATOR_DBIAS,
			  hp_cali_dbias, PMU_HP_MODEM_HP_REGULATOR_DBIAS_S);
	SET_PERI_REG_BITS(PMU_HP_SLEEP_LP_REGULATOR0_REG, PMU_HP_SLEEP_LP_REGULATOR_DBIAS,
			  lp_cali_dbias, PMU_HP_SLEEP_LP_REGULATOR_DBIAS_S);
#endif /* !CONFIG_ESP_ENABLE_PVT */

#if !defined(CONFIG_SOC_SERIES_ESP32P4)
	clk_ll_rc_fast_tick_conf();
#endif
	esp_rom_output_tx_wait_idle(0);
#if !defined(CONFIG_SOC_SERIES_ESP32C5)
	rtc_clk_xtal_freq_update(rtc_clk_cfg.xtal_freq);
#endif

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	clk_ll_mspi_fast_set_hs_divider(6);
#endif
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

#if defined(CONFIG_SOC_SERIES_ESP32C5) || defined(CONFIG_SOC_SERIES_ESP32P4)
	if (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_XTAL) {
#else
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_XTAL) {
#endif
		uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_XTAL);
		uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
				     CONFIG_ESP_CONSOLE_UART_BAUDRATE, cpu_cfg->xtal_freq * MHZ(1));
		esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
	}

	rtc_clk_cpu_freq_get_config(&old_config);

	ret = rtc_clk_cpu_freq_mhz_to_config(cpu_cfg->cpu_freq, &new_config);
#if defined(CONFIG_SOC_SERIES_ESP32C5)
	/*
	 * C5 DT binding values (ESP32_CPU_CLK_SRC_*) don't match HAL
	 * enums (SOC_CPU_CLK_SRC_*). Map each DT value to its HAL
	 * equivalent(s) for validation.
	 */
	if (!ret || (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_PLL
			     ? (new_config.source != SOC_CPU_CLK_SRC_PLL_F160M &&
				new_config.source != SOC_CPU_CLK_SRC_PLL_F240M)
			     : (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_XTAL
					? (new_config.source != SOC_CPU_CLK_SRC_XTAL)
					: (new_config.source != cpu_cfg->clk_src)))) {
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
	/*
	 * P4 uses CPLL as its PLL source.
	 * The DT binding uses ESP32_CPU_CLK_SRC_PLL (1) as a generic PLL marker.
	 */
	if (!ret ||
	    (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_PLL ? (new_config.source != SOC_CPU_CLK_SRC_CPLL)
						       : (new_config.source != cpu_cfg->clk_src))) {
#else
	if (!ret || (new_config.source != cpu_cfg->clk_src)) {
#endif
		LOG_ERR("invalid CPU frequency value");
		return -EINVAL;
	}

#if !defined(CONFIG_SOC_SERIES_ESP32P4)
#if defined(CONFIG_SOC_SERIES_ESP32C5)
	bool keep_pll = (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_XTAL);
#else
	bool keep_pll = (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_XTAL);
#endif

	if (keep_pll) {
		rtc_clk_bbpll_add_consumer();
	}
#endif

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_PLL) {
		clk_ll_mspi_fast_set_hs_divider(6);
		/*
		 * The I2C analog master may be clocked from 160MHz PLL.
		 * If PLL was disabled, that clock is dead and REGI2C
		 * writes will hang. Switch to XTAL-based clock before
		 * re-enabling PLL, then restore 160MHz after PLL is up.
		 */
		MODEM_LPCON.i2c_mst_clk_conf.clk_i2c_mst_sel_160m = 0;
	}
#endif

	unsigned int key = irq_lock();

	rtc_clk_cpu_freq_set_config(&new_config);

	esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * cpu_cfg->cpu_freq /
				old_config.freq_mhz);
	irq_unlock(key);

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_PLL) {
		regi2c_ctrl_ll_master_configure_clock();
	}
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32P4)
	if (keep_pll) {
		rtc_clk_bbpll_remove_consumer();
	}
#endif

#if defined(CONFIG_ESP_CONSOLE_UART)
#if defined(CONFIG_SOC_SERIES_ESP32P4)
	if (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_PLL) {
		/*
		 * The ROM selects XTAL as the console UART source, and XTAL
		 * is independent of the CPU/PLL frequency. Keep the console
		 * on XTAL across the CPU clock change instead of re-sourcing
		 * it to PLL_F80M: that reference clock is later gated by
		 * esp_perip_clk_init(), which would leave the console
		 * without a clock and stall the first transmit.
		 */
		uint32_t uart_sclk_freq = (uint32_t)rtc_clk_xtal_freq_get() * MHZ(1);

		uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_XTAL);
		uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
				     CONFIG_ESP_CONSOLE_UART_BAUDRATE, uart_sclk_freq);
	}
#else
#if defined(CONFIG_SOC_SERIES_ESP32C5)
	if (cpu_cfg->clk_src == ESP32_CPU_CLK_SRC_PLL) {
#else
	if (cpu_cfg->clk_src == SOC_CPU_CLK_SRC_PLL) {
#endif
		uint32_t uart_sclk_freq;

		esp_clk_tree_src_get_freq_hz(
			UART_SCLK_DEFAULT, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &uart_sclk_freq);
#if defined(CONFIG_SOC_SERIES_ESP32C5)
		esp_clk_tree_enable_src(UART_SCLK_DEFAULT, true);
#endif
		uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_DEFAULT);
		uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM),
				     CONFIG_ESP_CONSOLE_UART_BAUDRATE, uart_sclk_freq);
	}
#endif
#endif
	return 0;
}
