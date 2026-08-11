/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RT266x clock bring-up, adapted from the MCUXpresso SDK clock_config.c
 * (re-licensed Apache-2.0; NXP owns both copies). Diff behaviour, not text,
 * when resyncing. Only Over Drive Run is carried; __itcm_section replaces
 * AT_QUICKACCESS_SECTION_CODE; all stacks are in DTCM (CONFIG_ARM_STACKS_IN_DTCM)
 * so the window can run without a separate stack switch; interrupts are masked here.
 *
 * Clock sequence:
 *   soc_clock_init -> POWER_EnterHpRun --(callback)--> soc_clock_apply_od_run
 *     soc_clock_prepare: ConfigCGUAna, compute Sys-PLL images into DTCM, then
 *       soc_clock_handover_to_sxosc  <-- ITCM window, both XSPI parked, stack in DTCM
 *       then InitAudioPll / InitVideoPll (lock on the new reference)
 *     then from flash: InitCorePll, InitMainPll, then
 *       SYSCON_common, SYSCON_HPRUN, ConfigCGUDig_SS x6 domains
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "soc_clock.h"
#include <fsl_clock.h>
#include <fsl_iomuxc.h>
#include <fsl_power.h>
#include <fsl_modcon.h>

/*
 * This file addresses hardware via SDK <PERIPHERAL>_BASE macros, as does the
 * SDK on our behalf. Those macros follow __ARM_FEATURE_CMSE, so they resolve to
 * the secure or non-secure view by how this file is compiled. The assertion
 * below verifies devicetree and the SDK headers agree on that view; any single
 * node is enough because the secure alias is a uniform +0x10000000 offset.
 */
BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(syscon_ccm)) == SYSCON__CCM_BASE,
	     "devicetree and SDK headers disagree on the secure/non-secure view");

/*******************************************************************************
 * Definitions
 *****************************************************************************
 */

/* Over Drive Run frequencies (orientation; devicetree is the source of truth):
 *   SXOSC 24 MHz crystal, Core PLL 792 MHz, Main PLL VCO 2 GHz (DIVOUT1 1000 MHz),
 *   Sys PLL VCO 2 GHz (DIV4 500 MHz), Audio PLL 49.152 MHz, Video PLL 70.64 MHz.
 */

/*******************************************************************************
 * Variables
 *****************************************************************************
 */

/*
 * Analog source configuration, derived from devicetree.
 *
 * Each mirrors one nxp,imx-cguana-* node; boards change their crystal or CPU
 * frequency by editing devicetree rather than this file.
 */

#define SXOSC_NODE    DT_NODELABEL(sxosc)
#define FRO192M_NODE  DT_NODELABEL(fro192m)
#define FRO12M_NODE   DT_NODELABEL(fro12m)
#define CORE_PLL_NODE DT_NODELABEL(core_pll)
#define MAIN_PLL_NODE DT_NODELABEL(main_pll)
#define SYS_PLL_NODE  DT_NODELABEL(sys_pll)

/*
 * Steady-state clock roots, each from an nxp,imx-ccm-rev3-root node.
 * SOC_SET_ROOT_CLOCK() programs one root from its node; SOC_SET_ROOT_CLOCK_SD()
 * also carries clock-second-div for roots with a divided and second-divided
 * output (MAIN_ROOTCLK).
 */
#define SOC_SET_ROOT_CLOCK(node)                                                                   \
	do {                                                                                       \
		clock_root_config_t _cfg = {                                                       \
			.mux = DT_PROP(node, clock_mux),                                           \
			.div = DT_PROP(node, clock_div),                                           \
			.sndDiv = 1U,                                                              \
		};                                                                                 \
		CLOCK_SetRootClock((clock_root_t)DT_PROP(node, nxp_root_id), &_cfg);               \
	} while (0)

#define SOC_SET_ROOT_CLOCK_SD(node)                                                                \
	do {                                                                                       \
		clock_root_config_t _cfg = {                                                       \
			.mux = DT_PROP(node, clock_mux),                                           \
			.div = DT_PROP(node, clock_div),                                           \
			.sndDiv = DT_PROP(node, clock_second_div),                                 \
		};                                                                                 \
		CLOCK_SetRootClock((clock_root_t)DT_PROP(node, nxp_root_id), &_cfg);               \
	} while (0)

/* A PLL's reference is a clocks phandle, so its rate cannot disagree with it. */
#define SOC_PLL_REF_HZ(node) DT_PROP(DT_CLOCKS_CTLR(node), clock_frequency)

#define SOC_CGUANA_REF_FREQ(hz)                                                                    \
	((hz) == 19200000   ? kCLOCK_CguanaRefFreq19p2M                                            \
	 : (hz) == 24000000 ? kCLOCK_CguanaRefFreq24M                                              \
	 : (hz) == 32000000 ? kCLOCK_CguanaRefFreq32M                                              \
	 : (hz) == 40000000 ? kCLOCK_CguanaRefFreq40M                                              \
			    : -1)

#define SOC_SXOSC_MODE(node)                                                                       \
	(DT_ENUM_HAS_VALUE(node, nxp_mode, crystal)     ? kCLOCK_CguanaSxoscModeCrystal            \
	 : DT_ENUM_HAS_VALUE(node, nxp_mode, ac_slave)  ? kCLOCK_CguanaSxoscModeAcSlave            \
	 : DT_ENUM_HAS_VALUE(node, nxp_mode, dc_bypass) ? kCLOCK_CguanaSxoscModeDcBypass           \
							: kCLOCK_CguanaSxoscModeDcBypassNoDet)

BUILD_ASSERT(SOC_CGUANA_REF_FREQ(SOC_PLL_REF_HZ(CORE_PLL_NODE)) != -1,
	     "core PLL reference frequency is not one the hardware offers");
BUILD_ASSERT(SOC_CGUANA_REF_FREQ(SOC_PLL_REF_HZ(MAIN_PLL_NODE)) != -1,
	     "main PLL reference frequency is not one the hardware offers");
BUILD_ASSERT(SOC_CGUANA_REF_FREQ(SOC_PLL_REF_HZ(SYS_PLL_NODE)) != -1,
	     "sys PLL reference frequency is not one the hardware offers");

/*
 * The core PLL output is its reference multiplied by the loop divider, halved if
 * the post divider is on. Check the output devicetree states against that rather
 * than trusting it, so a mismatch is a build error and not a surprise on the
 * bench.
 */
BUILD_ASSERT(DT_PROP(CORE_PLL_NODE, clock_frequency) ==
		     (SOC_PLL_REF_HZ(CORE_PLL_NODE) * DT_PROP(CORE_PLL_NODE, nxp_loop_div)) /
			     (DT_PROP(CORE_PLL_NODE, nxp_post_div_by_2) ? 2 : 1),
	     "core PLL clock-frequency disagrees with its loop and post dividers");

static const clock_cguana_sxosc_config_t s_sxoscConfig = {
	.modeSel = SOC_SXOSC_MODE(SXOSC_NODE),
	.gmSel = DT_PROP(SXOSC_NODE, nxp_gm_sel),
	.xtal1CapTrim = DT_PROP_BY_IDX(SXOSC_NODE, nxp_cap_trim, 0),
	.xtal2CapTrim = DT_PROP_BY_IDX(SXOSC_NODE, nxp_cap_trim, 1),
	.detTrim = DT_PROP(SXOSC_NODE, nxp_det_trim),
	.clkDiv2En = DT_PROP(SXOSC_NODE, nxp_div2_enable),
};

#define SOC_FRO192M_OTWB(hz)                                                                       \
	((hz) == 96000000    ? kCLOCK_CguanaFro192mOtwb96M                                         \
	 : (hz) == 120000000 ? kCLOCK_CguanaFro192mOtwb120M                                        \
	 : (hz) == 144000000 ? kCLOCK_CguanaFro192mOtwb144M                                        \
	 : (hz) == 192000000 ? kCLOCK_CguanaFro192mOtwb192M                                        \
	 : (hz) == 240000000 ? kCLOCK_CguanaFro192mOtwb240M                                        \
	 : (hz) == 288000000 ? kCLOCK_CguanaFro192mOtwb288M                                        \
	 : (hz) == 384000000 ? kCLOCK_CguanaFro192mOtwb384M                                        \
	 : (hz) == 480000000 ? kCLOCK_CguanaFro192mOtwb480M                                        \
			     : -1)

#define SOC_FRO12M_OTWB(hz)                                                                        \
	((hz) == 8000000    ? kCLOCK_CguanaFro12mOtwb8M                                            \
	 : (hz) == 10000000 ? kCLOCK_CguanaFro12mOtwb10M                                           \
	 : (hz) == 12000000 ? kCLOCK_CguanaFro12mOtwb12M                                           \
	 : (hz) == 12288000 ? kCLOCK_CguanaFro12mOtwb12p288M                                       \
	 : (hz) == 16000000 ? kCLOCK_CguanaFro12mOtwb16M                                           \
	 : (hz) == 20000000 ? kCLOCK_CguanaFro12mOtwb20M                                           \
	 : (hz) == 24000000 ? kCLOCK_CguanaFro12mOtwb24M                                           \
	 : (hz) == 32000000 ? kCLOCK_CguanaFro12mOtwb32M                                           \
			    : -1)

BUILD_ASSERT(SOC_FRO192M_OTWB(DT_PROP(FRO192M_NODE, clock_frequency)) != -1,
	     "fro192m clock-frequency is not one of the bands the hardware offers");
BUILD_ASSERT(SOC_FRO12M_OTWB(DT_PROP(FRO12M_NODE, clock_frequency)) != -1,
	     "fro12m clock-frequency is not one of the bands the hardware offers");

static const clock_cguana_fro192m_config_t s_fro192mConfig = {
	.otwb = SOC_FRO192M_OTWB(DT_PROP(FRO192M_NODE, clock_frequency)),
};

static const clock_cguana_fro12m_config_t s_fro12mConfig = {
	.otwb = SOC_FRO12M_OTWB(DT_PROP(FRO12M_NODE, clock_frequency)),
};

static const clock_cguana_core_pll_config_t s_corePllConfig = {
	.vcoSelHf = DT_PROP(CORE_PLL_NODE, nxp_vco_high_band),
	.refFreq = SOC_CGUANA_REF_FREQ(SOC_PLL_REF_HZ(CORE_PLL_NODE)),
	.loopDivNint = DT_PROP(CORE_PLL_NODE, nxp_loop_div),
	.postDivBy2 = DT_PROP(CORE_PLL_NODE, nxp_post_div_by_2),
};

#define SOC_FRAC_PLL_VCO(hz)                                                                       \
	((hz) == 2000000000   ? kCLOCK_CguanaFracPllVco2000M                                       \
	 : (hz) == 1950000000 ? kCLOCK_CguanaFracPllVco1950M                                       \
	 : (hz) == 1900000000 ? kCLOCK_CguanaFracPllVco1900M                                       \
	 : (hz) == 1850000000 ? kCLOCK_CguanaFracPllVco1850M                                       \
			      : -1)

#define SOC_INT_DIV_MATCH(node, prop, idx, d) (DT_PROP_BY_IDX(node, prop, idx) == (d)) ||

#define SOC_HAS_INT_DIV(node, d)                                                                   \
	(DT_FOREACH_PROP_ELEM_VARGS(node, nxp_int_divs, SOC_INT_DIV_MATCH, d) false)

/* A selector of zero leaves that fractional output disabled. */
#define SOC_FRAC_DIV(node, idx)                                                                    \
	{                                                                                          \
		.en = DT_PROP_BY_IDX(node, nxp_frac_div_sels, idx) != 0,                           \
		.range = false,                                                                    \
		.sel = DT_PROP_BY_IDX(node, nxp_frac_div_sels, idx),                               \
	}

#define SOC_FRAC_PLL_CONFIG(node)                                                                  \
	{                                                                                          \
		.refFreq = SOC_CGUANA_REF_FREQ(SOC_PLL_REF_HZ(node)),                              \
		.lowFreq = SOC_FRAC_PLL_VCO(DT_PROP(node, nxp_vco_frequency)),                     \
		.div5En = SOC_HAS_INT_DIV(node, 5),                                                \
		.div8En = SOC_HAS_INT_DIV(node, 8),                                                \
		.div10En = SOC_HAS_INT_DIV(node, 10),                                              \
		.div20En = SOC_HAS_INT_DIV(node, 20),                                              \
		.fracDiv = {SOC_FRAC_DIV(node, 0), SOC_FRAC_DIV(node, 1), SOC_FRAC_DIV(node, 2)},  \
		.sscgEn = false,                                                                   \
		.sscg = NULL,                                                                      \
	}

BUILD_ASSERT(SOC_FRAC_PLL_VCO(DT_PROP(MAIN_PLL_NODE, nxp_vco_frequency)) != -1,
	     "main PLL nxp,vco-frequency is not one the hardware offers");
BUILD_ASSERT(SOC_FRAC_PLL_VCO(DT_PROP(SYS_PLL_NODE, nxp_vco_frequency)) != -1,
	     "sys PLL nxp,vco-frequency is not one the hardware offers");

/*
 * The main PLL's second fractional output drives the CPU: PLL_PFDX ->
 * MAIN_ROOTCLK -> cpu_clk. Selector counts quarter steps; output =
 * vco-frequency * 4 / selector. Check against cpu0 clock-frequency so
 * a mismatch is a build error.
 */
#define SOC_MAIN_PLL_DIVOUT1_HZ                                                                    \
	((DT_PROP(MAIN_PLL_NODE, nxp_vco_frequency) /                                              \
	  DT_PROP_BY_IDX(MAIN_PLL_NODE, nxp_frac_div_sels, 1)) *                                   \
	 4U)

BUILD_ASSERT(SOC_MAIN_PLL_DIVOUT1_HZ == DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),
	     "cpu0 clock-frequency disagrees with the main PLL output that drives it");

static const clock_cguana_frac_pll_config_t s_mainPllConfig = SOC_FRAC_PLL_CONFIG(MAIN_PLL_NODE);
static const clock_cguana_frac_pll_config_t s_sysPllConfig = SOC_FRAC_PLL_CONFIG(SYS_PLL_NODE);

/*
 * Audio and video PLLs are re-locked after the reference hand-over because the
 * window moves the reference they were locked to. Parameters come from their
 * nxp,imx-cguana-avpll nodes; refFreq and start mode are C constants.
 */
#define AVPLL_AUDIO_NODE DT_NODELABEL(avpll_audio)
#define AVPLL_VIDEO_NODE DT_NODELABEL(avpll_video)

/* Audio PLL: 49.152 MHz (48 kHz audio family), CCO band 6, POSTDIV=16. */
static const clock_cguana_avpll_config_t s_audioPllConfig = {
	.refFreq = kCLOCK_CguanaRefFreq24M,
	.ccoBandSel = DT_PROP(AVPLL_AUDIO_NODE, nxp_cco_band_sel),
	.postDivRatio = DT_PROP(AVPLL_AUDIO_NODE, nxp_post_div_ratio),
	.dnum = DT_PROP(AVPLL_AUDIO_NODE, nxp_dnum),
	.sscgEn = false,
	.sscg = NULL,
};

/* Video PLL: 70.64 MHz (display panel reference), CCO band 5, POSTDIV=11,
 * DNUM=0x181B4E82 (fractional offset to hit 70.64 MHz exactly).
 */
static const clock_cguana_avpll_config_t s_videoPllConfig = {
	.refFreq = kCLOCK_CguanaRefFreq24M,
	.ccoBandSel = DT_PROP(AVPLL_VIDEO_NODE, nxp_cco_band_sel),
	.postDivRatio = DT_PROP(AVPLL_VIDEO_NODE, nxp_post_div_ratio),
	/* dnum: (113/300) x 2^30 -> 70.64 MHz */
	.dnum = DT_PROP(AVPLL_VIDEO_NODE, nxp_dnum),
	.sscgEn = false,
	.sscg = NULL,
};

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 *****************************************************************************
 */
/* Called only through the POWER_EnterHpRun callback in soc_clock_init(). */
static void soc_clock_apply_od_run(void);

void soc_clock_init(void)
{
	POWER_EnterHpRun(soc_clock_apply_od_run);
}

/*******************************************************************************
 ************************ Configuration soc_clock_apply_od_run *******************
 *****************************************************************************
 */
/* CGUDig is split into a shared (mode-invariant) block and a per-mode block for
 * the three CGU slices whose source/divider depend on the operating point
 * (CGU_MAIN_ROOTCLK, CGU_NPU_ROOTCLK, CGU_MEDIABUS_ROOTCLK).
 */
static void ConfigCGUAna(void);
static void ConfigCGUDig_SS(void);
static void ConfigCGUDig_SYSCON_common(void);
static void ConfigCGUDig_SYSCON_HPRUN(void);
static void ConfigCGUDig_CMPT(void);
static void ConfigCGUDig_MAIN(void);
static void ConfigCGUDig_MEDIA(void);
static void ConfigCGUDig_AUDIO(void);
static void ConfigCGUDig_COMM(void);
static void ConfigCGUDig_WAKE(void);

/* XSPI clock-switch step for the two per-controller window helpers below. Each helper is
 * called twice by soc_clock_handover_to_sxosc -- once to park the controller before
 * the PLL work, once to move it onto the re-locked Sys PLL afterwards.
 */
typedef enum _board_xspi_clock_step {
	/*!<
	 * Quiesce + disable the XSPI, park its clock on FRO192M (window entry,
	 * before any PLL is touched).
	 */
	kSocXspiParkOnFro192m = 0U,
	/*!<
	 * Move the XSPI clock to SysPLL DIV4 and re-enable it (window exit,
	 * after the SYS PLL has re-locked).
	 */
	kSocXspiRunOnSysPllDiv4,
} soc_xspi_park_step_t;

/*
 * The sys PLL register images the hand-over programs, in DTCM.
 *
 * Computed before the window opens, while the flash this code executes from is
 * still readable, and read inside it once neither external memory is available.
 */
static uint32_t __dtcm_noinit_section soc_clock_syspll_regs[4];

/* XSPI0 (NOR XIP) window helper -- two steps called by soc_clock_handover_to_sxosc:
 *
 *   kSocXspiParkOnFro192m: ensure the clock gate is on, quiesce (block prefetch,
 *     abort in-flight, wait for idle), MDIS, park its clock on FRO192M.
 *     MDIS'ing mid-access wedges XIP.
 *
 *   kSocXspiRunOnSysPllDiv4: switch fclock to SYSPLLDIV4 (500 MHz) with
 *     div=2 (2x=250 MHz) and sndDiv=2 (SCK=125 MHz) -- SND_DIV=1 would make
 *     2x == SCK and corrupt DQS-loopback reads. Restore prefetch and re-enable.
 */
static void __itcm_section soc_clock_park_xspi0(soc_xspi_park_step_t step)
{
	uint32_t v;
	uint32_t i;

	if (step == kSocXspiParkOnFro192m) {
		/* CGC_ROOT must be on before any clock root mux is touched. */
		MAIN__CCM->CGC_ROOT[(uint32_t)kCLOCK_MAIN_xspi0 - (uint32_t)kCLOCK_MAIN_START]
			.SLICE_CONTROL |= CCM_SLICE_CONTROL_LPCG_CFG_MASK;
		__DSB();
		__ISB();

		/* Quiesce: block new AHB read-prefetch, abort in-flight, bounded wait for idle. */
		MAIN__XSPI_0->SPTRCLR |= XSPI_SPTRCLR_PREFETCH_DIS_MASK;
		MAIN__XSPI_0->SPTRCLR |= XSPI_SPTRCLR_ABRT_CLR_MASK;
		__DSB();
		__ISB();
		for (i = 0U; ((MAIN__XSPI_0->SR & (XSPI_SR_BUSY_MASK | XSPI_SR_AHB_ACC_MASK |
						   XSPI_SR_IP_ACC_MASK)) != 0U) &&
			     (i < 1000000U);
		     i++) {
		}

		/* Now safe to disable; stays disabled until the kSocXspiRunOnSysPllDiv4 step. */
		MAIN__XSPI_0->MCR |= XSPI_MCR_MDIS_MASK;
		__DSB();
		__ISB();

		/* Park the clock path on FRO192M: PERI_ROOTCLK0 -> BASE, xspi0_fclk -> mux0. */
		v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_PERI_ROOTCLK0].SLICE_CONTROL;
		v &= ~CCM_SLICE_CONTROL_MUX_MASK;
		v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_PERI0_ClockRoot_BASE);
		SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_PERI_ROOTCLK0].SLICE_CONTROL = v;

		v = MAIN__CCM->CLOCK_ROOT[1].SLICE_CONTROL; /* xspi0_fclk */
		v &= ~CCM_SLICE_CONTROL_MUX_MASK;
		v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_XSPI0_ClockRoot_MAIN_PERI0_DIV2);
		MAIN__CCM->CLOCK_ROOT[1].SLICE_CONTROL = v;
		__DSB();
		__ISB();
		(void)MAIN__CCM->CLOCK_ROOT[1]
			.SLICE_CONTROL; /* CM85 posted-write read-back flush */
	} else {
		/* SYSPLLDIV4 mux, div=2 (250 MHz 2x), sndDiv=2 (125 MHz SCK). */
		v = MAIN__CCM->CLOCK_ROOT[1].SLICE_CONTROL;
		v &= ~(CCM_SLICE_CONTROL_MUX_MASK | CCM_SLICE_CONTROL_DIV_MASK |
		       CCM_SLICE_CONTROL_SND_DIV_MASK);
		v |= CCM_SLICE_CONTROL_MUX(2U) | CCM_SLICE_CONTROL_DIV(2U - 1U) |
		     CCM_SLICE_CONTROL_SND_DIV(2U - 1U);
		MAIN__CCM->CLOCK_ROOT[1].SLICE_CONTROL = v;
		__DSB();
		__ISB();
		(void)MAIN__CCM->CLOCK_ROOT[1]
			.SLICE_CONTROL; /* CM85 posted-write read-back flush */

		/* Restore normal prefetch FIRST (still disabled), THEN the window's single
		 * re-enable.
		 */
		MAIN__XSPI_0->SPTRCLR &= ~XSPI_SPTRCLR_PREFETCH_DIS_MASK;
		MAIN__XSPI_0->MCR &= ~XSPI_MCR_MDIS_MASK;
		__DSB();
		__ISB();
	}
}

/* XSPI1 (DDR PSRAM) window helper -- mirror of soc_clock_park_xspi0:
 *
 *   kSocXspiParkOnFro192m: gate on, quiesce, MDIS, park on FRO192M.
 *
 *   kSocXspiRunOnSysPllDiv4: switch to SYSPLLDIV4 (500 MHz), copying ROM's
 *     DIV(/1)/SND_DIV(/2) so the PSRAM SCK (250 MHz) and ROM DLL calibration
 *     point are preserved. The clock SOURCE still changes (Main->Sys PLL), so
 *     re-lock the controller DLL -- but ONLY if XSPI1 holds a live DDR PSRAM
 *     (runtime gate on MCR.X16_EN). DLLCR[0] is zeroed then re-armed to
 *     0xC260001C (DLLEN|FREQEN|REFCNTR2|RES6|CDL8|AUTO_UPD|SLV_EN);
 *     SMPR tap=4. XSPI_UpdateDllValue is NOT used (drops FREQEN below its
 *     auto-update threshold). Restore prefetch, re-enable, pulse serial
 *     soft-reset (live PSRAM only).
 *
 * Must be RAM-resident for the psram_txt build where code runs from PSRAM.
 */
static void __itcm_section soc_clock_park_xspi1(soc_xspi_park_step_t step)
{
	uint32_t v;
	uint32_t i;
	bool livePsram;

	if (step == kSocXspiParkOnFro192m) {
		/* CGC_ROOT must be on before any clock root mux is touched. */
		MAIN__CCM->CGC_ROOT[(uint32_t)kCLOCK_MAIN_xspi1 - (uint32_t)kCLOCK_MAIN_START]
			.SLICE_CONTROL |= CCM_SLICE_CONTROL_LPCG_CFG_MASK;
		__DSB();
		__ISB();

		/* Quiesce: block new AHB read-prefetch, abort in-flight, bounded wait for idle. */
		MAIN__XSPI_1->SPTRCLR |= XSPI_SPTRCLR_PREFETCH_DIS_MASK;
		MAIN__XSPI_1->SPTRCLR |= XSPI_SPTRCLR_ABRT_CLR_MASK;
		__DSB();
		__ISB();
		for (i = 0U; ((MAIN__XSPI_1->SR & (XSPI_SR_BUSY_MASK | XSPI_SR_AHB_ACC_MASK |
						   XSPI_SR_IP_ACC_MASK)) != 0U) &&
			     (i < 1000000U);
		     i++) {
		}

		/* Now safe to disable; stays disabled until the kSocXspiRunOnSysPllDiv4 step. */
		MAIN__XSPI_1->MCR |= XSPI_MCR_MDIS_MASK;
		__DSB();
		__ISB();

		/* Park the clock path on FRO192M: PERI_ROOTCLK1 -> BASE, xspi1_fclk -> mux0. */
		v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_PERI_ROOTCLK1].SLICE_CONTROL;
		v &= ~CCM_SLICE_CONTROL_MUX_MASK;
		v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_PERI1_ClockRoot_BASE);
		SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_PERI_ROOTCLK1].SLICE_CONTROL = v;

		v = MAIN__CCM->CLOCK_ROOT[2].SLICE_CONTROL; /* xspi1_fclk */
		v &= ~CCM_SLICE_CONTROL_MUX_MASK;
		v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_XSPI1_ClockRoot_MAIN_PERI1_DIV2);
		MAIN__CCM->CLOCK_ROOT[2].SLICE_CONTROL = v;
		__DSB();
		__ISB();
		(void)MAIN__CCM->CLOCK_ROOT[2]
			.SLICE_CONTROL; /* CM85 posted-write read-back flush */
	} else {
		livePsram = ((MAIN__XSPI_1->MCR & XSPI_MCR_X16_EN_MASK) !=
			     0U); /* ROM brought up DDR PSRAM? */

		/* SYSPLLDIV4 mux, div=1 (500 MHz 2x), sndDiv=2 (250 MHz SCK). */
		v = MAIN__CCM->CLOCK_ROOT[2].SLICE_CONTROL;
		v &= ~(CCM_SLICE_CONTROL_MUX_MASK | CCM_SLICE_CONTROL_DIV_MASK |
		       CCM_SLICE_CONTROL_SND_DIV_MASK);
		v |= CCM_SLICE_CONTROL_MUX(2U) | CCM_SLICE_CONTROL_DIV(1U - 1U) |
		     CCM_SLICE_CONTROL_SND_DIV(2U - 1U);
		MAIN__CCM->CLOCK_ROOT[2].SLICE_CONTROL = v;
		__DSB();
		__ISB();
		(void)MAIN__CCM->CLOCK_ROOT[2]
			.SLICE_CONTROL; /* CM85 posted-write read-back flush */

		/* Still MDIS'd: re-lock the DDR read DLL for the new clock SOURCE (live PSRAM
		 * only).
		 */
		if (livePsram) {
			MAIN__XSPI_1->DLLCR[0] = 0U; /* drop the stale (Main-PLL-clock) lock */
			MAIN__XSPI_1->DLLCR[0] =
				0xC260001CU; /* re-arm auto-DLL for the Sys-PLL clock */
			for (i = 0U; ((MAIN__XSPI_1->DLLSR & XSPI_DLLSR_SLVA_LOCK_MASK) == 0U) &&
				     (i < 100000U);
			     i++) {
			}
			MAIN__XSPI_1->SMPR =
				0x04000000U; /* DLLFSMPFA tap = 4 (working-state value) */
		}

		/* Restore normal prefetch FIRST (still disabled), THEN the window's single
		 * re-enable. For live PSRAM, a serial soft-reset pulse settles the interface on the
		 * new clock.
		 */
		MAIN__XSPI_1->SPTRCLR &= ~XSPI_SPTRCLR_PREFETCH_DIS_MASK;
		MAIN__XSPI_1->MCR &= ~XSPI_MCR_MDIS_MASK;
		if (livePsram) {
			MAIN__XSPI_1->MCR |= XSPI_MCR_SWRSTSD_MASK;
			MAIN__XSPI_1->MCR &= ~XSPI_MCR_SWRSTSD_MASK;
		}
		__DSB();
		__ISB();
	}
}

/* Select SXOSC as the OSC_24M source (MODCON CLK24M_SEL: 0=FRO24M reset default,
 * 1=SXOSC). Done from RAM with a direct register write to avoid fetching over
 * XSPI0 flash across the transient. SXOSC must already be running.
 */
static void __itcm_section soc_clock_select_sxosc_ref(void)
{
	MAIN__MODCON->IP[getModConOffset((uint32_t)kModCon_MAIN_CLK24M_SEL)].CFG[0] = 0x1U;

	__DSB();
	__ISB();
	(void)MAIN__MODCON->IP[getModConOffset((uint32_t)kModCon_MAIN_CLK24M_SEL)].CFG[0];
}

/* Precompute the four SYSPLL register images from the SYSPLL config while flash
 * is still accessible, so soc_clock_handover_to_sxosc needs no .rodata read.
 * Mirrors CGUANA_ConfigFracPllRegs; SYSPLL and MAINPLL share the PLLn layout.
 */
static void soc_clock_compute_sys_pll_regs(const clock_cguana_frac_pll_config_t *cfg,
					   uint32_t *pll1, uint32_t *pll2, uint32_t *pll3,
					   uint32_t *pll4)
{
	*pll1 = (cfg->div5En ? CGUANA_CGUA_MAINPLL_PLL1_REG_MAINPLL_DIV5_EN_MASK : 0U) |
		(cfg->div8En ? CGUANA_CGUA_MAINPLL_PLL1_REG_MAINPLL_DIV8_EN_MASK : 0U) |
		(cfg->div10En ? CGUANA_CGUA_MAINPLL_PLL1_REG_MAINPLL_DIV10_EN_MASK : 0U) |
		(cfg->div20En ? CGUANA_CGUA_MAINPLL_PLL1_REG_MAINPLL_DIV20_EN_MASK : 0U);

	*pll2 = (cfg->fracDiv[0].en ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC0_EN_MASK : 0U) |
		(cfg->fracDiv[0].range ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC0_RANGE_MASK
				       : 0U) |
		CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC0_SEL(cfg->fracDiv[0].sel) |
		(cfg->fracDiv[1].en ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC1_EN_MASK : 0U) |
		(cfg->fracDiv[1].range ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC1_RANGE_MASK
				       : 0U) |
		CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC1_SEL(cfg->fracDiv[1].sel) |
		(cfg->fracDiv[2].en ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC2_EN_MASK : 0U) |
		(cfg->fracDiv[2].range ? CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC2_RANGE_MASK
				       : 0U) |
		CGUANA_CGUA_MAINPLL_PLL2_REG_MAINPLL_DIVFRAC2_SEL(cfg->fracDiv[2].sel);

	*pll3 = CGUANA_CGUA_MAINPLL_PLL3_REG_MAINPLL_FREF_SET((uint32_t)cfg->refFreq) |
		CGUANA_CGUA_MAINPLL_PLL3_REG_MAINPLL_LOWFREQ(cfg->lowFreq);

	/* s_sysPllConfig.sscgEn is false -> PLL4 = 0. (SSCG path intentionally omitted; the
	 * RAM window must not depend on the flash-resident sscg sub-struct.)
	 */
	*pll4 = 0U;
}

/* Glitchless crystal hand-over for the CMS PLLs -- the hazardous RAM window.
 * Direct register writes only; SDK driver code and its tables live in the XSPI0
 * flash that is torn down mid-window.
 *
 * Flipping the CMS PLL reference while the ROM Main PLL feeds XIP flash and
 * PSRAM glitches the PLL and it can fail to re-lock. Every CMS PLL is powered
 * down first, the reference switched with no running loop, then PLLs re-locked
 * fresh on the crystal.
 *
 * Ordering: both XSPI clock roots MUST be parked on FRO192M before the core is
 * parked, before bus roots are detached, and before any PLL is powered down.
 * Doing it after the core/bus move corrupts XIP.
 *
 *   [0] Disable L1 caches and turn the LLC off.
 *   [1] Force BASE_CLK -> FRO192M.
 *   [2] Quiesce, MDIS and park both XSPI on FRO192M.
 *   [3] Park CM85 core on FRO192M; detach remaining CGU bus roots from PLLs.
 *   [4] Power down MAIN + CORE + SYS PLL; wait for RDY to clear.
 *   [5] Switch CMS and AV PLL reference to SXOSC; switch OSC_24M with it.
 *   [6] Re-program and power up SYS PLL on the crystal reference; wait for RDY.
 *   [7] Bring up SYSPLLDIV4_ROOTCLK (500 MHz).
 *   [8] Hop both XSPI fclocks onto SysPLL DIV4 and re-enable. XIP and PSRAM live.
 *   [9] Restore LLC policy and L1 caches.
 *
 * Core PLL and Main PLL are left down; the flash-resident caller re-inits them.
 */
static void __itcm_section soc_clock_handover_to_sxosc(void)
{
	/* CORE/MAIN/SYS PLLs, all three powered down before the reference switch
	 * so none can pass the glitch to a consumer.
	 */
	const uint32_t cmsFsm =
		CLOCK_CGUANA_FSM_MAINPLL | CLOCK_CGUANA_FSM_COREPLL | CLOCK_CGUANA_FSM_SYSPLL;
	uint32_t v;
	uint32_t i;
	uint32_t llcCtcr;
	bool dCacheOn;
	bool iCacheOn;

	/* [Step 0] Disable L1 D/I caches (only the ones currently on) and the LLC.
	 * The LLC write MUST run from ITCM: in the psram_txt build the caller runs
	 * from the PSRAM path this clears, and the next fetch after LOOKUPEN|FILLEN
	 * clear would bus-fault. Policy is restored in step 9.
	 */
	dCacheOn = ((SCB->CCR & SCB_CCR_DC_Msk) != 0U);
	iCacheOn = ((SCB->CCR & SCB_CCR_IC_Msk) != 0U);
	if (dCacheOn) {
		SCB_DisableDCache(); /* clean + invalidate + disable L1 D-cache */
	}
	if (iCacheOn) {
		SCB_DisableICache();
	}
	llcCtcr = CMPT__LLC->CCUCTCR;
	CMPT__LLC->CCUCTCR = llcCtcr & ~(LLC_CCUCTCR_LOOKUPEN_MASK | LLC_CCUCTCR_FILLEN_MASK);
	__DSB();
	__ISB();

	/* [Step 1] Force BASE_CLK -> FRO192M first; it is the shared park source for
	 * every root parked below. Read-back + DSB/ISB order the posted write.
	 */
	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_BASE_CLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_BASE_ClockRoot_FRO_192M);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_BASE_CLK].SLICE_CONTROL = v;
	__DSB();
	__ISB();
	(void)SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_BASE_CLK].SLICE_CONTROL;

	/* [Step 2] Park BOTH flash controllers on FRO192M before any PLL/reference
	 * perturbation, while the system is still fully clocked and stable.
	 */
	soc_clock_park_xspi0(kSocXspiParkOnFro192m);
	soc_clock_park_xspi1(kSocXspiParkOnFro192m);

	/* [Step 3a] Park CM85 core on FRO192M (MAIN_ROOTCLK mux -> BASE); runs from
	 * ITCM so the CPU keeps executing across the hop.
	 */
	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_MAIN_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_CGU_MAIN_ClockRoot_BASE);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_MAIN_ROOTCLK].SLICE_CONTROL = v;
	__DSB();
	__ISB();
	(void)SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_MAIN_ROOTCLK].SLICE_CONTROL;

	/* [Step 3b] Detach remaining CMS-PLL-fed CGU bus roots onto FRO192M so nothing
	 * rides MAIN/CORE/SYS PLL when they are powered down in step 4. WAKEBUS has no
	 * BASE mux option, so takes FRO_192M directly. SYSCON_PDMAIN_CLK has no BASE mux
	 * either -- park on FRO_48M; ConfigCGUDig_SYSCON_common() restores it post-window.
	 */
	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_NPU_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_NPU_ClockRoot_BASE);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_NPU_ROOTCLK].SLICE_CONTROL = v;

	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_MEDIABUS_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_MEDIABUS_ClockRoot_BASE);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_MEDIABUS_ROOTCLK].SLICE_CONTROL = v;

	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_AUDIOBUS_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_AUDIOBUS_ClockRoot_BASE);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_AUDIOBUS_ROOTCLK].SLICE_CONTROL = v;

	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_COMMBUS_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_COMMBUS_ClockRoot_BASE);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_COMMBUS_ROOTCLK].SLICE_CONTROL = v;

	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_WAKEBUS_ROOTCLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_WAKEBUS_ClockRoot_FRO_192M);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_WAKEBUS_ROOTCLK].SLICE_CONTROL = v;

	/* SYSCON_PDMAIN_CLK: park on FRO_48M (no BASE mux option on this slice).
	 * ConfigCGUDig_SYSCON_common() moves it back to SYSPLL_DIV10 post-window.
	 * On re-entry (runtime power-mode switch) it may be on SYSPLL_DIV10 already;
	 * powering SYS PLL down with it still sourced there kills the SYSCON bus.
	 */
	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_SYSCON_PDMAIN_CLK].SLICE_CONTROL;
	v &= ~CCM_SLICE_CONTROL_MUX_MASK;
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_SYSCON_PDMAIN_ClockRoot_FRO_48M);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_SYSCON_PDMAIN_CLK].SLICE_CONTROL = v;
	__DSB();
	__ISB();
	(void)SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_SYSCON_PDMAIN_CLK].SLICE_CONTROL;

	/* [Step 4] Power down MAIN + CORE + SYS PLL together; wait for RDY to clear
	 * before re-enabling SYS PLL in step 6 (asserting SW_ON while SW_OFF is in
	 * flight can wedge the FSM).
	 */
	v = SYSCON__CGUANA->CGUAD_CTRL_REG;
	v &= ~CGUANA_CGUAD_CTRL_REG_CGUAD_FSM_SW_ON_REQ(cmsFsm);
	v |= CGUANA_CGUAD_CTRL_REG_CGUAD_FSM_SW_OFF_REQ(cmsFsm);
	SYSCON__CGUANA->CGUAD_CTRL_REG = v;
	__DSB();
	__ISB();
	for (i = 0U; ((SYSCON__CGUANA->CGUAD_CTRL_STS &
		       (cmsFsm & CGUANA_CGUAD_CTRL_STS_CGUAD_FSM_RDY_MASK)) != 0U) &&
		     (i < 1000000U);
	     i++) {
	}

	/* [Step 5] Switch CMS and AV PLL reference from FRO192M-derived 24M to SXOSC
	 * (CGUA_CTRL_REG.{CMS,AV}_PLL_CKIN_SEL). Glitchless because all CMS PLLs are off.
	 * CRITICAL: also set CLKGEN_PLL_CKIN_EN together with the SELs -- selecting SXOSC
	 * without enabling PLLCKIN leaves the PLLs with no reference; SYS PLL never
	 * reaches RDY in step 6 and boot dies.
	 */
	v = SYSCON__CGUANA->CGUA_CTRL_REG;
	v |= CGUANA_CGUA_CTRL_REG_CGUA_CLKGEN_PLL_CKIN_EN(1U);
	v |= CGUANA_CGUA_CTRL_REG_CGUA_CLKGEN_CMS_PLL_CKIN_SEL(1U);
	v |= CGUANA_CGUA_CTRL_REG_CGUA_CLKGEN_AV_PLL_CKIN_SEL(1U);
	SYSCON__CGUANA->CGUA_CTRL_REG = v;
	__DSB();
	__ISB();
	soc_clock_select_sxosc_ref(); /* MODCON CLK24M_SEL -> SXOSC */

	/* [Step 6] Re-program SYS PLL from DTCM images and power it up; wait for RDY.
	 * Only SYS PLL is brought back here -- it is the XSPI hop target (steps 7/8).
	 */
	SYSCON__CGUANA->CGUA_SYSPLL_PLL1_REG = soc_clock_syspll_regs[0];
	SYSCON__CGUANA->CGUA_SYSPLL_PLL2_REG = soc_clock_syspll_regs[1];
	SYSCON__CGUANA->CGUA_SYSPLL_PLL3_REG = soc_clock_syspll_regs[2];
	SYSCON__CGUANA->CGUA_SYSPLL_PLL4_REG = soc_clock_syspll_regs[3];
	v = SYSCON__CGUANA->CGUAD_CTRL_REG;
	v &= ~CGUANA_CGUAD_CTRL_REG_CGUAD_FSM_SW_OFF_REQ(CLOCK_CGUANA_FSM_SYSPLL);
	v |= CGUANA_CGUAD_CTRL_REG_CGUAD_FSM_SW_ON_REQ(CLOCK_CGUANA_FSM_SYSPLL);
	SYSCON__CGUANA->CGUAD_CTRL_REG = v;
	for (i = 0U;
	     ((SYSCON__CGUANA->CGUAD_CTRL_STS &
	       (CLOCK_CGUANA_FSM_SYSPLL & CGUANA_CGUAD_CTRL_STS_CGUAD_FSM_RDY_MASK)) == 0U) &&
	     (i < 1000000U);
	     i++) {
	}

	/* [Step 7] Bring up SYSPLLDIV4_ROOTCLK (SysPLL DIV4 = 500 MHz, div=1). */
	v = SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_SYSPLLDIV4_ROOTCLK].SLICE_CONTROL;
	v &= ~(CCM_SLICE_CONTROL_MUX_MASK | CCM_SLICE_CONTROL_DIV_MASK);
	v |= CCM_SLICE_CONTROL_MUX((uint32_t)kCLOCK_SYSPLLDIV4_ClockRoot_SYSPLL_DIV4) |
	     CCM_SLICE_CONTROL_DIV(1U - 1U);
	SYSCON__CCM->CLOCK_ROOT[kCLOCK_Root_CGU_SYSPLLDIV4_ROOTCLK].SLICE_CONTROL = v;
	__DSB();
	__ISB();

	/* [Step 8] Hop both XSPI fclocks onto SYS PLL and re-enable. On return,
	 * XSPI0 XIP and XSPI1 PSRAM are both alive.
	 */
	soc_clock_park_xspi0(kSocXspiRunOnSysPllDiv4);
	soc_clock_park_xspi1(kSocXspiRunOnSysPllDiv4);

	/* [Step 9] Restore LLC policy and L1 caches now that XIP + PSRAM are alive. */
	CMPT__LLC->CCUCTCR = llcCtcr;
	__DSB();
	__ISB();
	if (iCacheOn) {
		SCB_EnableICache();
	}
	if (dCacheOn) {
		SCB_EnableDCache();
	}
}

/* Shared, mode-invariant prologue: FROs + SXOSC (pre-window), the RAM window,
 * then Audio/Video PLLs post-window. Core/Main PLL are deferred to the caller.
 */
static void soc_clock_prepare(void)
{
	unsigned int key;

	/*
	 * The SDK sets the LPUART0 functional-clock root to the crystal here so
	 * its examples have a console before the PLLs exist. Zephyr's console
	 * comes up long after this hook, and the root belongs to the
	 * nxp,imx-ccm-rev3-root node in devicetree, so it is not set here.
	 */

	/* Step 1 (pre-window, flash): FROs + SXOSC. CLOCK_InitSxosc's FSM RDY wait ensures
	 * the crystal is stable before the window references it. This does not disturb the
	 * running ROM Main PLL that XSPI0/XSPI1 ride.
	 */
	ConfigCGUAna();

	/* Compute the SYSPLL register images while flash is up (the window is flash-free). */
	soc_clock_compute_sys_pll_regs(&s_sysPllConfig, &soc_clock_syspll_regs[0],
				       &soc_clock_syspll_regs[1], &soc_clock_syspll_regs[2],
				       &soc_clock_syspll_regs[3]);

	/* Step 2 (RAM window): glitchless crystal hand-over.
	 *
	 * Both XSPI controllers are disabled inside the window, so any instruction,
	 * vector or literal fetch that lands in external memory while it is open
	 * wedges XIP. The SDK leaves interrupt masking to the caller; do it here so
	 * the requirement travels with the window rather than with whoever calls
	 * this. The window functions themselves are __itcm_section, and the data they
	 * touch is on the stack or in .data.
	 *
	 * The stack must not reside in any memory region whose clock is modified
	 * during the window. Both XSPI controllers are parked here, so PSRAM
	 * (XSPI1) and NOR XIP (XSPI0) are off-limits. CONFIG_ARM_STACKS_IN_DTCM
	 * places all stacks in DTCM, which has no dependency on either XSPI clock,
	 * and must remain enabled for this board.
	 */
	key = irq_lock();
	soc_clock_handover_to_sxosc();
	irq_unlock(key);

	/* Step 3 (post-window, flash): AV PLLs lock fresh on the SXOSC reference. */
	CLOCK_InitAudioPll(&s_audioPllConfig);
	CLOCK_InitVideoPll(&s_videoPllConfig);
}

/* Over Drive Run (HpRun): CorePLL 792 MHz, MainPLL DIVOUT1 1000 MHz. */
static void soc_clock_apply_od_run(void)
{
	soc_clock_prepare();
	/* Per-mode tail -- config globals used directly. Both PLLs must be locked before
	 * ConfigCGUDig_SYSCON_common() (it sources PLL_PFDX <- MAINPLL_DIVOUT1 and
	 * COMMPFDX <- COREPLL_OUT).
	 */
	CLOCK_InitCorePll(&s_corePllConfig); /* 792 MHz */
	CLOCK_InitMainPll(&s_mainPllConfig); /* DIVOUT1 = 1000 MHz */
	ConfigCGUDig_SYSCON_common();        /* CGU slices identical across modes */
	ConfigCGUDig_SYSCON_HPRUN();         /* CGU slices 30/31/32 for HpRun */
	ConfigCGUDig_SS();                   /* CMPT / MAIN / MEDIA / AUDIO / COMM / WAKE */
}

/*******************************************************************************
 * Static helpers
 *****************************************************************************
 */

/* Initialize the pre-window analog oscillators: FRO12M, FRO192M, SXOSC. PLLs are deferred. */
static void ConfigCGUAna(void)
{
	/* LPOSC_12M, LPOSC_1M and LPOSC_32K stay at reset; nothing here uses them. */

	/* FRO12M first: the fallback reference, kept alive for the subsequent analog init. */
	CLOCK_InitFro12M(&s_fro12mConfig);

	/* FRO 192 MHz -- the core/bus/XSPI park clock for the RAM window. */
	CLOCK_InitFro192M(&s_fro192mConfig);

	/* SXOSC 24 MHz crystal -- the new PLL reference; the FSM RDY wait inside
	 * CLOCK_InitSxosc ensures it is stable before the RAM window references it.
	 */
	CLOCK_InitSxosc(&s_sxoscConfig);
}

/* The six subsystem-domain helpers, all mode-invariant. The per-mode CGU slices
 * (30 MAIN_ROOTCLK, 31 NPU_ROOTCLK, 32 MEDIABUS_ROOTCLK) are programmed
 * separately by ConfigCGUDig_SYSCON_HPRUN() -- see soc_clock_apply_od_run().
 *
 * The CMPT / MAIN / MEDIA domain slices track their upstream CGU roots via
 * mux=MAIN/NPU/MEDIABUS with div!=1 (e.g. CMPT.main_clk_divided = MAIN/3), so
 * they follow whatever the per-mode CGU root is set to -- no per-mode variant of
 * CMPT/MAIN/MEDIA is needed.
 *
 * Each helper below keeps one mutable rootCfg and reuses it across its
 * CLOCK_SetRootClock() calls, with sndDiv = 1 set once as a baseline:
 * CLOCK_SetRootClock writes div and sndDiv using a (value-1) encoding, so a
 * zero-initialised sndDiv would write an all-ones SND_DIV field and corrupt the
 * secondary divider on any root that uses it.
 */
static void ConfigCGUDig_SS(void)
{
	ConfigCGUDig_MEDIA();
	ConfigCGUDig_AUDIO();
	ConfigCGUDig_CMPT();
	ConfigCGUDig_MAIN();
	ConfigCGUDig_COMM();
	ConfigCGUDig_WAKE();
}

/*
 * The 49 SYSCON_CCM slices, in slice-index order. Slices with a single possible
 * source (BASE, LOW, the SAI master clocks, SXOSC and the LPOSC fan-out) are set
 * here because there is nothing to choose; the PLL-distribution slices come from
 * their nxp,imx-ccm-rev3-root nodes, which is where their values live. Slices no
 * consumer in this baseline needs are left at reset.
 */
static void ConfigCGUDig_SYSCON_common(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	/*
	 * MODCON clock-tree muxes. The six Mx /2 selects feed XSPI0/XSPI1 (MAIN) and
	 * USDHC0/USDHC1 (COMM); default them to pass-through so consumers see the
	 * full root frequency. OSC_24M is not routed here -- the RAM window already
	 * switched it to SXOSC with the core parked.
	 */
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_MAINPFDX_DIV2, false);
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_MAIN_PERI0_DIV2, false);
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_MAIN_PERI1_DIV2, false);
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_COMMPFDX_DIV2, false);
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_COMM_PERI1_DIV2, false);
	CLOCK_SetClockSrcDiv2(kCLOCK_SRC_COMM_PERI2_DIV2, false);

	rootCfg.mux = kCLOCK_BASE_ClockRoot_FRO_192M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_BASE_CLK, &rootCfg);

	rootCfg.mux = kCLOCK_LOW_ClockRoot_FRO_24M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_LOW_CLK, &rootCfg);

	/* PLL-distribution and PFD layer: steady values from devicetree. */
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(mainpll_divx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(syspll_divx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(pll_pfdx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(media_pfdx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(mainpfdx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(commpfdx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(maindivx_rootclk));

	rootCfg.mux = kCLOCK_SAIMCLK_ClockRoot_SAI1_MCLK;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_SAIMCLK_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_SAIMCLK0_ClockRoot_SAI0_MCLK;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_SAIMCLK0_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_SAIMCLK1_ClockRoot_SAI1_MCLK;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_SAIMCLK1_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_SAIMCLK2_ClockRoot_SAI2_MCLK;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_SAIMCLK2_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_SXOSC_ClockRoot_OSC_24M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_SXOSC_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_LP12M_CORE_ClockRoot_LPOSC_12M_CORE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_LP12M_CORE_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_LP1M_CORE_ClockRoot_LPOSC_1M_CORE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_LP1M_CORE_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_ULP32K_ClockRoot_LPOSC32K;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_ULP32K_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_FRO192M_ClockRoot_FRO_192M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_FRO192M_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_FRO96M_ClockRoot_FRO_96M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_FRO96M_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_FRO48M_ClockRoot_FRO_48M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_FRO48M_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_FRO24M_ClockRoot_FRO_24M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_FRO24M_ROOTCLK, &rootCfg);

	/* SYSPLL/MAINPLL distribution roots on the core/bus/XSPI path: from devicetree. */
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(sysplldiv4_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(sysplldiv5_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(sysplldivx_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(mainplldivx_rootclk));

	rootCfg.mux = kCLOCK_MAINPLLDIV8_ClockRoot_MAINPLL_DIV8;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_MAINPLLDIV8_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_MAINPLLDIV10_ClockRoot_MAINPLL_DIV10;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_MAINPLLDIV10_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_MAINPLLDIV20_ClockRoot_MAINPLL_DIV20;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_MAINPLLDIV20_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_AUDIOPLL_ClockRoot_AUDIOPLL_DIVOUT;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_AUDIOPLL_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_VIDEOPLL_ClockRoot_VIDEOPLL_DIVOUT;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_VIDEOPLL_ROOTCLK, &rootCfg);

	/* CGU slices 30 (MAIN_ROOTCLK / CM85), 31 (NPU_ROOTCLK), 32 (MEDIABUS_ROOTCLK)
	 * are programmed by ConfigCGUDig_SYSCON_HPRUN() -- they are the per-mode
	 * slices, and only HP RUN is carried here.
	 */

	/* Bus roots (33-36): steady values from devicetree. */
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(audiobus_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(commbus_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(wakebus_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(syscon_pdmain_rootclk));

	rootCfg.mux = kCLOCK_PERI0_ClockRoot_MAINPLL_DIVOUT0;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK0, &rootCfg);

	rootCfg.mux = kCLOCK_PERI1_ClockRoot_MAINPLL_DIVOUT1;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK1, &rootCfg);

	rootCfg.mux = kCLOCK_PERI2_ClockRoot_MAINPLL_DIVOUT2;
	rootCfg.div = 2U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK2, &rootCfg);

	/* PERI3 feeds the console LPUART; steady value from devicetree. */
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(peri3_rootclk));

	rootCfg.mux = kCLOCK_PERI4_ClockRoot_MAINPLL_DIVX;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK4, &rootCfg);

	rootCfg.mux = kCLOCK_PERI5_ClockRoot_MAINPLL_DIVX;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK5, &rootCfg);

	rootCfg.mux = kCLOCK_PERI6_ClockRoot_BASE;
	rootCfg.div = 8U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK6, &rootCfg);

	rootCfg.mux = kCLOCK_PERI7_ClockRoot_FRO_192M;
	rootCfg.div = 5U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_PERI_ROOTCLK7, &rootCfg);

	rootCfg.mux = kCLOCK_AUDIO_ClockRoot_LOW;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_AUDIO_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_VIDEO_ClockRoot_BASE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_VIDEO_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_USB1_ClockRoot_FRO_48M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_USB1_ROOTCLK, &rootCfg);

	rootCfg.mux = kCLOCK_ETH_ClockRoot_SYSPLL_DIV20;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CGU_ETH_ROOTCLK, &rootCfg);
}

/* Per-mode CGU slices for HpRun (Over Drive Run FBB) -- Jira table column 1:
 *
 *  Slice 30  MAIN_ROOTCLK      PLL_PFDX          -> 1000 MHz  (CM85)
 *  Slice 31  NPU_ROOTCLK       COREPLL_OUT       -> 792 MHz   (NPU; spec-nominal 800)
 *  Slice 32  MEDIABUS_ROOTCLK  PLL_PFDX / 3      -> 333 MHz   (MEDIA)
 *
 * Downstream: CMPT.cpu_clk = MAIN_ROOTCLK/1 = 1000 (CM85 CPU clock);
 * CMPT.cmpt_clk = MAIN_ROOTCLK/1 with sndDiv=1 -> also 1000; MAIN.main_clk_divided
 * = MAIN_ROOTCLK/3 -> 333 MHz (MAIN bus / CMPT bus target).
 */
static void ConfigCGUDig_SYSCON_HPRUN(void)
{
	/* The three per-mode slices come from devicetree:
	 *   root 30 MAIN     -- two dividers: div -> CPU_ROOTCLK (CM85 core), sndDiv ->
	 *                       MAIN_ROOTCLK (MAIN bus). HpRun: CM85 = 1000, MAIN = 333 MHz,
	 *                       so clock-div=<1> / clock-second-div=<3>. Uses the *_SD macro.
	 *   root 31 NPU      -- COREPLL_OUT (= 792 MHz per s_corePllConfig).
	 *   root 32 MEDIABUS -- PLL_PFDX (=1000 MHz) / 3 = 333.33 MHz.
	 */
	SOC_SET_ROOT_CLOCK_SD(DT_NODELABEL(main_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(npu_rootclk));
	SOC_SET_ROOT_CLOCK(DT_NODELABEL(mediabus_rootclk));
}

/*
 * CMPT domain clock roots (5 slices on CMPT_CCM). Slice 3 is listed for
 * completeness but is not set here -- see the note in the body.
 *
 *  Slice  Root Name       Mux  Source   Div  Yield
 *  -----  -------------  ---  ------  ---  ----------
 *     0   cmpt_clk         0  MAIN      1  1000 MHz
 *     1   cpu_clk          0  CPU       1  1000 MHz
 *     2   npu_clk          0  NPU       1  792 MHz
 *     3   systick_clk0     1  SXOSC     1  24 MHz   (from devicetree)
 *     4   systick_clk1     1  SXOSC     1  24 MHz
 */
static void ConfigCGUDig_CMPT(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_CMPT_ClockRoot_MAIN;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CMPT_cmpt_clk, &rootCfg);

	rootCfg.mux = kCLOCK_CPU_ClockRoot_CPU;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CMPT_cpu_clk, &rootCfg);

	rootCfg.mux = kCLOCK_NPU_ClockRoot_NPU;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CMPT_npu_clk, &rootCfg);

	/*
	 * systick_clk0 (SXOSC = 24 MHz) is the secure SysTick reference and is
	 * programmed from its nxp,imx-ccm-rev3-root devicetree node, which is
	 * also where CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC has to agree with it.
	 * systick_clk1 below is the non-secure reference; this image runs
	 * secure, so nothing consumes it and it stays here.
	 */

	rootCfg.mux = kCLOCK_SYSTICK1_ClockRoot_SXOSC;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_CMPT_systick_clk1, &rootCfg);
}

/*
 * MAIN domain (37 slices on MAIN_CCM). Only the two roots that no single
 * device owns are set here: fro192m (FRO192M) and ulp32k (ULP32K).
 *
 * Of the per-peripheral leaf roots, the LPUART ones are carried as an inline
 * "source" clocks entry on each lpuart node and programmed by that driver at
 * init; the XSPI ones are declared the same way but written by the hand-over
 * window above. The rest (lpspi, lpi2c, i3c, flexcan, flexio, adc, lpit, qtpm,
 * sinc, tpiu, cssi, otp, clkout) are described nowhere yet and keep their reset
 * values -- add a "source" entry, or a MAIN_CCM root child, when a driver needs
 * a rate other than that.
 */
static void ConfigCGUDig_MAIN(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_MAIN_MAIN_FRO192M_ClockRoot_FRO192M;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_MAIN_fro192m, &rootCfg);

	rootCfg.mux = kCLOCK_MAIN_MAIN_ULP32K_ClockRoot_ULP32K;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_MAIN_ulp32k, &rootCfg);
}

/*
 * MEDIA domain (10 slices on MEDIA_CCM). Only the two domain-base roots are
 * set here: media_clk (MEDIABUS) and mediapll_clk (MAINPLLDIV10). The
 * per-peripheral leaf roots (mipicsi, mipidsi, reformat, dcpixel, csi) are not
 * described anywhere yet and keep their reset values.
 */
static void ConfigCGUDig_MEDIA(void)
{
	POWER_SetDomainRunMode(kPOWER_DomainMedia, kPDCON_EventNoneOrActive);
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_MEDIA_ClockRoot_MEDIABUS;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_MEDIA_media_clk, &rootCfg);

	rootCfg.mux = kCLOCK_MEDIAPLL_ClockRoot_MAINPLLDIV10;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_MEDIA_mediapll_clk, &rootCfg);
}

/*
 * AUDIO domain (11 slices on AUDIO_CCM). Only the domain-base root is set
 * here: audio_clk (AUDIOBUS). The per-peripheral leaf roots (dmic, sai,
 * spdif, asrc) are not described anywhere yet and keep their reset values.
 */
static void ConfigCGUDig_AUDIO(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_AUDIO_CLK_ClockRoot_AUDIOBUS;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_AUDIO_audio_clk, &rootCfg);
}

/*
 * COMM domain (17 slices on COMM_CCM). Only the two domain-base roots are
 * set here: comm_clk (COMMBUS) and comm_ulp32k (ULP32K). The per-peripheral
 * leaf roots (usdhc, xspir, usb, eth, xeno, dll) are not described anywhere yet
 * and keep their reset values.
 */
static void ConfigCGUDig_COMM(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_COMM_ClockRoot_COMMBUS;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_COMM_comm_clk, &rootCfg);

	rootCfg.mux = kCLOCK_COMM_ULP32K_ClockRoot_ULP32K;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_COMM_comm_ulp32k, &rootCfg);
}

/*
 * WAKE domain (27 slices on WAKE_CCM), the always-on peripheral roots. Only the
 * six domain-base roots are set here: wake_clk (WAKEBUS), wake_sxosc (SXOSC),
 * wake_lp1m (LP1M_WAKE), wake_lp12m (LP12M_WAKE), wake_ulp32k (ULP32K),
 * wake_lpclk (LP1M_WAKE).
 *
 * Per-peripheral leaf roots (lpspi, lpi2c, i3c, qtpm, lptmr, swt, ewm, acmp,
 * dmic) keep their reset values; the two WAKE LPUARTs use inline "source" clock
 * entries and are programmed by their driver at init.
 *
 * TBD: confirm with IC team that LP1M_WAKE, LP12M_WAKE, LP2M_WAKE are routed to
 * the WAKE_CCM domain at boot, and whether any need explicit PMU enable.
 */
static void ConfigCGUDig_WAKE(void)
{
	clock_root_config_t rootCfg = {0};

	rootCfg.sndDiv = 1U;

	rootCfg.mux = kCLOCK_WAKE_ClockRoot_WAKEBUS;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_clk, &rootCfg);

	rootCfg.mux = kCLOCK_WAKE_SXOSC_ClockRoot_SXOSC;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_sxosc, &rootCfg);

	rootCfg.mux = kCLOCK_WAKE_LP1M_ClockRoot_LP1M_WAKE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_lp1m, &rootCfg);

	rootCfg.mux = kCLOCK_WAKE_LP12M_ClockRoot_LP12M_WAKE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_lp12m, &rootCfg);

	rootCfg.mux = kCLOCK_WAKE_ULP32K_ClockRoot_ULP32K;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_ulp32k, &rootCfg);

	rootCfg.mux = kCLOCK_WAKE_LPCLK_ClockRoot_LP1M_WAKE;
	rootCfg.div = 1U;
	CLOCK_SetRootClock(kCLOCK_Root_WAKE_wake_lpclk, &rootCfg);

	SystemCoreClockUpdate();
}
