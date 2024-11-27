/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#define CPU_RESET_REASON RTC_SW_CPU_RESET

#if defined(CONFIG_SOC_SERIES_ESP32)
#define DT_CPU_COMPAT espressif_xtensa_lx6
#undef CPU_RESET_REASON
#define CPU_RESET_REASON SW_CPU_RESET
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include <esp32/rom/rtc.h>
#include <soc/dport_reg.h>
#include <soc/i2s_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include <esp32s2/rom/rtc.h>
#include <soc/dport_reg.h>
#include <soc/i2s_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s3_clock.h>
#include <esp32s3/rom/rtc.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C2)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c2_clock.h>
#include <esp32c2/rom/rtc.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#include <esp32c3/rom/rtc.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/regi2c_dig_reg.h>
#include <regi2c_ctrl.h>
#include <esp32c6/rom/rtc.h>
#include <soc/dport_access.h>
#include <hal/clk_tree_ll.h>
#include <hal/usb_serial_jtag_ll.h>
#include <esp_private/esp_pmu.h>
#include <esp_private/esp_modem_clock.h>
#include <ocode_init.h>
#endif

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <esp_rom_caps.h>
#include <esp_rom_sys.h>
#include <esp_rom_uart.h>
#include <soc/periph_defs.h>
#include <soc/rtc.h>
#include <hal/clk_gate_ll.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/esp_clk.h>
#include <esp_cpu.h>
#include <hal/regi2c_ctrl_ll.h>
#include <hal/clk_tree_hal.h>
#include <esp_private/esp_clk_tree_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static bool reset_reason_is_cpu_reset(void)
{
	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	if ((rst_reason == RESET_REASON_CPU0_MWDT0 || rst_reason == RESET_REASON_CPU0_SW ||
	     rst_reason == RESET_REASON_CPU0_RTC_WDT
#if !defined(CONFIG_SOC_SERIES_ESP32) && !defined(CONFIG_SOC_SERIES_ESP32C2)
	     || rst_reason == RESET_REASON_CPU0_MWDT1
#endif
	     )) {
		return true;
	}
	return false;
}

#if defined(CONFIG_SOC_SERIES_ESP32C6)
static void esp32_clock_perip_init(void)
{
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = rtc_clk_slow_src_get();
	modem_clock_lpclk_src_t modem_lpclk_src =
		(modem_clock_lpclk_src_t)((rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC_SLOW)
						  ? MODEM_CLOCK_LPCLK_SRC_RC_SLOW
					  : (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K)
						  ? MODEM_CLOCK_LPCLK_SRC_XTAL32K
					  : (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K)
						  ? MODEM_CLOCK_LPCLK_SRC_RC32K
					  : (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_OSC_SLOW)
						  ? MODEM_CLOCK_LPCLK_SRC_EXT32K
						  : SOC_RTC_SLOW_CLK_SRC_RC_SLOW);

	modem_clock_select_lp_clock_source(PERIPH_WIFI_MODULE, modem_lpclk_src, 0);

	soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);

	if ((rst_reason != RESET_REASON_CPU0_MWDT0) && (rst_reason != RESET_REASON_CPU0_MWDT1) &&
	    (rst_reason != RESET_REASON_CPU0_SW) && (rst_reason != RESET_REASON_CPU0_RTC_WDT)) {

		periph_ll_disable_clk_set_rst(PERIPH_UART1_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_I2C0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_RMT_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_LEDC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TIMG1_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TWAI0_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_TWAI1_MODULE);
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
		periph_ll_disable_clk_set_rst(PERIPH_SDIO_SLAVE_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_RSA_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_AES_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_SHA_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_ECC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_HMAC_MODULE);
		periph_ll_disable_clk_set_rst(PERIPH_DS_MODULE);

		REG_CLR_BIT(PCR_CTRL_TICK_CONF_REG, PCR_TICK_ENABLE);
		REG_CLR_BIT(PCR_TRACE_CONF_REG, PCR_TRACE_CLK_EN);
		REG_CLR_BIT(PCR_RETENTION_CONF_REG, PCR_RETENTION_CLK_EN);
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

		periph_ll_disable_clk_set_rst(PERIPH_LP_I2C0_MODULE);

		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_RNG_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_UART_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_OTP_DBG_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_EXT_I2C_CK_EN);
		CLEAR_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_LP_CPU_CK_EN);
		WRITE_PERI_REG(LP_CLKRST_LP_CLK_PO_EN_REG, 0);
	}
}
#else
static void esp32_clock_perip_init(void)
{
	uint32_t common_perip_clk;
	uint32_t hwcrypto_perip_clk;
	uint32_t wifi_bt_sdio_clk;
#if !defined(CONFIG_SOC_SERIES_ESP32)
	uint32_t common_perip_clk1;
#endif
	/* Avoid APPCPU to mess with the clocks. */
#if defined(CONFIG_SOC_ESP32_APPCPU) || defined(CONFIG_SOC_ESP32S3_APPCPU)
	return;
#endif
	/* For reason that only reset CPU, do not disable the clocks
	 * that have been enabled before reset.
	 */
	if (reset_reason_is_cpu_reset()) {
#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
		common_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN0_REG);
		hwcrypto_perip_clk = ~READ_PERI_REG(SYSTEM_PERIP_CLK_EN1_REG);
		wifi_bt_sdio_clk = ~READ_PERI_REG(SYSTEM_WIFI_CLK_EN_REG);
#else /* CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 */
		common_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN_REG);
		hwcrypto_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERI_CLK_EN_REG);
		wifi_bt_sdio_clk = ~DPORT_READ_PERI_REG(DPORT_WIFI_CLK_EN_REG);
#endif

#if defined(CONFIG_SOC_SERIES_ESP32S2)
		hwcrypto_perip_clk = ~DPORT_READ_PERI_REG(DPORT_PERIP_CLK_EN1_REG);
#endif
	} else {
		common_perip_clk =
#if defined(CONFIG_SOC_SERIES_ESP32C2)
			SYSTEM_SPI2_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 0
			SYSTEM_UART_CLK_EN |
#endif
#if ESP_CONSOLE_UART_NUM != 1
			SYSTEM_UART1_CLK_EN |
#endif
			SYSTEM_LEDC_CLK_EN |
			SYSTEM_I2C_EXT0_CLK_EN |
			SYSTEM_LEDC_CLK_EN;
#elif (defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32S3))
			SYSTEM_WDG_CLK_EN |
			SYSTEM_I2S0_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 0
			SYSTEM_UART_CLK_EN |
#endif
#if ESP_CONSOLE_UART_NUM != 1
			SYSTEM_UART1_CLK_EN |
#endif
#if defined(CONFIG_SOC_SERIES_ESP32S3)
#if ESP_CONSOLE_UART_NUM != 2
			SYSTEM_UART2_CLK_EN |
#endif
			SYSTEM_USB_CLK_EN |
			SYSTEM_PCNT_CLK_EN |
			SYSTEM_LEDC_CLK_EN |
			SYSTEM_PWM0_CLK_EN |
			SYSTEM_PWM1_CLK_EN |
			SYSTEM_PWM2_CLK_EN |
			SYSTEM_PWM3_CLK_EN |
#endif /* CONFIG_SOC_SERIES_ESP32S3 */
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
#else /* CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 */
			DPORT_WDG_CLK_EN |
			DPORT_PCNT_CLK_EN |
			DPORT_LEDC_CLK_EN |
			DPORT_TIMERGROUP1_CLK_EN |
			DPORT_PWM0_CLK_EN |
			DPORT_TWAI_CLK_EN |
			DPORT_PWM1_CLK_EN |
			DPORT_PWM2_CLK_EN |
#if defined(CONFIG_SOC_SERIES_ESP32S2)
			DPORT_I2S0_CLK_EN |
			DPORT_SPI2_CLK_EN |
			DPORT_I2C_EXT0_CLK_EN |
			DPORT_UHCI0_CLK_EN |
			DPORT_RMT_CLK_EN |
			DPORT_SPI3_CLK_EN |
			DPORT_PWM0_CLK_EN |
			DPORT_TWAI_CLK_EN |
			DPORT_I2S1_CLK_EN |
			DPORT_SPI2_DMA_CLK_EN |
			DPORT_SPI3_DMA_CLK_EN |
#endif /* CONFIG_SOC_SERIES_ESP32S2 */
			DPORT_PWM3_CLK_EN;
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32)
		common_perip_clk1 = 0;
#endif
		hwcrypto_perip_clk =
#if defined(CONFIG_SOC_SERIES_ESP32)
			DPORT_PERI_EN_AES |
			DPORT_PERI_EN_SHA |
			DPORT_PERI_EN_RSA |
			DPORT_PERI_EN_SECUREBOOT;
#endif /* CONFIG_SOC_SERIES_ESP32 */
#if defined(CONFIG_SOC_SERIES_ESP32S2)
			DPORT_CRYPTO_AES_CLK_EN |
			DPORT_CRYPTO_SHA_CLK_EN |
			DPORT_CRYPTO_RSA_CLK_EN;
#endif /* CONFIG_SOC_SERIES_ESP32S2 */
#if defined(CONFIG_SOC_SERIES_ESP32C2)
			SYSTEM_CRYPTO_SHA_CLK_EN;
#endif
#if (defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32S3))
			SYSTEM_CRYPTO_AES_CLK_EN |
			SYSTEM_CRYPTO_SHA_CLK_EN |
			SYSTEM_CRYPTO_RSA_CLK_EN;
#endif /* CONFIG_SOC_SERIES_ESP32C3 ||  CONFIG_SOC_SERIES_ESP32S3 */

		wifi_bt_sdio_clk =
#if defined(CONFIG_SOC_SERIES_ESP32C2)
			SYSTEM_WIFI_CLK_WIFI_EN |
			SYSTEM_WIFI_CLK_BT_EN_M |
			SYSTEM_WIFI_CLK_UNUSED_BIT5 |
			SYSTEM_WIFI_CLK_UNUSED_BIT12;
#elif (defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32S3))
			SYSTEM_WIFI_CLK_WIFI_EN |
			SYSTEM_WIFI_CLK_BT_EN_M |
			SYSTEM_WIFI_CLK_I2C_CLK_EN |
#if defined(CONFIG_SOC_SERIES_ESP32S3)
			SYSTEM_WIFI_CLK_SDIO_HOST_EN |
#endif /* CONFIG_SOC_SERIES_ESP32S3 */
			SYSTEM_WIFI_CLK_UNUSED_BIT12;
#else /* CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 */
			DPORT_WIFI_CLK_WIFI_EN |
			DPORT_WIFI_CLK_BT_EN_M |
			DPORT_WIFI_CLK_UNUSED_BIT5 |
			DPORT_WIFI_CLK_UNUSED_BIT12 |
			DPORT_WIFI_CLK_SDIOSLAVE_EN |
			DPORT_WIFI_CLK_SDIO_HOST_EN |
			DPORT_WIFI_CLK_EMAC_EN;
#endif /* CONFIG_SOC_SERIES_ESP32C3 */
	}

	/* Reset peripherals like I2C, SPI, UART, I2S and bring them to known state */
	common_perip_clk |=
#if defined(CONFIG_SOC_SERIES_ESP32C2)
			SYSTEM_SPI2_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 0
			SYSTEM_UART_CLK_EN |
#endif
#if ESP_CONSOLE_UART_NUM != 1
			SYSTEM_UART1_CLK_EN |
#endif
			SYSTEM_I2C_EXT0_CLK_EN;
#elif (defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32S3))
			SYSTEM_I2S0_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 0
			SYSTEM_UART_CLK_EN |
#endif
#if ESP_CONSOLE_UART_NUM != 1
			SYSTEM_UART1_CLK_EN |
#endif
#if defined(CONFIG_SOC_SERIES_ESP32S3)
#if ESP_CONSOLE_UART_NUM != 2
			SYSTEM_UART2_CLK_EN |
#endif
			SYSTEM_USB_CLK_EN |
#endif
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
#else
			DPORT_I2S0_CLK_EN |
			DPORT_SPI2_CLK_EN |
			DPORT_I2C_EXT0_CLK_EN |
			DPORT_UHCI0_CLK_EN |
			DPORT_RMT_CLK_EN |
			DPORT_UHCI1_CLK_EN |
			DPORT_SPI3_CLK_EN |
			DPORT_I2C_EXT1_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 0
			DPORT_UART_CLK_EN |
#endif
#if ESP_CONSOLE_UART_NUM != 1
			DPORT_UART1_CLK_EN |
#endif
#if defined(CONFIG_SOC_SERIES_ESP32)
			DPORT_SPI_DMA_CLK_EN |
#if ESP_CONSOLE_UART_NUM != 2
			DPORT_UART2_CLK_EN |
#endif
#endif /* CONFIG_SOC_SERIES_ESP32 */
#if defined(CONFIG_SOC_SERIES_ESP32S2)
			DPORT_USB_CLK_EN |
			DPORT_SPI2_DMA_CLK_EN |
			DPORT_SPI3_DMA_CLK_EN |
#endif /* CONFIG_SOC_SERIES_ESP32S2 */
			DPORT_I2S1_CLK_EN;
#endif /* CONFIG_SOC_SERIES_ESP32C3 */

#if !defined(CONFIG_SOC_SERIES_ESP32)
	common_perip_clk1 = 0;
#endif

#if defined(CONFIG_SOC_SERIES_ESP32)
	common_perip_clk &= ~DPORT_SPI01_CLK_EN;
#if defined(CONFIG_SPIRAM_SPEED_80M)
	/*
	 * 80MHz SPIRAM uses SPI2/SPI3 as well; it's initialized before this is called. Because it
	 * is used in a weird mode where clock to the peripheral is disabled but reset is also
	 * disabled, it 'hangs' in a state where it outputs a continuous 80MHz signal. Mask its bit
	 * here because we should not modify that state, regardless of what we calculated earlier.
	 */
	common_perip_clk &= ~DPORT_SPI2_CLK_EN;
	common_perip_clk &= ~DPORT_SPI3_CLK_EN;
#endif
#endif /* CONFIG_SOC_SERIES_ESP32 */

	/* Change I2S clock to audio PLL first. Because if I2S uses 160MHz clock,
	 * the current is not reduced when disable I2S clock.
	 */
#if defined(CONFIG_SOC_SERIES_ESP32)
	DPORT_SET_PERI_REG_MASK(I2S_CLKM_CONF_REG(0), I2S_CLKA_ENA);
	DPORT_SET_PERI_REG_MASK(I2S_CLKM_CONF_REG(1), I2S_CLKA_ENA);
#endif /* CONFIG_SOC_SERIES_ESP32 */
#if defined(CONFIG_SOC_SERIES_ESP32S2)
	REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);
	REG_SET_FIELD(I2S_CLKM_CONF_REG(1), I2S_CLK_SEL, I2S_CLK_AUDIO_PLL);
#endif /* CONFIG_SOC_SERIES_ESP32S2 */

	/* Disable some peripheral clocks. */
#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, common_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, common_perip_clk);

	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, common_perip_clk1);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, common_perip_clk1);
#else /* CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, common_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, common_perip_clk);
#endif /* CONFIG_SOC_SERIES_ESP32C3 || CONFIG_SOC_SERIES_ESP32S3 */

#if defined(CONFIG_SOC_SERIES_ESP32S2)
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, common_perip_clk1);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, common_perip_clk1);
#endif

	/* Disable hardware crypto clocks. */
#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, hwcrypto_perip_clk);
#elif defined(CONFIG_SOC_SERIES_ESP32)
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERI_CLK_EN_REG, hwcrypto_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERI_RST_EN_REG, hwcrypto_perip_clk);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_CLK_EN1_REG, hwcrypto_perip_clk);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN1_REG, hwcrypto_perip_clk);
#endif /* CONFIG_SOC_SERIES_ESP32C3 || CONFIG_SOC_SERIES_ESP32S3 */

#if defined(CONFIG_SOC_SERIES_ESP32S3)
	/* Force clear backup dma reset signal. This is a fix to the backup dma
	 * implementation in the ROM, the reset signal was not cleared when the
	 * backup dma was started, which caused the backup dma operation to fail.
	 */
	CLEAR_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_PERI_BACKUP_RST);
#endif /* CONFIG_SOC_SERIES_ESP32S3 */

	/* Disable WiFi/BT/SDIO clocks. */
#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	CLEAR_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);
	SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);
#else /* CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 */
	DPORT_CLEAR_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, wifi_bt_sdio_clk);
#endif /* CONFIG_SOC_SERIES_ESP32C3 || CONFIG_SOC_SERIES_ESP32S3 */

#if defined(CONFIG_SOC_SERIES_ESP32S2)
	/* Enable WiFi MAC and POWER clocks */
	DPORT_SET_PERI_REG_MASK(DPORT_WIFI_CLK_EN_REG, DPORT_WIFI_CLK_WIFI_EN);
#endif

#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	/* Set WiFi light sleep clock source to RTC slow clock */
	REG_SET_FIELD(SYSTEM_BT_LPCK_DIV_INT_REG, SYSTEM_BT_LPCK_DIV_NUM, 0);
	CLEAR_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_8M);
	SET_PERI_REG_MASK(SYSTEM_BT_LPCK_DIV_FRAC_REG, SYSTEM_LPCLK_SEL_RTC_SLOW);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	/* Set WiFi light sleep clock source to RTC slow clock */
	DPORT_REG_SET_FIELD(DPORT_BT_LPCK_DIV_INT_REG, DPORT_BT_LPCK_DIV_NUM, 0);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_8M);
	DPORT_SET_PERI_REG_MASK(DPORT_BT_LPCK_DIV_FRAC_REG, DPORT_LPCLK_SEL_RTC_SLOW);
#endif

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);

#if defined(CONFIG_SOC_SERIES_ESP32C2) || \
	defined(CONFIG_SOC_SERIES_ESP32C3) || \
	defined(CONFIG_SOC_SERIES_ESP32S3)
	periph_module_enable(PERIPH_TIMG0_MODULE);
#endif
}
#endif

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
		*rate = esp_clk_tree_lp_fast_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX);
		break;
	case ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW:
		*rate = clk_hal_lp_slow_get_freq_hz();
		break;
	default:
		*rate = clk_hal_cpu_get_freq_hz();
	}

	return 0;
}

static int esp32_select_rtc_slow_clk(uint8_t slow_clk)
{
#if !defined(CONFIG_SOC_SERIES_ESP32C6)
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk & RTC_CNTL_ANA_CLK_RTC_SEL_V;
#else
	soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk;
#endif
	uint32_t cal_val = 0;
	/* number of times to repeat 32k XTAL calibration
	 * before giving up and switching to the internal RC
	 */
	int retry_32k_xtal = 3;

	do {
#if defined(CONFIG_SOC_SERIES_ESP32C2)
		if (rtc_slow_clk_src == ESP32_RTC_SLOW_CLK_SRC_OSC_SLOW) {
			/* external clock needs to be connected to PIN0 before it can
			 * be used. Here we use rtc_clk_cal function to count
			 * the number of ext clk cycles in the given number of ext clk
			 * cycles. If the ext clk has not started up, calibration
			 * will time out, returning 0.
			 */
			LOG_DBG("waiting for external clock by pin0 to start up");
			rtc_clk_32k_enable_external();
#else
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
			} else if (slow_clk == ESP32_RTC_SLOW_CLK_32K_EXT_OSC) {
				rtc_clk_32k_enable_external();
			}
#endif
			/* When CONFIG_RTC_CLK_CAL_CYCLES is set to 0, clock calibration will not be
			 * performed at startup.
			 */
			if (CONFIG_RTC_CLK_CAL_CYCLES > 0) {
#if defined(CONFIG_SOC_SERIES_ESP32C2)
				cal_val = rtc_clk_cal(RTC_CAL_32K_OSC_SLOW,
								CONFIG_RTC_CLK_CAL_CYCLES);
#else
				cal_val = rtc_clk_cal(RTC_CAL_32K_XTAL, CONFIG_RTC_CLK_CAL_CYCLES);
#endif
				if (cal_val == 0) {
					if (retry_32k_xtal-- > 0) {
						continue;
					}
					LOG_ERR("32 kHz XTAL not found");
					return -ENODEV;
				}
			}
#if defined(CONFIG_SOC_SERIES_ESP32C6)
		} else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K) {
			rtc_clk_rc32k_enable(true);
		}
#else
		} else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256) {
			rtc_clk_8m_enable(true, true);
		}
#endif
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

static int esp32_cpu_clock_configure(const struct esp32_cpu_clock_config *cpu_cfg)
{
	rtc_cpu_freq_config_t old_config;
	rtc_cpu_freq_config_t new_config;
	rtc_clk_config_t rtc_clk_cfg = RTC_CLK_CONFIG_DEFAULT();
	bool ret;

	rtc_clk_cfg.xtal_freq = cpu_cfg->xtal_freq;
	rtc_clk_cfg.cpu_freq_mhz = cpu_cfg->cpu_freq;

	esp_rom_uart_tx_wait_idle(ESP_CONSOLE_UART_NUM);

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	rtc_clk_modem_clock_domain_active_state_icg_map_preinit();

	REG_SET_FIELD(LP_CLKRST_FOSC_CNTL_REG, LP_CLKRST_FOSC_DFREQ, rtc_clk_cfg.clk_8m_dfreq);
	REGI2C_WRITE_MASK(I2C_DIG_REG, I2C_DIG_REG_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REG_SET_FIELD(LP_CLKRST_RC32K_CNTL_REG, LP_CLKRST_RC32K_DFREQ, rtc_clk_cfg.rc32k_dfreq);
#else
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_SCK_DCAP, rtc_clk_cfg.slow_clk_dcap);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DFREQ, rtc_clk_cfg.clk_8m_dfreq);
#endif

#if defined(CONFIG_SOC_SERIES_ESP32)
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, rtc_clk_cfg.clk_8m_div - 1);
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
	clk_ll_rc_fast_tick_conf();
#else
	/* Configure 150k clock division */
	rtc_clk_divider_set(rtc_clk_cfg.clk_rtc_clk_div);

	/* Configure 8M clock division */
	rtc_clk_8m_divider_set(rtc_clk_cfg.clk_8m_clk_div);
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32C6)
	/* Reset (disable) i2c internal bus for all regi2c registers */
	regi2c_ctrl_ll_i2c_reset();
	/* Enable the internal bus used to configure BBPLL */
	regi2c_ctrl_ll_i2c_bbpll_enable();
#endif

#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32)
	regi2c_ctrl_ll_i2c_apll_enable();
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32S2)
	rtc_clk_xtal_freq_update(rtc_clk_cfg.xtal_freq);
#endif
#if defined(CONFIG_SOC_SERIES_ESP32C6)
	/* On ESP32C6, MSPI source clock's default HS divider leads to 120MHz,
	 * which is unusable before calibration. Therefore, before switching
	 * SOC_ROOT_CLK to HS, we need to set MSPI source clock HS divider
	 * to make it run at 80MHz after the switch. PLL = 480MHz, so divider is 6.
	 */
	clk_ll_mspi_fast_set_hs_divider(6);
#else
	rtc_clk_apb_freq_update(rtc_clk_cfg.xtal_freq * MHZ(1));
#endif

	/* Set CPU frequency */
	rtc_clk_cpu_freq_get_config(&old_config);

	ret = rtc_clk_cpu_freq_mhz_to_config(rtc_clk_cfg.cpu_freq_mhz, &new_config);
	if (!ret || (new_config.source != cpu_cfg->clk_src)) {
		LOG_ERR("invalid CPU frequency value");
		return -EINVAL;
	}

	rtc_clk_cpu_freq_set_config(&new_config);

	/* Re-calculate the ccount to make time calculation correct. */
	esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * rtc_clk_cfg.cpu_freq_mhz /
				old_config.freq_mhz);

#if !defined(ESP_CONSOLE_UART_NONE)
#if !defined(CONFIG_SOC_SERIES_ESP32C2) && !defined(CONFIG_SOC_SERIES_ESP32C6)
#if defined(CONFIG_MCUBOOT) && defined(ESP_ROM_UART_CLK_IS_XTAL)
	uint32_t uart_clock_src_hz = (uint32_t)rtc_clk_xtal_freq_get() * MHZ(1);
#else
	uint32_t uart_clock_src_hz = esp_clk_apb_freq();
#endif
	esp_rom_uart_set_clock_baudrate(ESP_CONSOLE_UART_NUM, uart_clock_src_hz,
					ESP_CONSOLE_UART_BAUDRATE);
#endif
#endif
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
	const struct esp32_clock_config *cfg = dev->config;
	bool ret;
	soc_reset_reason_t rst_reas;

	rst_reas = esp_rom_get_reset_reason(0);

#if defined(CONFIG_SOC_SERIES_ESP32C6)
	pmu_init();
	if (rst_reas == RESET_REASON_CHIP_POWER_ON) {
		esp_ocode_calib_init();
	}
#else /* CONFIG_SOC_SERIES_ESP32C6 */
	rtc_config_t rtc_cfg = RTC_CONFIG_DEFAULT();

#if !defined(CONFIG_SOC_SERIES_ESP32)
	if (rst_reas == RESET_REASON_CHIP_POWER_ON
#if SOC_EFUSE_HAS_EFUSE_RST_BUG
	    || rst_reas == RESET_REASON_CORE_EFUSE_CRC
#endif /* SOC_EFUSE_HAS_EFUSE_RST_BUG */
	) {
		rtc_cfg.cali_ocode = 1;
	}
#endif /* !CONFIG_SOC_SERIES_ESP32 */
	rtc_init(rtc_cfg);
#endif /* CONFIG_SOC_SERIES_ESP32C6 */

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

	esp32_clock_perip_init();

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
	.rtc_fast_clock_src = DT_PROP(DT_INST(0, espressif_esp32_rtc), fast_clk_src),
	.rtc_slow_clock_src = DT_PROP(DT_INST(0, espressif_esp32_rtc), slow_clk_src),
};

static const struct esp32_clock_config esp32_clock_config0 = {
	.cpu = esp32_cpu_clock_config0,
	.rtc = esp32_rtc_clock_config0
};

DEVICE_DT_DEFINE(DT_NODELABEL(rtc),
		 clock_control_esp32_init,
		 NULL,
		 NULL,
		 &esp32_clock_config0,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_esp32_api);
