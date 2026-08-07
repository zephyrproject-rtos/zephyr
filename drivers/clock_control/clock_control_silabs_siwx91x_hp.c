/* Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Architecture notes:
 * - HP manager controls high-performance peripheral clocks and all PLL
 *   programming (SOC/INTF/I2S) for the M4 domain.
 * - Device tree provides two boot-time datasets: route muxes and PLL frequency
 *   targets; init applies them in that order after moving HP_PROC to a safe ref.
 * - Runtime API keeps routing/gating local to HP clocks and resolves parent
 *   rates through the common clock-control graph.
 * - clock-mux can be configured at runtime by the user through the clock-control API
 *   but it needs to first call clock_control_configure() and then clock_control_on().
 */

#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs_siwx91x.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#include "clock_control_silabs_siwx91x_common.h"

#include "rsi_pll.h"
#include "rsi_power_save.h"
#include "rsi_rom_clks.h"

#include "si91x_device.h"
#include "sl_si91x_power_manager.h"

#define SIWX91X_HP_PREFETCH_LIMIT_HZ        (120000000UL) /* PREFETCH_FREQUENCY_LIMIT */
#define SIWX91X_HP_MAX_PLL_FREQUENCY_HZ     (180000000UL) /* MAX_PLL_FREQUENCY */
#define SIWX91X_HP_LOW_VOLTAGE_PLL_LIMIT_HZ (90000000UL)

#define DT_DRV_COMPAT silabs_siwx91x_cmu_hp

LOG_MODULE_REGISTER(siwx91x_hp_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct siwx91x_hp_clock_config {
	M4CLK_Type *m4clk_reg;
	struct silabs_siwx91x_clock_control_pll_config *pll_cfg;
	size_t pll_cfg_count;
	const struct pinctrl_dev_config *pcfg;
};

struct siwx91x_hp_clock_data {
	struct silabs_siwx91x_clock_control_config *hp_clk_mux;
	size_t hp_clk_mux_count;
	/* These values can't be retrieved from the hardware. We have to remember them */
	uint64_t hp_enabled_bitmap;
	uint32_t soc_pll_hz;
	uint32_t intf_pll_hz;
	uint32_t i2s_pll_hz;
};

static const struct {
	uint32_t clkid;
	uint32_t ref_clkid;
	uint32_t reg_value;
} clk_reg_map[] = {
	/*clockid             ref_clkid        reg_value */
	/*   |                     |                   | */
	{ SIWX91X_CLK_CPU_LP,     SIWX91X_CLK_RC_KHZ,    0 },
	{ SIWX91X_CLK_CPU_LP,     SIWX91X_CLK_XTAL_KHZ,  1 },

	{ SIWX91X_CLK_CPU,        SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_CPU,        SIWX91X_CLK_PLL_SOC,   2 },
	{ SIWX91X_CLK_CPU,        SIWX91X_CLK_PLL_INTF,  4 },
	{ SIWX91X_CLK_CPU,        SIWX91X_CLK_CPU_LP,    5 },

	{ SIWX91X_CLK_QSPI,       SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_QSPI,       SIWX91X_CLK_PLL_INTF,  1 },
	{ SIWX91X_CLK_QSPI,       SIWX91X_CLK_PLL_SOC,   3 },

	{ SIWX91X_CLK_QSPI2,      SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_QSPI2,      SIWX91X_CLK_PLL_INTF,  1 },
	{ SIWX91X_CLK_QSPI2,      SIWX91X_CLK_PLL_SOC,   3 },

	{ SIWX91X_CLK_UART0,      SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_UART0,      SIWX91X_CLK_PLL_SOC,   1 },
	{ SIWX91X_CLK_UART0,      SIWX91X_CLK_PLL_INTF,  3 },

	{ SIWX91X_CLK_UART1,      SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_UART1,      SIWX91X_CLK_PLL_SOC,   1 },
	{ SIWX91X_CLK_UART1,      SIWX91X_CLK_PLL_INTF,  3 },

	{ SIWX91X_CLK_SSI,        SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_SSI,        SIWX91X_CLK_PLL_SOC,   1 },
	{ SIWX91X_CLK_SSI,        SIWX91X_CLK_PLL_INTF,  3 },

	{ SIWX91X_CLK_I2S,        SIWX91X_CLK_PLL_I2S,   0 },

	{ SIWX91X_CLK_GSPI,       SIWX91X_CLK_REF_HP,    0 },
	{ SIWX91X_CLK_GSPI,       SIWX91X_CLK_PLL_SOC,   1 },
	{ SIWX91X_CLK_GSPI,       SIWX91X_CLK_PLL_INTF,  3 },

	{ SIWX91X_CLK_HP_REF_PLL, SIWX91X_CLK_XTAL_MHZ,  0 },
	{ SIWX91X_CLK_HP_REF_PLL, SIWX91X_CLK_RC_MHZ,    1 },

	{ SIWX91X_CLK_HP_REF_ULP, SIWX91X_CLK_CPU,       0 },

	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_RC_MHZ,    1 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_XTAL_MHZ,  2 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_RC_KHZ,    7 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_XTAL_KHZ,  8 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_PLL_INTF, 10 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_PLL_SOC,  13 },
	{ SIWX91X_CLK_PIN_OUT,    SIWX91X_CLK_PLL_I2S,  14 },
};

static void siwx91x_hp_clk_proc_change_update()
{
	uint32_t rate;

	if (clock_control_get_rate(siwx91x_clock_control_get_device(SIWX91X_CLK_CPU),
				   (clock_control_subsys_t)SIWX91X_CLK_CPU, &rate) != 0) {
		return;
	}

	/* Historically, only attach to PLL (because only PLL can go above 120MHz) */
	if (rate >= SIWX91X_HP_PREFETCH_LIMIT_HZ) {
		RSI_PS_PS4SetRegisters();
	} else {
		RSI_PS_PS4ClearRegisters();
	}

	if (IS_ENABLED(CONFIG_CORTEX_M_SYSTICK)) {
		/* Reconfigure the system tick timer after changing the core clock. */
		SysTick_Config(rate / 1000);
	}
}

/* PLL code*/

/*
 * PLL analog programming (SOC / INTF / I2S).
 *
 * Unlike M4CLK (0x46000000) and MISC_CONFIG (0x46008000), the PLL macro registers
 * are not on the M4 APB bus. The CPU reaches them through REG_SPI_PLL at 0x46180000:
 * each load/store via SPI_MEM_MAP_PLL() (base + 0x8000 + index*4, see rsi_reg_spi.h)
 * triggers an SPI transaction to the analog block.
 *
 * Lock status is the exception: M4CLK PLL_STAT_REG reflects SOC/INTF/I2S lock flags.
 *
 * Each PLL analog macro exposes a *_CTRL_REG9 (SPI-mapped, see rsi_pll.h) that
 * selects the manual lock-detection path. WiseConnect hardcodes 0xD900:
 *
 *   0xD900 = 0b1101_1001_0000_0000
 *     [15] MANUAL_LOCK_ENABLE  = 1  — use manual-mode phase lock generation
 *     [14] BYPASS_LOCK_PLL     = 1  — bypass phase-detector logic in manual mode
 *     [13:6] MM_COUNT_LIMIT    = 0x64 (100) — debounce count before lock is asserted
 *     [5:0]                    = 0  — not documented in the public SDK headers
 *
 */
static int siwx91x_hp_set_pll_freq(const struct device *dev, uint32_t pll_clkid, uint32_t pll_freq)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	struct siwx91x_hp_clock_data *data = dev->data;
	rsi_error_t ret = RSI_OK;
	uint32_t ref_clk_rate;

	if (pll_freq > SIWX91X_HP_MAX_PLL_FREQUENCY_HZ) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(dev, (clock_control_subsys_t)SIWX91X_CLK_HP_REF_PLL,
				     &ref_clk_rate);
	if (ret != 0) {
		return ret;
	}

	switch (pll_clkid) {
	case SIWX91X_CLK_PLL_SOC:
		RSI_CLK_SocPllTurnOn();
		SPI_MEM_MAP_PLL(SOC_PLL_500_CTRL_REG9) = 0xD900;

		ret = RSI_CLK_SetSocPllFreq2(cfg->m4clk_reg, pll_freq, ref_clk_rate);
		if (ret != RSI_OK) {
			return ret;
		}

		data->soc_pll_hz = pll_freq;
		if (pll_freq < SIWX91X_HP_LOW_VOLTAGE_PLL_LIMIT_HZ) {
			RSI_PS_PowerStateChangePs4toPs3();
			RSI_PS_SetDcDcToLowerVoltage();
		} else {
			RSI_PS_SetDcDcToHigerVoltage();
			RSI_PS_PowerStateChangePs3toPs4();
		}
		break;

	case SIWX91X_CLK_PLL_INTF:
		RSI_CLK_IntfPLLTurnOn();
		SPI_MEM_MAP_PLL(INTF_PLL_500_CTRL_REG9) = 0xD900;

		ret = RSI_CLK_SetIntfPllFreq2(cfg->m4clk_reg, pll_freq, ref_clk_rate);
		if (ret != RSI_OK) {
			return ret;
		}

		data->intf_pll_hz = pll_freq;
		break;

	case SIWX91X_CLK_PLL_I2S:
		RSI_CLK_I2sPllTurnOn();
		SPI_MEM_MAP_PLL(I2S_PLL_CTRL_REG9) = 0xD900;

		ret = RSI_CLK_SetI2sPllFreq2(cfg->m4clk_reg, pll_freq, ref_clk_rate);
		if (ret != RSI_OK) {
			return ret;
		}
		data->i2s_pll_hz = pll_freq;
		break;

	default:
		break;
	}

	return ret;
}

static int siwx91x_check_pll_lock(const struct device *dev, uint32_t pll_clkid)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;

	switch (pll_clkid) {
	case SIWX91X_CLK_PLL_SOC:
		if (cfg->m4clk_reg->PLL_STAT_REG_b.SOCPLL_LOCK != 1) {
			return -EBUSY;
		}
		break;
	case SIWX91X_CLK_PLL_INTF:
		if (cfg->m4clk_reg->PLL_STAT_REG_b.INTFPLL_LOCK != 1) {
			return -EBUSY;
		}
		break;
	case SIWX91X_CLK_PLL_I2S:
		if (cfg->m4clk_reg->PLL_STAT_REG_b.I2SPLL_LOCK != 1) {
			return -EBUSY;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* End PLL code */

/* Legacy functions required by power manager */

#define PS4_PERFORMANCE_MODE_SOC_FREQ  (180000000UL) /* PS4 high-power SOC PLL frequency */
#define PS4_PERFORMANCE_MODE_INTF_FREQ (160000000UL) /* PS4 high-power INTF PLL frequency */
#define PS4_POWERSAVE_MODE_FREQ        (100000000UL) /* PS4 low-power frequency */
#define PS3_PERFORMANCE_MODE_FREQ      (80000000UL)  /* PS3 high-power frequency */
#define PS3_POWERSAVE_MODE_FREQ        (40000000UL)  /* PS3 low-power frequency */

static int siwx91x_clk_cfg(uint32_t clockid, uint32_t ref_clkid, uint32_t clock_div)
{
	struct silabs_siwx91x_clock_control_config clk_cfg = {
		.clkid = clockid,
		.ref_clkid = ref_clkid,
		.clock_div = clock_div,
	};

	return clock_control_configure(siwx91x_clock_control_get_device(clockid),
				       (clock_control_subsys_t)clockid, &clk_cfg);
}

static int siwx91x_clk_set_rate_pll(uint32_t clockid, uint32_t ref_clkid, uint32_t freq)
{
	return clock_control_set_rate(siwx91x_clock_control_get_device(clockid),
				      (clock_control_subsys_t)clockid,
				      (clock_control_subsys_rate_t)freq);
}

static int siwx91x_clk_on(uint32_t clockid)
{
	return clock_control_on(siwx91x_clock_control_get_device(clockid),
				(clock_control_subsys_t)clockid);
}

/* This function is used by the power manager to turn on/off the PLLs */
sl_status_t sl_si91x_clock_manager_control_pll(PLL_TYPE_T pll_type, bool enable)
{
	sl_status_t status = SL_STATUS_OK;

	switch (pll_type) {
	case SOC_PLL:
		/* Turn On/Off the SOC PLL */
		enable ? RSI_CLK_SocPllTurnOn() : RSI_CLK_SocPllTurnOff();
		break;

	case INTF_PLL:
		/* Turn On/Off the INTF PLL */
		enable ? RSI_CLK_IntfPLLTurnOn() : RSI_CLK_IntfPLLTurnOff();
		break;

	case I2S_PLL:
		/* Turn On/Off the I2S PLL */
		enable ? RSI_CLK_I2sPllTurnOn() : RSI_CLK_I2sPllTurnOff();
		break;

	default:
		status = SL_STATUS_INVALID_PARAMETER;
		break;
	}

	return status;
}

int siwx91x_clk_prepare_sleep(void)
{
	const struct device *hp_dev = siwx91x_clock_control_get_device(SIWX91X_CLK_CPU);
	struct siwx91x_hp_clock_data *hp_data = hp_dev->data;

	/* Change ref clocks to RC clock before moving to PS2/Sleep and not requested from PS2 */
	if (sl_si91x_power_manager_get_current_state() != SL_SI91X_POWER_MANAGER_PS2) {
		/* Change Subsystems' ref clocks from 40MHz XTAL to MHz RC */
		siwx91x_clk_cfg(SIWX91X_CLK_REF_HP, SIWX91X_CLK_RC_MHZ, 0);
		siwx91x_clk_cfg(SIWX91X_CLK_REF_ULP, SIWX91X_CLK_RC_MHZ, 0);

		/* Configure M4 source to HP REF clock (not depending on XTAL_CLK) */
		siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_REF_HP, 0);
		siwx91x_clk_on(SIWX91X_CLK_CPU);

		/* Update prefetch optimization + systick after core clock update*/
		siwx91x_hp_clk_proc_change_update();

		/* Switch QSPI and QSPI2 clocks to HP REF clock (not depending on XTAL_CLK) */
		siwx91x_clk_cfg(SIWX91X_CLK_QSPI, SIWX91X_CLK_REF_HP, 0);
		siwx91x_clk_cfg(SIWX91X_CLK_QSPI2, SIWX91X_CLK_REF_HP, 0);

		/* Apply new configuration */
		siwx91x_clk_on(SIWX91X_CLK_QSPI);
		siwx91x_clk_on(SIWX91X_CLK_QSPI2);
	}

	/* Reset the enabled bitmap */
	hp_data->hp_enabled_bitmap = 0U;

	return 0;
}

sl_status_t sli_si91x_clock_manager_config_clks_on_ps_change(sl_power_state_t power_state,
							     boolean_t power_mode)
{
	sl_status_t sli_status = SL_STATUS_OK;
	uint32_t freq;

	/* TODO: Add PS2 support */

	switch (power_state) {
	case SL_SI91X_POWER_MANAGER_PS4:

		/* Configure Ref clocks to 40MHz crystal */
		siwx91x_clk_cfg(SIWX91X_CLK_REF_HP, SIWX91X_CLK_XTAL_MHZ, 0);
		siwx91x_clk_cfg(SIWX91X_CLK_REF_ULP, SIWX91X_CLK_XTAL_MHZ, 0);

		/* Switch proc to HP ref clock */
		siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_REF_HP, 0);
		siwx91x_clk_on(SIWX91X_CLK_CPU);

		/* Set SOC PLL */
		if (power_mode == SL_SI91X_POWER_MANAGER_PERFORMANCE) {
			freq = PS4_PERFORMANCE_MODE_SOC_FREQ;
		} else {
			freq = PS4_POWERSAVE_MODE_FREQ;
		}
		siwx91x_clk_set_rate_pll(SIWX91X_CLK_PLL_SOC, SIWX91X_CLK_HP_REF_PLL, freq);

		/* Switch proc to SOC PLL */
		siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_PLL_SOC, 1);
		siwx91x_clk_on(SIWX91X_CLK_CPU);

		/* Update prefetch optimization + systick after core clock update*/
		siwx91x_hp_clk_proc_change_update();

		/* Set INTF PLL based on current state and mode */
		if (power_mode == SL_SI91X_POWER_MANAGER_PERFORMANCE) {
			freq = PS4_PERFORMANCE_MODE_INTF_FREQ;
			siwx91x_clk_set_rate_pll(SIWX91X_CLK_PLL_INTF, SIWX91X_CLK_HP_REF_PLL,
						 freq);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI, SIWX91X_CLK_PLL_INTF, 1);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI2, SIWX91X_CLK_PLL_INTF, 1);

		} else {
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI, SIWX91X_CLK_REF_HP, 1);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI2, SIWX91X_CLK_REF_HP, 1);
		}

		siwx91x_clk_on(SIWX91X_CLK_QSPI);
		siwx91x_clk_on(SIWX91X_CLK_QSPI2);
		break;

	case SL_SI91X_POWER_MANAGER_PS3:

		/* Configure Ref clocks to 40MHz crystal */
		siwx91x_clk_cfg(SIWX91X_CLK_REF_HP, SIWX91X_CLK_XTAL_MHZ, 0);
		siwx91x_clk_cfg(SIWX91X_CLK_REF_ULP, SIWX91X_CLK_XTAL_MHZ, 0);

		/* Switch proc to HP ref clock */
		siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_REF_HP, 0);
		siwx91x_clk_on(SIWX91X_CLK_CPU);

		/* Switch proc to SOC PLL depending on power_mode */
		if (power_mode) {
			/* Set SOC PLL */
			freq = PS3_PERFORMANCE_MODE_FREQ;
			siwx91x_clk_set_rate_pll(SIWX91X_CLK_PLL_SOC, SIWX91X_CLK_HP_REF_PLL, freq);
			siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_PLL_SOC, 1);
			siwx91x_clk_on(SIWX91X_CLK_CPU);
		}

		/* Update prefetch optimization + systick after core clock update*/
		siwx91x_hp_clk_proc_change_update();

		/* Set INTF PLL based on current state and mode */
		if (power_mode) {
			freq = PS3_PERFORMANCE_MODE_FREQ;
			siwx91x_clk_set_rate_pll(SIWX91X_CLK_PLL_INTF, SIWX91X_CLK_HP_REF_PLL,
						 freq);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI, SIWX91X_CLK_PLL_INTF, 1);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI2, SIWX91X_CLK_PLL_INTF, 1);
		} else {
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI, SIWX91X_CLK_REF_HP, 2);
			siwx91x_clk_cfg(SIWX91X_CLK_QSPI2, SIWX91X_CLK_REF_HP, 2);
		}

		siwx91x_clk_on(SIWX91X_CLK_QSPI);
		siwx91x_clk_on(SIWX91X_CLK_QSPI2);
		break;

	case SL_SI91X_POWER_MANAGER_SLEEP:
		/* Power modes are not applicable for Sleep state */
		UNUSED_PARAMETER(power_mode);

		/* Configure clocks as per sleep state */
		sli_status = siwx91x_clk_prepare_sleep();
		break;

	case SL_SI91X_POWER_MANAGER_PS1:
	case SL_SI91X_POWER_MANAGER_PS0:
	case SL_SI91X_POWER_MANAGER_STANDBY:
		/* Not needed for these states */
		sli_status = SL_STATUS_INVALID_STATE;
		break;

	default:
		/* If reaches here, return error code */
		sli_status = SL_STATUS_INVALID_PARAMETER;
		break;
	}

	return sli_status;
}

/* STOP LEGACY FUNCTIONS TO BE COMPLIANT WITH POWER MANAGER*/

static uint32_t siwx91x_hp_clock_get_reg(uint32_t clockid, uint32_t ref_clkid)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && ref_clkid == clk_reg_map[i].ref_clkid) {
			return clk_reg_map[i].reg_value;
		}
	}
	return UINT32_MAX;
}

static uint32_t siwx91x_hp_clock_get_ref_clock(uint32_t clockid, uint32_t reg_value)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_reg_map); i++) {
		if (clockid == clk_reg_map[i].clkid && reg_value == clk_reg_map[i].reg_value) {
			return clk_reg_map[i].ref_clkid;
		}
	}
	return UINT32_MAX;
}

static bool siwx91x_hp_clk_valid(uint32_t clockid, uint32_t ref_clkid)
{
	return siwx91x_hp_clock_get_reg(clockid, ref_clkid) != UINT32_MAX;
}

static int siwx91x_hp_clk_get_div_factor(const struct device *dev, uint32_t clockid,
					  uint32_t *div_factor)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	uint32_t reg;

	if (div_factor == NULL) {
		return -EINVAL;
	}

	switch (clockid) {
	case SIWX91X_CLK_CPU:
		*div_factor = cfg->m4clk_reg->CLK_CONFIG_REG5_b.M4_SOC_CLK_DIV_FAC;
		break;
	case SIWX91X_CLK_QSPI: {
		reg = cfg->m4clk_reg->CLK_CONFIG_REG1_b.QSPI_CLK_DIV_FAC;

		*div_factor = 2 * (reg != 0U ? reg : 1U);
		break;
	}
	case SIWX91X_CLK_QSPI2: {
		reg = cfg->m4clk_reg->CLK_CONFIG_REG6_b.QSPI_2_CLK_DIV_FAC;

		*div_factor = 2 * (reg != 0U ? reg : 1U);
		break;
	}
	case SIWX91X_CLK_UART0:
		*div_factor = cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART1_SCLK_DIV_FAC;
		break;
	case SIWX91X_CLK_UART1:
		*div_factor = cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART2_SCLK_DIV_FAC;
		break;
	case SIWX91X_CLK_SSI:
		*div_factor = cfg->m4clk_reg->CLK_CONFIG_REG1_b.SSI_MST_SCLK_DIV_FAC;
		break;
	case SIWX91X_CLK_I2S: {
		reg = cfg->m4clk_reg->CLK_CONFIG_REG5_b.I2S_CLK_DIV_FAC;

		*div_factor = 2 * (reg != 0U ? reg : 1U);
		break;
	}
	case SIWX91X_CLK_HP_REF_ULP: {
		reg = cfg->m4clk_reg->CLK_CONFIG_REG4_b.ULPSS_CLK_DIV_FAC;

		*div_factor = 2 * (reg != 0U ? reg : 1U);
		break;
	}
	case SIWX91X_CLK_PIN_OUT: {
		reg = cfg->m4clk_reg->CLK_CONFIG_REG3_b.MCU_CLKOUT_DIV_FAC;

		*div_factor = 2 * (reg != 0U ? reg : 1U);
		break;
	}
	default:
		break;
	}

	return 0;
}

static int siwx91x_hp_clk_wait_switched(const struct device *dev, uint32_t clockid)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	int ret = 0;

	switch (clockid) {
	case SIWX91X_CLK_CPU:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.M4_SOC_CLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_CPU_LP:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.SLEEP_CLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_QSPI:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.QSPI_CLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_QSPI2:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.QSPI_2_CLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_UART0:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.USART1_SCLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_UART1:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.USART2_SCLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_SSI:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.SSI_MST_SCLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_I2S:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.I2S_CLK_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_GSPI:
		/* GEN_SPI_MST1_SCLK_SWITCHED is unreliable; do not block here. */
		return 0;
	case SIWX91X_CLK_PIN_OUT:
		ret = WAIT_FOR(cfg->m4clk_reg->PLL_STAT_REG_b.MCU_CLKOUT_SWITCHED, 1000, NULL);
		break;
	case SIWX91X_CLK_HP_REF_ULP:
	case SIWX91X_CLK_HP_REF_PLL:
		return 0;
	default:
		return 0;
	}

	if (!ret) {
		return -EIO;
	}

	return 0;
}

static int siwx91x_hp_clk_apply_div_factor(const struct device *dev,
					   const struct silabs_siwx91x_clock_control_config *mux)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;

	if (mux->clock_div == 0U) {
		return 0;
	}

	switch (mux->clkid) {
	case SIWX91X_CLK_CPU:
		cfg->m4clk_reg->CLK_CONFIG_REG5_b.M4_SOC_CLK_DIV_FAC = mux->clock_div;
		break;
	case SIWX91X_CLK_QSPI:
		/* TODO: Implement ODD/SWALLOW selection for div */
		cfg->m4clk_reg->CLK_CONFIG_REG1_b.QSPI_CLK_DIV_FAC = mux->clock_div;
		break;
	case SIWX91X_CLK_QSPI2:
		/* TODO: Implement ODD/SWALLOW selection for div */
		cfg->m4clk_reg->CLK_CONFIG_REG6_b.QSPI_2_CLK_DIV_FAC = mux->clock_div;
		break;
	case SIWX91X_CLK_UART0:
		/* Swallow is default -> clk_out = clk_in/USARTn_SCLK_DIV_FAC */
		cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART1_SCLK_DIV_FAC = mux->clock_div;
		/* TODO: implement FRAC USARTn_SCLK_FRAC_SEL=1 ->
		 * clk_out = clk_in/(USARTn_SCLK_DIV_FAC+0.5)
		 */
		break;
	case SIWX91X_CLK_UART1:
		/* Swallow is default -> clk_out = clk_in/USARTn_SCLK_DIV_FAC */
		cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART2_SCLK_DIV_FAC = mux->clock_div;
		/* TODO: implement FRAC USARTn_SCLK_FRAC_SEL=1 ->
		 * clk_out = clk_in/(USARTn_SCLK_DIV_FAC+0.5)
		 */
		break;
	case SIWX91X_CLK_SSI:
		cfg->m4clk_reg->CLK_CONFIG_REG1_b.SSI_MST_SCLK_DIV_FAC = mux->clock_div;
		break;
	case SIWX91X_CLK_I2S:
		/* clk_out = clk_in/(2*I2S_CLK_DIV_FAC) */
		/* if clock-div is 0, clk_out = clk_in */
		cfg->m4clk_reg->CLK_CONFIG_REG5_b.I2S_CLK_DIV_FAC = mux->clock_div;
		break;
	case SIWX91X_CLK_HP_REF_ULP:
		/* EVEN is default -> clk_out = clk_in/(2*ULPSS_CLK_DIV_FAC) */
		cfg->m4clk_reg->CLK_CONFIG_REG4_b.ULPSS_CLK_DIV_FAC = mux->clock_div;
		/* TODO: implement ODD div selection
		 * ULPSS_ODD_DIV_SEL=1 -> clk_out = clk_in/(2*ULPSS_CLK_DIV_FAC+1)
		 */
		break;
	case SIWX91X_CLK_PIN_OUT:
		/* clk_out = clk_in/(2*MCU_CLKOUT_DIV_FAC) */
		cfg->m4clk_reg->CLK_CONFIG_REG3_b.MCU_CLKOUT_DIV_FAC = mux->clock_div;
		break;
	default:
		LOG_WRN("HP clkid %u: clock-div %u not supported in hardware", mux->clkid,
			mux->clock_div);
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_hp_clk_config(const struct device *dev,
				 struct silabs_siwx91x_clock_control_config *mux)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	uint32_t reg_val;
	int ret;

	if (mux->ref_clkid == SIWX91X_CLK_PLL_SOC || mux->ref_clkid == SIWX91X_CLK_PLL_INTF ||
	    mux->ref_clkid == SIWX91X_CLK_PLL_I2S) {
		ret = siwx91x_check_pll_lock(dev, mux->ref_clkid);
		if (ret) {
			LOG_ERR("PLL %u not locked, please check PLL status in Device tree",
				mux->ref_clkid);
			return ret;
		}
	}

	if (mux->ref_clkid == SIWX91X_CLK_XTAL_MHZ) {
		siwx91x_clock_request_xtal_to_nwp();
	}

	reg_val = siwx91x_hp_clock_get_reg(mux->clkid, mux->ref_clkid);
	if (reg_val == UINT32_MAX) {
		return -EINVAL;
	}

	/* By default, dynamic clock gatting is on for all clocks and some clock is enabled during
	 * boot. We disable the clock and the dynamic clock gatting for all peripheral during
	 * configuration. This is the responsabilty of the peripheral to enable his cock with
	 * clock_control_on() Some execption is made for QSPI (because of XIP) of obsiously for the
	 * proc clock.
	 */
	switch (mux->clkid) {
	case SIWX91X_CLK_CPU:
		cfg->m4clk_reg->CLK_CONFIG_REG5_b.M4_SOC_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_CPU_LP:
		cfg->m4clk_reg->CLK_CONFIG_REG4_b.SLEEP_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_QSPI:
		cfg->m4clk_reg->CLK_CONFIG_REG1_b.QSPI_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_QSPI2:
		cfg->m4clk_reg->CLK_CONFIG_REG6_b.QSPI_2_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_UART0:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_UART0);
		cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART1_SCLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_UART1:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_UART1);
		cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART2_SCLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_SSI:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_SSI);
		cfg->m4clk_reg->CLK_CONFIG_REG1_b.SSI_MST_SCLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_I2S:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_I2S);
		cfg->m4clk_reg->CLK_CONFIG_REG5_b.I2S_CLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_GSPI:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_GSPI);
		cfg->m4clk_reg->CLK_CONFIG_REG1_b.GEN_SPI_MST1_SCLK_SEL = reg_val;
		break;
	case SIWX91X_CLK_HP_REF_ULP:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_HP_REF_ULP);
		break;
	case SIWX91X_CLK_PIN_OUT:
		clock_control_off(dev, (clock_control_subsys_t)SIWX91X_CLK_PIN_OUT);
		cfg->m4clk_reg->CLK_CONFIG_REG3_b.MCU_CLKOUT_SEL = reg_val;
		break;
	case SIWX91X_CLK_HP_REF_PLL:
		/* Shared ref mux (bits [15:14] of SOCPLLMACROREG2) — SPI-mapped */
		RSI_CLK_SocPllRefClkConfig(reg_val);
		break;
	default:
		return -EINVAL;
	}

	ret = siwx91x_hp_clk_wait_switched(dev, mux->clkid);
	if (ret) {
		return ret;
	}

	return siwx91x_hp_clk_apply_div_factor(dev, mux);
}

static int siwx91x_hp_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	struct siwx91x_hp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	int ret;

	/* Reconfigure the clock before turning it on. Registers are lost after deep-sleep.*/
	for (size_t i = 0; i < data->hp_clk_mux_count; i++) {
		if (data->hp_clk_mux[i].clkid != clockid) {
			continue;
		}
		ret = siwx91x_hp_clk_config(dev, &data->hp_clk_mux[i]);
		if (ret) {
			return ret;
		}
	}

	switch (clockid) {
	case SIWX91X_CLK_CPU:
		RSI_CLK_PeripheralClkEnable3(cfg->m4clk_reg, M4_CORE_CLK_ENABLE);
		break;
	case SIWX91X_CLK_CPU_LP:
		/* no gates */
		break;
	case SIWX91X_CLK_QSPI:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, QSPI_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_QSPI2:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, QSPI_2_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_UART0:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, USART1_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_UART1:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, USART2_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_SSI:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, SSIMST_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_I2S:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, I2SM_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_STATIC_I2S:
		/* Compliance with previous clock controller API*/
		MISC_CFG_MISC_CTRL1 |= (1 << 23);
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, I2SM_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_GSPI:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, GSPI_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_HP_REF_ULP:
		RSI_CLK_PeripheralClkEnable1(cfg->m4clk_reg, ULPSS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_PIN_OUT:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, MCUCLKOUT_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ICACHE:
		RSI_CLK_PeripheralClkEnable1(cfg->m4clk_reg, ICACHE_CLK_ENABLE);
		break;
	case SIWX91X_CLK_UDMA:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, UDMA_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_GPDMA:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, RPDMA_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_RNG:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, HWRNG_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_PWM:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, PWM_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_I2C0:
		RSI_CLK_PeripheralClkEnable2(cfg->m4clk_reg, I2C_BUS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_I2C1:
		RSI_CLK_PeripheralClkEnable2(cfg->m4clk_reg, I2C_2_BUS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_GPIO:
		RSI_CLK_PeripheralClkEnable(cfg->m4clk_reg, EGPIO_CLK, ENABLE_STATIC_CLK);
		break;
	/* PLL on/off unauthorized to simplify the code */
	case SIWX91X_CLK_PLL_SOC:
	case SIWX91X_CLK_PLL_INTF:
	case SIWX91X_CLK_PLL_I2S:
	default:
		return -EINVAL;
	}

	data->hp_enabled_bitmap |= BIT64(clockid);

	return 0;
}

static int siwx91x_hp_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	struct siwx91x_hp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);

	switch (clockid) {
	case SIWX91X_CLK_CPU:
		RSI_CLK_PeripheralClkDisable3(cfg->m4clk_reg, M4_CORE_CLK_ENABLE);
		break;
	case SIWX91X_CLK_CPU_LP:
		/* no gates */
		break;
	case SIWX91X_CLK_QSPI:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, QSPI_CLK);
		break;
	case SIWX91X_CLK_QSPI2:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, QSPI_2_CLK);
		break;
	case SIWX91X_CLK_UART0:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, USART1_CLK);
		break;
	case SIWX91X_CLK_UART1:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, USART2_CLK);
		break;
	case SIWX91X_CLK_SSI:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, SSIMST_CLK);
		break;
	case SIWX91X_CLK_I2S:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, I2SM_CLK);
		break;
	case SIWX91X_CLK_GSPI:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, GSPI_CLK);
		break;
	case SIWX91X_CLK_HP_REF_ULP:
		RSI_CLK_PeripheralClkDisable1(cfg->m4clk_reg, ULPSS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_PIN_OUT:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, MCUCLKOUT_CLK);
		break;
	case SIWX91X_CLK_ICACHE:
		RSI_CLK_PeripheralClkDisable1(cfg->m4clk_reg, ICACHE_CLK_ENABLE);
		break;
	case SIWX91X_CLK_UDMA:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, UDMA_CLK);
		break;
	case SIWX91X_CLK_GPDMA:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, RPDMA_CLK);
		break;
	case SIWX91X_CLK_RNG:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, HWRNG_CLK);
		break;
	case SIWX91X_CLK_PWM:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, PWM_CLK);
		break;
	case SIWX91X_CLK_I2C0:
		RSI_CLK_PeripheralClkDisable2(cfg->m4clk_reg, I2C_BUS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_I2C1:
		RSI_CLK_PeripheralClkDisable2(cfg->m4clk_reg, I2C_2_BUS_CLK_ENABLE);
		break;
	case SIWX91X_CLK_GPIO:
		RSI_CLK_PeripheralClkDisable(cfg->m4clk_reg, EGPIO_CLK);
		break;
	/* PLL on/off unauthorized to simplify the code */
	case SIWX91X_CLK_PLL_SOC:
	case SIWX91X_CLK_PLL_INTF:
	case SIWX91X_CLK_PLL_I2S:
	default:
		return -EINVAL;
	}

	data->hp_enabled_bitmap &= ~BIT64(clockid);

	return 0;
}

static int siwx91x_hp_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
				     uint32_t *rate)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	struct siwx91x_hp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	uint32_t ref_clkid = UINT32_MAX;
	const struct device *parent_clk_dev;
	uint32_t div_factor = 0;
	uint32_t reg = 0;
	int ret = 0;

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = 0;

	switch (clockid) {
	case SIWX91X_CLK_PLL_SOC:
		*rate = data->soc_pll_hz;
		break;
	case SIWX91X_CLK_PLL_INTF:
		*rate = data->intf_pll_hz;
		break;
	case SIWX91X_CLK_PLL_I2S:
		*rate = data->i2s_pll_hz;
		break;
	case SIWX91X_CLK_CPU:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG5_b.M4_SOC_CLK_SEL;
		break;
	case SIWX91X_CLK_CPU_LP:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG4_b.SLEEP_CLK_SEL;
		break;
	case SIWX91X_CLK_QSPI:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG1_b.QSPI_CLK_SEL;
		break;
	case SIWX91X_CLK_QSPI2:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG6_b.QSPI_2_CLK_SEL;
		break;
	case SIWX91X_CLK_UART0:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART1_SCLK_SEL;
		break;
	case SIWX91X_CLK_UART1:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG2_b.USART2_SCLK_SEL;
		break;
	case SIWX91X_CLK_SSI:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG1_b.SSI_MST_SCLK_SEL;
		break;
	case SIWX91X_CLK_I2S:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG5_b.I2S_CLK_SEL;
		break;
	case SIWX91X_CLK_GSPI:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG1_b.GEN_SPI_MST1_SCLK_SEL;
		break;
	case SIWX91X_CLK_PIN_OUT:
		reg = cfg->m4clk_reg->CLK_CONFIG_REG3_b.MCU_CLKOUT_SEL;
		break;
	case SIWX91X_CLK_HP_REF_PLL:
		ref_clkid = SIWX91X_CLK_XTAL_MHZ;
		break;
	case SIWX91X_CLK_HP_REF_ULP:
	case SIWX91X_CLK_ICACHE:
	case SIWX91X_CLK_UDMA:
	case SIWX91X_CLK_GPDMA:
	case SIWX91X_CLK_RNG:
	case SIWX91X_CLK_PWM:
	case SIWX91X_CLK_I2C0:
	case SIWX91X_CLK_I2C1:
	case SIWX91X_CLK_GPIO:
		ref_clkid = SIWX91X_CLK_CPU;
		break;
	default:
		return -EINVAL;
	}

	/* Not directly connected to an oscillator */
	if (*rate == 0) {
		/* No fixed ref clock, get the ref clock from the register */
		if (ref_clkid == UINT32_MAX) {
			ref_clkid = siwx91x_hp_clock_get_ref_clock(clockid, reg);
			if (ref_clkid == UINT32_MAX) {
				return -EINVAL;
			}
		}

		parent_clk_dev = siwx91x_clock_control_get_device(ref_clkid);
		if (parent_clk_dev == NULL) {
			return -EINVAL;
		}

		ret = clock_control_get_rate(parent_clk_dev, (clock_control_subsys_t)ref_clkid,
					     rate);
		if (ret != 0) {
			return ret;
		}
	}

	ret = siwx91x_hp_clk_get_div_factor(dev, clockid, &div_factor);
	if (ret) {
		return ret;
	}

	if (div_factor != 0 && div_factor != 1) {
		*rate = *rate / div_factor;
	}

	return ret;
}

static int siwx91x_hp_clock_set_rate(const struct device *dev, clock_control_subsys_t sys,
				     clock_control_subsys_rate_t raw_rate)
{
	struct siwx91x_hp_clock_data *hp_data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	rsi_error_t ret;
	uint32_t rate;

	if (raw_rate == NULL) {
		return -EINVAL;
	}

	rate = POINTER_TO_UINT(raw_rate);
	if (rate == 0) {
		return -EINVAL;
	}

	switch (clockid) {
	case SIWX91X_CLK_PLL_SOC:
		ret = siwx91x_hp_set_pll_freq(dev, SIWX91X_CLK_PLL_SOC, rate);
		if (ret != 0) {
			return -EIO;
		}
		hp_data->soc_pll_hz = rate;
		return 0;
	case SIWX91X_CLK_PLL_INTF:
		ret = siwx91x_hp_set_pll_freq(dev, SIWX91X_CLK_PLL_INTF, rate);
		if (ret != 0) {
			return -EIO;
		}
		hp_data->intf_pll_hz = rate;
		return 0;
	case SIWX91X_CLK_PLL_I2S:
		ret = siwx91x_hp_set_pll_freq(dev, SIWX91X_CLK_PLL_I2S, rate);
		if (ret != 0) {
			return -EIO;
		}
		hp_data->i2s_pll_hz = rate;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int siwx91x_hp_clock_configure(const struct device *dev, clock_control_subsys_t sys,
				      void *cfg)
{
	struct silabs_siwx91x_clock_control_config *new_cfg = cfg;
	struct siwx91x_hp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);

	if (new_cfg == NULL) {
		return -EINVAL;
	}

	if (new_cfg->clkid != clockid) {
		return -EINVAL;
	}

	if (!siwx91x_hp_clk_valid(new_cfg->clkid, new_cfg->ref_clkid)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < data->hp_clk_mux_count; i++) {
		if (data->hp_clk_mux[i].clkid == new_cfg->clkid) {
			data->hp_clk_mux[i].ref_clkid = new_cfg->ref_clkid;
			data->hp_clk_mux[i].clock_div = new_cfg->clock_div;
			return 0;
		}
	}

	return -EINVAL;
}

static enum clock_control_status siwx91x_hp_clock_get_status(const struct device *dev,
							     clock_control_subsys_t sys)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	const struct siwx91x_hp_clock_data *data = dev->data;
	uint32_t clockid = POINTER_TO_UINT(sys);
	bool val;
	int ret;

	switch (clockid) {
	case SIWX91X_CLK_CPU:
	case SIWX91X_CLK_QSPI:
	case SIWX91X_CLK_QSPI2:
	case SIWX91X_CLK_UART0:
	case SIWX91X_CLK_UART1:
	case SIWX91X_CLK_SSI:
	case SIWX91X_CLK_I2S:
	case SIWX91X_CLK_GSPI:
	case SIWX91X_CLK_HP_REF_ULP:
	case SIWX91X_CLK_ICACHE:
	case SIWX91X_CLK_UDMA:
	case SIWX91X_CLK_GPDMA:
	case SIWX91X_CLK_RNG:
	case SIWX91X_CLK_PWM:
	case SIWX91X_CLK_I2C0:
	case SIWX91X_CLK_I2C1:
	case SIWX91X_CLK_GPIO:
	  	val = data->hp_enabled_bitmap & BIT64(clockid);
		break;
	case SIWX91X_CLK_CPU_LP:
		/* if reg = 2, then clock is gated */
		val = (cfg->m4clk_reg->CLK_CONFIG_REG4_b.SLEEP_CLK_SEL == 2) ? 0 : 1;
		break;
	case SIWX91X_CLK_PIN_OUT:
		val = cfg->m4clk_reg->CLK_CONFIG_REG3_b.MCU_CLKOUT_ENABLE;
		break;
	case SIWX91X_CLK_HP_REF_PLL:
		val = true;
		break;
	case SIWX91X_CLK_PLL_SOC:
		ret = siwx91x_check_pll_lock(dev, SIWX91X_CLK_PLL_SOC);
		val = ret ? 0 : 1;
		break;
	case SIWX91X_CLK_PLL_INTF:
		ret = siwx91x_check_pll_lock(dev, SIWX91X_CLK_PLL_INTF);
		val = ret ? 0 : 1;
		break;
	case SIWX91X_CLK_PLL_I2S:
		ret = siwx91x_check_pll_lock(dev, SIWX91X_CLK_PLL_I2S);
		val = ret ? 0 : 1;
		break;
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
	return val ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int siwx91x_hp_clock_init(const struct device *dev)
{
	const struct siwx91x_hp_clock_config *cfg = dev->config;
	struct siwx91x_hp_clock_data *data = dev->data;
	struct silabs_siwx91x_clock_control_config *mux;
	struct silabs_siwx91x_clock_control_pll_config *pll_cfg;
	uint32_t proc_clk_ref = SIWX91X_CLK_REF_HP;
	uint32_t proc_clk_div = 0;
	int ret;

	/* All the clocks that is not declared in siwx917-clock.h is dynamic by default and can be
	 * managed by lower layers (bootloader, SystemInit)
	 * Main peripherals missing: MVP, CT
	 */

	/* Power gate for HP domain is enabled by default but make sure everything is powered up */
	RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EXT_ROM);
	RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_M4_CORE);
	/* Power gate the PERI_EFUSE power domain, This power domain contains the different M4SS
	 * peripherals those are SPI/SSI Master, I2C, USART, Micro-DMA Controller,  UART, SPI/SSI
	 * Slave, Generic-SPI Master, Config Timer, Random-Number Generator, CRC Accelerator, SIO,
	 * I2C, I2S Master/Slave, QEI, MCPWM ,EFUSE and MVP*/
	RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
	RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_QSPI_ICACHE);
	RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_M4_DEBUG_FPU);

	/* Keep SOC "other" clocks ungated (REG3) as done by WiseConnect boot paths. */
	RSI_CLK_PeripheralClkEnable3(cfg->m4clk_reg, M4_SOC_CLK_FOR_OTHER_ENABLE);

	/* 1 - Set up PLL reference clock */
	for (size_t i = 0; i < data->hp_clk_mux_count; i++) {
		if (data->hp_clk_mux[i].clkid == SIWX91X_CLK_HP_REF_PLL) {
			mux = &data->hp_clk_mux[i];

			ret = siwx91x_hp_clk_valid(mux->clkid, mux->ref_clkid);
			if (!ret) {
				LOG_ERR("Invalid HP mux clockid %u ref %u", mux->clkid,
					mux->ref_clkid);
				continue;
			}

			ret = siwx91x_hp_clk_config(dev, mux);
			if (ret) {
				LOG_ERR("HP route %zu (clkid %u) failed: %d", i, mux->clkid, ret);
			}
		}
		/* Save the proc clock defined in dt to reapply after pll configuration - this is
		 * done in case bootloader or other code let the M4 processor clocked with the
		 * soc pll. We need to ensure that the Proc is clocked to HP REF before configuring
		 * the PLLs.
		 */
		if (data->hp_clk_mux[i].clkid == SIWX91X_CLK_CPU) {
			proc_clk_ref = data->hp_clk_mux[i].ref_clkid;
			proc_clk_div = data->hp_clk_mux[i].clock_div;
		}
	}

	/* 2 -Switch proc to HP ref clock */
	siwx91x_clk_cfg(SIWX91X_CLK_CPU, SIWX91X_CLK_REF_HP, 0);
	siwx91x_clk_on(SIWX91X_CLK_CPU);

	/* 3 - Set up PLL frequencies */
	for (size_t i = 0; i < cfg->pll_cfg_count; i++) {
		pll_cfg = &cfg->pll_cfg[i];
		ret = siwx91x_hp_set_pll_freq(dev, pll_cfg->clkid, pll_cfg->frequency);
		if (ret != 0) {
			LOG_ERR("HP PLL boot init failed: %d", ret);
		}
	}

	/* 4 -Set up hp clock routes */
	for (size_t i = 0; i < data->hp_clk_mux_count; i++) {
		mux = &data->hp_clk_mux[i];

		ret = siwx91x_hp_clk_valid(mux->clkid, mux->ref_clkid);
		if (!ret) {
			LOG_ERR("Invalid HP mux clockid %u ref %u", mux->clkid, mux->ref_clkid);
			continue;
		}

		ret = siwx91x_hp_clk_config(dev, mux);
		if (ret) {
			LOG_ERR("HP route %zu (clkid %u) failed: %d", i, mux->clkid, ret);
		}
	}

	/* 5 - Reapply the dt proc clock configuration */
	siwx91x_clk_cfg(SIWX91X_CLK_CPU, proc_clk_ref, proc_clk_div);
	siwx91x_clk_on(SIWX91X_CLK_CPU);

	/* 6 - Apply prefetch optimization + systick after core clock update */
	siwx91x_hp_clk_proc_change_update();

	/* 7 - Configure the CLKOUT clock */
	/* TODO: Handle CLKOUT reconfiguration after deep-sleep */
	if (cfg->pcfg != NULL) {

		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Failed to apply clockout pinctrl state: %d", ret);
		}

		siwx91x_clk_on(SIWX91X_CLK_PIN_OUT);
	}

	return 0;
}

static DEVICE_API(clock_control, siwx91x_hp_clock_api) = {
	.on = siwx91x_hp_clock_on,
	.off = siwx91x_hp_clock_off,
	.get_rate = siwx91x_hp_clock_get_rate,
	.set_rate = siwx91x_hp_clock_set_rate,
	.configure = siwx91x_hp_clock_configure,
	.get_status = siwx91x_hp_clock_get_status,
};

#define SIWX91X_HP_CLOCK_PINCTRL_DEFINE(inst)                                                      \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(inst, default),                                       \
		    (PINCTRL_DT_INST_DEFINE(inst)), ())

#define SIWX91X_HP_CLOCK_PINCTRL_CONFIG(inst)                                                      \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(inst, default),                                       \
		    (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),), (.pcfg = NULL,))

#define SIWX91X_HP_CLOCK_INIT(inst)                                                                \
	static struct silabs_siwx91x_clock_control_config hp_clk_mux_##inst[] = {                  \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SIWX91X_CLOCK_MANAGER_CHILD_INIT)          \
	};                                                                                         \
	static struct silabs_siwx91x_clock_control_pll_config pll_cfg_##inst[] = {                 \
		DT_INST_FOREACH_CHILD(inst, SIWX91X_CLOCK_MANAGER_PLL_INIT)                        \
	};                                                                                         \
	SIWX91X_HP_CLOCK_PINCTRL_DEFINE(inst);                                                     \
	static const struct siwx91x_hp_clock_config siwx91x_hp_clock_config_##inst = {             \
		.m4clk_reg = (M4CLK_Type *)DT_INST_REG_ADDR_BY_NAME(inst, m4clk),                  \
		.pll_cfg = pll_cfg_##inst,                                                         \
		.pll_cfg_count = ARRAY_SIZE(pll_cfg_##inst),                                       \
		SIWX91X_HP_CLOCK_PINCTRL_CONFIG(inst)                                              \
	};                                                                                         \
	static struct siwx91x_hp_clock_data siwx91x_hp_clock_data_##inst= {                        \
		.hp_clk_mux = hp_clk_mux_##inst,                                                   \
		.hp_clk_mux_count = ARRAY_SIZE(hp_clk_mux_##inst),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_hp_clock_init, NULL, &siwx91x_hp_clock_data_##inst,    \
			      &siwx91x_hp_clock_config_##inst, PRE_KERNEL_1,                       \
			      CONFIG_SIWX91X_HP_CLOCK_INIT_PRIORITY, &siwx91x_hp_clock_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_HP_CLOCK_INIT)
