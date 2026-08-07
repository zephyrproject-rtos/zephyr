/* Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Architecture notes:
 * - ULP manager owns clock muxing/gating for ULP peripherals and applies static
 *   routes from device tree at boot.
 * - ULP sources are rooted in AON/HP clocks; this driver resolves parent rates
 *   through the generic clock-control API.
 * - Power-domain sequencing is kept local to each peripheral clock operation so
 *   on/off semantics remain aligned with hardware gates.
 * - clock-mux can be configrud at runtime by the user through the clock-control API
 *   but it needs to first call clock_control_configure() and then clock_control_on().
 */

#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs_siwx91x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "clock_control_silabs_siwx91x_common.h"

#include "rsi_power_save.h"
#include "rsi_rom_ulpss_clk.h"
#include "si91x_device.h"

#define DT_DRV_COMPAT silabs_siwx91x_cmu_ulp

LOG_MODULE_REGISTER(siwx91x_ulp_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct siwx91x_ulp_clock_config {
	ULPCLK_Type *ulpclk_reg;
};

struct siwx91x_ulp_clock_data {
	struct silabs_siwx91x_clock_control_config *ulp_clk_mux;
	size_t ulp_clk_mux_count;
};

/*
 * ULP Clock Register Value Mapping
 *
 * Maps ULP peripheral clocks to their available clock sources and corresponding register values.
 * Based on SiWx917 Family Reference Manual Section 6.14 MCU ULP Clock Architecture.
 */
static const struct {
	uint32_t clkid;
	uint32_t ref_clkid;
	uint32_t reg_value;
} clk_reg_map[] = {
	/*  clockid                ref_clkid        reg_value */
	/*     |                       |                    | */
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_REF_CPU, SIWX91X_CLK_GATED,       8 },

	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_PLL_INTF,    8 },
	{ SIWX91X_CLK_ULP_UART,    SIWX91X_CLK_GATED,       9 },

	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_PLL_I2S,     8 },
	{ SIWX91X_CLK_ULP_I2S,     SIWX91X_CLK_GATED,      15 },

	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_PLL_I2S,     8 },
	{ SIWX91X_CLK_ULP_ADC,     SIWX91X_CLK_GATED,      15 },

	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_SSI,     SIWX91X_CLK_GATED,      15 },

	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_REF_ULP,     0 },
	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_RC_KHZ,      2 },
	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_XTAL_KHZ,    3 },
	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_RC_MHZ,      4 },
	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_HP_REF_ULP,  6 },
	{ SIWX91X_CLK_ULP_TIMER,   SIWX91X_CLK_GATED,      15 },

	{ SIWX91X_CLK_ULP_REF_AON, SIWX91X_CLK_ULP_REF_CPU, 0 },
};

static uint32_t siwx91x_ulp_clock_get_reg(uint32_t clockid, uint32_t ref_clkid)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && ref_clkid == clk_reg_map[i].ref_clkid) {
			return clk_reg_map[i].reg_value;
		}
	}
	return UINT32_MAX;
}

static uint32_t siwx91x_ulp_clock_get_ref_clock(uint32_t clockid, uint32_t reg_value)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && reg_value == clk_reg_map[i].reg_value) {
			return clk_reg_map[i].ref_clkid;
		}
	}
	return UINT32_MAX;
}

static bool siwx91x_ulp_clk_valid(uint32_t clockid, uint32_t ref_clkid)
{
	return siwx91x_ulp_clock_get_reg(clockid, ref_clkid) != UINT32_MAX;
}

static bool siwx91x_ulp_clk_get_i2s_pll_status(void)
{
	const struct device *dev = siwx91x_clock_control_get_device(SIWX91X_CLK_PLL_I2S);
	int ret;

	ret = clock_control_get_status(dev, (clock_control_subsys_t)SIWX91X_CLK_PLL_I2S);

	return (ret == CLOCK_CONTROL_STATUS_ON) ? true : false;
}

static int siwx91x_ulp_clk_wait_switched(const struct device *dev, uint32_t clockid)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;
	ULPCLK_Type *ulpclk = config->ulpclk_reg;
	int ret = 0;

	switch (clockid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_PROC_CLK_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_UART:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_UART_CLK_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_I2S:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_I2S_CLK_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_TIMER:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_TIMER_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_SSI:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_SSI_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_ADC:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_AUXADC_b, 1000, NULL);
		break;
	case SIWX91X_CLK_ULP_I2C:
		ret = WAIT_FOR(ulpclk->CLOCK_STAUS_REG_b.CLOCK_SWITCHED_I2C_b, 1000, NULL);
		break;
	default:
		ret = true;
		break;
	}

	if (!ret) {
		return -EIO;
	}

	return 0;
}

static int siwx91x_ulp_clk_get_div_factor(const struct device *dev, uint32_t clockid,
					   uint32_t *div_factor)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;

	if (div_factor == NULL) {
		return -EINVAL;
	}

	switch (clockid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		*div_factor = config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP_PROC_CLK_DIV_FACTOR;
		break;
	case SIWX91X_CLK_ULP_UART:
		*div_factor = config->ulpclk_reg->ULP_UART_CLK_GEN_REG_b.ULP_UART_CLKDIV_FACTOR;
		break;
	case SIWX91X_CLK_ULP_I2S:
		*div_factor = config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_CLKDIV_FACTOR;
		break;
	case SIWX91X_CLK_ULP_SSI:
		*div_factor = config->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_SSI_CLK_DIV_FACTOR;
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		*div_factor = config->ulpclk_reg->SLP_SENSOR_CLK_REG_b.DIVISON_FACTOR;
		break;
	default:
		break;
	}

	return 0;
}

static void siwx91x_ulp_clk_apply_div_factor(const struct device *dev,
					    const struct silabs_siwx91x_clock_control_config *mux)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;

	switch (mux->clkid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP_PROC_CLK_DIV_FACTOR = mux->clock_div;
		break;
	case SIWX91X_CLK_ULP_UART:
		config->ulpclk_reg->ULP_UART_CLK_GEN_REG_b.ULP_UART_CLKDIV_FACTOR = mux->clock_div;
		break;
	case SIWX91X_CLK_ULP_I2S:
		config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_CLKDIV_FACTOR = mux->clock_div;
		break;
	case SIWX91X_CLK_ULP_SSI:
		config->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_SSI_CLK_DIV_FACTOR =
			mux->clock_div;
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		config->ulpclk_reg->SLP_SENSOR_CLK_REG_b.DIVISON_FACTOR = mux->clock_div;
		break;
	default:
		break;
	}

	return;
}

static int siwx91x_ulp_clk_config(const struct device *dev,
				  struct silabs_siwx91x_clock_control_config *mux)
{
	const struct siwx91x_ulp_clock_config *cfg = dev->config;
	uint32_t reg_val = siwx91x_ulp_clock_get_reg(mux->clkid, mux->ref_clkid);
	int ret;

	if (reg_val == UINT32_MAX) {
		return -EINVAL;
	}

	if (mux->ref_clkid == SIWX91X_CLK_XTAL_MHZ) {
		siwx91x_clock_request_xtal_to_nwp();
	}

	if (mux->ref_clkid == SIWX91X_CLK_PLL_I2S) {
		if (!siwx91x_ulp_clk_get_i2s_pll_status()) {
			LOG_ERR("I2S PLL clock is assigned to ULP_I2S but I2S PLL is not "
				"available");
		}
	}

	/* Disable the peripheral to disable the dynamic clocking before configuring the clk mux */
	switch (mux->clkid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		cfg->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP_PROC_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_ULP_UDMA:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_UDMA_CLK);
		break;
	case SIWX91X_CLK_ULP_UART:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_UART_CLK);
		cfg->ulpclk_reg->ULP_UART_CLK_GEN_REG_b.ULP_UART_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_ULP_GPIO:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_EGPIO_CLK);
		break;
	case SIWX91X_CLK_ULP_I2C:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_I2C_CLK);
		break;
	case SIWX91X_CLK_ULP_I2S:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_I2S_CLK);
		cfg->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_MASTER_SLAVE_MODE_b = 0;
		cfg->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_CLK_SEL_b = reg_val;
		/* In case of I2S PLL, we need to check that the I2S PLL is available.
		 * Few things to take care:
		 *  - I2S PLL is not supported in PS2 state. (PLL are shut down)
		 *  - When waking from PS4-SLEEP, I2S PLL needs to restart. That's why there is a
		 * possibility to bypass the I2S PLL while the I2S PLL
		 * TODO: Add I2S PLL bypass functionality
		 */
		break;
	case SIWX91X_CLK_ULP_ADC:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_AUX_CLK);
		cfg->ulpclk_reg->ULP_AUXADC_CLK_GEN_REG_b.ULP_AUX_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_ULP_SSI:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_SSI_CLK);
		cfg->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_SSI_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_ULP_TIMER:
		RSI_ULPSS_PeripheralDisable(cfg->ulpclk_reg, ULP_TIMER_CLK);
		cfg->ulpclk_reg->ULP_TIMER_CLK_GEN_REG_b.ULP_TIMER_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		/* Nothing to do, clock is hard linked to the ULP_PROC clock and there is no gate*/
		break;
	default:
		return -EINVAL;
	}

	ret = siwx91x_ulp_clk_wait_switched(dev, mux->clkid);
	if (ret) {
		return ret;
	}

	siwx91x_ulp_clk_apply_div_factor(dev, mux);

	return ret;
}

static int siwx91x_ulp_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;
	const struct siwx91x_ulp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	int ret;

	for (size_t i = 0; i < data->ulp_clk_mux_count; i++) {
		if (data->ulp_clk_mux[i].clkid != clockid) {
			continue;
		}
		ret = siwx91x_ulp_clk_config(dev, &data->ulp_clk_mux[i]);
		if (ret) {
			return ret;
		}
	}

	switch (clockid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		/* ULP_PROC is part of the Misc domain for power */
		config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP2M4_A2A_BRDG_CLK_EN_b = 1;
		break;
	case SIWX91X_CLK_ULP_UART:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_UART);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_UART_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_I2C:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_I2C);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_I2C_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_UDMA:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_UDMA);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_UDMA_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_I2S:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_I2S);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_I2S_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_STATIC_I2S:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_I2S);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_I2S_CLK, ENABLE_STATIC_CLK);
		config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_MASTER_SLAVE_MODE_b = 1;
		break;
	case SIWX91X_CLK_ULP_GPIO:
		/* ULP GPIO is part of the Misc domain for power */
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_EGPIO_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_ADC:
		RSI_IPMU_PowerGateSet(AUXADC_PG_ENB);
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_AUX);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_AUX_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_SSI:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_SSI);
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_SSI_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_TIMER:
		/* ULP_TIMER is part of the Misc domain for power */
		RSI_ULPSS_PeripheralEnable(config->ulpclk_reg, ULP_TIMER_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		config->ulpclk_reg->SLP_SENSOR_CLK_REG_b.ENABLE_b = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_ulp_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;
	uint32_t clockid = POINTER_TO_UINT(sys);

	/* Can't power down some peripheral because they are part of the Misc domain */

	switch (clockid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		/* ULP_PROC is part of the Misc domain for power */
		config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP2M4_A2A_BRDG_CLK_EN_b = 0;
		break;
	case SIWX91X_CLK_ULP_GPIO:
		/* ULP_GPIO is part of the Misc domain for power */
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_EGPIO_CLK);
		break;
	case SIWX91X_CLK_ULP_I2C:
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_I2C_CLK);
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_I2C);
		break;
	case SIWX91X_CLK_ULP_SSI:
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_SSI_CLK);
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_SSI);
		break;
	case SIWX91X_CLK_ULP_TIMER:
		/* ULP_TIMER is part of the Misc domain for power */
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_TIMER_CLK);
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		/* You don't want to do this, it will break the system */
		config->ulpclk_reg->SLP_SENSOR_CLK_REG_b.ENABLE_b = 0;
		break;
	case SIWX91X_CLK_ULP_UDMA:
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_UDMA_CLK);
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_UDMA);
		break;
	case SIWX91X_CLK_ULP_I2S:
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_I2S_CLK);
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_I2S);
		break;
	case SIWX91X_CLK_ULP_ADC:
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_AUX);
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_AUX_CLK);
		RSI_IPMU_PowerGateClr(AUXADC_PG_ENB);
		break;
	case SIWX91X_CLK_ULP_UART:
		RSI_ULPSS_PeripheralDisable(config->ulpclk_reg, ULP_UART_CLK);
		RSI_PS_UlpssPeriPowerDown(ULPSS_PWRGATE_ULP_UART);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int siwx91x_ulp_clock_get_rate(const struct device *dev, clock_control_subsys_t clk,
				      uint32_t *rate)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;
	uint32_t clockid = POINTER_TO_UINT(clk);
	const struct device *parent_clk_dev;
	uint32_t ref_clkid = UINT32_MAX;
	
	uint32_t div_factor = 0;
	uint32_t reg = 0;
	int ret = 0;

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = 0;

	switch (clockid) {
	case SIWX91X_CLK_GATED:
		*rate = 0;
		return 0;
	case SIWX91X_CLK_ULP_REF_CPU:
		reg = config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP_PROC_CLK_SEL;
		break;
	case SIWX91X_CLK_ULP_UART:
		reg = config->ulpclk_reg->ULP_UART_CLK_GEN_REG_b.ULP_UART_CLK_SEL;
		break;
	case SIWX91X_CLK_ULP_I2S:
		reg = config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_CLK_SEL_b;
		break;
	case SIWX91X_CLK_ULP_SSI:
		reg = config->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_SSI_CLK_SEL;
		break;
	case SIWX91X_CLK_ULP_TIMER:
		reg = config->ulpclk_reg->ULP_TIMER_CLK_GEN_REG_b.ULP_TIMER_CLK_SEL;
		break;
	case SIWX91X_CLK_ULP_ADC:
		reg = config->ulpclk_reg->ULP_AUXADC_CLK_GEN_REG_b.ULP_AUX_CLK_SEL;
		break;
	case SIWX91X_CLK_ULP_UDMA:
	case SIWX91X_CLK_ULP_GPIO:
	case SIWX91X_CLK_ULP_I2C:
	case SIWX91X_CLK_ULP_REF_AON:
		ref_clkid = SIWX91X_CLK_ULP_REF_CPU;
		break;
	default:
		return -EINVAL;
	}

	/* No fixed ref clock, get the ref clock from the register */
	if (ref_clkid == UINT32_MAX) {
		ref_clkid = siwx91x_ulp_clock_get_ref_clock(clockid, reg);
		if (ref_clkid == UINT32_MAX) {
			return -EINVAL;
		}
	}

	if (ref_clkid == SIWX91X_CLK_GATED) {
		*rate = 0U;
	} else {
		parent_clk_dev = siwx91x_clock_control_get_device(ref_clkid);
		if (parent_clk_dev == NULL) {
			return -EINVAL;
		}

		ret = clock_control_get_rate(parent_clk_dev,
					     (clock_control_subsys_t)ref_clkid, rate);
		if (ret != 0) {
			return ret;
		}
	}

	/* Apply the divider factor if any */
	ret = siwx91x_ulp_clk_get_div_factor(dev, clockid, &div_factor);
	if (ret) {
		return ret;
	}

	if (div_factor != 0 && div_factor != 1) {
		*rate = *rate / div_factor;
	}

	return ret;
}

static int siwx91x_ulp_clock_set_rate(const struct device *dev, clock_control_subsys_t sys,
				      clock_control_subsys_rate_t raw_rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);
	ARG_UNUSED(raw_rate);

	return -ENOTSUP;
}

static int siwx91x_ulp_clock_configure(const struct device *dev, clock_control_subsys_t sys,
				       void *cfg)
{
	struct silabs_siwx91x_clock_control_config *new_cfg = cfg;
	struct siwx91x_ulp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	uint32_t ret;

	if (new_cfg == NULL) {
		return -EINVAL;
	}

	if (new_cfg->clkid != clockid) {
		return -EINVAL;
	}

	ret = siwx91x_ulp_clk_valid(new_cfg->clkid, new_cfg->ref_clkid);
	if (!ret) {
		return -EINVAL;
	}

	for (size_t i = 0; i < data->ulp_clk_mux_count; i++) {
		if (data->ulp_clk_mux[i].clkid == new_cfg->clkid) {
			data->ulp_clk_mux[i].ref_clkid = new_cfg->ref_clkid;
			data->ulp_clk_mux[i].clock_div = new_cfg->clock_div;
			return 0;
		}
	}

	return -EINVAL;
}

static enum clock_control_status siwx91x_ulp_clock_get_status(const struct device *dev,
							      clock_control_subsys_t sys)
{
	const struct siwx91x_ulp_clock_config *config = dev->config;
	uint32_t clockid = POINTER_TO_UINT(sys);
	bool val;

	/* Theoricaly, we need to verify that dynamic clocking is disabled before returning the
	 * status of a clock but in practice, it is not needed because the defined clocks are
	 * configured in static mode. */

	switch (clockid) {
	case SIWX91X_CLK_ULP_REF_CPU:
		val = config->ulpclk_reg->ULP_TA_CLK_GEN_REG_b.ULP2M4_A2A_BRDG_CLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_UART:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.PCLK_ENABLE_UART_b &&
		      config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.SCLK_ENABLE_UART_b;
		break;
	case SIWX91X_CLK_ULP_I2C:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.PCLK_ENABLE_I2C_b &&
		      config->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_I2C_CLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_UDMA:
		val = config->ulpclk_reg->ULP_DYN_CLK_CTRL_DISABLE_b.UDMA_CLK_ENABLE_b;
		break;
	case SIWX91X_CLK_ULP_I2S:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.CLK_ENABLE_I2S_b &&
		      config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_CLK_EN_b &&
		      config->ulpclk_reg->ULP_I2S_CLK_GEN_REG_b.ULP_I2S_PCLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_ADC:
		val = config->ulpclk_reg->ULP_DYN_CLK_CTRL_DISABLE_b.AUX_CLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_SSI:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.SCLK_ENABLE_SSI_MASTER_b &&
		      config->ulpclk_reg->ULP_I2C_SSI_CLK_GEN_REG_b.ULP_SSI_CLK_EN_b &&
		      config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.PCLK_ENABLE_SSI_MASTER_b;
		break;
	case SIWX91X_CLK_ULP_TIMER:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.CLK_ENABLE_TIMER_b &&
		      config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.TIMER_PCLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_GPIO:
		val = config->ulpclk_reg->ULP_MISC_SOFT_SET_REG_b.EGPIO_CLK_EN_b;
		break;
	case SIWX91X_CLK_ULP_REF_AON:
		val = config->ulpclk_reg->SLP_SENSOR_CLK_REG_b.ENABLE_b;
		break;
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return val ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int siwx91x_ulp_clock_init(const struct device *dev)
{
	const struct siwx91x_ulp_clock_data *data = dev->data;
	int ret;

	/* Enable the Misc domain power - This is where we control the clocks for the ULP domain.
	 * It is "on" by default at startup, but we need to make sure it is enabled.
	 * This also means that the ULP clock driver will need to be linked with the ULP power
	 * domain in the future. If we don't do this, we will not be able to control the clocks for
	 * the ULP peripherals (clock_control_on() will not work). This is not necessary at the
	 * moment because no one explicitly shuts down the ULP_MISC domain.
	 */
	RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_MISC);

	/* By default, most ULP clocks are dynamic and managed automatically by the hardware.
	 * All clocks present in the device tree are set to static mode.
	 * Once a clock has been configured, it will always remain in static mode. We do not allow
	 * switching it back to dynamic mode, in order to match the hardware description of the
	 * clock in the device tree.
	 */

	/* ULP_PROC is the bus clock for the ULP peripherals, and SLP_SENSOR is the bus clock for
	 * the UULP peripherals. These are enabled by default at startup, which is why
	 * clock_control_on() is not necessary for these clocks. It is possible to turn off these
	 * clocks, but this must be done with caution, as it will disable all ULP and UULP
	 * peripherals and break peripherals that are directly mapped to the ULP bus clock (UDMA,
	 * I2C, and ULP_GPIO).
	 */

	/* Note: ulpclk_reg->M4LP_CTRL_REG is not used at the moment. This register allows
	 * enabling the clock for the M4 Core and M4 memories when M4_SOC_CLK_SEL switches to
	 * SLEEP_CLK (HP clock driver). This is not currently used, and WiseConnect uses it only
	 * when transitioning from PS4 to PS2. When going to PS4-SLEEP, we manually switch the
	 * M4_SOC_CLK_SEL to ref_ulp_mux.
	 */

	for (size_t i = 0; i < data->ulp_clk_mux_count; i++) {
		ret = siwx91x_ulp_clk_valid(data->ulp_clk_mux[i].clkid,
					    data->ulp_clk_mux[i].ref_clkid);
		if (!ret) {
			LOG_ERR("Invalid ULP mux clockid %u ref %u", data->ulp_clk_mux[i].clkid,
				data->ulp_clk_mux[i].ref_clkid);
			continue;
		}
		ret = siwx91x_ulp_clk_config(dev, &data->ulp_clk_mux[i]);
		if (ret) {
			LOG_ERR("ULP mux %zu failed: %d", i, ret);
		}
	}

	return 0;
}

static DEVICE_API(clock_control, siwx91x_ulp_clock_api) = {
	.on = siwx91x_ulp_clock_on,
	.off = siwx91x_ulp_clock_off,
	.get_rate = siwx91x_ulp_clock_get_rate,
	.set_rate = siwx91x_ulp_clock_set_rate,
	.configure = siwx91x_ulp_clock_configure,
	.get_status = siwx91x_ulp_clock_get_status,
};

#define SIWX91X_ULP_CLOCK_INIT(inst)                                                               \
	static struct silabs_siwx91x_clock_control_config ulp_clk_mux_##inst[] = {                 \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SIWX91X_CLOCK_MANAGER_CHILD_INIT)};        \
	static const struct siwx91x_ulp_clock_config siwx91x_ulp_clock_config_##inst = {           \
		.ulpclk_reg = (ULPCLK_Type *)DT_INST_REG_ADDR_BY_NAME(inst, ulpclk)                \
	};                                                                                         \
	static struct siwx91x_ulp_clock_data siwx91x_ulp_clock_data_##inst = {                     \
		.ulp_clk_mux = ulp_clk_mux_##inst,                                                 \
		.ulp_clk_mux_count = ARRAY_SIZE(ulp_clk_mux_##inst)                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_ulp_clock_init, NULL, &siwx91x_ulp_clock_data_##inst,  \
			      &siwx91x_ulp_clock_config_##inst, PRE_KERNEL_1,                      \
			      CONFIG_SIWX91X_ULP_CLOCK_INIT_PRIORITY, &siwx91x_ulp_clock_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_ULP_CLOCK_INIT)
