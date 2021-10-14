/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#include <dt-bindings/clock/esp32s2_clock.h>
#include <soc/rtc.h>
#include <soc/apb_ctrl_reg.h>
#include <soc/timer_group_reg.h>
#include <regi2c_ctrl.h>
#include <hal/clk_gate_ll.h>
#include <rtc_clk_common.h>

#include <soc.h>
#include <drivers/clock_control.h>
#include <driver/periph_ctrl.h>

struct esp32_clock_config {
	uint32_t clk_src_sel;
	uint32_t cpu_freq;
	uint32_t xtal_freq_sel;
	uint32_t xtal_div;
};

#define DEV_CFG(dev)                ((struct esp32_clock_config *)(dev->config))

/*
 * Current PLL frequency, in MHZ (320 or 480). Zero if PLL is not enabled.
 * On the ESP32-S2, 480MHz PLL is enabled at reset.
 */
static uint32_t s_cur_pll_freq = 480U;

/* function prototypes */
extern void rtc_clk_cpu_freq_to_xtal(int freq, int div);
extern void rtc_clk_bbpll_configure(rtc_xtal_freq_t xtal_freq, int pll_freq);

static void esp_clk_cpu_freq_to_8m(void)
{
	ets_update_cpu_frequency(8);
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, DIG_DBIAS_XTAL);
	REG_SET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_PRE_DIV_CNT, 0);
	REG_SET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_SOC_CLK_SEL, DPORT_SOC_CLK_SEL_8M);
	rtc_clk_apb_freq_update(RTC_FAST_CLK_FREQ_8M);
}

static void esp_clk_bbpll_disable(void)
{
	SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_BB_I2C_FORCE_PD |
			RTC_CNTL_BBPLL_FORCE_PD | RTC_CNTL_BBPLL_I2C_FORCE_PD);

	s_cur_pll_freq = 0;
}

static void esp_clk_bbpll_enable(void)
{
	CLEAR_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_BB_I2C_FORCE_PD |
				RTC_CNTL_BBPLL_FORCE_PD | RTC_CNTL_BBPLL_I2C_FORCE_PD);
}

static int esp_clk_cpu_freq_to_pll_mhz(int cpu_freq_mhz)
{
	int dbias = DIG_DBIAS_80M_160M;
	int per_conf = DPORT_CPUPERIOD_SEL_80;

	if (cpu_freq_mhz == 80) {
		/* nothing to do */
	} else if (cpu_freq_mhz == 160) {
		per_conf = DPORT_CPUPERIOD_SEL_160;
	} else if (cpu_freq_mhz == 240) {
		dbias = DIG_DBIAS_240M;
		per_conf = DPORT_CPUPERIOD_SEL_240;
	} else {
		return -EINVAL;
	}
	REG_SET_FIELD(DPORT_CPU_PER_CONF_REG, DPORT_CPUPERIOD_SEL, per_conf);
	REG_SET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_PRE_DIV_CNT, 0);
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, dbias);
	REG_SET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_SOC_CLK_SEL, DPORT_SOC_CLK_SEL_PLL);
	rtc_clk_apb_freq_update(MHZ(80));
	ets_update_cpu_frequency(cpu_freq_mhz);

	return 0;
}

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
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	uint32_t soc_clk_sel = REG_GET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_SOC_CLK_SEL);
	uint32_t cpuperiod_sel;
	uint32_t source_freq_mhz;
	uint32_t clk_div;

	switch (soc_clk_sel) {
	case DPORT_SOC_CLK_SEL_XTAL:
		clk_div = REG_GET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_PRE_DIV_CNT) + 1;
		source_freq_mhz = (uint32_t) rtc_clk_xtal_freq_get();
		*rate = MHZ(source_freq_mhz / clk_div);
		return 0;
	case DPORT_SOC_CLK_SEL_PLL:
		cpuperiod_sel = DPORT_REG_GET_FIELD(DPORT_CPU_PER_CONF_REG, DPORT_CPUPERIOD_SEL);
		if (cpuperiod_sel == DPORT_CPUPERIOD_SEL_80) {
			*rate = MHZ(80);
		} else if (cpuperiod_sel == DPORT_CPUPERIOD_SEL_160) {
			*rate = MHZ(160);
		} else if (cpuperiod_sel == DPORT_CPUPERIOD_SEL_240) {
			*rate = MHZ(240);
		} else {
			*rate = 0;
			return -ENOTSUP;
		}
		return 0;
	case DPORT_SOC_CLK_SEL_8M:
		*rate = MHZ(8);
		return 0;
	default:
		*rate = 0;
		return -ENOTSUP;
	}
}

static int clock_control_esp32_init(const struct device *dev)
{
	struct esp32_clock_config *cfg = DEV_CFG(dev);
	uint32_t soc_clk_sel = cfg->clk_src_sel;
	rtc_cpu_freq_config_t cpu_freq_conf;

	if (soc_clk_sel != DPORT_SOC_CLK_SEL_XTAL) {
		rtc_clk_cpu_freq_to_xtal(ESP32_CLK_CPU_40M, 1);
	}

	if (soc_clk_sel == DPORT_SOC_CLK_SEL_PLL && cfg->cpu_freq != s_cur_pll_freq) {
		esp_clk_bbpll_disable();
	}

	switch (cfg->clk_src_sel) {
	case ESP32_CLK_SRC_XTAL:
		if (cfg->xtal_freq_sel != ESP32_CLK_XTAL_40M) {
			return -ENOTSUP;
		}
		if (cfg->xtal_div > 1) {
			rtc_clk_cpu_freq_to_xtal(ESP32_CLK_CPU_40M, cfg->xtal_div);
		}
		break;
	case ESP32_CLK_SRC_PLL:
		esp_clk_bbpll_enable();
		rtc_clk_cpu_freq_mhz_to_config(cfg->cpu_freq, &cpu_freq_conf);
		rtc_clk_bbpll_configure(rtc_clk_xtal_freq_get(), cpu_freq_conf.source_freq_mhz);
		s_cur_pll_freq = cfg->cpu_freq;
		esp_clk_cpu_freq_to_pll_mhz(cfg->cpu_freq);
		break;
	case ESP32_CLK_SRC_RTC8M:
		esp_clk_cpu_freq_to_8m();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct clock_control_driver_api clock_control_esp32_api = {
	.on = clock_control_esp32_on,
	.off = clock_control_esp32_off,
	.async_on = clock_control_esp32_async_on,
	.get_rate = clock_control_esp32_get_rate,
	.get_status = clock_control_esp32_get_status,
};

static const struct esp32_clock_config esp32_clock_config0 = {
	.clk_src_sel = DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx7), clock_source),
	.cpu_freq = DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx7), clock_frequency),
	.xtal_freq_sel = DT_INST_PROP(0, xtal_freq),
	.xtal_div =  DT_INST_PROP(0, xtal_div)
};

DEVICE_DT_DEFINE(DT_NODELABEL(rtc),
		 &clock_control_esp32_init,
		 NULL,
		 NULL,
		 &esp32_clock_config0,
		 PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		 &clock_control_esp32_api);

BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) ==
		    MHZ(DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx7), clock_frequency)),
		    "SYS_CLOCK_HW_CYCLES_PER_SEC Value must be equal to CPU_Freq");

BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST(0, cdns_tensilica_xtensa_lx7), clock_source),
		"CPU clock-source property must be set to ESP32_CLK_SRC_XTAL or ESP32_CLK_SRC_PLL");
