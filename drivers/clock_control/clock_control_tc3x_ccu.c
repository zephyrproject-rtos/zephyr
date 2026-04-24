/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/tc3x_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>

#include "soc.h"
#include "IfxScu_reg.h"
#include "clock_control_aurix.h"

#define DT_DRV_COMPAT infineon_tc3x_ccu

#define PLL_LOCK_TIME_US     100
#define PLL_POWER_UP_TIME_US 1000

static inline int clock_control_tc3x_ccu_osc_init(void)
{
	Ifx_SCU_OSCCON osccon = MODULE_SCU.OSCCON;

	osccon.B.MODE = 0;
	osccon.B.OSCVAL = ((CLOCK_FREQ(fosc) / 1000000U) - 15);
	aurix_safety_endinit_enable(false);
	MODULE_SCU.OSCCON = osccon;
	aurix_safety_endinit_enable(true);

	if (!WAIT_FOR((osccon = MODULE_SCU.OSCCON, osccon.B.PLLHV == 1 && osccon.B.PLLLV == 1),
		      10000, ccu_busy_wait_fback(1))) {
		return -EIO;
	}

	return 0;
}

static inline void clock_control_tc3x_ccu_osc_disable(void)
{
	MODULE_SCU.OSCCON.B.MODE = 1;
}

static inline bool clock_control_tc3x_ccu_osc_ok(void)
{
	Ifx_SCU_OSCCON osccon = MODULE_SCU.OSCCON;

	return osccon.B.PLLHV == 1 && osccon.B.PLLLV == 1 && osccon.B.MODE == 0 &&
	       osccon.B.OSCVAL == ((CLOCK_FREQ(fosc) / 1000000U) - 15);
}

static inline void clock_control_tc3x_ccu_pll_init(void)
{

	Ifx_SCU_SYSPLLCON0 syspllcon0 = {.B.NDIV = PLL_N_DIV(sys_pll) - 1,
					 .B.PDIV = PLL_P_DIV(sys_pll) - 1,
					 .B.PLLPWD = 1,
					 .B.INSEL = CLOCK_IS_CLOCK_SOURCE(fback, sys_pll)  ? 0
						    : CLOCK_IS_CLOCK_SOURCE(fosc, sys_pll) ? 1
											   : 2};
	Ifx_SCU_PERPLLCON0 perpllcon0 = {.B.NDIV = PLL_N_DIV(per_pll) - 1,
					 .B.PDIV = PLL_P_DIV(per_pll) - 1,
					 .B.DIVBY = PLL_K_PRE_DIV(per_pll, 3) == 1.6 ? 0 : 1,
					 .B.PLLPWD = 1};
	;

	aurix_safety_endinit_enable(false);
	MODULE_SCU.SYSPLLCON0 = syspllcon0;
	MODULE_SCU.PERPLLCON0 = perpllcon0;
	aurix_safety_endinit_enable(true);
}

static inline int clock_control_tc3x_ccu_pll_power_down(void)
{

	Ifx_SCU_PERPLLSTAT perpllstat;
	Ifx_SCU_SYSPLLSTAT syspllstat;

	aurix_safety_endinit_enable(false);
	MODULE_SCU.SYSPLLSTAT.B.PWDSTAT = 0;
	MODULE_SCU.PERPLLSTAT.B.PWDSTAT = 0;
	aurix_safety_endinit_enable(true);

	if (!WAIT_FOR((perpllstat = MODULE_SCU.PERPLLSTAT, syspllstat = MODULE_SCU.SYSPLLSTAT,
		       perpllstat.B.PWDSTAT == 1 && syspllstat.B.PWDSTAT == 1),
		      100, ccu_busy_wait_fback(1))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc3x_ccu_pll_wait_power_up(void)
{
	Ifx_SCU_PERPLLSTAT perpllstat;
	Ifx_SCU_SYSPLLSTAT syspllstat;

	/* Wait 1ms to stabilize the pll */
	uint64_t start_time = k_ticks_to_us_floor64(k_uptime_ticks());

	if (!WAIT_FOR((perpllstat = MODULE_SCU.PERPLLSTAT, syspllstat = MODULE_SCU.SYSPLLSTAT,
		       perpllstat.B.PWDSTAT == 0 && syspllstat.B.PWDSTAT == 0),
		      1000, ccu_busy_wait_fback(1))) {
		return -EIO;
	}
	uint64_t stop_time = k_ticks_to_us_floor64(k_uptime_ticks());

	k_busy_wait(PLL_POWER_UP_TIME_US - (stop_time - start_time));

	return 0;
}

static inline int clock_control_tc3x_ccu_pll_set_devider(void)
{
	Ifx_SCU_SYSPLLCON1 syspllcon1 = MODULE_SCU.SYSPLLCON1;
	uint32_t currentDiv = syspllcon1.B.K2DIV + 1;
	bool moveUp = currentDiv < PLL_K_DIV(sys_pll, 2);

	while (currentDiv != PLL_K_DIV(sys_pll, 2)) {
		currentDiv += moveUp ? 1 : -1;
		syspllcon1.B.K2DIV = currentDiv - 1;

		aurix_safety_endinit_enable(false);
		MODULE_SCU.SYSPLLCON1 = syspllcon1;
		aurix_safety_endinit_enable(true);

		k_busy_wait(100);
	}

	return 0;
}

static inline int clock_control_tc3x_ccu_pll_wait_lock(void)
{
	Ifx_SCU_PERPLLSTAT perpllstat;
	Ifx_SCU_SYSPLLSTAT syspllstat;

	aurix_safety_endinit_enable(false);
	MODULE_SCU.SYSPLLCON0.B.RESLD = 1;
	MODULE_SCU.PERPLLCON0.B.RESLD = 1;
	aurix_safety_endinit_enable(true);

	if (!WAIT_FOR((perpllstat = MODULE_SCU.PERPLLSTAT, syspllstat = MODULE_SCU.SYSPLLSTAT,
		       perpllstat.B.LOCK == 1 && syspllstat.B.LOCK == 1),
		      PLL_LOCK_TIME_US, ccu_busy_wait_fback(1))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc3x_ccu_pll_initial_divider(void)
{
	Ifx_SCU_SYSPLLCON1 syspllcon1 = {.B.K2DIV = PLL_K_DIV(sys_pll, 2) - 1 + 3};
	Ifx_SCU_PERPLLCON1 perpllcon1 = {
		.B.K2DIV = PLL_K_DIV(per_pll, 2) - 1,
		.B.K3DIV = PLL_K_DIV(per_pll, 3) - 1,
	};
	Ifx_SCU_PERPLLSTAT perpllstat;

	aurix_safety_endinit_enable(false);
	MODULE_SCU.SYSPLLCON1 = syspllcon1;
	MODULE_SCU.PERPLLCON1 = perpllcon1;
	aurix_safety_endinit_enable(true);

	if (!WAIT_FOR((perpllstat = MODULE_SCU.PERPLLSTAT, MODULE_SCU.SYSPLLSTAT.B.K2RDY == 1 &&
								   perpllstat.B.K2RDY == 1 &&
								   perpllstat.B.K3RDY == 1),
		      1000, k_busy_wait(1))) {
		return -EIO;
	}
	return 0;
}

static inline int clock_control_tc3x_ccu_select_clock(uint8_t clksel)
{
	Ifx_SCU_CCUCON0 ccucon0 = MODULE_SCU.CCUCON0;

	ccucon0.B.CLKSEL = clksel;
	ccucon0.B.UP = 1;
	aurix_safety_endinit_enable(false);
	MODULE_SCU.CCUCON0 = ccucon0;
	aurix_safety_endinit_enable(true);

	/* Wait for new clock config to be active */
	if (clksel) {
		WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(0, fpll_div3)
	} else {
		WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(0, fback)
	}

	return 0;
}

static inline int clock_control_tc3x_ccu_set_divider(void)
{
	Ifx_SCU_CCUCON0 ccucon0 = {
		.B.BBBDIV = fbbb_div,
		.B.FSI2DIV = ffsi2_div,
		.B.FSIDIV = ffsi_div,
		.B.GTMDIV = CLOCK_DIV_WITH_STATUS(fgtm),
		.B.SPBDIV = fspb_div,
		.B.SRIDIV = fsri_div,
		.B.STMDIV = fstm_div,
	};
	Ifx_SCU_CCUCON5 ccucon5 = {
		.B.GETHDIV = CLOCK_DIV_WITH_INST(fgeth, infineon_tc3x_geth),
		.B.MCANHDIV = CLOCK_DIV_WITH_INST(fmcanh, infineon_mcmcan),
		.B.UP = 1,
	};
	Ifx_SCU_CCUCON1 ccucon1 = {
		.B.CLKSELMCAN = CLOCK_SEL_WITH_INST(fmcan, fmcani, fosc, infineon_mcmcan),
		.B.CLKSELMSC = 0,
		.B.CLKSELQSPI = CLOCK_SEL_WITH_INST(fsourceqspi, fsource1, fsource2, infineon_qspi),
		.B.MCANDIV = CLOCK_DIV_WITH_INST(fmcani, infineon_mcmcan),
		.B.MSCDIV = 0,
		.B.QSPIDIV = CLOCK_DIV_WITH_INST(fqspi, infineon_qspi),
		.B.I2CDIV = 0,
		.B.PLL1DIVDIS = 1, /* TODO: Implement pll1 div */
	};
	Ifx_SCU_CCUCON2 ccucon2 = {
		.B.CLKSELASCLINS =
			CLOCK_SEL_WITH_INST(fasclins, fasclinsi, fosc, infineon_asclin_uart),
		.B.ASCLINFDIV = CLOCK_DIV_WITH_INST(fasclinf, infineon_asclin_uart),
		.B.ASCLINSDIV = CLOCK_DIV_WITH_INST(fasclinsi, infineon_asclin_uart),
	};

	aurix_safety_endinit_enable(false);
	MODULE_SCU.CCUCON0 = ccucon0;
	MODULE_SCU.CCUCON5 = ccucon5;
	aurix_safety_endinit_enable(true);
	WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(5, fback)
	aurix_safety_endinit_enable(false);
	MODULE_SCU.CCUCON1 = ccucon1;
	aurix_safety_endinit_enable(true);
	WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(1, fback)
	aurix_safety_endinit_enable(false);
	MODULE_SCU.CCUCON2 = ccucon2;
	aurix_safety_endinit_enable(true);
	WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(2, fback)

	return 0;
}

static int clock_control_tc3x_ccu_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint32_t id = *(uint32_t *)sys;

	switch (id) {
	case CLOCK_FSRI:
		return 0;
	case CLOCK_FSPB:
		return 0;
	case CLOCK_FGTM:
		return CLOCK_DIV_WITH_STATUS(fgtm) != 0 ? 0 : -EIO;
	case CLOCK_FSTM:
		return 0;
	case CLOCK_FMSC:
		return -EIO;
	case CLOCK_FGETH:
		return CLOCK_DIV_WITH_INST(fgeth, infineon_tc3x_geth) != 0 ? 0 : -EIO;
	case CLOCK_FADAS:
		return 0;
	case CLOCK_FMCANH:
		return CLOCK_DIV_WITH_INST(fmcanh, infineon_tc3x_geth) != 0 ? 0 : -EIO;
	case CLOCK_FASCLINF:
	case CLOCK_FASCLINS:
		return CLOCK_DIV_WITH_INST(fasclinf, infineon_asclin_uart) != 0 ? 0 : -EIO;
	case CLOCK_FQSPI:
		return CLOCK_DIV_WITH_INST(fqspi, infineon_qspi) != 0 ? 0 : -EIO;
	case CLOCK_FADC:
		return 0;
	case CLOCK_FI2C:
		return -EIO;
	case CLOCK_FEBU:
		return 0;
	default:
		return -EINVAL;
	}

	return 0;
}

static int clock_control_tc3x_ccu_off(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOSYS;
}

static enum clock_control_status clock_control_tc3x_ccu_get_status(const struct device *dev,
								    clock_control_subsys_t sys)
{
	switch ((uintptr_t)sys) {
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static int clock_control_tc3x_ccu_get_rate(const struct device *dev, clock_control_subsys_t sys,
					    uint32_t *rate)
{
	uint32_t id = *(uint32_t *)sys;

	switch (id) {
	case CLOCK_FSRI:
		*rate = fsource0_freq / fsri_div;
		return 0;
	case CLOCK_FSPB:
		*rate = fsource0_freq / fspb_div;
		return 0;
	case CLOCK_FGTM:
		*rate = 0;
		return 0;
	case CLOCK_FSTM:
		*rate = fsource0_freq / fstm_div;
		return 0;
	case CLOCK_FMSC:
		return -EINVAL;
	case CLOCK_FGETH:
		*rate = fsource0_freq / fgeth_div;
		return 0;
	case CLOCK_FADAS:
		return -EINVAL;
	case CLOCK_FMCANH:
		*rate = fsource0_freq / fmcanh_div;
		;
		return 0;
	case CLOCK_FASCLINF:
		*rate = fsource2_freq / fasclinf_div;
		return 0;
	case CLOCK_FASCLINS:
		*rate = CLOCK_SOURCE_IS(fasclins, fasclinsi) ? fsource1_freq / fasclinsi_div
							     : fosc_freq;
		return 0;
	case CLOCK_FQSPI:
		return CLOCK_DIV_WITH_INST(fqspi, infineon_qspi) != 0 ? 0 : -EIO;
	case CLOCK_FADC:
		return 0;
	case CLOCK_FI2C:
		return -EIO;
	case CLOCK_FEBU:
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}

static int clock_control_tc3x_ccu_init(const struct device *dev)
{
	int ret = 0;

	/* Enable osc if requrired */
	if (CLOCK_IS_CLOCK_SOURCE(fosc, FOSC_SOURCES)) {
		if (!clock_control_tc3x_ccu_osc_ok()) {
			ret = clock_control_tc3x_ccu_osc_init();
			if (ret) {
				return ret;
			}
		}
	} else {
		clock_control_tc3x_ccu_osc_disable();
	}

	if (CLOCK_IS_CLOCK_SOURCE(fpll0, fsource0)) {

		if (ret) {
			return ret;
		}

		/* Configure pll parameters*/
		clock_control_tc3x_ccu_pll_init();
		if (ret) {
			return ret;
		}

		ret = clock_control_tc3x_ccu_pll_wait_lock();
		if (ret) {
			return ret;
		}

		ret = clock_control_tc3x_ccu_pll_initial_divider();
		if (ret) {
			return ret;
		}
	} else {
		/* Disable pll power */
		clock_control_tc3x_ccu_pll_power_down();

		if (ret) {
			return ret;
		}
	}

	ret = clock_control_tc3x_ccu_set_divider();
	if (ret) {
		return -EIO;
	}

	/* Select clock for operation */
	ret = clock_control_tc3x_ccu_select_clock(
		DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(fsource0)), DT_NODELABEL(fpll0)));
	if (ret) {
		return ret;
	}

	if (CLOCK_IS_CLOCK_SOURCE(fpll0, fsource0)) {
		/* Frequency stepping, to avoid load jumps */
		ret = clock_control_tc3x_ccu_pll_set_devider();
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static const struct clock_control_driver_api clock_control_tc3x_ccu_api = {
	.on = clock_control_tc3x_ccu_on,
	.off = clock_control_tc3x_ccu_off,
	.get_rate = clock_control_tc3x_ccu_get_rate,
	.get_status = clock_control_tc3x_ccu_get_status,
};

DEVICE_DT_INST_DEFINE(0, &clock_control_tc3x_ccu_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_tc3x_ccu_api);
