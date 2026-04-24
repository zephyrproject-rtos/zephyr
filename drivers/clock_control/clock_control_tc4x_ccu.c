/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/tc4x_clock.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include <IfxClock_reg.h>
#include "clock_control_aurix.h"

#define DT_DRV_COMPAT infineon_tc4x_ccu

struct clock_control_tc4x_divider {
	uint32_t fasclinsi_div: 4;
	uint32_t fasclinf_div: 4;
};
struct clock_control_tc4x_data {
	uint32_t fsource0;
	uint32_t fsource1;
	uint32_t fsource2;
	uint32_t fsource3;

	struct clock_control_tc4x_divider divider;
};

static inline int clock_control_tc4x_ccu_osc_init(void)
{
	Ifx_CLOCK_OSCCON osc = CLOCK_OSCCON;

	/* TODO: Enable oscillator */
	/* TODO: extclk needed*/
	osc.B.MODE = 0;
	osc.B.INSEL = 1;
	CLOCK_OSCCON = osc;

	Ifx_CLOCK_OSCMON1 oscmon1;

	oscmon1.B.OSCVAL = DT_PROP(DT_NODELABEL(fosc), clock_frequency) / 1000000 - 15;
	CLOCK_OSCMON1 = oscmon1;

	return 0;
}

static inline int clock_control_tc4x_ccu_syspll_default(void)
{
	Ifx_CLOCK_SYSPLLCON1 con1 = CLOCK_SYSPLLCON1;

	con1.B.K3PREDIV = 0xA;
	CLOCK_SYSPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	con1.B.K2DIV = 0x1;
	con1.B.K3DIV = 0x1;
	CLOCK_SYSPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	return 0;
}

static inline int clock_control_tc4x_ccu_syspll_power(bool enable)
{
	CLOCK_SYSPLLCON0.B.PLLPWR = enable;

	if (!WAIT_FOR(CLOCK_SYSPLLSTAT.B.PWRSTAT == enable, 100, ccu_busy_wait_fback(100))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc4x_ccu_syspll_init(void)
{
	Ifx_CLOCK_SYSPLLCON0 con0;

	con0.B.NDIV = PLL_N_DIV(sys_pll) - 1;
	con0.B.PDIV = PLL_P_DIV(sys_pll) - 1;
	con0.B.PLLPWR = 1;
	con0.B.RESLD = 1;
	CLOCK_SYSPLLCON0 = con0;

	if (!WAIT_FOR(CLOCK_SYSPLLSTAT.B.PWRSTAT == 1, 100, ccu_busy_wait_fback(100))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc4x_ccu_syspll_divider(void)
{
	Ifx_CLOCK_SYSPLLCON1 con1 = CLOCK_SYSPLLCON1;

#if DT_NODE_EXISTS(DT_NODELABEL(fpllppu))
	if (PLL_K_PRE_DIV(sys_pll, 3) != 2.0) {
		con1.B.K3PREDIV = PLL_K_PRE_DIV_REG(sys_pll, 3);
		CLOCK_SYSPLLCON1 = con1;
		ccu_busy_wait_fback(10);
		WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);
	}
#endif

	con1.B.K2DIV = PLL_K_DIV(sys_pll, 2) - 1;
#if DT_NODE_EXISTS(DT_NODELABEL(fpllppu))
	con1.B.K3DIV = PLL_K_DIV(sys_pll, 3) - 1;
#endif
	CLOCK_SYSPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	return 0;
}

static inline int clock_control_tc4x_ccu_perpll_default(void)
{
	Ifx_CLOCK_PERPLLCON1 con1 = CLOCK_PERPLLCON1;

	con1.B.K2PREDIV = 0x1;
	con1.B.K3PREDIV = 0xA;
	con1.B.K4PREDIV = 0x1;
	CLOCK_PERPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	con1.B.K2DIV = 0x1;
	con1.B.K3DIV = 0x1;
	con1.B.K4DIV = 0x1;
	CLOCK_PERPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	return 0;
}

static inline int clock_control_tc4x_ccu_perpll_power(bool enable)
{
	CLOCK_PERPLLCON0.B.PLLPWR = enable;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	if (!WAIT_FOR(CLOCK_PERPLLSTAT.B.PWRSTAT == enable, 100, ccu_busy_wait_fback(100))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc4x_ccu_perpll_init(void)
{
	Ifx_CLOCK_PERPLLCON0 con0;

	con0.B.NDIV = PLL_N_DIV(per_pll) - 1;
	con0.B.PDIV = PLL_P_DIV(per_pll) - 1;
	con0.B.PLLPWR = 1;
	con0.B.RESLD = 1;
	CLOCK_PERPLLCON0 = con0;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	if (!WAIT_FOR(CLOCK_PERPLLSTAT.B.PWRSTAT == 1, 100, ccu_busy_wait_fback(100))) {
		return -EIO;
	}

	return 0;
}

static inline int clock_control_tc4x_ccu_perpll_divider(void)
{
	Ifx_CLOCK_PERPLLCON1 con1 = CLOCK_PERPLLCON1;

	if (PLL_K_PRE_DIV(per_pll, 2) != 2.0 || PLL_K_PRE_DIV(per_pll, 3) != 2.0 ||
	    PLL_K_PRE_DIV(per_pll, 4) != 2.0) {
		con1.B.K2PREDIV = PLL_K_PRE_DIV_REG(per_pll, 2);
		con1.B.K3PREDIV = PLL_K_PRE_DIV_REG(per_pll, 3);
		con1.B.K4PREDIV = PLL_K_PRE_DIV_REG(per_pll, 4);
		CLOCK_PERPLLCON1 = con1;
		ccu_busy_wait_fback(10);
		WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);
	}

	con1.B.K2DIV = PLL_K_DIV(per_pll, 2) - 1;
	con1.B.K3DIV = PLL_K_DIV(per_pll, 3) - 1;
	con1.B.K4DIV = PLL_K_DIV(per_pll, 4) - 1;
	CLOCK_PERPLLCON1 = con1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	return 0;
}

enum sys_clock_source {
	SYS_SOURCE_PLL,
	SYS_SOURCE_FBACK,
	SYS_SOURCE_FRAMP
};

static inline void clock_control_tc4x_ccu_set_sys_source(enum sys_clock_source src)
{
	CLOCK_CCUCON.B.CLKSELS = src;
}

enum per_clock_source {
	PER_SOURCE_PLL,
	PER_SOURCE_FBACK
};

static inline void clock_control_tc4x_ccu_set_per_source(enum per_clock_source src)
{
	CLOCK_CCUCON.B.CLKSELP = src;
}

static inline int clock_control_tc4x_ccu_ramposc_init(void)
{
	/* Enable ramp oscillator */
	CLOCK_RAMPCON0.B.PWR = 1;

	/* Wait for Ramp Osc to be active*/
	if (!WAIT_FOR(CLOCK_RAMPSTAT.B.ACTIVE == 1, 1000, ccu_busy_wait_fback(1))) {
		return -EIO;
	}

	/* Wait for frequency movement disabled */
	if (!WAIT_FOR(CLOCK_RAMPSTAT.B.FSTAT != 0, 1000, ccu_busy_wait_fback(1))) {
		return -EIO;
	}

	if (CLOCK_RAMPSTAT.B.FSTAT != 1) {
		CLOCK_RAMPCON0.B.CMD = 2;

		if (!WAIT_FOR(CLOCK_RAMPSTAT.B.FLLLOCK == 1, 1000, ccu_busy_wait_fback(1))) {
			return -EIO;
		}
	}

	return 0;
}

static inline int clock_control_tc4x_ccu_ramposc_move(bool to_top)
{
	Ifx_CLOCK_RAMPCON0 rampcon0 = {
		.B = {.PWR = 1, .UFL = MAX(130, fpll0_freq / 1000000), .CMD = to_top ? 0x1 : 0x2}};
	Ifx_CLOCK_RAMPSTAT rampstat = {.U = CLOCK_RAMPSTAT.U};

	if (rampstat.B.ACTIVE != 0 && rampstat.B.SSTAT != 0 &&
	    rampstat.B.FSTAT != (to_top ? 0x1 : 0x2)) {
		return -EINVAL;
	}

	CLOCK_RAMPCON0 = rampcon0;

	return 0;
}

static inline int clock_control_tc4x_ccu_set_divider(void)
{
	/* clang-format off */
	Ifx_CLOCK_SYSCCUCON0 sysccu0 = {
		.B.SRIDIV = fsri_div, .B.SRICSDIV = 0, .B.SPBDIV = fspb_div,
		.B.TPBDIV = ftpb_div, .B.FSI2DIV = 1,  .B.FSIDIV = ffsi_div,
		.B.STMDIV = fstm_div, .B.LPDIV = 0,
#if DT_NODE_EXISTS(DT_NODELABEL(fcpb))
		.B.CPBDIV = fcpb_div,
#endif
		.B.UP = 1,
	};
	Ifx_CLOCK_SYSCCUCON1 sysccu1 = {
		.B.MCANHDIV = CLOCK_DIV_WITH_INST(fmcanh, infineon_mcmcan),
		.B.LETHDIV = CLOCK_DIV_WITH_INST(fleth, infineon_tc4x_leth),
#if DT_NODE_EXISTS(DT_NODELABEL(fcanxl))
		.B.CANXLHDIV = CLOCK_DIV_WITH_INST(fcanxl, infineon_canxl),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fgtm))
		.B.GTMDIV = CLOCK_DIV_WITH_STATUS(fgtm_div),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fegtm))
		.B.EGTMDIV = CLOCK_DIV_WITH_STATUS(fegtm),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fgeth))
		.B.GETHDIV = CLOCK_DIV_WITH_INST(fgeth, infineon_tc4x_geth),
#endif
	};
	Ifx_CLOCK_PERCCUCON0 perccu0 = {
		.B.CLKSELMCAN = CLOCK_SEL_WITH_INST(fmcan,
			fsource1, fosc, infineon_mcmcan),
		.B.MCANDIV = CLOCK_DIV_WITH_INST(fmcani, infineon_mcmcan),
		.B.CLKSELQSPI = CLOCK_SEL_WITH_INST(fsourceqspi,
			fsource1, fsource2, infineon_aurix_qspi),
		.B.QSPIDIV = CLOCK_DIV_WITH_INST(fqspi, infineon_aurix_qspi),
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(fppu)), (.B.PPUDIV = fppu_div,))
		.B.I2CDIV = CLOCK_DIV_WITH_INST(fi2c, infineon_aurix_i2c),
	};
	Ifx_CLOCK_PERCCUCON1 perccu1 = {
		.B.ASCLINFDIV = CLOCK_DIV_WITH_INST(fasclinf,
			infineon_asclin_uart),
		.B.CLKSELASCLINS = CLOCK_SEL_WITH_INST(fasclins,
			fasclinsi, fosc, infineon_asclin_uart),
		.B.ASCLINSDIV = CLOCK_DIV_WITH_INST(fasclinsi,
			infineon_asclin_uart),
		.B.LETH100PERON = !!CLOCK_DIV_WITH_STATUS(fleth100),
	};

	/* clang-format on */
	CLOCK_SYSCCUCON1 = sysccu1;
	CLOCK_SYSCCUCON0 = sysccu0;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback_div);
	/* Disable all peripheral clocks before setting them again to change clock sources. */
	CLOCK_PERCCUCON0.U = 0;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback_div);
	CLOCK_PERCCUCON0 = perccu0;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback_div);
	CLOCK_PERCCUCON1.U = 0;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback_div);
	CLOCK_PERCCUCON1 = perccu1;
	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback_div);
	return 0;
}

static int clock_control_tc4x_ccu_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint32_t clock_id = *((uint32_t *)sys);

	switch (clock_id) {
	case CLOCK_FSRI:
		return fsri_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FSPB:
		return fspb_div != 0 ? 0 : -ENOSYS;
#if DT_NODE_EXISTS(DT_NODELABEL(fcpb))
	case CLOCK_FCPB:
		return fcpb_div != 0 ? 0 : -ENOSYS;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fgtm))
	case CLOCK_FGTM:
		return fgtm_div != 0 ? 0 : -ENOSYS;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fegtm))
	case CLOCK_FEGTM:
		return fegtm_div != 0 ? 0 : -ENOSYS;
#endif
	case CLOCK_FSTM:
		return fstm_div != 0 ? 0 : -ENOSYS;
		/* case CLOCK_FMSC: return fmsc_div != 0 ? 0 : -ENOSYS; */
#if DT_NODE_EXISTS(DT_NODELABEL(fgeth))
	case CLOCK_FGETH:
		return fgeth_div != 0 ? 0 : -ENOSYS;
#endif
	case CLOCK_FLETH:
		return fleth_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FMCANH:
		return fmcanh_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FMCAN:
		return fmcani_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FASCLINF:
		return fasclinf_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FASCLINS:
		return fasclinsi_div != 0 ? 0 : -ENOSYS;
	case CLOCK_FQSPI:
		return fqspi_div != 0 ? 0 : -ENOSYS;
	/*case CLOCK_FADC: return fadc_div != 0 ? 0 : -ENOSYS; */
	case CLOCK_FI2C:
		return fi2c_div != 0 ? 0 : -ENOSYS;
	default:
		return -EINVAL;
	}
}

static int clock_control_tc4x_ccu_off(const struct device *dev, clock_control_subsys_t sys)
{

	return -ENOSYS;
}

static enum clock_control_status clock_control_tc4x_ccu_get_status(const struct device *dev,
								    clock_control_subsys_t sys)
{
	return -ENOSYS;
}

static int clock_control_tc4x_ccu_get_rate(const struct device *dev, clock_control_subsys_t sys,
					    uint32_t *rate)
{
	uint32_t clock_id = *((uint32_t *)sys);
	struct clock_control_tc4x_data *data = dev->data;

	switch (clock_id) {
	case CLOCK_FSRI:
		*rate = data->fsource0 / fspb_div;
		return 0;
	case CLOCK_FSPB:
		*rate = data->fsource0 / fspb_div;
		return 0;
#if DT_NODE_EXISTS(DT_NODELABEL(fcpb))
	case CLOCK_FCPB:
		*rate = data->fsource0 / fcpb_div;
		return 0;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fgtm))
	case CLOCK_FGTM:
		*rate = data->fsource0 / fgtm_div;
		return 0;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fegtm))
	case CLOCK_FEGTM:
		*rate = data->fsource0 / fegtm_div;
		return 0;
#endif
	case CLOCK_FSTM:
		*rate = data->fsource0 / fstm_div;
		return 0;
#if DT_NODE_EXISTS(DT_NODELABEL(fgeth))
	case CLOCK_FGETH:
		*rate = data->fsource0 / fgeth_div;
		return 0;
#endif
	case CLOCK_FLETH:
		*rate = data->fsource0 / fleth_div;
		return 0;
	case CLOCK_FMCANH:
		*rate = data->fsource0 / fmcanh_div;
		return 0;
	case CLOCK_FMCAN:
		*rate = data->fsource1 / fmcani_div;
		return 0;
	case CLOCK_FASCLINF:
		*rate = data->fsource2 / fasclinf_div;
		return 0;
	case CLOCK_FASCLINS:
		*rate = data->fsource1 / fasclinsi_div;
		return 0;
	case CLOCK_FQSPI:
		*rate = (CLOCK_SOURCE_IS(fsourceqspi, fsource1) ? data->fsource1 : data->fsource2) /
			fqspi_div;
		return 0;
	case CLOCK_FI2C:
		*rate = data->fsource2 / fi2c_div;
		return 0;
	default:
		return -EINVAL;
	}
}

static int clock_control_tc4x_ccu_init(const struct device *dev)
{
	int ret;

	if (CONFIG_TRICORE_CORE_ID != 0) {
		return 0;
	}

	WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(fback);

	if (CLOCK_IS_CLOCK_SOURCE(fosc, FOSC_SOURCES)) {
		ret = clock_control_tc4x_ccu_osc_init();
		if (ret) {
			return ret;
		}
	}

	if (CLOCK_SOURCE_IS(fsource0, fback)) {
		clock_control_tc4x_ccu_set_sys_source(SYS_SOURCE_FBACK);
	} else {
		ret = clock_control_tc4x_ccu_ramposc_init();
		if (ret) {
			return ret;
		}
		clock_control_tc4x_ccu_set_sys_source(SYS_SOURCE_FRAMP);
	}

	if (CLOCK_SOURCE_IS(fsource0, fpll0)) {
		/* TODO: Check for App Reset and Clock config */
		ret = clock_control_tc4x_ccu_syspll_power(false);
		if (ret) {
			return ret;
		}

		ret = clock_control_tc4x_ccu_syspll_default();
		if (ret) {
			return ret;
		}

		ret = clock_control_tc4x_ccu_syspll_init();
		if (ret) {
			return ret;
		}
	}

	/* Switch to peripherals to backup source for runtime/pll configuration */
	clock_control_tc4x_ccu_set_per_source(PER_SOURCE_FBACK);

	if (CLOCK_SOURCE_IS(fsource1, fpll1)) {
		/* TODO: Check for App Reset and Clock config */
		ret = clock_control_tc4x_ccu_perpll_power(false);
		if (ret) {
			return ret;
		}

		ret = clock_control_tc4x_ccu_perpll_default();
		if (ret) {
			return ret;
		}

		ret = clock_control_tc4x_ccu_perpll_init();
		if (ret) {
			return ret;
		}
	}

	ret = clock_control_tc4x_ccu_set_divider();
	if (ret) {
		return ret;
	}

	ret = clock_control_tc4x_ccu_ramposc_move(true);
	if (ret) {
		return ret;
	}

	ccu_busy_wait_framp(1000);

	if (CLOCK_RAMPSTAT.B.FLLLOCK != 1) {
		return -EIO;
	}
	if (CLOCK_SYSPLLSTAT.B.PLLLOCK != 1) {
		return -EIO;
	}
	if (CLOCK_PERPLLSTAT.B.PLLLOCK != 1) {
		return -EIO;
	}

	ret = clock_control_tc4x_ccu_syspll_divider();
	if (ret) {
		return ret;
	}
	clock_control_tc4x_ccu_set_sys_source(SYS_SOURCE_PLL);

	ret = clock_control_tc4x_ccu_perpll_divider();
	if (ret) {
		return ret;
	}
	clock_control_tc4x_ccu_set_per_source(PER_SOURCE_PLL);

	return 0;
}

#if CONFIG_CLOCK_CONTROL_TC4X_CCU_INIT_CHECK_ONLY
static int clock_control_tc4x_ccu_init_check(const struct device *dev)
{

	return 0;
}

#define CCU_INIT clock_control_tc4x_ccu_init_check
#else
#define CCU_INIT clock_control_tc4x_ccu_init
#endif

static struct clock_control_tc4x_data clock_control_tc4x_data = {
	.fsource0 = fsource0_freq,
	.fsource1 = fsource1_freq,
	.fsource2 = fsource2_freq,
	.fsource3 = fsource3_freq,

	.divider = {
		.fasclinsi_div = fasclinsi_div,
		.fasclinf_div = fasclinf_div,
	}};

static const struct clock_control_driver_api clock_control_tc4x_ccu_api = {
	.on = clock_control_tc4x_ccu_on,
	.off = clock_control_tc4x_ccu_off,
	.get_rate = clock_control_tc4x_ccu_get_rate,
	.get_status = clock_control_tc4x_ccu_get_status,
};

DEVICE_DT_INST_DEFINE(0, &CCU_INIT, NULL, &clock_control_tc4x_data, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_tc4x_ccu_api);
