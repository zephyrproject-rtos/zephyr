/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#define CPU_RESET_REASON RTC_SW_CPU_RESET

#ifdef CONFIG_SOC_ESP32
#define DT_CPU_COMPAT cdns_tensilica_xtensa_lx6
#undef CPU_RESET_REASON
#define CPU_RESET_REASON SW_CPU_RESET
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include "esp32/rom/rtc.h"
#include "soc/dport_reg.h"
#elif defined(CONFIG_SOC_ESP32S2)
#define DT_CPU_COMPAT cdns_tensilica_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#include "esp32c3/rom/rtc.h"
#include <soc/soc_caps.h>
#include <soc/soc.h>
#include <soc/rtc.h>
#endif

#include <soc/rtc.h>
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
#ifdef CONFIG_SOC_ESP32
	[ESP32_CLK_XTAL_24M] = 24,
	[ESP32_CLK_XTAL_26M] = 26,
	[ESP32_CLK_XTAL_40M] = 40,
	[ESP32_CLK_XTAL_AUTO] = 0
#elif defined(CONFIG_SOC_ESP32S2)
	[ESP32_CLK_XTAL_40M] = 40,
#elif defined(CONFIG_SOC_ESP32C3)
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

	/* Enable RNG clock. */
	periph_module_enable(PERIPH_RNG_MODULE);

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
	.cpu_freq = DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_frequency),
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

#ifndef CONFIG_SOC_ESP32C3
BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) ==
		    MHZ(DT_PROP(DT_INST(0, DT_CPU_COMPAT), clock_frequency)),
		    "SYS_CLOCK_HW_CYCLES_PER_SEC Value must be equal to CPU_Freq");
#endif
