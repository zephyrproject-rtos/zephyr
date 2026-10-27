/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc4x_ccu

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/infineon-aurix-ccu.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "clock_control_infineon_aurix.h"

/* CLOCK module registers (TC4Dx UM v1.1, chapter 33.4) */
#define OSCCON     0x100
#define RAMPCON0   0x200
#define RAMPSTAT   0x20C
#define SYSPLLCON0 0x300
#define SYSPLLCON1 0x304
#define SYSPLLSTAT 0x30C
#define PERPLLCON0 0x380
#define PERPLLCON1 0x384
#define PERPLLCON2 0x388
#define PERPLLSTAT 0x38C
#define CCUCON     0x400
#define CCUSTAT    0x404
#define SYSCCUCON0 0x410
#define SYSCCUCON1 0x414
#define PERCCUCON0 0x420
#define PERCCUCON1 0x424

#define OSCCON_MODE  GENMASK(10, 8)
#define OSCCON_INSEL GENMASK(25, 24)

#define OSCCON_MODE_XTAL   0
#define OSCCON_INSEL_BACK  0
#define OSCCON_INSEL_OSC   1
#define OSCCON_INSEL_OSC_F 3

#define RAMPCON0_UFL GENMASK(9, 0)
#define RAMPCON0_CMD GENMASK(18, 16)
#define RAMPCON0_PWR BIT(19)

#define RAMPCON0_CMD_UP   1
#define RAMPCON0_CMD_DOWN 2

#define RAMPSTAT_FSTAT   GENMASK(17, 16)
#define RAMPSTAT_SSTAT   GENMASK(21, 20)
#define RAMPSTAT_ACTIVE  BIT(23)
#define RAMPSTAT_FLLLOCK BIT(24)

#define RAMPSTAT_FSTAT_BASE 1
#define RAMPSTAT_FSTAT_TOP  2

/* Ramp oscillator powered, idle and locked at the base or top frequency */
#define RAMPSTAT_READY_MASK (RAMPSTAT_FSTAT | RAMPSTAT_SSTAT | RAMPSTAT_ACTIVE | RAMPSTAT_FLLLOCK)
#define RAMPSTAT_READY(fstat)                                                                      \
	(FIELD_PREP(RAMPSTAT_FSTAT, fstat) | RAMPSTAT_ACTIVE | RAMPSTAT_FLLLOCK)

#define PLLCON0_PLLPWR BIT(0)
#define PLLCON0_RESLD  BIT(1)
#define PLLCON0_NDIV   GENMASK(14, 8)
#define PLLCON0_PDIV   GENMASK(18, 16)

#define PLLCON1_K2DIV    GENMASK(3, 0)
#define PLLCON1_K2PREDIV GENMASK(5, 4)
#define PLLCON1_K3DIV    GENMASK(10, 8)
#define PLLCON1_K3PREDIV GENMASK(15, 12)
#define PLLCON1_K4DIV    GENMASK(18, 16)
#define PLLCON1_K4PREDIV GENMASK(21, 20)

/* Pre-divider codes: K2/K4 1.0 and 2.0, K3 2.0 */
#define PREDIV24_1_0 0
#define PREDIV24_2_0 1
#define PREDIV3_2_0  0xA

#define PERPLLCON2_CLKSELSRC1 GENMASK(17, 16)

#define CLKSELSRC1_SRC1 1

#define PLLSTAT_PWRSTAT BIT(0)
#define PLLSTAT_PLLLOCK BIT(1)
#define PLLSTAT_READY   (PLLSTAT_PWRSTAT | PLLSTAT_PLLLOCK)

#define CCUCON_CLKSELS GENMASK(1, 0)
#define CCUCON_CLKSELP BIT(16)

#define CLKSELS_PLL  0
#define CLKSELS_BACK 1
#define CLKSELS_RAMP 2
#define CLKSELP_PLL  0
#define CLKSELP_BACK 1

#define CCUSTAT_CLKSELS GENMASK(1, 0)
#define CCUSTAT_CLKSELP BIT(16)
#define CCUSTAT_LCK     BIT(31)

#define SYSCCUCON0_SPBDIV GENMASK(3, 0)
#define SYSCCUCON0_CPBDIV GENMASK(7, 4)
#define SYSCCUCON0_TPBDIV GENMASK(11, 8)
#define SYSCCUCON0_SRIDIV GENMASK(15, 12)
#define SYSCCUCON0_FSIDIV GENMASK(19, 16)
#define SYSCCUCON0_STMDIV GENMASK(27, 24)
#define SYSCCUCON0_LPDIV  BIT(28)
#define SYSCCUCON0_UP     BIT(30)

#define SYSCCUCON1_MCANHDIV GENMASK(19, 16)

#define PERCCUCON0_MCANDIV    GENMASK(3, 0)
#define PERCCUCON0_CLKSELMCAN GENMASK(5, 4)

#define CLKSELMCAN_OFF   0
#define CLKSELMCAN_MCANI 1
#define CLKSELMCAN_OSC   2

#define PERCCUCON1_ASCLINFDIV GENMASK(3, 0)

#define CORE_ID_MASK GENMASK(2, 0)

/* CCUSTAT.LCK clears within 70 us; CCU_TC.012 asks for at least 100 us */
#define LCK_TIMEOUT_US  1000
#define PWR_TIMEOUT_US  1000
#define LOCK_TIMEOUT_US 10000
#define RAMP_TIMEOUT_US 10000
#define PLL_SETTLE_US   1000
#define PREDIV_WAIT_US  10
#define OSC_STARTUP_US  5000
/* 20 fSPB cycles between RAMPCON0 writes, fSPB >= 100 MHz / 15 */
#define RAMP_WRITE_US   5

#define CCU_BASE ((mem_addr_t)DT_INST_REG_ADDR(0))

#define SYS_PLL      DT_INST_CHILD(0, sys_pll)
#define PER_PLL      DT_INST_CHILD(0, per_pll)
#define SYS_PLL_USED DT_NODE_HAS_STATUS_OKAY(SYS_PLL)
#define PER_PLL_USED DT_NODE_HAS_STATUS_OKAY(PER_PLL)

#define HAS_OSC DT_INST_NODE_HAS_PROP(0, clocks)
#define FOSC_HZ COND_CODE_1(HAS_OSC, (DT_PROP(DT_INST_CLOCKS_CTLR(0), clock_frequency)), (0))

#define SYS_P     DT_PROP_OR(SYS_PLL, p_div, 1)
#define SYS_N     DT_PROP_OR(SYS_PLL, n_div, 1)
#define SYS_K2    DT_PROP_OR(SYS_PLL, k2_div, 1)
#define PER_P     DT_PROP_OR(PER_PLL, p_div, 1)
#define PER_N     DT_PROP_OR(PER_PLL, n_div, 1)
#define PER_K2    DT_PROP_OR(PER_PLL, k2_div, 1)
#define PER_K3    DT_PROP_OR(PER_PLL, k3_div, 1)
#define PER_K3PRE DT_PROP_OR(PER_PLL, k3_prediv, 10)
#define PER_K4    DT_PROP_OR(PER_PLL, k4_div, 1)
#define RAMP_UFL  DT_INST_PROP_OR(0, ramp_upper_mhz, 0)

#define DT_FPLL0    ((uint64_t)FOSC_HZ * SYS_N / (SYS_P * SYS_K2))
#define DT_FSOURCE0 (SYS_PLL_USED ? DT_FPLL0 : AURIX_CCU_FBACK_HZ)
#define DT_FSOURCE1                                                                                \
	(PER_PLL_USED ? (uint64_t)FOSC_HZ * PER_N / (PER_P * PER_K2) : AURIX_CCU_FBACK_HZ)

#define HAS_DIVS(a, b)  (DT_INST_NODE_HAS_PROP(0, a) && DT_INST_NODE_HAS_PROP(0, b))
#define DIV_RATIO(a, b) (AURIX_CCU_DT_DIV(a) / AURIX_CCU_DT_DIV(b))
#define DIV_INT(a, b)   (AURIX_CCU_DT_DIV(a) % AURIX_CCU_DT_DIV(b) == 0)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "one CLOCK module expected");

AURIX_CCU_CHECK_DIV(sri_div);
AURIX_CCU_CHECK_DIV(spb_div);
AURIX_CCU_CHECK_DIV(fsi_div);
AURIX_CCU_CHECK_DIV(stm_div);
AURIX_CCU_CHECK_DIV(tpb_div);
/* Also keeps CPBDIV != 0, which can cause a bus error (CCU_TC.026) */
AURIX_CCU_CHECK_DIV(cpb_div);
AURIX_CCU_CHECK_DIV(asclinf_div);
AURIX_CCU_CHECK_DIV(mcan_div);
AURIX_CCU_CHECK_DIV(mcanh_div);

BUILD_ASSERT(!(SYS_PLL_USED || PER_PLL_USED) || HAS_OSC, "PLLs need the oscillator in clocks");
BUILD_ASSERT(!SYS_PLL_USED ||
		     (IN_RANGE(SYS_P, 1, 8) && IN_RANGE(SYS_N, 1, 128) && IN_RANGE(SYS_K2, 1, 16)),
	     "sys-pll divider out of range");
BUILD_ASSERT(!PER_PLL_USED ||
		     (DT_NODE_HAS_PROP(PER_PLL, k3_div) && DT_NODE_HAS_PROP(PER_PLL, k4_div) &&
		      IN_RANGE(PER_P, 1, 8) && IN_RANGE(PER_N, 1, 128) && IN_RANGE(PER_K2, 1, 16) &&
		      IN_RANGE(PER_K3, 1, 8) && IN_RANGE(PER_K4, 1, 8)),
	     "per-pll needs k3-div and k4-div, dividers in range");
/* TC4Dx: fSYS at most 500 MHz */
BUILD_ASSERT(!SYS_PLL_USED || DT_FPLL0 <= MHZ(500), "fPLL0 above 500 MHz");
/* RAMPCON0.UFL: multiple of 100 MHz, at least 130 MHz, at most fPLL0 - 100 MHz */
BUILD_ASSERT(!SYS_PLL_USED || (RAMP_UFL >= 200 && RAMP_UFL % 100 == 0 &&
			       (uint64_t)MHZ(RAMP_UFL) + MHZ(100) <= DT_FPLL0),
	     "ramp-upper-mhz: multiple of 100, at least 200, at most fPLL0 - 100 MHz");

/* Divider ratios, TC4Dx UM Table 1525 with CCU_TC.022 */
BUILD_ASSERT(!HAS_DIVS(spb_div, sri_div) || DIV_INT(spb_div, sri_div), "fSRI:fSPB not integer");
BUILD_ASSERT(!HAS_DIVS(spb_div, fsi_div) || DIV_INT(spb_div, fsi_div), "fFSI:fSPB not integer");
BUILD_ASSERT(!HAS_DIVS(fsi_div, sri_div) ||
		     (DIV_INT(fsi_div, sri_div) && DIV_RATIO(fsi_div, sri_div) <= 5),
	     "fSRI:fFSI must be 1 to 5");
BUILD_ASSERT(!HAS_DIVS(stm_div, sri_div) || DIV_INT(stm_div, sri_div), "fSRI:fSTM not integer");
BUILD_ASSERT(!HAS_DIVS(tpb_div, sri_div) ||
		     (DIV_INT(tpb_div, sri_div) && DIV_RATIO(tpb_div, sri_div) <= 2),
	     "fSRI:fTPB must be 1 or 2");
BUILD_ASSERT(!HAS_DIVS(spb_div, tpb_div) ||
		     AURIX_CCU_DT_DIV(spb_div) >= 2 * AURIX_CCU_DT_DIV(tpb_div),
	     "fTPB:fSPB must be at least 2");
BUILD_ASSERT(!HAS_DIVS(cpb_div, sri_div) || DIV_INT(cpb_div, sri_div), "fSRI:fCPB not integer");
BUILD_ASSERT(!HAS_DIVS(mcan_div, mcanh_div) ||
		     DT_FSOURCE0 * 100 / AURIX_CCU_DT_DIV(mcanh_div) >=
			     DT_FSOURCE1 * 101 / AURIX_CCU_DT_DIV(mcan_div),
	     "fMCANH:fMCAN must be at least 1.01");

#ifdef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/* The kernel timer rate is a build-time constant */
BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(0, stm_div) ||
		     DT_FSOURCE0 / AURIX_CCU_DT_DIV(stm_div) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "fSTM differs from CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");
#endif

static uint32_t ccu_read(uint32_t off)
{
	return sys_read32(CCU_BASE + off);
}

static void ccu_write(uint32_t off, uint32_t val)
{
	sys_write32(val, CCU_BASE + off);
}

static uint32_t clksels(void)
{
	return FIELD_GET(CCUSTAT_CLKSELS, ccu_read(CCUSTAT));
}

static uint32_t pll_dco(uint32_t con0)
{
	uint32_t fin;

	switch (FIELD_GET(OSCCON_INSEL, ccu_read(OSCCON))) {
	case OSCCON_INSEL_BACK:
		fin = AURIX_CCU_FBACK_HZ;
		break;
	case OSCCON_INSEL_OSC:
	case OSCCON_INSEL_OSC_F:
		fin = FOSC_HZ;
		break;
	default:
		fin = 0;
		break;
	}

	return (uint32_t)((uint64_t)fin * (FIELD_GET(PLLCON0_NDIV, con0) + 1) /
			  (FIELD_GET(PLLCON0_PDIV, con0) + 1));
}

/* K2/K4 pre-divider in tenths: 1.0, 2.0, 1.2, 1.6 */
static uint32_t prediv24_tenths(uint32_t code)
{
	static const uint8_t tenths[] = {10, 20, 12, 16};

	return tenths[code & 0x3];
}

/* K3 pre-divider in tenths; codes not listed act as 1.0 */
static uint32_t prediv3_tenths(uint32_t code)
{
	switch (code) {
	case 1:
		return 11;
	case 2:
		return 12;
	case 4:
		return 14;
	case 6:
		return 16;
	case 7:
		return 17;
	case 0xA:
		return 20;
	default:
		return 10;
	}
}

static uint32_t prediv3_code(uint32_t tenths)
{
	for (uint32_t code = 0; code <= 0xA; code++) {
		if (prediv3_tenths(code) == tenths) {
			return code;
		}
	}

	return 0;
}

static uint32_t pll_out(uint32_t dco, uint32_t k, uint32_t pre_tenths)
{
	return (uint32_t)((uint64_t)dco * 10 / ((uint64_t)k * pre_tenths));
}

static uint32_t fpll0(void)
{
	uint32_t con1 = ccu_read(SYSPLLCON1);

	if ((ccu_read(SYSPLLSTAT) & PLLSTAT_PWRSTAT) == 0U) {
		return 0;
	}

	return pll_out(pll_dco(ccu_read(SYSPLLCON0)), FIELD_GET(PLLCON1_K2DIV, con1) + 1,
		       prediv24_tenths(FIELD_GET(PLLCON1_K2PREDIV, con1)));
}

/* fsource0; with upper set, an upper bound while the ramp oscillator moves */
static uint32_t fsource0(bool upper)
{
	uint32_t ufl = MAX(MHZ(FIELD_GET(RAMPCON0_UFL, ccu_read(RAMPCON0))), AURIX_CCU_FBACK_HZ);

	switch (clksels()) {
	case CLKSELS_PLL:
		return fpll0();
	case CLKSELS_BACK:
		return AURIX_CCU_FBACK_HZ;
	case CLKSELS_RAMP:
		if (upper || FIELD_GET(RAMPSTAT_FSTAT, ccu_read(RAMPSTAT)) == RAMPSTAT_FSTAT_TOP) {
			return ufl;
		}
		return AURIX_CCU_FBACK_HZ;
	default:
		/* Switch in progress */
		return upper ? MAX(fpll0(), ufl) : AURIX_CCU_FBACK_HZ;
	}
}

/* fsource2: K3 output of the peripheral PLL, or fBACK */
static uint32_t fsource2(void)
{
	uint32_t con1 = ccu_read(PERPLLCON1);

	if ((ccu_read(CCUSTAT) & CCUSTAT_CLKSELP) != 0U) {
		return AURIX_CCU_FBACK_HZ;
	}

	if ((ccu_read(PERPLLSTAT) & PLLSTAT_PWRSTAT) == 0U) {
		return 0;
	}

	return pll_out(pll_dco(ccu_read(PERPLLCON0)), FIELD_GET(PLLCON1_K3DIV, con1) + 1,
		       prediv3_tenths(FIELD_GET(PLLCON1_K3PREDIV, con1)));
}

static uint32_t fsource1(void)
{
	uint32_t con1 = ccu_read(PERPLLCON1);

	if (FIELD_GET(PERPLLCON2_CLKSELSRC1, ccu_read(PERPLLCON2)) != CLKSELSRC1_SRC1) {
		return 0;
	}

	if ((ccu_read(CCUSTAT) & CCUSTAT_CLKSELP) != 0U) {
		return AURIX_CCU_FBACK_HZ;
	}

	if ((ccu_read(PERPLLSTAT) & PLLSTAT_PWRSTAT) == 0U) {
		return 0;
	}

	return pll_out(pll_dco(ccu_read(PERPLLCON0)), FIELD_GET(PLLCON1_K2DIV, con1) + 1,
		       prediv24_tenths(FIELD_GET(PLLCON1_K2PREDIV, con1)));
}

/* Upper bound of the current STM frequency, for busy waits */
static uint32_t fstm_upper(void)
{
	return fsource0(true) / MAX(FIELD_GET(SYSCCUCON0_STMDIV, ccu_read(SYSCCUCON0)), 1U);
}

static int wait_reg(uint32_t off, uint32_t mask, uint32_t val, uint32_t timeout_us)
{
	return aurix_ccu_wait(CCU_BASE + off, mask, val, fstm_upper(), timeout_us);
}

static int lck_wait(void)
{
	return wait_reg(CCUSTAT, CCUSTAT_LCK, 0, LCK_TIMEOUT_US);
}

/* CLOCK registers take writes only while CCUSTAT.LCK is clear */
static int ccu_locked_write(uint32_t off, uint32_t val)
{
	int ret = lck_wait();

	if (ret != 0) {
		return ret;
	}

	ccu_write(off, val);

	return lck_wait();
}

static int ccu_set_clksels(uint32_t sel)
{
	return ccu_locked_write(CCUCON, AURIX_CCU_FIELD_SET(ccu_read(CCUCON), CCUCON_CLKSELS, sel));
}

static int ccu_set_clkselp(uint32_t sel)
{
	return ccu_locked_write(CCUCON, AURIX_CCU_FIELD_SET(ccu_read(CCUCON), CCUCON_CLKSELP, sel));
}

static int osc_init(void)
{
	uint32_t osccon = ccu_read(OSCCON);
	int ret;

	osccon = AURIX_CCU_FIELD_SET(osccon, OSCCON_MODE, OSCCON_MODE_XTAL);
	osccon = AURIX_CCU_FIELD_SET(osccon, OSCCON_INSEL, OSCCON_INSEL_OSC);
	if (osccon == ccu_read(OSCCON)) {
		return 0;
	}

	ret = ccu_locked_write(OSCCON, osccon);
	if (ret == 0) {
		/* No ready flag; let the crystal start before a PLL uses it */
		aurix_ccu_delay_us(fstm_upper(), OSC_STARTUP_US);
	}

	return ret;
}

static int ramp_write(uint32_t val)
{
	int ret = ccu_locked_write(RAMPCON0, val);

	aurix_ccu_delay_us(fstm_upper(), RAMP_WRITE_US);

	return ret;
}

static int ramp_wait(uint32_t fstat)
{
	return wait_reg(RAMPSTAT, RAMPSTAT_READY_MASK, RAMPSTAT_READY(fstat), RAMP_TIMEOUT_US);
}

/* Run fsource0 from the ramp oscillator at its 100 MHz base frequency */
static int sys_to_ramp_base(void)
{
	uint32_t rampcon0 = ccu_read(RAMPCON0);
	int ret = 0;

	/* Leave a system PLL set up by an earlier boot stage through the ramp */
	if (clksels() == CLKSELS_PLL) {
		bool at_top = (ccu_read(RAMPSTAT) & RAMPSTAT_READY_MASK) ==
			      RAMPSTAT_READY(RAMPSTAT_FSTAT_TOP);

		ret = ccu_set_clksels(at_top ? CLKSELS_RAMP : CLKSELS_BACK);
	}

	if (ret == 0 && (rampcon0 & RAMPCON0_PWR) == 0U) {
		ret = ramp_write(rampcon0 | RAMPCON0_PWR);
	}

	/* A ramp command is only taken while the sequencer is idle */
	if (ret == 0) {
		ret = wait_reg(RAMPSTAT, RAMPSTAT_ACTIVE | RAMPSTAT_SSTAT, RAMPSTAT_ACTIVE,
			       RAMP_TIMEOUT_US);
	}

	if (ret == 0 && FIELD_GET(RAMPSTAT_FSTAT, ccu_read(RAMPSTAT)) != RAMPSTAT_FSTAT_BASE) {
		ret = ramp_write(
			AURIX_CCU_FIELD_SET(ccu_read(RAMPCON0), RAMPCON0_CMD, RAMPCON0_CMD_DOWN));
	}

	if (ret == 0) {
		ret = ramp_wait(RAMPSTAT_FSTAT_BASE);
	}

	return (ret == 0) ? ccu_set_clksels(CLKSELS_RAMP) : ret;
}

/* PLL set-up sequence of erratum CCU_TC.012 */
static int pll_configure(uint32_t con0_off, uint32_t stat_off, uint32_t con0, uint32_t tmp_pre,
			 uint32_t tmp_div, uint32_t final_pre, uint32_t final)
{
	uint32_t con1_off = con0_off + 4;
	int ret;

	ret = ccu_locked_write(con0_off, ccu_read(con0_off) & ~PLLCON0_PLLPWR);
	if (ret == 0) {
		ret = wait_reg(stat_off, PLLSTAT_PWRSTAT, 0, PWR_TIMEOUT_US);
	}
	if (ret == 0) {
		ret = ccu_locked_write(con1_off, tmp_pre);
	}
	if (ret == 0) {
		ret = ccu_locked_write(con1_off, tmp_div);
	}
	if (ret == 0) {
		ret = ccu_locked_write(con0_off, con0);
	}
	if (ret == 0) {
		ret = ccu_locked_write(con0_off, con0 | PLLCON0_PLLPWR | PLLCON0_RESLD);
	}
	if (ret == 0) {
		ret = wait_reg(stat_off, PLLSTAT_PWRSTAT, PLLSTAT_PWRSTAT, PWR_TIMEOUT_US);
	}
	if (ret != 0) {
		return ret;
	}

	aurix_ccu_delay_us(fstm_upper(), PLL_SETTLE_US);

	ret = wait_reg(stat_off, PLLSTAT_PLLLOCK, PLLSTAT_PLLLOCK, LOCK_TIMEOUT_US);
	if (ret == 0 && final_pre != tmp_div) {
		ret = lck_wait();
		if (ret == 0) {
			ccu_write(con1_off, final_pre);
			aurix_ccu_delay_us(fstm_upper(), PREDIV_WAIT_US);
			ret = lck_wait();
		}
	}

	return (ret == 0) ? ccu_locked_write(con1_off, final) : ret;
}

static int sys_pll_init(void)
{
	uint32_t con0 = FIELD_PREP(PLLCON0_NDIV, SYS_N - 1) | FIELD_PREP(PLLCON0_PDIV, SYS_P - 1);
	uint32_t pre = ccu_read(SYSPLLCON1);
	uint32_t divider;

	/* K2PREDIV stays 1.0 (CCU_TC.023) */
	pre = AURIX_CCU_FIELD_SET(pre, PLLCON1_K2PREDIV, PREDIV24_1_0);
	pre = AURIX_CCU_FIELD_SET(pre, PLLCON1_K3PREDIV, PREDIV3_2_0);
	divider = AURIX_CCU_FIELD_SET(pre, PLLCON1_K2DIV, 1);
	divider = AURIX_CCU_FIELD_SET(divider, PLLCON1_K3DIV, 1);

	return pll_configure(SYSPLLCON0, SYSPLLSTAT, con0, pre, divider, divider,
			     AURIX_CCU_FIELD_SET(divider, PLLCON1_K2DIV, SYS_K2 - 1));
}

static int per_pll_init(void)
{
	uint32_t con0 = FIELD_PREP(PLLCON0_NDIV, PER_N - 1) | FIELD_PREP(PLLCON0_PDIV, PER_P - 1);
	uint32_t pre = ccu_read(PERPLLCON1);
	uint32_t divider, final_pre, final;

	pre = AURIX_CCU_FIELD_SET(pre, PLLCON1_K2PREDIV, PREDIV24_2_0);
	pre = AURIX_CCU_FIELD_SET(pre, PLLCON1_K3PREDIV, PREDIV3_2_0);
	pre = AURIX_CCU_FIELD_SET(pre, PLLCON1_K4PREDIV, PREDIV24_2_0);
	divider = AURIX_CCU_FIELD_SET(pre, PLLCON1_K2DIV, 1);
	divider = AURIX_CCU_FIELD_SET(divider, PLLCON1_K3DIV, 1);
	divider = AURIX_CCU_FIELD_SET(divider, PLLCON1_K4DIV, 1);

	/* K2 and K4 pre-dividers 1.0 in normal operation */
	final_pre = AURIX_CCU_FIELD_SET(divider, PLLCON1_K2PREDIV, PREDIV24_1_0);
	final_pre = AURIX_CCU_FIELD_SET(final_pre, PLLCON1_K3PREDIV, prediv3_code(PER_K3PRE));
	final_pre = AURIX_CCU_FIELD_SET(final_pre, PLLCON1_K4PREDIV, PREDIV24_1_0);
	final = AURIX_CCU_FIELD_SET(final_pre, PLLCON1_K2DIV, PER_K2 - 1);
	final = AURIX_CCU_FIELD_SET(final, PLLCON1_K3DIV, PER_K3 - 1);
	final = AURIX_CCU_FIELD_SET(final, PLLCON1_K4DIV, PER_K4 - 1);

	return pll_configure(PERPLLCON0, PERPLLSTAT, con0, pre, divider, final_pre, final);
}

static uint32_t div_field(uint32_t reg, uint32_t mask, uint32_t divider)
{
	return (divider != 0U) ? AURIX_CCU_FIELD_SET(reg, mask, divider) : reg;
}

static int mcan_init(void)
{
	uint32_t per0 = ccu_read(PERCCUCON0);
	uint32_t val;
	int ret = 0;

	if (!DT_INST_NODE_HAS_PROP(0, mcan_div)) {
		return 0;
	}

	val = AURIX_CCU_FIELD_SET(per0, PERCCUCON0_MCANDIV, DT_INST_PROP_OR(0, mcan_div, 0));
	val = AURIX_CCU_FIELD_SET(val, PERCCUCON0_CLKSELMCAN, CLKSELMCAN_MCANI);
	if (val == per0) {
		return 0;
	}

	/* A change between two fMCAN sources goes through CLKSELMCAN = 0 */
	if (FIELD_GET(PERCCUCON0_CLKSELMCAN, per0) != CLKSELMCAN_OFF &&
	    FIELD_GET(PERCCUCON0_CLKSELMCAN, per0) != CLKSELMCAN_MCANI) {
		ret = ccu_locked_write(PERCCUCON0, AURIX_CCU_FIELD_SET(per0, PERCCUCON0_CLKSELMCAN,
								       CLKSELMCAN_OFF));
	}

	return (ret == 0) ? ccu_locked_write(PERCCUCON0, val) : ret;
}

static int dividers_init(void)
{
	uint32_t sys0 = ccu_read(SYSCCUCON0) & ~SYSCCUCON0_LPDIV;
	uint32_t sys1 = ccu_read(SYSCCUCON1);
	uint32_t per1 = ccu_read(PERCCUCON1);
	int ret = 0;

	sys0 = div_field(sys0, SYSCCUCON0_SPBDIV, DT_INST_PROP_OR(0, spb_div, 0));
	sys0 = div_field(sys0, SYSCCUCON0_CPBDIV, DT_INST_PROP_OR(0, cpb_div, 0));
	sys0 = div_field(sys0, SYSCCUCON0_TPBDIV, DT_INST_PROP_OR(0, tpb_div, 0));
	sys0 = div_field(sys0, SYSCCUCON0_SRIDIV, DT_INST_PROP_OR(0, sri_div, 0));
	sys0 = div_field(sys0, SYSCCUCON0_FSIDIV, DT_INST_PROP_OR(0, fsi_div, 0));
	sys0 = div_field(sys0, SYSCCUCON0_STMDIV, DT_INST_PROP_OR(0, stm_div, 0));
	sys1 = div_field(sys1, SYSCCUCON1_MCANHDIV, DT_INST_PROP_OR(0, mcanh_div, 0));
	per1 = div_field(per1, PERCCUCON1_ASCLINFDIV, DT_INST_PROP_OR(0, asclinf_div, 0));

	if (sys1 != ccu_read(SYSCCUCON1)) {
		ret = ccu_locked_write(SYSCCUCON1, sys1);
	}

	/* UP transfers SYSCCUCON0 and SYSCCUCON1 */
	if (ret == 0) {
		ret = ccu_locked_write(SYSCCUCON0, sys0 | SYSCCUCON0_UP);
	}
	if (ret == 0 && per1 != ccu_read(PERCCUCON1)) {
		ret = ccu_locked_write(PERCCUCON1, per1);
	}

	return (ret == 0) ? mcan_init() : ret;
}

/* Ramp fsource0 up to ramp-upper-mhz, then switch it to fPLL0 */
static int sys_switch_to_pll(void)
{
	uint32_t rampcon0 = ccu_read(RAMPCON0) | RAMPCON0_PWR;
	int ret;

	rampcon0 = AURIX_CCU_FIELD_SET(rampcon0, RAMPCON0_UFL, RAMP_UFL);
	rampcon0 = AURIX_CCU_FIELD_SET(rampcon0, RAMPCON0_CMD, RAMPCON0_CMD_UP);

	ret = ramp_write(rampcon0);
	if (ret == 0) {
		ret = ramp_wait(RAMPSTAT_FSTAT_TOP);
	}
	if (ret == 0) {
		ret = wait_reg(SYSPLLSTAT, PLLSTAT_READY, PLLSTAT_READY, LOCK_TIMEOUT_US);
	}

	return (ret == 0) ? ccu_set_clksels(CLKSELS_PLL) : ret;
}

static int per_switch_to_pll(void)
{
	int ret = wait_reg(PERPLLSTAT, PLLSTAT_READY, PLLSTAT_READY, LOCK_TIMEOUT_US);

	return (ret == 0) ? ccu_set_clkselp(CLKSELP_PLL) : ret;
}

static int ccu_tc4x_init(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	/* Only CPU0 configures the clock tree */
	if ((cr_read(TRICORE_CORE_ID) & CORE_ID_MASK) != 0U) {
		return 0;
	}

	/* Start-up of the TC4Dx UM, 33.3.5.1.3, with erratum CCU_TC.020 */
	ret = lck_wait();
	if (ret == 0 && (ccu_read(CCUSTAT) & CCUSTAT_CLKSELP) == 0U) {
		ret = ccu_set_clkselp(CLKSELP_BACK);
	}
	if (ret == 0 && (SYS_PLL_USED || clksels() != CLKSELS_BACK)) {
		ret = sys_to_ramp_base();
	}
	if (ret == 0 && !SYS_PLL_USED && clksels() != CLKSELS_BACK) {
		ret = ccu_set_clksels(CLKSELS_BACK);
	}
	if (ret == 0 && (SYS_PLL_USED || PER_PLL_USED)) {
		ret = osc_init();
	}
	if (ret == 0 && SYS_PLL_USED) {
		ret = sys_pll_init();
	}
	if (ret == 0 && PER_PLL_USED) {
		ret = per_pll_init();
	}
	if (ret == 0) {
		ret = dividers_init();
	}
	if (ret == 0 && SYS_PLL_USED) {
		ret = sys_switch_to_pll();
	}
	if (ret == 0 && PER_PLL_USED) {
		ret = per_switch_to_pll();
	}

	return ret;
}

static uint32_t domain_div(uint32_t id)
{
	uint32_t sys0 = ccu_read(SYSCCUCON0);

	switch (id) {
	case AURIX_CCU_SRI:
		return FIELD_GET(SYSCCUCON0_SRIDIV, sys0);
	case AURIX_CCU_SPB:
		return FIELD_GET(SYSCCUCON0_SPBDIV, sys0);
	case AURIX_CCU_FSI:
		return FIELD_GET(SYSCCUCON0_FSIDIV, sys0);
	case AURIX_CCU_STM:
		return FIELD_GET(SYSCCUCON0_STMDIV, sys0);
	case AURIX_CCU_ASCLINF:
		return FIELD_GET(PERCCUCON1_ASCLINFDIV, ccu_read(PERCCUCON1));
	case AURIX_CCU_MCAN:
		if (FIELD_GET(PERCCUCON0_CLKSELMCAN, ccu_read(PERCCUCON0)) != CLKSELMCAN_MCANI) {
			return 0;
		}
		return FIELD_GET(PERCCUCON0_MCANDIV, ccu_read(PERCCUCON0));
	case AURIX_CCU_MCANH:
		return FIELD_GET(SYSCCUCON1_MCANHDIV, ccu_read(SYSCCUCON1));
	default:
		return UINT32_MAX;
	}
}

static int ccu_tc4x_get_rate(const struct device *dev, clock_control_subsys_t sys,
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
		*rate = fsource0(false) / divider;
	}

	return 0;
}

static int ccu_tc4x_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint32_t divider = domain_div((uint32_t)(uintptr_t)sys);

	ARG_UNUSED(dev);

	if (divider == UINT32_MAX) {
		return -EINVAL;
	}

	/* Domain clocks are shared and not gated here; usable when the divider runs */
	return (divider != 0U) ? 0 : -EIO;
}

static int ccu_tc4x_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOTSUP;
}

static enum clock_control_status ccu_tc4x_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	uint32_t divider = domain_div((uint32_t)(uintptr_t)sys);

	ARG_UNUSED(dev);

	if (divider == UINT32_MAX) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return (divider != 0U) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, ccu_tc4x_api) = {
	.on = ccu_tc4x_on,
	.off = ccu_tc4x_off,
	.get_rate = ccu_tc4x_get_rate,
	.get_status = ccu_tc4x_get_status,
};

DEVICE_DT_INST_DEFINE(0, ccu_tc4x_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ccu_tc4x_api);
