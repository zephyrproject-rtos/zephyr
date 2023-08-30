/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#define CPU_RESET_REASON RTC_SW_CPU_RESET

#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
#define DT_CPU_COMPAT cdns_tensilica_xtensa_lx6
#undef CPU_RESET_REASON
#define CPU_RESET_REASON SW_CPU_RESET
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include "esp32/rom/rtc.h"
#include "soc/dport_reg.h"
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#define DT_CPU_COMPAT cdns_tensilica_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include "esp32s2/rom/rtc.h"
#include "soc/dport_reg.h"
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#define DT_CPU_COMPAT cdns_tensilica_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s3_clock.h>
#include "esp32s3/rom/rtc.h"
#include "soc/dport_reg.h"
#include "esp32s3/clk.h"
#elif CONFIG_SOC_SERIES_ESP32C3
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#include "esp32c3/rom/rtc.h"
#include <soc/soc_caps.h>
#include <soc/soc.h>
#include <soc/rtc.h>
#endif /* CONFIG_SOC_SERIES_ESP32xx */

#include "esp_rom_sys.h"
#include <soc/rtc.h>
#include <soc/i2s_reg.h>
#include <soc/apb_ctrl_reg.h>
#include <soc/timer_group_reg.h>
#include <hal/clk_gate_ll.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <driver/periph_ctrl.h>
#include <hal/cpu_hal.h>

struct esp32_clock_config {
	int clk_src_sel;
	uint32_t cpu_freq;
	uint32_t xtal_freq_sel;
	int xtal_div;
};

static uint8_t const xtal_freq[] = {
#if defined(CONFIG_SOC_SERIES_ESP32) || \
	defined(CONFIG_SOC_SERIES_ESP32_NET) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	[ESP32_CLK_XTAL_24M] = 24,
	[ESP32_CLK_XTAL_26M] = 26,
	[ESP32_CLK_XTAL_40M] = 40,
	[ESP32_CLK_XTAL_AUTO] = 0
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	[ESP32_CLK_XTAL_40M] = 40,
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
	[ESP32_CLK_XTAL_32M] = 32,
	[ESP32_CLK_XTAL_40M] = 40,
#endif
};

static int clock_control_esp32_on(const struct device *dev,
				  clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	periph_module_enable((periph_module_t)sys);
	return 0;
}

static int clock_control_esp32_off(const struct device *dev,
				   clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	periph_module_disable((periph_module_t)sys);
	return 0;
}

static int clock_control_esp32_async_on(const struct device *dev,
					clock_control_subsys_t sys,
					clock_control_cb_t cb,
					void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
}

static enum clock_control_status clock_control_esp32_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t clk_en_reg = periph_ll_get_clk_en_reg((periph_module_t)sys);
	uint32_t clk_en_mask =  periph_ll_get_clk_en_mask((periph_module_t)sys);

	if (DPORT_GET_PERI_REG_MASK(clk_en_reg, clk_en_mask)) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_esp32_get_rate(const struct device *dev,
					clock_control_subsys_t sub_system,
					uint32_t *rate)
{
	ARG_UNUSED(sub_system);

	rtc_cpu_freq_config_t config;

	rtc_clk_cpu_freq_get_config(&config);

	*rate = config.freq_mhz;

	return 0;
}

#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
static void esp32_clock_perip_init(void)
{
	uint32_t common_perip_clk;
	uint32_t hwcrypto_perip_clk;
	uint32_t wifi_bt_sdio_clk;

#if !CONFIG_SMP
	soc_reset_reason_t rst_reas[1];
#else
	soc_reset_reason_t rst_reas[2];
#endif

	rst_reas[0] = esp_rom_get_reset_reason(0);
#if CONFIG_SMP
	rst_reas[1] = esp_rom_get_reset_reason(1);
#endif

	/* For reason that only reset CPU, do not disable the clocks
	 * that have been enabled before reset.
	 */
	if ((rst_reas[0] == RESET_REASON_CPU0_MWDT0 || rst_reas[0] == RESET_REASON_CPU0_SW ||
		rst_reas[0] == RESET_REASON_CPU0_RTC_WDT)
#if CONFIG_SMP
		|| (rst_reas[1] == RESET_REASON_CPU1_MWDT1 || rst_reas[1] == RESET_REASON_CPU1_SW ||
			rst_reas[1] == RESET_REASON_CPU1_RTC_WDT)
#endif
	) {
		common_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN_REG);
		hwcrypto_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERI_CLK_EN_REG);
		wifi_bt_sdio_clk = ~DPORT_READ_PERI_REG(DPORT_WIFI_CLK_EN_REG);
	} else {
		common_perip_clk = DPORT_WDG_CLK_EN |
			DPORT_PCNT_CLK_EN |
			DPORT_LEDC_CLK_EN |
			DPORT_TIMERGROUP1_CLK_EN |
			DPORT_PWM0_CLK_EN |
			DPORT_TWAI_CLK_EN |
			DPORT_PWM1_CLK_EN |
			DPORT_PWM2_CLK_EN |
			DPORT_PWM3_CLK_EN;

		hwcrypto_perip_clk = DPORT_PERI_EN_AES |
			DPORT_PERI_EN_SHA |
			DPORT_PERI_EN_RSA |
			DPORT_PERI_EN_SECUREBOOT;

		wifi_bt_sdio_clk = DPORT_WIFI_CLK_WIFI_EN |
			DPORT_WIFI_CLK_BT_EN_M |
			DPORT_WIFI_CLK_UNUSED_BIT5 |
			DPORT_WIFI_CLK_UNUSED_BIT12 |
			DPORT_WIFI_CLK_SDIOSLAVE_EN |
			DPORT_WIFI_CLK_SDIO_HOST_EN |
			DPORT_WIFI_CLK_EMAC_EN;
	}

	/* Reset peripherals like I2C, SPI, UART, I2S and bring them to known state */
	common_perip_clk |= DPORT_I2S0_CLK_EN |
			DPORT_UART_CLK_EN |
			DPORT_SPI2_CLK_EN |
			DPORT_I2C_EXT0_CLK_EN |
			DPORT_UHCI0_CLK_EN |
			DPORT_RMT_CLK_EN |
			DPORT_UHCI1_CLK_EN |
			DPORT_SPI3_CLK_EN |
			DPORT_I2C_EXT1_CLK_EN |
			DPORT_I2S1_CLK_EN |
			DPORT_SPI_DMA_CLK_EN;

	common_perip_clk &= ~DPORT_SPI01_CLK_EN;
	common_perip_clk &= ~DPORT_SPI2_CLK_EN;
	common_perip_clk &= ~DPORT_SPI3_CLK_EN;

	/* Change I2S clock to audio PLL first. Because if I2S uses 160MHz clock,
	 * the current is not reduced when disable I2S clock.
	 */
	DPORT_SET_PERI_REG_MASK(I2S_CLKM_CONF_REG(0), I2S_CLKA_ENA);
	DPORT_SET_PERI_REG_MASK(I2S_CLKM_CONF_REG(1), I2S_CLKA_ENA);

	/* Disable some peripheral clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, common_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, common_perip_clk);

	/* Disable hardware crypto clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERI_CLK_EN_REG, hwcrypto_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERI_RST_EN_REG, hwcrypto_perip_clk);

	/* Disable WiFi/BT/SDIO clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);
}
#endif /* CONFIG_SOC_SERIES_ESP32 */

#if defined(CONFIG_SOC_SERIES_ESP32S2)
static void esp32_clock_perip_init(void)
{
	uint32_t common_perip_clk;
	uint32_t hwcrypto_perip_clk;
	uint32_t wifi_bt_sdio_clk;
	uint32_t common_perip_clk1;

	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	/* For reason that only reset CPU, do not disable the clocks
	 * that have been enabled before reset.
	 */
	if (rst_reason == RESET_REASON_CPU0_MWDT0 || rst_reason == RESET_REASON_CPU0_SW ||
		rst_reason == RESET_REASON_CPU0_RTC_WDT || rst_reason == RESET_REASON_CPU0_MWDT1) {
		common_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN_REG);
		hwcrypto_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN1_REG);
		wifi_bt_sdio_clk = ~DPORT_READ_PERI_REG(DPORT_WIFI_CLK_EN_REG);
	} else {
		common_perip_clk = DPORT_WDG_CLK_EN |
			DPORT_I2S0_CLK_EN |
			DPORT_UART1_CLK_EN |
			DPORT_SPI2_CLK_EN |
			DPORT_I2C_EXT0_CLK_EN |
			DPORT_UHCI0_CLK_EN |
			DPORT_RMT_CLK_EN |
			DPORT_PCNT_CLK_EN |
			DPORT_LEDC_CLK_EN |
			DPORT_TIMERGROUP1_CLK_EN |
			DPORT_SPI3_CLK_EN |
			DPORT_PWM0_CLK_EN |
			DPORT_TWAI_CLK_EN |
			DPORT_PWM1_CLK_EN |
			DPORT_I2S1_CLK_EN |
			DPORT_SPI2_DMA_CLK_EN |
			DPORT_SPI3_DMA_CLK_EN |
			DPORT_PWM2_CLK_EN |
			DPORT_PWM3_CLK_EN;

		common_perip_clk1 = 0;

		hwcrypto_perip_clk = DPORT_CRYPTO_AES_CLK_EN |
				DPORT_CRYPTO_SHA_CLK_EN |
				DPORT_CRYPTO_RSA_CLK_EN;

		wifi_bt_sdio_clk = DPORT_WIFI_CLK_WIFI_EN |
			DPORT_WIFI_CLK_BT_EN_M |
			DPORT_WIFI_CLK_UNUSED_BIT5 |
			DPORT_WIFI_CLK_UNUSED_BIT12 |
			DPORT_WIFI_CLK_SDIOSLAVE_EN |
			DPORT_WIFI_CLK_SDIO_HOST_EN |
			DPORT_WIFI_CLK_EMAC_EN;
	}

	/* Reset peripherals like I2C, SPI, UART, I2S and bring them to known state */
	common_perip_clk |= DPORT_I2S0_CLK_EN |
			DPORT_UART1_CLK_EN |
			DPORT_USB_CLK_EN |
			DPORT_SPI2_CLK_EN |
			DPORT_I2C_EXT0_CLK_EN |
			DPORT_UHCI0_CLK_EN |
			DPORT_RMT_CLK_EN |
			DPORT_UHCI1_CLK_EN |
			DPORT_SPI3_CLK_EN |
			DPORT_I2C_EXT1_CLK_EN |
			DPORT_I2S1_CLK_EN |
			DPORT_SPI2_DMA_CLK_EN |
			DPORT_SPI3_DMA_CLK_EN;

	common_perip_clk1 = 0;

	/* Change I2S clock to audio PLL first. Because if I2S uses 160MHz clock,
	 * the current is not reduced when disable I2S clock.
	 */
	REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);
	REG_SET_FIELD(I2S_CLKM_CONF_REG(1), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);

	/* Disable some peripheral clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, common_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, common_perip_clk);

	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, common_perip_clk1);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, common_perip_clk1);

	/* Disable hardware crypto clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, hwcrypto_perip_clk);

	/* Disable WiFi/BT/SDIO clocks. */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);

	/* Enable WiFi MAC and POWER clocks */
	DPORT_SET_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, DPORT_WIFI_CLK_WIFI_EN);

	/* Set WiFi light sleep clock source to RTC slow clock */
	DPORT_REG_SET_FIELD(DPORT_BT_LPCK_DIV_INT_REG, DPORT_BT_LPCK_DIV_NUM, 0);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_8M);
	DPORT_SET_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_RTC_SLOW);

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);
}
#endif /* CONFIG_SOC_SERIES_ESP32S2 */

#if defined(CONFIG_SOC_SERIES_ESP32S3)
static void esp32_clock_perip_init(void)
{
#if defined(CONFIG_SOC_ESP32S3_APPCPU)
	/* skip APPCPU configuration */
	return;
#endif

	uint32_t common_perip_clk, hwcrypto_perip_clk, wifi_bt_sdio_clk = 0;
	uint32_t common_perip_clk1 = 0;

	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	/* For reason that only reset CPU, do not disable the clocks
	 * that have been enabled before reset.
	 */
	if (rst_reason == RESET_REASON_CPU0_MWDT0 || rst_reason == RESET_REASON_CPU0_SW ||
		rst_reason == RESET_REASON_CPU0_RTC_WDT || rst_reason == RESET_REASON_CPU0_MWDT1) {
		common_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN0_REG);
		hwcrypto_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN1_REG);
		wifi_bt_sdio_clk = ~READ_PERI_REG(SYSTEM_WIFI_CLK_EN_REG);
	} else {
		common_perip_clk = SYSTEM_WDG_CLK_EN |
			SYSTEM_I2S0_CLK_EN |
			SYSTEM_UART1_CLK_EN |
			SYSTEM_UART2_CLK_EN |
			SYSTEM_USB_CLK_EN |
			SYSTEM_SPI2_CLK_EN |
			SYSTEM_I2C_EXT0_CLK_EN |
			SYSTEM_UHCI0_CLK_EN |
			SYSTEM_RMT_CLK_EN |
			SYSTEM_PCNT_CLK_EN |
			SYSTEM_LEDC_CLK_EN |
			SYSTEM_TIMERGROUP1_CLK_EN |
			SYSTEM_SPI3_CLK_EN |
			SYSTEM_SPI4_CLK_EN |
			SYSTEM_PWM0_CLK_EN |
			SYSTEM_TWAI_CLK_EN |
			SYSTEM_PWM1_CLK_EN |
			SYSTEM_I2S1_CLK_EN |
			SYSTEM_SPI2_DMA_CLK_EN |
			SYSTEM_SPI3_DMA_CLK_EN |
			SYSTEM_PWM2_CLK_EN |
			SYSTEM_PWM3_CLK_EN;

		common_perip_clk1 = 0;

		hwcrypto_perip_clk = SYSTEM_CRYPTO_AES_CLK_EN |
			  SYSTEM_CRYPTO_SHA_CLK_EN |
			  SYSTEM_CRYPTO_RSA_CLK_EN;

		wifi_bt_sdio_clk = SYSTEM_WIFI_CLK_WIFI_EN |
			SYSTEM_WIFI_CLK_BT_EN_M |
			SYSTEM_WIFI_CLK_UNUSED_BIT5 |
			SYSTEM_WIFI_CLK_UNUSED_BIT12 |
			SYSTEM_WIFI_CLK_SDIO_HOST_EN;
	}

	/* Reset peripherals like I2C, SPI, UART, I2S and bring them to known state */
	common_perip_clk |= SYSTEM_I2S0_CLK_EN |
			SYSTEM_UART1_CLK_EN |
			SYSTEM_UART2_CLK_EN |
			SYSTEM_USB_CLK_EN |
			SYSTEM_SPI2_CLK_EN |
			SYSTEM_I2C_EXT0_CLK_EN |
			SYSTEM_UHCI0_CLK_EN |
			SYSTEM_RMT_CLK_EN |
			SYSTEM_UHCI1_CLK_EN |
			SYSTEM_SPI3_CLK_EN |
			SYSTEM_SPI4_CLK_EN |
			SYSTEM_I2C_EXT1_CLK_EN |
			SYSTEM_I2S1_CLK_EN |
			SYSTEM_SPI2_DMA_CLK_EN |
			SYSTEM_SPI3_DMA_CLK_EN;

	common_perip_clk1 = 0;

	/* Disable some peripheral clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, common_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, common_perip_clk);

	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, common_perip_clk1);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, common_perip_clk1);

	/* Disable hardware crypto clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, hwcrypto_perip_clk);

	/* Disable WiFi/BT/SDIO clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);
	SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

	/* Set WiFi light sleep clock source to RTC slow clock */
	REG_SET_FIELD(SYSTEM_BT_LPCK_DIV_INT_REG, SYSTEM_BT_LPCK_DIV_NUM, 0);
	CLEAR_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_8M);
	SET_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_RTC_SLOW);

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);

	esp_rom_uart_tx_wait_idle(0);
	esp_rom_uart_set_clock_baudrate(0, UART_CLK_FREQ_ROM, 115200);
}
#endif /* CONFIG_SOC_SERIES_ESP32S3 */

#if defined(CONFIG_SOC_SERIES_ESP32C3)
static void esp32_clock_perip_init(void)
{
	uint32_t common_perip_clk;
	uint32_t hwcrypto_perip_clk;
	uint32_t wifi_bt_sdio_clk;
	uint32_t common_perip_clk1;

	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	/* For reason that only reset CPU, do not disable the clocks
	 * that have been enabled before reset.
	 */
	if (rst_reason == RESET_REASON_CPU0_MWDT0 || rst_reason == RESET_REASON_CPU0_SW ||
		rst_reason == RESET_REASON_CPU0_RTC_WDT || rst_reason == RESET_REASON_CPU0_MWDT1) {
		common_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN0_REG);
		hwcrypto_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN1_REG);
		wifi_bt_sdio_clk = ~READ_PERI_REG(SYSTEM_WIFI_CLK_EN_REG);
	} else {
		common_perip_clk = SYSTEM_WDG_CLK_EN |
				SYSTEM_I2S0_CLK_EN |
				SYSTEM_UART1_CLK_EN |
				SYSTEM_SPI2_CLK_EN |
				SYSTEM_I2C_EXT0_CLK_EN |
				SYSTEM_UHCI0_CLK_EN |
				SYSTEM_RMT_CLK_EN |
				SYSTEM_LEDC_CLK_EN |
				SYSTEM_TIMERGROUP1_CLK_EN |
				SYSTEM_SPI3_CLK_EN |
				SYSTEM_SPI4_CLK_EN |
				SYSTEM_TWAI_CLK_EN |
				SYSTEM_I2S1_CLK_EN |
				SYSTEM_SPI2_DMA_CLK_EN |
				SYSTEM_SPI3_DMA_CLK_EN;

		common_perip_clk1 = 0;

		hwcrypto_perip_clk = SYSTEM_CRYPTO_AES_CLK_EN |
				SYSTEM_CRYPTO_SHA_CLK_EN |
				SYSTEM_CRYPTO_RSA_CLK_EN;

		wifi_bt_sdio_clk = SYSTEM_WIFI_CLK_WIFI_EN |
				SYSTEM_WIFI_CLK_BT_EN_M |
				SYSTEM_WIFI_CLK_UNUSED_BIT5 |
				SYSTEM_WIFI_CLK_UNUSED_BIT12;
	}

	/* Reset peripherals like I2C, SPI, UART, I2S and bring them to known state */
	common_perip_clk |= SYSTEM_I2S0_CLK_EN |
			SYSTEM_UART1_CLK_EN |
			SYSTEM_SPI2_CLK_EN |
			SYSTEM_I2C_EXT0_CLK_EN |
			SYSTEM_UHCI0_CLK_EN |
			SYSTEM_RMT_CLK_EN |
			SYSTEM_UHCI1_CLK_EN |
			SYSTEM_SPI3_CLK_EN |
			SYSTEM_SPI4_CLK_EN |
			SYSTEM_I2C_EXT1_CLK_EN |
			SYSTEM_I2S1_CLK_EN |
			SYSTEM_SPI2_DMA_CLK_EN |
			SYSTEM_SPI3_DMA_CLK_EN;

	common_perip_clk1 = 0;

	/* Disable some peripheral clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, common_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, common_perip_clk);

	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, common_perip_clk1);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, common_perip_clk1);

	/* Disable hardware crypto clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, hwcrypto_perip_clk);

	/* Disable WiFi/BT/SDIO clocks. */
	CLEAR_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);
	SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

	/* Set WiFi light sleep clock source to RTC slow clock */
	REG_SET_FIELD(SYSTEM_BT_LPCK_DIV_INT_REG, SYSTEM_BT_LPCK_DIV_NUM, 0);
	CLEAR_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_8M);
	SET_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_RTC_SLOW);

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);
}
#endif /* CONFIG_SOC_SERIES_ESP32C3 */

static int clock_control_esp32_init(const struct device *dev)
{
	const struct esp32_clock_config *cfg = dev->config;
	rtc_cpu_freq_config_t old_config;
	rtc_cpu_freq_config_t new_config;
	bool res;

	/* reset default config to use dts config */
	if (rtc_clk_apb_freq_get() < APB_CLK_FREQ || rtc_get_reset_reason(0) != CPU_RESET_REASON) {
		rtc_clk_config_t clk_cfg = RTC_CLK_CONFIG_DEFAULT();

		clk_cfg.xtal_freq = xtal_freq[cfg->xtal_freq_sel];
		clk_cfg.cpu_freq_mhz = cfg->cpu_freq;
		clk_cfg.slow_freq = rtc_clk_slow_freq_get();
		clk_cfg.fast_freq = rtc_clk_fast_freq_get();
		rtc_clk_init(clk_cfg);
	}

	rtc_clk_fast_freq_set(RTC_FAST_FREQ_8M);

	rtc_clk_cpu_freq_get_config(&old_config);

	const uint32_t old_freq_mhz = old_config.freq_mhz;
	const uint32_t new_freq_mhz = cfg->cpu_freq;

	res = rtc_clk_cpu_freq_mhz_to_config(cfg->cpu_freq, &new_config);
	if (!res) {
		return -ENOTSUP;
	}

	/* wait uart output to be cleared */
	esp_rom_uart_tx_wait_idle(0);

	if (cfg->xtal_div >= 0) {
		new_config.div = cfg->xtal_div;
	}

	if (cfg->clk_src_sel >= 0) {
		new_config.source = cfg->clk_src_sel;
	}

	/* set new configuration */
	rtc_clk_cpu_freq_set_config(&new_config);

	/* Re-calculate the ccount to make time calculation correct */
	cpu_hal_set_cycle_count((uint64_t)cpu_hal_get_cycle_count() * new_freq_mhz / old_freq_mhz);

	esp32_clock_perip_init();

	return 0;
}

static const struct clock_control_driver_api clock_control_esp32_api = {
	.on = clock_control_esp32_on,
	.off = clock_control_esp32_off,
	.async_on = clock_control_esp32_async_on,
	.get_rate = clock_control_esp32_get_rate,
	.get_status = clock_control_esp32_get_status,
};

#define ESP32_CLOCK_SOURCE	\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(0, DT_CPU_COMPAT), clock_source),	\
		    (DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_source)), (-1))

#define ESP32_CLOCK_XTAL_DIV	\
	COND_CODE_1(DT_NODE_HAS_PROP(0, xtal_div),	\
		    (DT_INST_PROP(0, xtal_div)), (-1))

static const struct esp32_clock_config esp32_clock_config0 = {
	.clk_src_sel = ESP32_CLOCK_SOURCE,
	.cpu_freq = DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_frequency) / 1000000,
	.xtal_freq_sel = DT_INST_PROP(0, xtal_freq),
	.xtal_div = ESP32_CLOCK_XTAL_DIV
};

DEVICE_DT_DEFINE(DT_NODELABEL(rtc),
		 &clock_control_esp32_init,
		 NULL,
		 NULL,
		 &esp32_clock_config0,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_esp32_api);

#ifndef CONFIG_SOC_SERIES_ESP32C3
BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) ==
		    DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_frequency),
		    "SYS_CLOCK_HW_CYCLES_PER_SEC Value must be equal to CPU_Freq");
#endif
