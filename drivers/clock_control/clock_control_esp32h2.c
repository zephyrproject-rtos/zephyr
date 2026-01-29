/*
 * Copyright (c) 2023-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <zephyr/dt-bindings/clock/esp32h2_clock.h>
#include <soc/lpperi_reg.h>
#include <soc/lp_clkrst_reg.h>
#include <regi2c_ctrl.h>
#include <esp32h2/rom/rtc.h>
#include <soc/dport_access.h>
#include <soc/rtc.h>

#include <esp_rom_sys.h>
#include <esp_rom_uart.h>
#include <esp_cpu.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/esp_clk.h>
#include <esp_private/esp_pmu.h>
#include <esp_sleep.h>
#include <hal/clk_gate_ll.h>
#include <hal/clk_tree_ll.h>
#include <hal/clk_tree_hal.h>
#include <hal/usb_serial_jtag_ll.h>

#include <zephyr/logging/log.h>

#include "clock_control_esp32_common.h"

LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

void esp32_clock_peripheral_init(void)
{
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = rtc_clk_slow_src_get();
	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	esp_sleep_pd_domain_t pu_domain =
		(esp_sleep_pd_domain_t)((rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K)
						? ESP_PD_DOMAIN_XTAL32K
					: (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K)
						? ESP_PD_DOMAIN_RC32K
						: ESP_PD_DOMAIN_MAX);
	esp_sleep_pd_config(pu_domain, ESP_PD_OPTION_ON);

	if ((rst_reason != RESET_REASON_CPU0_MWDT0) && (rst_reason != RESET_REASON_CPU0_MWDT1) &&
	    (rst_reason != RESET_REASON_CPU0_SW) && (rst_reason != RESET_REASON_CPU0_RTC_WDT)) {

#if CONFIG_ESP_CONSOLE_UART_NUM != 0
		periph_ll_disable_clk_set_rst(PERIPH_UART0_MODULE);
#endif
#if CONFIG_ESP_CONSOLE_UART_NUM != 1
		periph_ll_disable_clk_set_rst(PERIPH_UART1_MODULE);
#endif
		periph_ll_disable_clk_set_rst(PERIPH_I2C0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_I2C1_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_RMT_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_LEDC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TIMG1_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TWAI0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_I2S1_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_PCNT_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_ETM_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_MCPWM0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_PARLIO_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_GDMA_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_SPI2_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TEMPSENSOR_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_UHCI0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_SARADC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_RSA_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_AES_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_SHA_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_ECC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_HMAC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_DS_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_ECDSA_MODULE);

		REG_CLR_BIT(PCR_CTRL_TICK_CONF_REG, PCR_TICK_ENABLE);
		REG_CLR_BIT(PCR_TRACE_CONF_REG, PCR_TRACE_CLK_EN);
		REG_CLR_BIT(PCR_MEM_MONITOR_CONF_REG, PCR_MEM_MONITOR_CLK_EN);
		REG_CLR_BIT(PCR_PVT_MONITOR_CONF_REG, PCR_PVT_MONITOR_CLK_EN);
		REG_CLR_BIT(PCR_PVT_MONITOR_FUNC_CLK_CONF_REG, PCR_PVT_MONITOR_FUNC_CLK_EN);
		WRITE_PERI_REG(PCR_CTRL_CLK_OUT_EN_REG, 0);

#if CONFIG_SERIAL_ESP32_USB
		usb_serial_jtag_ll_enable_bus_clock(false);
#endif
	}

	if ((rst_reason == RESET_REASON_CHIP_POWER_ON) ||
	    (rst_reason == RESET_REASON_CHIP_BROWN_OUT) ||
	    (rst_reason == RESET_REASON_SYS_RTC_WDT) ||
	    (rst_reason == RESET_REASON_SYS_SUPER_WDT)) {

		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_RNG_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_UART_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_OTP_DBG_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_EXT_I2C_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_CPU_CK_EN);
		WRITE_PERI_REG(LP_CLKRST_LP_CLK_PO_EN_REG, 0);
	}
}

int esp32_clock_early_init(void)
{
	pmu_init();

	return 0;
}

int esp32_cpu_clock_configure(const struct esp32_cpu_clock_config *cpu_cfg)
{
	rtc_cpu_freq_config_t old_config;
	rtc_cpu_freq_config_t new_config;
	rtc_clk_config_t rtc_clk_cfg = RTC_CLK_CONFIG_DEFAULT();
	bool ret;

	rtc_clk_cfg.xtal_freq = cpu_cfg->xtal_freq;
	rtc_clk_cfg.cpu_freq_mhz = cpu_cfg->cpu_freq;

	esp_rom_uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);

	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_OC_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_EN_I2C_RTC_DREG, 0);
	REGI2C_WRITE_MASK(I2C_PMU, I2C_PMU_EN_I2C_DIG_DREG, 0);

	REG_SET_FIELD(LP_CLKRST_FOSC_CNTL_REG, LP_CLKRST_FOSC_DFREQ, rtc_clk_cfg.clk_8m_dfreq);
	REG_SET_FIELD(LP_CLKRST_RC32K_CNTL_REG, LP_CLKRST_RC32K_DFREQ, rtc_clk_cfg.rc32k_dfreq);

	uint32_t hp_cali_dbias = get_act_hp_dbias();
	uint32_t lp_cali_dbias = get_act_lp_dbias();

	SET_PERI_REG_BITS(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS,
			  hp_cali_dbias, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_S);
	SET_PERI_REG_BITS(PMU_HP_MODEM_HP_REGULATOR0_REG, PMU_HP_MODEM_HP_REGULATOR_DBIAS,
			  hp_cali_dbias, PMU_HP_MODEM_HP_REGULATOR_DBIAS_S);
	SET_PERI_REG_BITS(PMU_HP_SLEEP_LP_REGULATOR0_REG, PMU_HP_SLEEP_LP_REGULATOR_DBIAS,
			  lp_cali_dbias, PMU_HP_SLEEP_LP_REGULATOR_DBIAS_S);

	clk_ll_rc_fast_tick_conf();

	esp_rom_uart_tx_wait_idle(0);
	rtc_clk_xtal_freq_update(rtc_clk_cfg.xtal_freq);

	/* Set CPU frequency */
	rtc_clk_cpu_freq_get_config(&old_config);

	ret = rtc_clk_cpu_freq_mhz_to_config(rtc_clk_cfg.cpu_freq_mhz, &new_config);
	if (!ret || (new_config.source != cpu_cfg->clk_src)) {
		LOG_ERR("invalid CPU frequency value");
		return -EINVAL;
	}

	rtc_clk_cpu_freq_set_config(&new_config);

	/* Re-calculate the ccount to make time calculation correct */
	esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * rtc_clk_cfg.cpu_freq_mhz /
				old_config.freq_mhz);

	return 0;
}
