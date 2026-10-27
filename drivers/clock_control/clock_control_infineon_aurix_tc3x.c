/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc3x_ccu

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/infineon-aurix-ccu.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <aurix_endinit.h>

#include "clock_control_infineon_aurix.h"

/* SCU clock registers (TC3xx UM v2.0.0, Table 276) */
#define OSCCON     0x10
#define SYSPLLSTAT 0x14
#define SYSPLLCON0 0x18
#define SYSPLLCON1 0x1C
#define PERPLLSTAT 0x24
#define PERPLLCON0 0x28
#define PERPLLCON1 0x2C
#define CCUCON0    0x30
#define CCUCON1    0x34
#define CCUCON2    0x40
#define CCUCON5    0x4C

#define OSCCON_PLLLV  BIT(1)
#define OSCCON_OSCRES BIT(2)
#define OSCCON_MODE   GENMASK(6, 5)
#define OSCCON_PLLHV  BIT(8)
#define OSCCON_OSCVAL GENMASK(20, 16)

#define OSCCON_MODE_XTAL 0
#define OSCCON_VALID     (OSCCON_PLLLV | OSCCON_PLLHV)

#define PLLCON0_DIVBY  BIT(0)
#define PLLCON0_NDIV   GENMASK(15, 9)
#define PLLCON0_PLLPWD BIT(16)
#define PLLCON0_RESLD  BIT(18)
#define PLLCON0_PDIV   GENMASK(26, 24)
#define PLLCON0_INSEL  GENMASK(31, 30)

#define INSEL_BACK 0
#define INSEL_OSC  1

#define PLLCON1_K2DIV GENMASK(2, 0)
#define PLLCON1_K3DIV GENMASK(10, 8)

#define PLLSTAT_PWDSTAT BIT(1)
#define PLLSTAT_LOCK    BIT(2)
#define PLLSTAT_K3RDY   BIT(4)
#define PLLSTAT_K2RDY   BIT(5)

#define CCUCON0_STMDIV GENMASK(3, 0)
#define CCUCON0_SRIDIV GENMASK(11, 8)
#define CCUCON0_LPDIV  GENMASK(14, 12)
#define CCUCON0_SPBDIV GENMASK(19, 16)
#define CCUCON0_FSIDIV GENMASK(25, 24)
#define CCUCON0_CLKSEL GENMASK(29, 28)
#define CCUCON0_UP     BIT(30)
#define CCUCON_LCK     BIT(31)

#define CLKSEL_BACK 0
#define CLKSEL_PLL  1

#define CCUCON1_MCANDIV    GENMASK(3, 0)
#define CCUCON1_CLKSELMCAN GENMASK(5, 4)
#define CCUCON1_PLL1DIVDIS BIT(7)
#define CCUCON2_ASCLINFDIV GENMASK(3, 0)
#define CCUCON5_MCANHDIV   GENMASK(7, 4)

#define CLKSELMCAN_OFF   0
#define CLKSELMCAN_MCANI 1

#define CORE_ID_MASK GENMASK(2, 0)

#define LCK_TIMEOUT_US  1000
#define OSC_TIMEOUT_US  10000
#define PWR_TIMEOUT_US  1000
#define LOCK_TIMEOUT_US 10000
#define PLL_SETTLE_US   1000
/* Settling time after each fPLL0 step (TC3xx UM, clock ramp-up example) */
#define STEP_WAIT_US    100

#define CCU_BASE ((mem_addr_t)DT_INST_REG_ADDR(0))

#define SYS_PLL  DT_INST_CHILD(0, sys_pll)
#define PER_PLL  DT_INST_CHILD(0, per_pll)
#define PLL_USED DT_NODE_HAS_STATUS_OKAY(SYS_PLL)

#define HAS_OSC DT_INST_NODE_HAS_PROP(0, clocks)
#define FOSC_HZ COND_CODE_1(HAS_OSC, (DT_PROP(DT_INST_CLOCKS_CTLR(0), clock_frequency)), (0))

#define SYS_P  DT_PROP_OR(SYS_PLL, p_div, 1)
#define SYS_N  DT_PROP_OR(SYS_PLL, n_div, 1)
#define SYS_K2 DT_PROP_OR(SYS_PLL, k2_div, 1)
#define PER_P  DT_PROP_OR(PER_PLL, p_div, 1)
#define PER_N  DT_PROP_OR(PER_PLL, n_div, 1)
#define PER_K2 DT_PROP_OR(PER_PLL, k2_div, 1)
#define PER_K3 DT_PROP_OR(PER_PLL, k3_div, 1)

#define DT_SYS_DCO  ((uint64_t)FOSC_HZ * SYS_N / SYS_P)
#define DT_PER_DCO  ((uint64_t)FOSC_HZ * PER_N / PER_P)
#define DT_FSOURCE0 (PLL_USED ? DT_SYS_DCO / SYS_K2 : AURIX_CCU_FBACK_HZ)
/* fsource0 is first switched to fPLL0 at about the fBACK frequency */
#define SYS_K2_START CLAMP(DIV_ROUND_UP(DT_SYS_DCO, AURIX_CCU_FBACK_HZ), SYS_K2, 8)

/* FSIDIV 2 and 3 divide fSRI only for SRIDIV 1 or 2 */
#define DT_FSI_DIV                                                                                 \
	((AURIX_CCU_DT_DIV(sri_div) <= 2) ? AURIX_CCU_DT_DIV(fsi_div) : 1)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "one CCU expected");

AURIX_CCU_CHECK_DIV(sri_div);
AURIX_CCU_CHECK_DIV(spb_div);
AURIX_CCU_CHECK_DIV(stm_div);
AURIX_CCU_CHECK_DIV(asclinf_div);
AURIX_CCU_CHECK_DIV(mcan_div);
AURIX_CCU_CHECK_DIV(mcanh_div);
BUILD_ASSERT(AURIX_CCU_DT_DIV(spb_div) >= 2, "SPBDIV 1 is reserved");
BUILD_ASSERT(IN_RANGE(AURIX_CCU_DT_DIV(fsi_div), 1, 3), "fsi-div must be 1 to 3");

/* fsource0, fsrc1 and fsource2 switch together, so both PLLs are needed */
BUILD_ASSERT(PLL_USED == DT_NODE_HAS_STATUS_OKAY(PER_PLL), "sys-pll and per-pll go together");
BUILD_ASSERT(!PLL_USED || IN_RANGE(FOSC_HZ, MHZ(16), MHZ(40)), "fOSC must be 16 to 40 MHz");
BUILD_ASSERT(!PLL_USED || (IN_RANGE(SYS_P, 1, 8) && IN_RANGE(SYS_N, 1, 128) &&
			   IN_RANGE(SYS_K2, 1, 8) && IN_RANGE(PER_P, 1, 8) &&
			   IN_RANGE(PER_N, 1, 128) && IN_RANGE(PER_K2, 1, 8)),
	     "PLL divider out of range");
/* PER_PLL_TC.002: K3DIV only 0 or 1 */
BUILD_ASSERT(!PLL_USED || (DT_NODE_HAS_PROP(PER_PLL, k3_div) && IN_RANGE(PER_K3, 1, 2)),
	     "per-pll k3-div must be 1 or 2");
/* TC37x data sheet: fPLL0 <= 300 MHz, fPLL1 <= 320 MHz, fPLL2 <= 200 MHz */
BUILD_ASSERT(!PLL_USED || (DT_FSOURCE0 <= MHZ(300) && DT_PER_DCO / PER_K2 <= MHZ(320) &&
			   DT_PER_DCO * 10 / (16 * PER_K3) <= MHZ(200)),
	     "PLL output frequency too high");

/* Divider ratios, TC3xx UM Table 280 */
BUILD_ASSERT(AURIX_CCU_DT_DIV(spb_div) % AURIX_CCU_DT_DIV(sri_div) == 0, "fSRI:fSPB not integer");
BUILD_ASSERT((2 * AURIX_CCU_DT_DIV(spb_div)) % (AURIX_CCU_DT_DIV(sri_div) * DT_FSI_DIV) == 0,
	     "fFSI:fSPB/2 not integer");

#ifdef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/* The kernel timer rate is a build-time constant */
BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(0, stm_div) ||
		     DT_FSOURCE0 / AURIX_CCU_DT_DIV(stm_div) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "fSTM differs from CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");
#endif

static uint32_t scu_read(uint32_t off)
{
	return sys_read32(CCU_BASE + off);
}

/* The clock registers are Safety ENDINIT protected */
static void scu_write(uint32_t off, uint32_t val)
{
	unsigned int key = aurix_endinit_safety_unlock();

	sys_write32(val, CCU_BASE + off);
	aurix_endinit_safety_lock(key);
}

static bool on_pll(void)
{
	return FIELD_GET(CCUCON0_CLKSEL, scu_read(CCUCON0)) == CLKSEL_PLL;
}

static uint32_t pll_dco(uint32_t con0)
{
	uint32_t fin;

	/* SYSPLLCON0.INSEL selects the input of both PLLs */
	switch (FIELD_GET(PLLCON0_INSEL, scu_read(SYSPLLCON0))) {
	case INSEL_BACK:
		fin = AURIX_CCU_FBACK_HZ;
		break;
	case INSEL_OSC:
		fin = FOSC_HZ;
		break;
	default:
		fin = 0;
		break;
	}

	return (uint32_t)((uint64_t)fin * (FIELD_GET(PLLCON0_NDIV, con0) + 1) /
			  (FIELD_GET(PLLCON0_PDIV, con0) + 1));
}

static uint32_t fsource0(void)
{
	if (!on_pll()) {
		return AURIX_CCU_FBACK_HZ;
	}

	return pll_dco(scu_read(SYSPLLCON0)) /
	       (FIELD_GET(PLLCON1_K2DIV, scu_read(SYSPLLCON1)) + 1);
}

/* fsource2: K3 output of the peripheral PLL after the 1.6 or 2 pre-divider */
static uint32_t fsource2(void)
{
	uint32_t con0 = scu_read(PERPLLCON0);
	uint32_t k3 = FIELD_GET(PLLCON1_K3DIV, scu_read(PERPLLCON1)) + 1;

	if (!on_pll()) {
		return AURIX_CCU_FBACK_HZ;
	}

	return (uint32_t)((uint64_t)pll_dco(con0) * 10 /
			  (k3 * (((con0 & PLLCON0_DIVBY) != 0U) ? 20U : 16U)));
}

static uint32_t fsource1(void)
{
	uint32_t fpll1;

	if (!on_pll()) {
		return AURIX_CCU_FBACK_HZ;
	}

	fpll1 = pll_dco(scu_read(PERPLLCON0)) /
		(FIELD_GET(PLLCON1_K2DIV, scu_read(PERPLLCON1)) + 1);

	return ((scu_read(CCUCON1) & CCUCON1_PLL1DIVDIS) != 0U) ? fpll1 : fpll1 / 2;
}

/* Upper bound of the current STM frequency, for busy waits */
static uint32_t fstm_upper(void)
{
	return fsource0() / MAX(FIELD_GET(CCUCON0_STMDIV, scu_read(CCUCON0)), 1U);
}

static int wait_reg(uint32_t off, uint32_t mask, uint32_t val, uint32_t timeout_us)
{
	return aurix_ccu_wait(CCU_BASE + off, mask, val, fstm_upper(), timeout_us);
}

/* CCUCONx take writes only while their LCK bit is clear */
static int ccucon_write(uint32_t off, uint32_t val)
{
	int ret = wait_reg(off, CCUCON_LCK, 0, LCK_TIMEOUT_US);

	if (ret != 0) {
		return ret;
	}

	scu_write(off, val);

	return wait_reg(off, CCUCON_LCK, 0, LCK_TIMEOUT_US);
}

static int set_clksel(uint32_t sel)
{
	uint32_t val = AURIX_CCU_FIELD_SET(scu_read(CCUCON0), CCUCON0_CLKSEL, sel);

	return ccucon_write(CCUCON0, val | CCUCON0_UP);
}

static int osc_init(void)
{
	uint32_t osccon = scu_read(OSCCON);
	uint32_t val;

	val = AURIX_CCU_FIELD_SET(osccon, OSCCON_MODE, OSCCON_MODE_XTAL);
	val = AURIX_CCU_FIELD_SET(val, OSCCON_OSCVAL, FOSC_HZ / MHZ(1) - 15);
	if (val == osccon && (osccon & OSCCON_VALID) == OSCCON_VALID) {
		return 0;
	}

	/* Restart the oscillator watchdog for the new reference */
	scu_write(OSCCON, val | OSCCON_OSCRES);

	return wait_reg(OSCCON, OSCCON_VALID, OSCCON_VALID, OSC_TIMEOUT_US);
}

/* A new P divider needs another value written first */
static void pll_power_up(uint32_t off, uint32_t con0)
{
	uint32_t cur = scu_read(off);
	uint32_t pdiv = FIELD_GET(PLLCON0_PDIV, con0);

	if (FIELD_GET(PLLCON0_PDIV, cur) != pdiv) {
		uint32_t tmp = (pdiv + 1) & 0x7;

		if (tmp == FIELD_GET(PLLCON0_PDIV, cur)) {
			tmp = (tmp + 1) & 0x7;
		}
		scu_write(off, AURIX_CCU_FIELD_SET(cur, PLLCON0_PDIV, tmp));
	}

	scu_write(off, con0 | PLLCON0_PLLPWD | PLLCON0_RESLD);
}

/* Both PLLs locked, K dividers running on their configured values */
static int plls_ready(void)
{
	const uint32_t sys_rdy = PLLSTAT_LOCK | PLLSTAT_K2RDY;
	const uint32_t per_rdy = PLLSTAT_LOCK | PLLSTAT_K2RDY | PLLSTAT_K3RDY;
	int ret = wait_reg(SYSPLLSTAT, sys_rdy, sys_rdy, LOCK_TIMEOUT_US);

	return (ret == 0) ? wait_reg(PERPLLSTAT, per_rdy, per_rdy, LOCK_TIMEOUT_US) : ret;
}

static int pll_init(void)
{
	int ret;

	pll_power_up(SYSPLLCON0, FIELD_PREP(PLLCON0_INSEL, INSEL_OSC) |
					 FIELD_PREP(PLLCON0_NDIV, SYS_N - 1) |
					 FIELD_PREP(PLLCON0_PDIV, SYS_P - 1));
	/* DIVBY = 0, the 1.6 pre-divider; DIVBY = 1 must not be used (PER_PLL_TC.001) */
	pll_power_up(PERPLLCON0,
		     FIELD_PREP(PLLCON0_NDIV, PER_N - 1) | FIELD_PREP(PLLCON0_PDIV, PER_P - 1));

	ret = wait_reg(SYSPLLSTAT, PLLSTAT_PWDSTAT, 0, PWR_TIMEOUT_US);
	if (ret == 0) {
		ret = wait_reg(PERPLLSTAT, PLLSTAT_PWDSTAT, 0, PWR_TIMEOUT_US);
	}
	if (ret != 0) {
		return ret;
	}

	aurix_ccu_delay_us(fstm_upper(), PLL_SETTLE_US);

	/* K dividers are locked while their RDY bit is clear */
	ret = plls_ready();
	if (ret != 0) {
		return ret;
	}

	scu_write(SYSPLLCON1, FIELD_PREP(PLLCON1_K2DIV, SYS_K2_START - 1));
	scu_write(PERPLLCON1, FIELD_PREP(PLLCON1_K2DIV, PER_K2 - 1) |
				      FIELD_PREP(PLLCON1_K3DIV, PER_K3 - 1));

	return plls_ready();
}

static uint32_t div_field(uint32_t reg, uint32_t mask, uint32_t divider)
{
	return (divider != 0U) ? AURIX_CCU_FIELD_SET(reg, mask, divider) : reg;
}

static int dividers_init(void)
{
	uint32_t con0 = scu_read(CCUCON0);
	uint32_t con1 = scu_read(CCUCON1);
	uint32_t con2 = scu_read(CCUCON2);
	uint32_t con5 = scu_read(CCUCON5);
	int ret = 0;

	if (PLL_USED) {
		/* fsource1 = fPLL1 / 2 for K2DIV 0 or 1, else fPLL1 (TC3xx UM hint) */
		con1 = (PER_K2 <= 2) ? (con1 & ~CCUCON1_PLL1DIVDIS) : (con1 | CCUCON1_PLL1DIVDIS);
	}

	if (DT_INST_NODE_HAS_PROP(0, mcan_div)) {
		uint32_t sel = FIELD_GET(CCUCON1_CLKSELMCAN, con1);

		/* A change between two fMCAN sources goes through CLKSELMCAN = 0 */
		if (sel != CLKSELMCAN_OFF && sel != CLKSELMCAN_MCANI) {
			ret = ccucon_write(CCUCON1, AURIX_CCU_FIELD_SET(con1, CCUCON1_CLKSELMCAN,
									CLKSELMCAN_OFF));
		}
		con1 = AURIX_CCU_FIELD_SET(con1, CCUCON1_MCANDIV, DT_INST_PROP_OR(0, mcan_div, 0));
		con1 = AURIX_CCU_FIELD_SET(con1, CCUCON1_CLKSELMCAN, CLKSELMCAN_MCANI);
	}

	if (ret == 0 && con1 != scu_read(CCUCON1)) {
		ret = ccucon_write(CCUCON1, con1);
	}

	con2 = div_field(con2, CCUCON2_ASCLINFDIV, DT_INST_PROP_OR(0, asclinf_div, 0));
	if (ret == 0 && con2 != scu_read(CCUCON2)) {
		ret = ccucon_write(CCUCON2, con2);
	}

	con5 = div_field(con5, CCUCON5_MCANHDIV, DT_INST_PROP_OR(0, mcanh_div, 0));
	if (ret == 0 && con5 != scu_read(CCUCON5)) {
		ret = ccucon_write(CCUCON5, con5);
	}

	con0 = AURIX_CCU_FIELD_SET(con0, CCUCON0_LPDIV, 0);
	con0 = div_field(con0, CCUCON0_STMDIV, DT_INST_PROP_OR(0, stm_div, 0));
	con0 = div_field(con0, CCUCON0_SRIDIV, DT_INST_PROP_OR(0, sri_div, 0));
	con0 = div_field(con0, CCUCON0_SPBDIV, DT_INST_PROP_OR(0, spb_div, 0));
	con0 = div_field(con0, CCUCON0_FSIDIV, DT_INST_PROP_OR(0, fsi_div, 0));

	/* UP transfers CCUCON0 and CCUCON5 */
	return (ret == 0) ? ccucon_write(CCUCON0, con0 | CCUCON0_UP) : ret;
}

/* Raise fPLL0 in K2 steps to limit supply current transients */
static int sys_pll_step_up(void)
{
	int ret = 0;

	for (uint32_t k2 = SYS_K2_START; k2 > SYS_K2 && ret == 0; k2--) {
		scu_write(SYSPLLCON1, FIELD_PREP(PLLCON1_K2DIV, k2 - 2));
		ret = wait_reg(SYSPLLSTAT, PLLSTAT_K2RDY, PLLSTAT_K2RDY, LOCK_TIMEOUT_US);
		aurix_ccu_delay_us(fstm_upper(), STEP_WAIT_US);
	}

	return ret;
}

static int ccu_tc3x_init(const struct device *dev)
{
	int ret = 0;

	ARG_UNUSED(dev);

	/* Only CPU0 configures the clock tree */
	if ((cr_read(TRICORE_CORE_ID) & CORE_ID_MASK) != 0U) {
		return 0;
	}

	/* Clock ramp-up of the TC3xx UM, 10.5.3 */
	if (on_pll()) {
		ret = set_clksel(CLKSEL_BACK);
	}
	if (ret == 0 && PLL_USED) {
		ret = osc_init();
	}
	if (ret == 0 && PLL_USED) {
		ret = pll_init();
	}
	if (ret == 0) {
		ret = dividers_init();
	}
	if (ret == 0 && PLL_USED) {
		ret = set_clksel(CLKSEL_PLL);
	}
	if (ret == 0 && PLL_USED) {
		ret = sys_pll_step_up();
	}

	return ret;
}

static uint32_t domain_div(uint32_t id)
{
	uint32_t con0 = scu_read(CCUCON0);

	switch (id) {
	case AURIX_CCU_SRI:
		return FIELD_GET(CCUCON0_SRIDIV, con0);
	case AURIX_CCU_SPB:
		return FIELD_GET(CCUCON0_SPBDIV, con0);
	case AURIX_CCU_FSI:
		/* fFSI = fSRI / FSIDIV, divided only for SRIDIV 1 or 2 */
		if (FIELD_GET(CCUCON0_FSIDIV, con0) == 0U) {
			return 0;
		}
		return FIELD_GET(CCUCON0_SRIDIV, con0) *
		       ((FIELD_GET(CCUCON0_SRIDIV, con0) <= 2U) ? FIELD_GET(CCUCON0_FSIDIV, con0)
								: 1U);
	case AURIX_CCU_STM:
		return FIELD_GET(CCUCON0_STMDIV, con0);
	case AURIX_CCU_ASCLINF:
		return FIELD_GET(CCUCON2_ASCLINFDIV, scu_read(CCUCON2));
	case AURIX_CCU_MCAN:
		if (FIELD_GET(CCUCON1_CLKSELMCAN, scu_read(CCUCON1)) != CLKSELMCAN_MCANI) {
			return 0;
		}
		return FIELD_GET(CCUCON1_MCANDIV, scu_read(CCUCON1));
	case AURIX_CCU_MCANH:
		return FIELD_GET(CCUCON5_MCANHDIV, scu_read(CCUCON5));
	default:
		return UINT32_MAX;
	}
}

static int ccu_tc3x_get_rate(const struct device *dev, clock_control_subsys_t sys,
			     uint32_t *rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uint32_t divider = domain_div(id);

	ARG_UNUSED(dev);

	if (divider == UINT32_MAX) {
		return -EINVAL;
	}

	if (divider == 0U) {
		*rate = 0;
	} else if (id == AURIX_CCU_ASCLINF) {
		*rate = fsource2() / divider;
	} else if (id == AURIX_CCU_MCAN) {
		*rate = fsource1() / divider;
	} else {
		*rate = fsource0() / divider;
	}

	return 0;
}

static int ccu_tc3x_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint32_t divider = domain_div((uint32_t)(uintptr_t)sys);

	ARG_UNUSED(dev);

	if (divider == UINT32_MAX) {
		return -EINVAL;
	}

	/* Domain clocks are shared and not gated here; usable when the divider runs */
	return (divider != 0U) ? 0 : -EIO;
}

static int ccu_tc3x_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOTSUP;
}

static enum clock_control_status ccu_tc3x_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	uint32_t divider = domain_div((uint32_t)(uintptr_t)sys);

	ARG_UNUSED(dev);

	if (divider == UINT32_MAX) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return (divider != 0U) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, ccu_tc3x_api) = {
	.on = ccu_tc3x_on,
	.off = ccu_tc3x_off,
	.get_rate = ccu_tc3x_get_rate,
	.get_status = ccu_tc3x_get_status,
};

DEVICE_DT_INST_DEFINE(0, ccu_tc3x_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ccu_tc3x_api);
