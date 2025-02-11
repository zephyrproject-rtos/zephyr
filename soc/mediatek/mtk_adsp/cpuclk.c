/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* Be warned: the interface here is poorly understood.  I did the best
 * I could to transcribe it (with a little clarification and
 * optimization) from the SOF mt8195 source, but without docs this
 * needs to be treated with great care.
 *
 * Notes:
 * * power-on default is 26Mhz, confirmed with a hacked SOF that
 *   loads but stubs out the clk code.
 * * The original driver has a 13Mhz mode too, but it doesn't work (it
 *   hits all the same code and data paths as 26MHz and acts as a
 *   duplicate.
 * * The magic numbers in the pll_con2 field are from the original
 *   source.  No docs on the PLL register interface are provided.
 */

struct mtk_pll_control {
	uint32_t con0;
	uint32_t con1;
	uint32_t con2;
	uint32_t con3;
	uint32_t con4;
};

#define MTK_PLL_CTRL (*(volatile struct mtk_pll_control *)		\
		      DT_PROP(DT_NODELABEL(cpuclk), pll_ctrl_reg))

#define MTK_PLL_CON0_BASE_EN BIT(0)
#define MTK_PLL_CON0_EN      BIT(9)
#define MTK_PLL_CON4_ISO_EN  BIT(1)
#define MTK_PLL_CON4_PWR_ON  BIT(0)

struct mtk_clk_gen {
	uint32_t mode;
	uint32_t update[4];
	uint32_t _unused[3];
	struct {
		uint32_t cur;
		uint32_t set;
		uint32_t clr;
	} clk_cfg[29];
};

#define MTK_CLK_GEN (*(volatile struct mtk_clk_gen *) \
		     DT_REG_ADDR(DT_NODELABEL(cpuclk)))

#define MTK_CLK22_SEL_PLL 8
#define MTK_CLK22_SEL_26M 0

#define MTK_CLK28_SEL_PLL 7
#define MTK_CLK28_SEL_26M 0

#define MTK_CK_CG (*(volatile uint32_t *)			\
		   DT_PROP(DT_NODELABEL(cpuclk), cg_reg))

#define MTK_CK_CG_SW 1

const struct { uint16_t mhz; bool pll; uint32_t pll_con2; } freqs[] = {
	{  26, false, 0 },
	{ 370,  true, 0x831c7628 },
	{ 540,  true, 0x8214c4ed },
	{ 720,  true, 0x821bb13c },
};

static int cur_idx;

/* Can't use CPU-counted loops when changing CPU speed, and don't have
 * an OS timer driver yet.  Use the 13 MHz timer hardware directly.
 * (The ostimer is always running AFAICT, there's not even an
 * interface for a disable bit defined)
 */
#define TIMER (((volatile uint32_t *)DT_REG_ADDR(DT_NODELABEL(ostimer64)))[3])
static inline void delay_us(int us)
{
	uint32_t t0 = TIMER;

	while (TIMER - t0 < (us * 13)) {
	}
}

static void set_pll_power(bool on)
{
	if (on) {
		MTK_CK_CG &= ~MTK_CK_CG_SW;
		MTK_PLL_CTRL.con4 |= MTK_PLL_CON4_PWR_ON;
		delay_us(1);
		MTK_PLL_CTRL.con4 &= ~MTK_PLL_CON4_ISO_EN;
		delay_us(1);
		MTK_PLL_CTRL.con0 |= MTK_PLL_CON0_EN;
		delay_us(20);
	} else {
		MTK_PLL_CTRL.con0 &= ~MTK_PLL_CON0_EN;
		delay_us(1);
		MTK_PLL_CTRL.con4 |= MTK_PLL_CON4_ISO_EN;
		delay_us(1);
		MTK_PLL_CTRL.con4 &= ~MTK_PLL_CON4_PWR_ON;
	}
}

/* Oddball utility.  There is a giant array of clocks (of which SOF
 * only touches two), each with "clear" and "set" registers which are
 * used to set 4-bit fields at a specific offset.  After that, a
 * particular bit in one of the "update" registers must be written,
 * presumably to latch the input.
 */
static void setclk(int clk, int shift, int updreg, int ubit, int val)
{
	MTK_CLK_GEN.clk_cfg[clk].clr = (0xf << shift);
	if (val) {
		MTK_CLK_GEN.clk_cfg[clk].set = (val << shift);
	}
	MTK_CLK_GEN.update[updreg] = BIT(ubit);
}

#define SETCLK22(val) setclk(22, 0, 2, 24, (val))
#define SETCLK28(val) setclk(28, 16, 3, 18, (val))

void mtk_adsp_set_cpu_freq(int mhz)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(freqs); idx++) {
		if (freqs[idx].mhz == mhz) {
			break;
		}
	}

	if (idx == cur_idx || freqs[idx].mhz != mhz) {
		return;
	}

	if (freqs[idx].pll) {
		/* Switch to PLL from 26Mhz */
		set_pll_power(true);
		SETCLK22(MTK_CLK22_SEL_PLL);
		SETCLK28(MTK_CLK28_SEL_PLL);
		MTK_PLL_CTRL.con2 = freqs[idx].pll_con2;
	} else {
		/* Switch to 26Mhz from PLL */
		SETCLK28(MTK_CLK28_SEL_26M);
		SETCLK22(MTK_CLK22_SEL_26M);
		set_pll_power(false);
	}

	cur_idx = idx;
}

/* The CPU clock is not affected (!) by device reset, so we don't know
 * the speed we're at (on MT8195, hardware powers up at 26 Mhz, but
 * production SOF firmware sets it to 720 at load time and leaves it
 * there).  Set the lowest, then the highest speed unconditionally to
 * force the transition.
 */
void mtk_adsp_cpu_freq_init(void)
{
	mtk_adsp_set_cpu_freq(freqs[0].mhz);
	mtk_adsp_set_cpu_freq(freqs[ARRAY_SIZE(freqs) - 1].mhz);
}
