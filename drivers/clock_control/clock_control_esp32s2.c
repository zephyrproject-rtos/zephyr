/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include <esp32s2/rom/rtc.h>
#include <soc/dport_reg.h>
#include <soc/i2s_reg.h>
#include <soc/rtc.h>

#include <esp_rom_sys.h>
#include <esp_rom_uart.h>
#include <esp_cpu.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/esp_clk.h>
#include <hal/clk_gate_ll.h>
#include <hal/regi2c_ctrl_ll.h>
#include <hal/clk_tree_hal.h>

#include <zephyr/logging/log.h>

#include "clock_control_esp32_common.h"

LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static bool reset_reason_is_cpu_reset(void)
{
	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	if ((rst_reason == RESET_REASON_CPU0_MWDT0) || (rst_reason == RESET_REASON_CPU0_MWDT1) ||
	    (rst_reason == RESET_REASON_CPU0_SW) || (rst_reason == RESET_REASON_CPU0_RTC_WDT)) {
		return true;
	}
	return false;
}

void esp32_clock_peripheral_init(void)
{
	uint32_t common_perip_clk;
	uint32_t common_perip_clk1;
	uint32_t hwcrypto_perip_clk;
	uint32_t wifi_bt_sdio_clk;

	if (reset_reason_is_cpu_reset()) {
		common_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN_REG);
		hwcrypto_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN1_REG);
		wifi_bt_sdio_clk = ~DPORT_READ_PERI_REG(DPORT_WIFI_CLK_EN_REG);
	} else {
		common_perip_clk = DPORT_WDG_CLK_EN | DPORT_PCNT_CLK_EN | DPORT_LEDC_CLK_EN |
				   DPORT_TIMERGROUP1_CLK_EN | DPORT_PWM0_CLK_EN |
				   DPORT_TWAI_CLK_EN | DPORT_PWM1_CLK_EN | DPORT_PWM2_CLK_EN |
				   DPORT_I2S0_CLK_EN | DPORT_SPI2_CLK_EN | DPORT_I2C_EXT0_CLK_EN |
				   DPORT_UHCI0_CLK_EN | DPORT_RMT_CLK_EN | DPORT_SPI3_CLK_EN |
				   DPORT_PWM0_CLK_EN | DPORT_TWAI_CLK_EN | DPORT_I2S1_CLK_EN |
				   DPORT_SPI2_DMA_CLK_EN | DPORT_SPI3_DMA_CLK_EN |
				   DPORT_PWM3_CLK_EN;

		common_perip_clk1 = 0;

		hwcrypto_perip_clk =
			DPORT_CRYPTO_AES_CLK_EN | DPORT_CRYPTO_SHA_CLK_EN | DPORT_CRYPTO_RSA_CLK_EN;

		wifi_bt_sdio_clk = DPORT_WIFI_CLK_WIFI_EN | DPORT_WIFI_CLK_BT_EN_M |
				   DPORT_WIFI_CLK_UNUSED_BIT5 | DPORT_WIFI_CLK_UNUSED_BIT12 |
				   DPORT_WIFI_CLK_SDIOSLAVE_EN | DPORT_WIFI_CLK_SDIO_HOST_EN |
				   DPORT_WIFI_CLK_EMAC_EN;
	}

	common_perip_clk |= DPORT_I2S0_CLK_EN | DPORT_SPI2_CLK_EN | DPORT_I2C_EXT0_CLK_EN |
			    DPORT_UHCI0_CLK_EN | DPORT_RMT_CLK_EN | DPORT_UHCI1_CLK_EN |
			    DPORT_SPI3_CLK_EN | DPORT_I2C_EXT1_CLK_EN |
#if CONFIG_ESP_CONSOLE_UART_NUM != 0
			    DPORT_UART_CLK_EN |
#endif
#if CONFIG_ESP_CONSOLE_UART_NUM != 1
			    DPORT_UART1_CLK_EN |
#endif
			    DPORT_USB_CLK_EN | DPORT_SPI2_DMA_CLK_EN | DPORT_SPI3_DMA_CLK_EN |
			    DPORT_I2S1_CLK_EN;

	common_perip_clk1 = 0;

	/* Change I2S clock to audio PLL first */
	REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);
	REG_SET_FIELD(I2S_CLKM_CONF_REG(1), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);

	/* Disable some peripheral clocks */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, common_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, common_perip_clk);

	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, common_perip_clk1);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, common_perip_clk1);

	/* Disable hardware crypto clocks */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, hwcrypto_perip_clk);

	/* Disable WiFi/BT/SDIO clocks */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);

	/* Enable WiFi MAC and POWER clocks */
	DPORT_SET_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, DPORT_WIFI_CLK_WIFI_EN);

	/* Set WiFi light sleep clock source to RTC slow clock */
	DPORT_REG_SET_FIELD(DPORT_BT_LPCK_DIV_INT_REG, DPORT_BT_LPCK_DIV_NUM, 0);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_8M);
	DPORT_SET_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_RTC_SLOW);
}

int esp32_clock_early_init(void)
{
	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);
	rtc_config_t rtc_cfg = RTC_CONFIG_DEFAULT();

	if (rst_reason == RESET_REASON_CHIP_POWER_ON
#if SOC_EFUSE_HAS_EFUSE_RST_BUG
	    || rst_reason == RESET_REASON_CORE_EFUSE_CRC
#endif
	) {
		rtc_cfg.cali_ocode = 1;
	}

	rtc_init(rtc_cfg);

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

	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DFREQ, rtc_clk_cfg.clk_8m_dfreq);

	/* Configure 150k clock division */
	rtc_clk_divider_set(rtc_clk_cfg.clk_rtc_clk_div);

	/* Configure 8M clock division */
	rtc_clk_8m_divider_set(rtc_clk_cfg.clk_8m_clk_div);

	/* Reset (disable) i2c internal bus for all regi2c registers */
	regi2c_ctrl_ll_i2c_reset();
	/* Enable the internal bus used to configure BBPLL */
	regi2c_ctrl_ll_i2c_bbpll_enable();
	regi2c_ctrl_ll_i2c_apll_enable();

	rtc_clk_apb_freq_update(rtc_clk_cfg.xtal_freq * MHZ(1));

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

#if defined(CONFIG_ESP_CONSOLE_UART)
#if defined(CONFIG_MCUBOOT) && defined(ESP_ROM_UART_CLK_IS_XTAL)
	uint32_t uart_clock_src_hz = (uint32_t)rtc_clk_xtal_freq_get() * MHZ(1);
#else
	uint32_t uart_clock_src_hz = esp_clk_apb_freq();
#endif
	esp_rom_uart_set_clock_baudrate(CONFIG_ESP_CONSOLE_UART_NUM, uart_clock_src_hz,
					CONFIG_ESP_CONSOLE_UART_BAUDRATE);
#endif

	return 0;
}
