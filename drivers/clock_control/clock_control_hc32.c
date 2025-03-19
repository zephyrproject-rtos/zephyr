/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/hc32_clock_control.h>
#include "clock_control_hc32.h"

/**
 * @defgroup EV_HC32F460_LQFP100_V2_XTAL_CONFIG EV_HC32F460_LQFP100_V2 XTAL Configure definition
 * @{
 */
#define BSP_XTAL_PORT                   (GPIO_PORT_H)
#define BSP_XTAL_IN_PIN                 (GPIO_PIN_01)
#define BSP_XTAL_OUT_PIN                (GPIO_PIN_00)

/**
 * @defgroup EV_HC32F460_LQFP100_V2_XTAL32_CONFIG EV_HC32F460_LQFP100_V2 XTAL32 Configure definition
 * @{
 */
#define BSP_XTAL32_PORT                 (GPIO_PORT_C)
#define BSP_XTAL32_IN_PIN               (GPIO_PIN_15)
#define BSP_XTAL32_OUT_PIN              (GPIO_PIN_14)

static void hc32_clock_stale(uint32_t flag)
{
	uint32_t stable_time = 0;

	while (RESET == CLK_GetStableStatus(flag)) {
		if (stable_time ++ >= 20000) {
			break;
		}
	}
}

#if HC32_XTAL_ENABLED
static void hc32_clock_xtal_init(void)
{
	stc_clock_xtal_init_t     stcXtalInit;

	GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);

	(void)CLK_XtalStructInit(&stcXtalInit);
	stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
	stcXtalInit.u8Drv = XTAL_DRV;
	stcXtalInit.u8State = CLK_XTAL_ON;
	stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
	(void)CLK_XtalInit(&stcXtalInit);
	hc32_clock_stale(CLK_STB_FLAG_XTAL);
}
#endif

static void hc32_clock_xtal32_init(void)
{
	stc_clock_xtal32_init_t stcXtal32Init;

	(void)CLK_Xtal32StructInit(&stcXtal32Init);
	stcXtal32Init.u8State = CLK_XTAL32_ON;
	stcXtal32Init.u8Drv = CONFIG_HC32_XTAL32_DRV;
	stcXtal32Init.u8Filter = CLK_XTAL32_FILTER_ALL_MD;
	GPIO_AnalogCmd(BSP_XTAL32_PORT, BSP_XTAL32_IN_PIN | BSP_XTAL32_OUT_PIN, ENABLE);
	(void)CLK_Xtal32Init(&stcXtal32Init);
}

static void hc32_clock_hrc_init(void)
{
	CLK_HrcCmd(ENABLE);
	hc32_clock_stale(CLK_STB_FLAG_HRC);
}

static void hc32_clock_mrc_init(void)
{
	CLK_MrcCmd(ENABLE);
}

static void hc32_clock_lrc_init(void)
{
	CLK_LrcCmd(ENABLE);
}

#if HC32_PLL_ENABLED
static void hc32_clock_pll_init(void)
{
	stc_clock_pll_init_t      stcMPLLInit;

	(void)CLK_PLLStructInit(&stcMPLLInit);
	stcMPLLInit.PLLCFGR = 0UL;
	stcMPLLInit.PLLCFGR_f.PLLM = (HC32_PLL_M_DIVISOR - 1UL);
	stcMPLLInit.PLLCFGR_f.PLLN = (HC32_PLL_N_MULTIPLIER - 1UL);
	stcMPLLInit.PLLCFGR_f.PLLP = (HC32_PLL_P_DIVISOR - 1UL);
	stcMPLLInit.PLLCFGR_f.PLLQ = (HC32_PLL_Q_DIVISOR - 1UL);
	stcMPLLInit.PLLCFGR_f.PLLR = (HC32_PLL_R_DIVISOR - 1UL);
#if HC32_PLL_SRC_XTAL
	hc32_clock_xtal_init();
	stcMPLLInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
#elif HC32_PLL_SRC_HRC
	hc32_clock_hrc_init();
	stcMPLLInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_HRC;
#endif
	stcMPLLInit.u8PLLState = CLK_PLL_ON;
	(void)CLK_PLLInit(&stcMPLLInit);
	hc32_clock_stale(CLK_STB_FLAG_PLL);
}
#endif

static void hc32_clk_conf(void)
{
#if HC32_PLL_ENABLED
	hc32_clock_pll_init();
#endif
#if HC32_XTAL_ENABLED
	hc32_clock_xtal_init();
#endif
#if HC32_HRC_ENABLED
		hc32_clock_hrc_init();
#endif
#if HC32_MRC_ENABLED
		hc32_clock_mrc_init();
#endif
#if HC32_LRC_ENABLED
		hc32_clock_lrc_init();
#endif
#if HC32_XTAL32_ENABLED
		hc32_clock_xtal32_init();
#endif
}

#if defined (HC32F460)
static void hc32_run_mode_switch(uint32_t old_freq, uint32_t new_freq)
{
	uint8_t old_run_mode;
	uint8_t new_run_mode;

	new_run_mode = (new_freq >= 168000000) ? \
			2 : (new_freq >= 8000000) ? 1 : 0;
	old_run_mode = (old_freq >= 168000000) ? \
			2 : (old_freq >= 8000000) ? 1 : 0;
	if (old_run_mode == 0) {
		if (new_run_mode == 1) {
			PWC_LowSpeedToHighSpeed();
		} else if (new_run_mode == 2) {
			PWC_LowSpeedToHighPerformance();
		}
	} else if (old_run_mode == 1) {
		if (new_run_mode == 0) {
			PWC_HighSpeedToLowSpeed();
		} else if (new_run_mode == 2) {
			PWC_HighSpeedToHighPerformance();
		}
	} else if (old_run_mode == 2) {
		if (new_run_mode == 0) {
			PWC_HighPerformanceToLowSpeed();
		} else if (new_run_mode == 1) {
			PWC_HighPerformanceToHighSpeed();
		}
	}
}
#elif defined (HC32F4A0)
static void hc32_run_mode_switch(uint32_t old_freq, uint32_t new_freq)
{
	uint8_t old_run_mode;
	uint8_t new_run_mode;

	new_run_mode = (new_freq >= 8000000) ? 1 : 0;
	old_run_mode = (old_freq >= 8000000) ? 1 : 0;

	if (new_run_mode > old_run_mode) {
		PWC_LowSpeedToHighSpeed();
	} else if (new_run_mode < old_run_mode) {
		PWC_HighSpeedToLowSpeed();
	}
}
#endif

static int hc32_clock_control_init(const struct device *dev)
{
	uint32_t old_core_freq;
	uint32_t new_core_freq;
	stc_clock_freq_t stcClockFreq;

	ARG_UNUSED(dev);
	CLK_GetClockFreq(&stcClockFreq);
	old_core_freq = stcClockFreq.u32SysclkFreq;
	/* Set bus clk div. */
	CLK_SetClockDiv(CLK_BUS_CLK_ALL, (HC32_HCLK_DIV(HC32_HCLK_PRESCALER) | \
									HC32_EXCLK_DIV(HC32_EXCLK_PRESCALER) | \
									HC32_PCLK(0, HC32_PCLK0_PRESCALER) | \
									HC32_PCLK(1, HC32_PCLK1_PRESCALER) | \
									HC32_PCLK(2, HC32_PCLK2_PRESCALER) | \
									HC32_PCLK(3, HC32_PCLK3_PRESCALER) | \
									HC32_PCLK(4, HC32_PCLK4_PRESCALER)));

	/* sram init include read/write wait cycle setting */
	SRAM_SetWaitCycle(SRAM_SRAM_ALL, SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);
	SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);

	/* flash read wait cycle setting */
	(void)EFM_SetWaitCycle(EFM_WAIT_CYCLE);
	/* 3 cycles for 126MHz ~ 200MHz */
	GPIO_SetReadWaitCycle(GPIO_RD_WAIT);

	hc32_clk_conf();

#if HC32_SYSCLK_SRC_PLL
	CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
#elif HC32_SYSCLK_SRC_XTAL
	CLK_SetSysClockSrc(CLK_SYSCLK_SRC_XTAL);
#elif HC32_SYSCLK_SRC_HRC
	CLK_SetSysClockSrc(CLK_SYSCLK_SRC_HRC);
#elif HC32_SYSCLK_SRC_MRC
	CLK_SetSysClockSrc(CLK_SYSCLK_SRC_MRC);
#endif

	CLK_GetClockFreq(&stcClockFreq);
	new_core_freq = stcClockFreq.u32SysclkFreq;
	hc32_run_mode_switch(old_core_freq, new_core_freq);

	return 0;
}

static int hc32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct hc32_modules_clock_sys *clk_sys = \
		(struct hc32_modules_clock_sys *)sub_system;
	struct hc32_modules_clock_config *mod_conf = \
		(struct hc32_modules_clock_config *)dev->config;

	if (IN_RANGE(clk_sys->fcg, HC32_CLK_FCG0, HC32_CLK_FCG3) == 0) {
		return -ENOTSUP;
	}

	sys_clear_bits(mod_conf->addr + HC32_CLK_MODULES_OFFSET(clk_sys->fcg), \
					HC32_CLK_MODULES_BIT(clk_sys->bits));

	return 0;
}

static int hc32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct hc32_modules_clock_sys *clk_sys = \
		(struct hc32_modules_clock_sys *)sub_system;
	struct hc32_modules_clock_config *mod_conf = \
		(struct hc32_modules_clock_config *)dev->config;

	if (IN_RANGE(clk_sys->fcg, HC32_CLK_FCG0, HC32_CLK_FCG3) == 0) {
		return -ENOTSUP;
	}

	sys_set_bits(mod_conf->addr + HC32_CLK_MODULES_OFFSET(clk_sys->fcg), \
					HC32_CLK_MODULES_BIT(clk_sys->bits));

	return 0;
}

static int hc32_clock_control_get_subsys_rate(const struct device *dev,
						 clock_control_subsys_t sub_system,
						 uint32_t *rate)
{
	struct hc32_modules_clock_sys *clk_sys = \
		(struct hc32_modules_clock_sys *)sub_system;

	ARG_UNUSED(dev);
	switch (clk_sys->bus)
	{
#if HC32_HRC_ENABLED
	case HC32_CLK_SRC_HRC:
		*rate = HC32_HRC_FREQ;
		break;
#endif
#if HC32_MRC_ENABLED
	case HC32_CLK_SRC_MRC:
		*rate = HC32_MRC_FREQ;
		break;
#endif
#if HC32_XTAL_ENABLED
	case HC32_CLK_SRC_XTAL:
		*rate = HC32_XTAL_FREQ;
		break;
#endif
#if HC32_PLL_ENABLED
	case HC32_CLK_SRC_PLL:
		*rate = HC32_PLL_FREQ;
		break;
#endif
	case HC32_CLK_BUS_HCLK:
		*rate = CORE_CLK_FREQ;
		break;
	case HC32_CLK_BUS_PCLK0:
		*rate = PCLK0_FREQ;
		break;
	case HC32_CLK_BUS_PCLK1:
		*rate = PCLK1_FREQ;
		break;
	case HC32_CLK_BUS_PCLK2:
		*rate = PCLK2_FREQ;
		break;
	case HC32_CLK_BUS_PCLK3:
		*rate = PCLK3_FREQ;
		break;
	case HC32_CLK_BUS_PCLK4:
		*rate = PCLK4_FREQ;
		break;
	case HC32_SYS_CLK:
		*rate = SYS_CLK_FREQ;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status hc32_clock_control_get_status(
						 const struct device *dev,
						 clock_control_subsys_t sys)
{
	struct hc32_modules_clock_sys *clk_sys = \
		(struct hc32_modules_clock_sys *)sys;
	struct hc32_modules_clock_config *mod_conf = \
		(struct hc32_modules_clock_config *)dev->config;

	if (IN_RANGE(clk_sys->fcg, HC32_CLK_FCG0, HC32_CLK_FCG3) == 0) {
		return -CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (sys_test_bit(mod_conf->addr + HC32_CLK_MODULES_OFFSET(clk_sys->fcg), \
					clk_sys->bits)) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	return CLOCK_CONTROL_STATUS_ON;
}

static int hc32_clock_control_configure(const struct device *dev,
						 clock_control_subsys_t sys,
						 void *data)
{
	uint32_t clk_sys = *(uint32_t *)sys;
	uint32_t dat = *(uint32_t *)data;

	ARG_UNUSED(dev);
	if (IN_RANGE(clk_sys, HC32_CLK_CONF_MIN, HC32_CLK_CONF_MAX) == 0) {
		return -ENOTSUP;
	}
	switch (clk_sys)
	{
	case HC32_CLK_CONF_PERI:
		CLK_SetPeriClockSrc((uint16_t)dat);
		break;
	case HC32_CLK_CONF_USB:
		CLK_SetUSBClockSrc((uint16_t)dat);
		break;
	case HC32_CLK_CONF_I2S:
		CLK_SetI2SClockSrc((uint8_t)(dat >> 8), (uint8_t)dat);
		break;
	case HC32_CLK_CONF_TPIU:
		CLK_SetTpiuClockDiv((uint8_t)dat & 0x03);
		if ((uint8_t)dat & 0x80) {
			CLK_TpiuClockCmd(ENABLE);
		} else {
			CLK_TpiuClockCmd(DISABLE);
		}
		break;
	case HC32_CLK_CONF_SRC:
		CLK_SetSysClockSrc((uint8_t)dat);
		break;
	case HC32_CLK_CONF_MCO:
		CLK_MCOConfig(CLK_MCO1, (uint8_t)dat, CLK_MCO_DIV8);
		CLK_MCOCmd(CLK_MCO1, ENABLE);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static struct clock_control_driver_api hc32_clock_control_api = {
	.on = hc32_clock_control_on,
	.off = hc32_clock_control_off,
	.get_rate = hc32_clock_control_get_subsys_rate,
	.get_status = hc32_clock_control_get_status,
	.configure = hc32_clock_control_configure,
};

static const struct hc32_modules_clock_config hc32_modules_clk= {
	.addr = DT_REG_ADDR(DT_NODELABEL(bus_fcg)),
};

DEVICE_DT_DEFINE(DT_NODELABEL(clk_sys),
			&hc32_clock_control_init,
			NULL,
			NULL, &hc32_modules_clk,
			PRE_KERNEL_1,
			1,
			&hc32_clock_control_api);