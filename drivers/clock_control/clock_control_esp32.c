/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#include <dt-bindings/clock/esp32_clock.h>
#include <soc/dport_reg.h>
#include <soc/rtc.h>
#include <soc/rtc_cntl_reg.h>
#include <drivers/uart.h>
#include <soc/apb_ctrl_reg.h>

#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include "clock_control_esp32.h"

struct esp32_clock_config {
	uint32_t clk_src_sel;
	uint32_t cpu_freq;
	uint32_t xtal_freq_sel;
	uint32_t xtal_div;
};

struct control_regs {
	/** Peripheral control register */
	uint32_t clk;
	/** Peripheral reset register */
	uint32_t rst;
};

struct bbpll_cfg {
	uint8_t div_ref;
	uint8_t div7_0;
	uint8_t div10_8;
	uint8_t lref;
	uint8_t dcur;
	uint8_t bw;
};

struct pll_cfg {
	uint8_t dbias_wak;
	uint8_t endiv5;
	uint8_t bbadc_dsmp;
	struct bbpll_cfg bbpll[2];
};


#define PLL_APB_CLK_FREQ            80

#define RTC_PLL_FREQ_320M           0
#define RTC_PLL_FREQ_480M           1

#define DPORT_CPUPERIOD_SEL_80      0
#define DPORT_CPUPERIOD_SEL_160     1
#define DPORT_CPUPERIOD_SEL_240     2

#define DEV_CFG(dev)                ((struct esp32_clock_config *)(dev->config))
#define GET_REG_BANK(module_id)     ((uint32_t)module_id / 32U)
#define GET_REG_OFFSET(module_id)   ((uint32_t)module_id % 32U)

#define CLOCK_REGS_BANK_COUNT 	    3

const struct control_regs clock_control_regs[CLOCK_REGS_BANK_COUNT] = {
	[0] = { .clk = DPORT_PERIP_CLK_EN_REG, .rst = DPORT_PERIP_RST_EN_REG },
	[1] = { .clk = DPORT_PERI_CLK_EN_REG,  .rst = DPORT_PERI_RST_EN_REG },
	[2] = { .clk = DPORT_WIFI_CLK_EN_REG,  .rst = DPORT_CORE_RST_EN_REG }
};

static uint32_t const xtal_freq[] = {
	[ESP32_CLK_XTAL_40M] = 40,
	[ESP32_CLK_XTAL_26M] = 26
};

const struct pll_cfg pll_config[] = {
	[RTC_PLL_FREQ_320M] = {
		.dbias_wak = 0,
		.endiv5 = BBPLL_ENDIV5_VAL_320M,
		.bbadc_dsmp = BBPLL_BBADC_DSMP_VAL_320M,
		.bbpll[ESP32_CLK_XTAL_40M] = {
			/* 40mhz */
			.div_ref = 0,
			.div7_0 = 32,
			.div10_8 = 0,
			.lref = 0,
			.dcur = 6,
			.bw = 3,
		},
		.bbpll[ESP32_CLK_XTAL_26M] = {
			/* 26mhz */
			.div_ref = 12,
			.div7_0 = 224,
			.div10_8 = 4,
			.lref = 1,
			.dcur = 0,
			.bw = 1,
		}
	},
	[RTC_PLL_FREQ_480M] = {
		.dbias_wak = 0,
		.endiv5 = BBPLL_ENDIV5_VAL_480M,
		.bbadc_dsmp = BBPLL_BBADC_DSMP_VAL_480M,
		.bbpll[ESP32_CLK_XTAL_40M] = {
			/* 40mhz */
			.div_ref = 0,
			.div7_0 = 28,
			.div10_8 = 0,
			.lref = 0,
			.dcur = 6,
			.bw = 3,
		},
		.bbpll[ESP32_CLK_XTAL_26M] = {
			/* 26mhz */
			.div_ref = 12,
			.div7_0 = 144,
			.div10_8 = 4,
			.lref = 1,
			.dcur = 0,
			.bw = 1,
		}
	}
};

static void bbpll_configure(rtc_xtal_freq_t xtal_freq, uint32_t pll_freq)
{
	uint8_t dbias_wak = 0;

	const struct pll_cfg *cfg = &pll_config[pll_freq];
	const struct bbpll_cfg *bb_cfg = &pll_config[pll_freq].bbpll[xtal_freq];

	/* Enable PLL, Clear PowerDown (_PD) flags */
	CLEAR_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG,
			    RTC_CNTL_BIAS_I2C_FORCE_PD |
			    RTC_CNTL_BB_I2C_FORCE_PD |
			    RTC_CNTL_BBPLL_FORCE_PD |
			    RTC_CNTL_BBPLL_I2C_FORCE_PD);

	/* reset BBPLL configuration */
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_IR_CAL_DELAY, BBPLL_IR_CAL_DELAY_VAL);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_IR_CAL_EXT_CAP, BBPLL_IR_CAL_EXT_CAP_VAL);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_OC_ENB_FCAL, BBPLL_OC_ENB_FCAL_VAL);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_OC_ENB_VCON, BBPLL_OC_ENB_VCON_VAL);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_BBADC_CAL_7_0, BBPLL_BBADC_CAL_7_0_VAL);

	/* voltage needs to be changed for CPU@240MHz or
	 * 80MHz Flash (because of internal flash regulator)
	 */
	if (pll_freq == RTC_PLL_FREQ_320M) {
		dbias_wak = DIG_DBIAS_80M_160M;
	} else { /* RTC_PLL_FREQ_480M */
		dbias_wak = DIG_DBIAS_240M;
	}

	/* Configure the voltage */
	REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, dbias_wak);

	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_ENDIV5, cfg->endiv5);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_BBADC_DSMP, cfg->bbadc_dsmp);

	uint8_t i2c_bbpll_lref = (bb_cfg->lref << 7) | (bb_cfg->div10_8 << 4) | (bb_cfg->div_ref);

	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_OC_LREF, i2c_bbpll_lref);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_OC_DIV_7_0, bb_cfg->div7_0);
	I2C_WRITEREG_RTC(I2C_BBPLL, I2C_BBPLL_OC_DCUR, ((bb_cfg->bw << 6) | bb_cfg->dcur));
}

static void cpuclk_pll_configure(uint32_t xtal_freq, uint32_t cpu_freq)
{
	uint32_t pll_freq = RTC_PLL_FREQ_320M;
	uint32_t cpu_period_sel = DPORT_CPUPERIOD_SEL_80;

	switch (cpu_freq) {
	case ESP32_CLK_CPU_80M:
		pll_freq = RTC_PLL_FREQ_320M;
		cpu_period_sel = DPORT_CPUPERIOD_SEL_80;
		break;
	case ESP32_CLK_CPU_160M:
		pll_freq = RTC_PLL_FREQ_320M;
		cpu_period_sel = DPORT_CPUPERIOD_SEL_160;
		break;
	case ESP32_CLK_CPU_240M:
		pll_freq = RTC_PLL_FREQ_480M;
		cpu_period_sel = DPORT_CPUPERIOD_SEL_240;
		break;
	}

	/* Configure PLL based on XTAL Value */
	bbpll_configure(xtal_freq, pll_freq);
	/* Set CPU Speed (80,160,240) */
	DPORT_REG_WRITE(DPORT_CPU_PER_CONF_REG, cpu_period_sel);
	/* Set PLL as CPU Clock Source */
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, RTC_CNTL_SOC_CLK_SEL_PLL);

	/*
	 * Update REF_Tick,
	 * if PLL is the cpu clock source, APB frequency is always 80MHz
	 */
	REG_WRITE(APB_CTRL_PLL_TICK_CONF_REG, PLL_APB_CLK_FREQ - 1);
}

static int clock_control_esp32_on(struct device *dev,
				  clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t bank = GET_REG_BANK(sys);
	uint32_t offset =  GET_REG_OFFSET(sys);

	__ASSERT_NO_MSG(bank >= CLOCK_REGS_BANK_COUNT);

	esp32_set_mask32(BIT(offset), clock_control_regs[bank].clk);
	esp32_clear_mask32(BIT(offset), clock_control_regs[bank].rst);
	return 0;
}

static int clock_control_esp32_off(struct device *dev,
				   clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t bank = GET_REG_BANK(sys);
	uint32_t offset =  GET_REG_OFFSET(sys);

	__ASSERT_NO_MSG(bank >= CLOCK_REGS_BANK_COUNT);

	esp32_clear_mask32(BIT(offset), clock_control_regs[bank].clk);
	esp32_set_mask32(BIT(offset), clock_control_regs[bank].rst);
	return 0;
}

static enum clock_control_status clock_control_esp32_get_status(struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t bank = GET_REG_BANK(sys);
	uint32_t offset =  GET_REG_OFFSET(sys);

	if (DPORT_GET_PERI_REG_MASK(clock_control_regs[bank].clk, BIT(offset))) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_esp32_get_rate(struct device *dev,
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

static int clock_control_esp32_init(struct device *dev)
{
	struct esp32_clock_config *cfg = DEV_CFG(dev);

	/* Wait for UART first before changing freq to avoid garbage on console */
	esp32_rom_uart_tx_wait_idle(0);

	switch (cfg->clk_src_sel) {
	case ESP32_CLK_SRC_XTAL:
		REG_SET_FIELD(APB_CTRL_SYSCLK_CONF_REG, APB_CTRL_PRE_DIV_CNT, cfg->xtal_div);
		/* adjust ref_tick */
		REG_WRITE(APB_CTRL_XTAL_TICK_CONF_REG, xtal_freq[cfg->xtal_freq_sel] - 1);
		/* switch clock source */
		REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, RTC_CNTL_SOC_CLK_SEL_XTL);
		break;
	case ESP32_CLK_SRC_PLL:
		cpuclk_pll_configure(cfg->xtal_freq_sel, cfg->cpu_freq);
		break;
	default:
		return -EINVAL;
	}

	/* Re-calculate the CCOUNT register value to make time calculation correct.
	 * This should be updated on each frequency change
	 * New CCOUNT = Current CCOUNT * (new freq / old freq)
	 */
	XTHAL_SET_CCOUNT((uint64_t)XTHAL_GET_CCOUNT() * cfg->cpu_freq / xtal_freq[cfg->xtal_freq_sel]);
	return 0;
}

static const struct clock_control_driver_api clock_control_esp32_api = {
	.on = clock_control_esp32_on,
	.off = clock_control_esp32_off,
	.get_rate = clock_control_esp32_get_rate,
	.get_status = clock_control_esp32_get_status,
};

static const struct esp32_clock_config esp32_clock_config0 = {
	.clk_src_sel = DT_PROP(DT_INST(0, cadence_tensilica_xtensa_lx6), clock_source),
	.cpu_freq = DT_PROP(DT_INST(0, cadence_tensilica_xtensa_lx6), clock_frequency),
	.xtal_freq_sel = DT_INST_PROP(0, xtal_freq),
	.xtal_div =  DT_INST_PROP(0, xtal_div),
};

DEVICE_AND_API_INIT(clk_esp32, DT_INST_LABEL(0),
		    &clock_control_esp32_init,
		    NULL, &esp32_clock_config0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &clock_control_esp32_api);

BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) == MHZ(DT_PROP(DT_INST(0, cadence_tensilica_xtensa_lx6), clock_frequency)),
		"SYS_CLOCK_HW_CYCLES_PER_SEC Value must be equal to CPU_Freq");

BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST(0, cadence_tensilica_xtensa_lx6), clock_source),
		"CPU clock-source property must be set to ESP32_CLK_SRC_XTAL or ESP32_CLK_SRC_PLL");
