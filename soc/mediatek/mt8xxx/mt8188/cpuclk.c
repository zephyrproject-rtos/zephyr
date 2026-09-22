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
};

#define MTK_PLL_CTRL (*(volatile struct mtk_pll_control *)		\
		      DT_PROP(DT_NODELABEL(cpuclk), pll_ctrl_reg))

#define MTK_PLL_CON0_BASE_EN BIT(0)
#define MTK_PLL_CON0_EN      BIT(9)
#define MTK_PLL_CON3_ISO_EN  BIT(1)
#define MTK_PLL_CON3_PWR_ON  BIT(0)
#define MTK_NUM_CLK_CFG      27

struct mtk_clk_gen {
	uint32_t mode;
	uint32_t update[4];
	uint32_t _unused[3];
	struct {
		uint32_t cur;
		uint32_t set;
		uint32_t clr;
	} clk_cfg[MTK_NUM_CLK_CFG];
};

#define MTK_CLK_GEN (*(volatile struct mtk_clk_gen *) \
		     DT_REG_ADDR(DT_NODELABEL(cpuclk)))

#define MTK_CLK17_SEL_BUS_PLL 7
#define MTK_CLK17_SEL_ADSP_PLL 8
#define MTK_CLK17_SEL_26M 0

#define MTK_CLK22_SEL_PLL 8
#define MTK_CLK22_SEL_26M 0

#define MTK_CLK28_SEL_PLL 7
#define MTK_CLK28_SEL_26M 0

const struct { uint16_t mhz; bool pll; uint32_t pll_con1; } freqs[] = {
	{  26, false, 0 },
	{ 400,  true, 0x831ec4ed, },
	{ 800,  true, 0x810f6276, },
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
		MTK_PLL_CTRL.con3 |= MTK_PLL_CON3_PWR_ON;
		delay_us(1);
		MTK_PLL_CTRL.con3 &= ~MTK_PLL_CON3_ISO_EN;
		delay_us(1);
		MTK_PLL_CTRL.con0 |= MTK_PLL_CON0_EN;
		delay_us(20);
	} else {
		MTK_PLL_CTRL.con0 &= ~MTK_PLL_CON0_EN;
		delay_us(1);
		MTK_PLL_CTRL.con3 |= MTK_PLL_CON3_ISO_EN;
		delay_us(1);
		MTK_PLL_CTRL.con3 &= ~MTK_PLL_CON3_PWR_ON;
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

#define SETCLK17_ADSP(val) setclk(17, 0, 2, 4, (val))
#define SETCLK17_AUDIO_LOCAL_BUS(val) setclk(17, 8, 2, 5, (val))

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
		MTK_PLL_CTRL.con1 = freqs[idx].pll_con1;
		set_pll_power(true);
		SETCLK17_ADSP(MTK_CLK17_SEL_ADSP_PLL);
		SETCLK17_AUDIO_LOCAL_BUS(MTK_CLK17_SEL_BUS_PLL);
	} else {
		/* Switch to 26Mhz from PLL */
		SETCLK17_ADSP(MTK_CLK17_SEL_26M);
		SETCLK17_AUDIO_LOCAL_BUS(MTK_CLK17_SEL_26M);
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
