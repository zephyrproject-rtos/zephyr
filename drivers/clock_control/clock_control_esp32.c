/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#include <dt-bindings/clock/esp32_clock.h>
#include <soc/rtc.h>
#include <soc/apb_ctrl_reg.h>
#include <soc/timer_group_reg.h>
#include <regi2c_ctrl.h>
#include <hal/clk_gate_ll.h>

#include <soc.h>
#include <drivers/clock_control.h>
#include <driver/periph_ctrl.h>
#include "clock_control_esp32.h"

struct esp32_clock_config {
	uint32_t clk_src_sel;
	uint32_t cpu_freq;
	uint32_t xtal_freq_sel;
	uint32_t xtal_div;
};

#define DEV_CFG(dev)                ((struct esp32_clock_config *)(dev->config))

static uint32_t const xtal_freq[] = {
	[ESP32_CLK_XTAL_24M] = 24,
	[ESP32_CLK_XTAL_26M] = 26,
	[ESP32_CLK_XTAL_40M] = 40,
	[ESP32_CLK_XTAL_AUTO] = 0
};

/* function prototypes */
extern void rtc_clk_cpu_freq_to_xtal(int freq, int div);
extern void rtc_clk_bbpll_configure(rtc_xtal_freq_t xtal_freq, int pll_freq);

static inline uint32_t clk_val_to_reg_val(uint32_t val)
{
	return (val & UINT16_MAX) | ((val & UINT16_MAX) << 16);
}

static void esp_clk_cpu_freq_to_8m(void)
{
	ets_update_cpu_frequency(8);
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, DIG_DBIAS_XTAL);
	REG_SET_FIELD(APB_CTRL_SYSCLK_CONF_REG, APB_CTRL_PRE_DIV_CNT, 0);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, RTC_CNTL_SOC_CLK_SEL_8M);
	rtc_clk_apb_freq_update(ESP32_FAST_CLK_FREQ_8M);
}

static void esp_clk_bbpll_disable(void)
{
	SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG,
			RTC_CNTL_BB_I2C_FORCE_PD | RTC_CNTL_BBPLL_FORCE_PD |
			RTC_CNTL_BBPLL_I2C_FORCE_PD);

	/* is APLL under force power down? */
	uint32_t apll_fpd = REG_GET_FIELD(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_PLLA_FORCE_PD);

	if (apll_fpd) {
		/* then also power down the internal I2C bus */
		SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_BIAS_I2C_FORCE_PD);
	}
}

static void esp_clk_bbpll_enable(void)
{
	CLEAR_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG,
			RTC_CNTL_BIAS_I2C_FORCE_PD |
			RTC_CNTL_BB_I2C_FORCE_PD |
			RTC_CNTL_BBPLL_FORCE_PD |
			RTC_CNTL_BBPLL_I2C_FORCE_PD);

	/* reset BBPLL configuration */
	REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_IR_CAL_DELAY, BBPLL_IR_CAL_DELAY_VAL);
	REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_IR_CAL_EXT_CAP, BBPLL_IR_CAL_EXT_CAP_VAL);
	REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_OC_ENB_FCAL, BBPLL_OC_ENB_FCAL_VAL);
	REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_OC_ENB_VCON, BBPLL_OC_ENB_VCON_VAL);
	REGI2C_WRITE(I2C_BBPLL, I2C_BBPLL_BBADC_CAL_7_0, BBPLL_BBADC_CAL_7_0_VAL);
}

void IRAM_ATTR ets_update_cpu_frequency(uint32_t ticks_per_us)
{
	/* Update scale factors used by ets_delay_us */
	esp_rom_g_ticks_per_us_pro = ticks_per_us;
#if defined(CONFIG_SMP)
	esp_rom_g_ticks_per_us_app = ticks_per_us;
#endif
}

static void esp_clk_wait_for_slow_cycle(void)
{
	REG_CLR_BIT(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START_CYCLING | TIMG_RTC_CALI_START);
	REG_CLR_BIT(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_RDY);
	REG_SET_FIELD(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_CLK_SEL, RTC_CAL_RTC_MUX);
	/*
	 * Request to run calibration for 0 slow clock cycles.
	 * RDY bit will be set on the nearest slow clock cycle.
	 */
	REG_SET_FIELD(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_MAX, 0);
	REG_SET_BIT(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START);
	esp_rom_ets_delay_us(1); /* RDY needs some time to go low */
	while (!GET_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_RDY)) {
		esp_rom_ets_delay_us(1);
	}
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
	DPORT_REG_WRITE(DPORT_CPU_PER_CONF_REG, per_conf);
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, dbias);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, RTC_CNTL_SOC_CLK_SEL_PLL);
	rtc_clk_apb_freq_update(MHZ(80));
	ets_update_cpu_frequency(cpu_freq_mhz);
	esp_clk_wait_for_slow_cycle();
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
	ARG_UNUSED(sub_system);

	uint32_t xtal_freq_sel = DEV_CFG(dev)->xtal_freq_sel;
	uint32_t soc_clk_sel = REG_GET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL);

	switch (soc_clk_sel) {
	case RTC_CNTL_SOC_CLK_SEL_XTL:
		*rate = xtal_freq[xtal_freq_sel];
		return 0;
	case RTC_CNTL_SOC_CLK_SEL_PLL:
		*rate = MHZ(80);
		return 0;
	default:
		*rate = 0;
		return -ENOTSUP;
	}
}

static int clock_control_esp32_init(const struct device *dev)
{
	struct esp32_clock_config *cfg = DEV_CFG(dev);
	uint32_t soc_clk_sel = REG_GET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL);

	if (soc_clk_sel != RTC_CNTL_SOC_CLK_SEL_XTL) {
		rtc_clk_cpu_freq_to_xtal(xtal_freq[cfg->xtal_freq_sel], 1);
		esp_clk_wait_for_slow_cycle();
	}

	if (soc_clk_sel == RTC_CNTL_SOC_CLK_SEL_PLL) {
		esp_clk_bbpll_disable();
	}

	switch (cfg->clk_src_sel) {
	case ESP32_CLK_SRC_XTAL:
		if (cfg->xtal_div > 1) {
			rtc_clk_cpu_freq_to_xtal(xtal_freq[cfg->xtal_freq_sel], cfg->xtal_div);
		}
		break;
	case ESP32_CLK_SRC_PLL:
		esp_clk_bbpll_enable();
		esp_clk_wait_for_slow_cycle();
		rtc_clk_bbpll_configure(rtc_clk_xtal_freq_get(), cfg->cpu_freq);
		esp_clk_cpu_freq_to_pll_mhz(cfg->cpu_freq);
		break;
	case ESP32_CLK_SRC_RTC8M:
		esp_clk_cpu_freq_to_8m();
		break;
	default:
		return -EINVAL;
	}

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

static const struct esp32_clock_config esp32_clock_config0 = {
	.clk_src_sel = DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx6), clock_source),
	.cpu_freq = DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx6), clock_frequency),
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
		    MHZ(DT_PROP(DT_INST(0, cdns_tensilica_xtensa_lx6), clock_frequency)),
		    "SYS_CLOCK_HW_CYCLES_PER_SEC Value must be equal to CPU_Freq");

BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST(0, cdns_tensilica_xtensa_lx6), clock_source),
		"CPU clock-source property must be set to ESP32_CLK_SRC_XTAL or ESP32_CLK_SRC_PLL");
