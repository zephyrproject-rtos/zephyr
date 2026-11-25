/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_brownout

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <esp_attr.h>
#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <esp_cpu.h>
#include <esp_log.h>
#include <esp_private/system_internal.h>
#include <esp_rom_serial_output.h>
#include <esp_rom_sys.h>
#include <hal/brownout_hal.h>
#include <hal/brownout_ll.h>
#include <hal/efuse_hal.h>
#include <hal/uart_ll.h>
#include <soc/chip_revision.h>
#include <soc/soc_caps.h>

#if SOC_LP_PERIPH_SHARE_INTERRUPT
#include <esp_private/rtc_ctrl.h>
#include <soc/rtc_cntl_reg.h>
#endif

/*
 * Register codes follow the ESP-IDF brown-out level tables. The same code is
 * not the same voltage on every SoC, so the table is selected by series.
 * 0xff is not a hardware code; it marks a voltage the series does not
 * implement.
 */
#if defined(CONFIG_SOC_SERIES_ESP32)
#define BROWNOUT_CODE(uv)                                                                          \
	((uv) == 2430000   ? 0                                                                     \
	 : (uv) == 2480000 ? 1                                                                     \
	 : (uv) == 2580000 ? 2                                                                     \
	 : (uv) == 2620000 ? 3                                                                     \
	 : (uv) == 2670000 ? 4                                                                     \
	 : (uv) == 2700000 ? 5                                                                     \
	 : (uv) == 2770000 ? 6                                                                     \
	 : (uv) == 2800000 ? 7                                                                     \
			   : 0xff)
#elif defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32S3)
#define BROWNOUT_CODE(uv)                                                                          \
	((uv) == 3300000   ? 1                                                                     \
	 : (uv) == 3190000 ? 2                                                                     \
	 : (uv) == 2980000 ? 3                                                                     \
	 : (uv) == 2840000 ? 4                                                                     \
	 : (uv) == 2670000 ? 5                                                                     \
	 : (uv) == 2560000 ? 6                                                                     \
	 : (uv) == 2440000 ? 7                                                                     \
			   : 0xff)
#elif defined(CONFIG_SOC_SERIES_ESP32C2) || defined(CONFIG_SOC_SERIES_ESP32C3) ||                  \
	defined(CONFIG_SOC_SERIES_ESP32C5) || defined(CONFIG_SOC_SERIES_ESP32C6) ||                \
	defined(CONFIG_SOC_SERIES_ESP32C61)
#define BROWNOUT_CODE(uv)                                                                          \
	((uv) == 3270000   ? 2                                                                     \
	 : (uv) == 3100000 ? 3                                                                     \
	 : (uv) == 2920000 ? 4                                                                     \
	 : (uv) == 2760000 ? 5                                                                     \
	 : (uv) == 2640000 ? 6                                                                     \
	 : (uv) == 2510000 ? 7                                                                     \
			   : 0xff)
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
#define BROWNOUT_CODE(uv)                                                                          \
	((uv) == 2570000   ? 0                                                                     \
	 : (uv) == 2620000 ? 1                                                                     \
	 : (uv) == 2670000 ? 2                                                                     \
	 : (uv) == 2730000 ? 3                                                                     \
	 : (uv) == 2780000 ? 4                                                                     \
	 : (uv) == 2830000 ? 5                                                                     \
	 : (uv) == 2880000 ? 6                                                                     \
	 : (uv) == 2940000 ? 7                                                                     \
			   : 0xff)
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
#define BROWNOUT_CODE(uv) ((uv) == 2420000 ? 5 : (uv) == 2520000 ? 6 : (uv) == 2600000 ? 7 : 0xff)
#elif defined(CONFIG_SOC_SERIES_ESP32S31)
#define BROWNOUT_CODE(uv)                                                                          \
	((uv) == 3140000   ? 2                                                                     \
	 : (uv) == 2950000 ? 3                                                                     \
	 : (uv) == 2790000 ? 4                                                                     \
	 : (uv) == 2650000 ? 5                                                                     \
	 : (uv) == 2510000 ? 6                                                                     \
	 : (uv) == 2400000 ? 7                                                                     \
			   : 0xff)
#else
#error Unsupported Espressif SoC
#endif

static void IRAM_ATTR brownout_isr(void *arg)
{
	ARG_UNUSED(arg);

	brownout_ll_intr_clear();

#if SOC_CPU_CORES_NUM > 1
	{
		int core_id = esp_cpu_get_core_id();
		int other_core_id = (core_id == 0) ? 1 : 0;

		esp_cpu_stall(other_core_id);
	}
#endif

	esp_reset_reason_set_hint(ESP_RST_BROWNOUT);
	ESP_DRAM_LOGE("BOD", "Brownout detector was triggered\r\n");

	for (uint32_t uart = 0U; uart < SOC_UART_HP_NUM; uart++) {
		if (uart_ll_is_enabled(uart)) {
			esp_rom_output_tx_wait_idle((uint8_t)uart);
		}
	}

	esp_rom_software_reset_system();

	while (true) {
	}
}

static int brownout_enable_interrupt(int irq_source)
{
	int ret;

	brownout_ll_intr_clear();

#if SOC_LP_PERIPH_SHARE_INTERRUPT
	ARG_UNUSED(irq_source);

	ret = rtc_isr_register(brownout_isr, NULL, RTC_CNTL_BROWN_OUT_INT_ENA_M,
			       RTC_INTR_FLAG_IRAM);
	if (ret != ESP_OK) {
		return -EIO;
	}
#else
	ret = esp_intr_alloc_intrstatus(irq_source, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED,
					(uint32_t)brownout_ll_intr_get_status_reg(),
					BROWNOUT_DETECTOR_LL_INTERRUPT_MASK, brownout_isr, NULL,
					NULL);
	if (ret != 0) {
		return ret;
	}
#endif

	brownout_ll_intr_enable(true);

	return 0;
}

/*
 * hardware_init() has already run ana_reset_config(). brownout_hal_config()
 * replaces that analog reset with the mode 0 detector.
 */
static int brownout_configure(uint8_t threshold, bool dt_interrupt, bool flash_pd, bool rf_pd,
			      int irq_source)
{
	bool use_intr = dt_interrupt;
	int ret;

#if defined(CONFIG_SOC_SERIES_ESP32)
	/* ESP32 chip revision 1 cannot reset from the brown-out detector. */
	if (ESP_CHIP_REV_MAJOR_AND_ABOVE(efuse_hal_chip_revision(), 100)) {
		use_intr = true;
	}
#endif

	brownout_hal_config(&(brownout_hal_config_t){
		.threshold = threshold,
		.enabled = true,
		.reset_enabled = (use_intr == false),
		.flash_power_down = flash_pd,
		.rf_power_down = rf_pd,
	});

	if (use_intr == false) {
		return 0;
	}

	ret = brownout_enable_interrupt(irq_source);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#define BROWNOUT_DEFINE(inst)                                                                      \
	BUILD_ASSERT(BROWNOUT_CODE(DT_INST_PROP(inst, brownout_threshold_microvolt)) != 0xff,      \
		     "brownout threshold is not valid for this SoC");                              \
                                                                                                   \
	static int brownout_init_##inst(const struct device *dev)                                  \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		return brownout_configure(                                                         \
			(uint8_t)BROWNOUT_CODE(DT_INST_PROP(inst, brownout_threshold_microvolt)),  \
			DT_INST_ENUM_HAS_VALUE(inst, espressif_brownout_action, interrupt),        \
			DT_INST_ENUM_HAS_VALUE(inst, espressif_flash_power_down, enabled),         \
			DT_INST_ENUM_HAS_VALUE(inst, espressif_rf_power_down, enabled),            \
			DT_INST_IRQ_BY_IDX(inst, 0, irq));                                         \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, brownout_init_##inst, NULL, NULL, NULL, PRE_KERNEL_1,          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(BROWNOUT_DEFINE)
