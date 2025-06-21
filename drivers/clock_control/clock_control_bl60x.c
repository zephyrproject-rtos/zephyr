/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl60x_clock_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/bflb_bl60x_clock.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_bl60x, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <bouffalolab/bl60x/bflb_soc.h>
#include <bouffalolab/bl60x/aon_reg.h>
#include <bouffalolab/bl60x/glb_reg.h>
#include <bouffalolab/bl60x/hbn_reg.h>
#include <bouffalolab/bl60x/pds_reg.h>
#include <bouffalolab/bl60x/l1c_reg.h>
#include <bouffalolab/bl60x/extra_defines.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

#define CLOCK_TIMEOUT               1024
#define EFUSE_RC32M_TRIM_OFFSET     0x0C
#define EFUSE_RC32M_TRIM_EN_POS     19
#define EFUSE_RC32M_TRIM_PARITY_POS 18
#define EFUSE_RC32M_TRIM_POS        10
#define EFUSE_RC32M_TRIM_MSK        0x3FC00

#define CRYSTAL_ID_FREQ_32000000 0
#define CRYSTAL_ID_FREQ_24000000 1
#define CRYSTAL_ID_FREQ_38400000 2
#define CRYSTAL_ID_FREQ_40000000 3
#define CRYSTAL_ID_FREQ_26000000 4

#define CRYSTAL_FREQ_TO_ID(freq) CONCAT(CRYSTAL_ID_FREQ_, freq)

enum bl60x_clkid {
	bl60x_clkid_clk_root = BL60X_CLKID_CLK_ROOT,
	bl60x_clkid_clk_rc32m = BL60X_CLKID_CLK_RC32M,
	bl60x_clkid_clk_crystal = BL60X_CLKID_CLK_CRYSTAL,
	bl60x_clkid_clk_pll = BL60X_CLKID_CLK_PLL,
	bl60x_clkid_clk_bclk = BL60X_CLKID_CLK_BCLK,
};

struct clock_control_bl60x_pll_config {
	enum bl60x_clkid source;
	bool overclock;
};

struct clock_control_bl60x_root_config {
	enum bl60x_clkid source;
	uint8_t pll_select;
	uint8_t divider;
};

struct clock_control_bl60x_bclk_config {
	uint8_t divider;
};

struct clock_control_bl60x_config {
	uint32_t crystal_id;
};

struct clock_control_bl60x_data {
	bool crystal_enabled;
	bool pll_enabled;
	struct clock_control_bl60x_pll_config pll;
	struct clock_control_bl60x_root_config root;
	struct clock_control_bl60x_bclk_config bclk;
};

const static uint32_t clock_control_bl60x_crystal_SDMIN_table[5] = {
	/* 32M */
	0x3C0000,
	/* 24M */
	0x500000,
	/* 38.4M */
	0x320000,
	/* 40M */
	0x300000,
	/* 26M */
	0x49D39D,
};

static int clock_control_bl60x_deinit_crystal(void)
{
	uint32_t tmp;

	/* unpower crystal */
	tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	tmp = tmp & AON_PU_XTAL_AON_UMSK;
	tmp = tmp & AON_PU_XTAL_BUF_AON_UMSK;
	sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

	clock_bflb_settle();
	return 0;
}

static int clock_control_bl60x_init_crystal(void)
{
	uint32_t tmp;
	int count = CLOCK_TIMEOUT;

	/* power crystal */
	tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	tmp = (tmp & AON_PU_XTAL_AON_UMSK) | (1U << AON_PU_XTAL_AON_POS);
	tmp = (tmp & AON_PU_XTAL_BUF_AON_UMSK) | (1U << AON_PU_XTAL_BUF_AON_POS);
	sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

	/* wait for crystal to be powered on */
	do {
		clock_bflb_settle();
		tmp = sys_read32(AON_BASE + AON_TSEN_OFFSET);
		count--;
	} while (!(tmp & AON_XTAL_RDY_MSK) && count > 0);

	clock_bflb_settle();
	if (count < 1) {
		return -1;
	}
	return 0;
}

/* HCLK is the core clock */
static int clock_control_bl60x_set_root_clock_dividers(uint32_t hclk_div, uint32_t bclk_div)
{
	uint32_t tmp;
	uint32_t old_rootclk;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}

	/* set dividers */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_UMSK) | (hclk_div << GLB_REG_HCLK_DIV_POS);
	tmp = (tmp & GLB_REG_BCLK_DIV_UMSK) | (bclk_div << GLB_REG_BCLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* do something undocumented, probably acknowledging clock change by disabling then
	 * reenabling bclk
	 */
	sys_write32(0x00000001, 0x40000FFC);
	sys_write32(0x00000000, 0x40000FFC);

	clock_bflb_settle();

	/* enable clocks */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK) | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK) | (1U << GLB_REG_HCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();

	return 0;
}

static void clock_control_bl60x_set_machine_timer_clock_enable(bool enable)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
	if (enable) {
		tmp = (tmp & GLB_CPU_RTC_EN_UMSK) | (1U << GLB_CPU_RTC_EN_POS);
	} else {
		tmp = (tmp & GLB_CPU_RTC_EN_UMSK) | (0U << GLB_CPU_RTC_EN_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
}

/* clock:
 * 0: BCLK
 * 1: 32Khz Oscillator (RC32*K*)
 */
static void clock_control_bl60x_set_machine_timer_clock(bool enable, uint32_t clock,
							uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x1FFFF) {
		divider = 0x1FFFF;
	}
	if (clock > 1) {
		clock = 1;
	}

	/* disable first, then set div */
	clock_control_bl60x_set_machine_timer_clock_enable(false);

	tmp = sys_read32(GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
	tmp = (tmp & GLB_CPU_RTC_SEL_UMSK) | (clock << GLB_CPU_RTC_SEL_POS);
	tmp = (tmp & GLB_CPU_RTC_DIV_UMSK) | (divider << GLB_CPU_RTC_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);

	clock_control_bl60x_set_machine_timer_clock_enable(enable);
}

static void clock_control_bl60x_deinit_pll(void)
{
	uint32_t tmp;

	/* PLL Off */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_SFREG_UMSK) | (0U << PDS_PU_CLKPLL_SFREG_POS);
	tmp = (tmp & PDS_PU_CLKPLL_UMSK) | (0U << PDS_PU_CLKPLL_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	/* needs 2 steps ? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_PU_CP_UMSK) | (0U << PDS_CLKPLL_PU_CP_POS);
	tmp = (tmp & PDS_CLKPLL_PU_PFD_UMSK) | (0U << PDS_CLKPLL_PU_PFD_POS);
	tmp = (tmp & PDS_CLKPLL_PU_FBDV_UMSK) | (0U << PDS_CLKPLL_PU_FBDV_POS);
	tmp = (tmp & PDS_CLKPLL_PU_POSTDIV_UMSK) | (0U << PDS_CLKPLL_PU_POSTDIV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
}

/* RC32M : 0
 * XTAL : 1
 */
static void clock_control_bl60x_set_pll_source(uint32_t source)
{
	uint32_t tmp;

	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
	if (source > 0) {
		tmp = (tmp & PDS_CLKPLL_REFCLK_SEL_UMSK) | (1U << PDS_CLKPLL_REFCLK_SEL_POS);
		tmp = (tmp & PDS_CLKPLL_XTAL_RC32M_SEL_UMSK) |
		      (0U << PDS_CLKPLL_XTAL_RC32M_SEL_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_REFCLK_SEL_UMSK) | (0U << PDS_CLKPLL_REFCLK_SEL_POS);
		tmp = (tmp & PDS_CLKPLL_XTAL_RC32M_SEL_UMSK) |
		      (1U << PDS_CLKPLL_XTAL_RC32M_SEL_POS);
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
}

static void clock_control_bl60x_init_pll(enum bl60x_clkid source, uint32_t crystal_id)
{
	uint32_t tmp;
	uint32_t old_rootclk;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}

	clock_control_bl60x_deinit_pll();

	if (source == BL60X_CLKID_CLK_CRYSTAL) {
		clock_control_bl60x_set_pll_source(1);
	} else {
		clock_control_bl60x_set_pll_source(0);
	}

	/* 26M special treatment */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_CP_OFFSET);
	if (crystal_id == CRYSTAL_ID_FREQ_26000000) {
		tmp = (tmp & PDS_CLKPLL_ICP_1U_UMSK) | (1U << PDS_CLKPLL_ICP_1U_POS);
		tmp = (tmp & PDS_CLKPLL_ICP_5U_UMSK) | (0U << PDS_CLKPLL_ICP_5U_POS);
		tmp = (tmp & PDS_CLKPLL_INT_FRAC_SW_UMSK) | (1U << PDS_CLKPLL_INT_FRAC_SW_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_ICP_1U_UMSK) | (0U << PDS_CLKPLL_ICP_1U_POS);
		tmp = (tmp & PDS_CLKPLL_ICP_5U_UMSK) | (2U << PDS_CLKPLL_ICP_5U_POS);
		tmp = (tmp & PDS_CLKPLL_INT_FRAC_SW_UMSK) | (0U << PDS_CLKPLL_INT_FRAC_SW_POS);
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_CP_OFFSET);

	/* More 26M special treatment */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_RZ_OFFSET);
	if (crystal_id == CRYSTAL_ID_FREQ_26000000) {
		tmp = (tmp & PDS_CLKPLL_C3_UMSK) | (2U << PDS_CLKPLL_C3_POS);
		tmp = (tmp & PDS_CLKPLL_CZ_UMSK) | (2U << PDS_CLKPLL_CZ_POS);
		tmp = (tmp & PDS_CLKPLL_RZ_UMSK) | (5U << PDS_CLKPLL_RZ_POS);
		tmp = (tmp & PDS_CLKPLL_R4_SHORT_UMSK) | (0U << PDS_CLKPLL_R4_SHORT_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_C3_UMSK) | (3U << PDS_CLKPLL_C3_POS);
		tmp = (tmp & PDS_CLKPLL_CZ_UMSK) | (1U << PDS_CLKPLL_CZ_POS);
		tmp = (tmp & PDS_CLKPLL_RZ_UMSK) | (1U << PDS_CLKPLL_RZ_POS);
		tmp = (tmp & PDS_CLKPLL_R4_SHORT_UMSK) | (1U << PDS_CLKPLL_R4_SHORT_POS);
	}
	tmp = (tmp & PDS_CLKPLL_R4_UMSK) | (2U << PDS_CLKPLL_R4_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_RZ_OFFSET);

	/* set pll dividers */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_POSTDIV_UMSK) | ((uint32_t)(0x14) << PDS_CLKPLL_POSTDIV_POS);
	tmp = (tmp & PDS_CLKPLL_REFDIV_RATIO_UMSK) | (2U << PDS_CLKPLL_REFDIV_RATIO_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);

	/* set SDMIN */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_SDM_OFFSET);
	if (source == BL60X_CLKID_CLK_CRYSTAL) {
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK) |
		      (clock_control_bl60x_crystal_SDMIN_table[crystal_id]
		       << PDS_CLKPLL_SDMIN_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK) |
		      (clock_control_bl60x_crystal_SDMIN_table[CRYSTAL_ID_FREQ_32000000]
		       << PDS_CLKPLL_SDMIN_POS);
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_SDM_OFFSET);

	/* phase comparator settings? */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_FBDV_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SEL_FB_CLK_UMSK) | (1U << PDS_CLKPLL_SEL_FB_CLK_POS);
	tmp = (tmp & PDS_CLKPLL_SEL_SAMPLE_CLK_UMSK) | (1U << PDS_CLKPLL_SEL_SAMPLE_CLK_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_FBDV_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_SFREG_UMSK) | (1U << PDS_PU_CLKPLL_SFREG_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	clock_bflb_settle();

	/* enable PPL clock actual? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_UMSK) | (1U << PDS_PU_CLKPLL_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	/* More power up sequencing*/
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_PU_CP_UMSK) | (1U << PDS_CLKPLL_PU_CP_POS);
	tmp = (tmp & PDS_CLKPLL_PU_PFD_UMSK) | (1U << PDS_CLKPLL_PU_PFD_POS);
	tmp = (tmp & PDS_CLKPLL_PU_FBDV_UMSK) | (1U << PDS_CLKPLL_PU_FBDV_POS);
	tmp = (tmp & PDS_CLKPLL_PU_POSTDIV_UMSK) | (1U << PDS_CLKPLL_PU_POSTDIV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	clock_bflb_settle();

	/* reset couple things one by one? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SDM_RESET_UMSK) | (1U << PDS_CLKPLL_SDM_RESET_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_RESET_FBDV_UMSK) | (1U << PDS_CLKPLL_RESET_FBDV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_RESET_FBDV_UMSK) | (0U << PDS_CLKPLL_RESET_FBDV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SDM_RESET_UMSK) | (0U << PDS_CLKPLL_SDM_RESET_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();
}

/*
 * 0: 48M
 * 1: 120M
 * 2: 160M
 * 3: 192M
 */
static void clock_control_bl60x_select_PLL(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_SEL_UMSK) | (pll << GLB_REG_PLL_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);
}

static int clock_control_bl60x_clock_trim_32M(void)
{
	uint32_t tmp;
	int err;
	uint32_t trim, trim_parity;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);

	err = syscon_read_reg(efuse, EFUSE_RC32M_TRIM_OFFSET, &trim);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (!((trim >> EFUSE_RC32M_TRIM_EN_POS) & 1)) {
		LOG_ERR("RC32M trim disabled!");
		return -EINVAL;
	}

	trim_parity = (trim >> EFUSE_RC32M_TRIM_PARITY_POS) & 1;
	trim = (trim & EFUSE_RC32M_TRIM_MSK) >> EFUSE_RC32M_TRIM_POS;

	if (trim_parity != (POPCOUNT(trim) & 1)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	tmp = (tmp & PDS_RC32M_EXT_CODE_EN_UMSK) | 1 << PDS_RC32M_EXT_CODE_EN_POS;
	tmp = (tmp & PDS_RC32M_CODE_FR_EXT_UMSK) | trim << PDS_RC32M_CODE_FR_EXT_POS;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL0_OFFSET);

	clock_bflb_settle();

	return 0;
}

/* source for most clocks, either XTAL or RC32M */
static uint32_t clock_control_bl60x_get_xclk(const struct device *dev)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = tmp >> HBN_ROOT_CLK_SEL_POS;
	tmp &= 1;
	if (tmp == 0) {
		return BFLB_RC32M_FREQUENCY;
	} else if (tmp == 1) {
		return DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else {
		return 0;
	}
}

static uint32_t clock_control_bl60x_get_clk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t hclk_div;

	hclk_div = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	hclk_div = (hclk_div & GLB_REG_HCLK_DIV_MSK) >> GLB_REG_HCLK_DIV_POS;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = (tmp >> HBN_ROOT_CLK_SEL_POS) >> 1;
	tmp &= 1;

	if (tmp == 0) {
		return clock_control_bl60x_get_xclk(dev) / (hclk_div + 1);
	}
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_SEL_MSK) >> GLB_REG_PLL_SEL_POS;
	if (tmp == 3) {
		return MHZ(192) / (hclk_div + 1);
	} else if (tmp == 2) {
		return MHZ(160) / (hclk_div + 1);
	} else if (tmp == 1) {
		return MHZ(120) / (hclk_div + 1);
	} else if (tmp == 0) {
		return MHZ(48) / (hclk_div + 1);
	}
	return 0;
}

/* most peripherals clock */
static uint32_t clock_control_bl60x_get_bclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t clock_id;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_MSK) >> GLB_REG_BCLK_DIV_POS;
	clock_id = clock_control_bl60x_get_clk(dev);
	return clock_id / (tmp + 1);
}

static uint32_t clock_control_bl60x_mtimer_get_clk_src_div(const struct device *dev)
{
	return clock_control_bl60x_get_bclk(dev) / 1000 / 1000 - 1;
}

static void clock_control_bl60x_cache_2T(bool yes)
{
	uint32_t tmp;

	tmp = sys_read32(L1C_BASE + L1C_CONFIG_OFFSET);

	if (yes) {
		tmp |= L1C_IROM_2T_ACCESS_MSK;
	} else {
		tmp &= ~L1C_IROM_2T_ACCESS_MSK;
	}

	sys_write32(tmp, L1C_BASE + L1C_CONFIG_OFFSET);
}

/* HCLK: 0
 * PLL120M: 1
 */
static void clock_control_bl60x_set_PKA_clock(uint32_t pka_clock)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG2_OFFSET);
	tmp = (tmp & GLB_PKA_CLK_SEL_UMSK) | (pka_clock << GLB_PKA_CLK_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);
}

static void clock_control_bl60x_init_root_as_pll(const struct device *dev)
{
	struct clock_control_bl60x_data *data = dev->data;
	const struct clock_control_bl60x_config *config = dev->config;
	uint32_t tmp;

	clock_control_bl60x_init_pll(data->pll.source, config->crystal_id);

	/* enable all 'PDS' clocks */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_OUTPUT_EN_OFFSET);
	tmp |= 0x1FF;
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_OUTPUT_EN_OFFSET);

	/* glb enable pll actual? */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_EN_UMSK) | (1U << GLB_REG_PLL_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	clock_control_bl60x_select_PLL(data->root.pll_select);

	if (data->pll.source == bl60x_clkid_clk_crystal) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_XTAL);
	} else {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_RC32M);
	}

	if (clock_control_bl60x_get_clk(dev) > MHZ(120)) {
		clock_control_bl60x_cache_2T(true);
	}

	sys_write32(clock_control_bl60x_get_clk(dev), CORECLOCKREGISTER);
	clock_control_bl60x_set_PKA_clock(1);
}

static void clock_control_bl60x_init_root_as_crystal(const struct device *dev)
{
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_XTAL);
	sys_write32(clock_control_bl60x_get_clk(dev), CORECLOCKREGISTER);
}

static int clock_control_bl60x_update_root(const struct device *dev)
{
	struct clock_control_bl60x_data *data = dev->data;
	uint32_t tmp;
	int ret;

	/* make sure all clocks are enabled */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK) | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK) | (1U << GLB_REG_HCLK_EN_POS);
	tmp = (tmp & GLB_REG_FCLK_EN_UMSK) | (1U << GLB_REG_FCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* set root clock to internal 32MHz Oscillator as failsafe */
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	if (clock_control_bl60x_set_root_clock_dividers(0, 0) != 0) {
		return -EIO;
	}
	sys_write32(BFLB_RC32M_FREQUENCY, CORECLOCKREGISTER);

	clock_control_bl60x_set_PKA_clock(0);

	if (data->crystal_enabled) {
		if (clock_control_bl60x_init_crystal() < 0) {
			return -EIO;
		}
	} else {
		clock_control_bl60x_deinit_crystal();
	}

	ret = clock_control_bl60x_set_root_clock_dividers(data->root.divider - 1,
							  data->bclk.divider - 1);
	if (ret < 0) {
		return ret;
	}

	if (data->root.source == bl60x_clkid_clk_pll) {
		clock_control_bl60x_init_root_as_pll(dev);
	} else if (data->root.source == bl60x_clkid_clk_crystal) {
		clock_control_bl60x_init_root_as_crystal(dev);
	} else {
		/* Root clock already setup as RC32M */
	}

	ret = clock_control_bl60x_clock_trim_32M();
	if (ret < 0) {
		return ret;
	}
	clock_control_bl60x_set_machine_timer_clock(
		1, 0, clock_control_bl60x_mtimer_get_clk_src_div(dev));

	clock_bflb_settle();

	return ret;
}

static void clock_control_bl60x_uart_set_clock_enable(bool enable)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	if (enable) {
		tmp = (tmp & GLB_UART_CLK_EN_UMSK) | (1U << GLB_UART_CLK_EN_POS);
	} else {
		tmp = (tmp & GLB_UART_CLK_EN_UMSK) | (0U << GLB_UART_CLK_EN_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG2_OFFSET);
}

/* Clock:
 * FCLK: 0
 * 160 Mhz PLL: 1
 * When using PLL root clock, we can use either setting, when using the 32Mhz Oscillator with a
 * uninitialized PLL, only FCLK will be available.
 */
static void clock_control_bl60x_uart_set_clock(bool enable, uint32_t clock, uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x7) {
		divider = 0x7;
	}
	if (clock > 1) {
		clock = 1;
	}
	/* disable uart clock */
	clock_control_bl60x_uart_set_clock_enable(false);

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp = (tmp & GLB_UART_CLK_DIV_UMSK) | (divider << GLB_UART_CLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG2_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp = (tmp & HBN_UART_CLK_SEL_UMSK) | (clock << HBN_UART_CLK_SEL_POS);
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	clock_control_bl60x_uart_set_clock_enable(enable);
}

/* Simple function to enable all peripherals for now */
static void clock_control_bl60x_peripheral_clock_init(void)
{
	uint32_t regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	/* enable ADC clock routing */
	regval |= (1 << 2);
	/* enable UART0 clock routing */
	regval |= (1 << 16);
	/* enable I2C0 clock routing */
	regval |= (1 << 19);

	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	clock_control_bl60x_uart_set_clock(1, 0, 0);
}

static int clock_control_bl60x_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl60x_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;
	enum bl60x_clkid oldroot;

	key = irq_lock();

	if ((enum bl60x_clkid)sys == bl60x_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = true;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->crystal_enabled = false;
			}
		}
	} else if ((enum bl60x_clkid)sys == bl60x_clkid_clk_pll) {
		if (data->pll_enabled) {
			ret = 0;
		} else {
			data->pll_enabled = true;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->pll_enabled = false;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_RC32M) {
		if (data->root.source == bl60x_clkid_clk_rc32m) {
			ret = 0;
		} else {
			/* Cannot fail to set root to rc32m */
			data->root.source = bl60x_clkid_clk_rc32m;
			ret = clock_control_bl60x_update_root(dev);
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_CRYSTAL) {
		if (data->root.source == bl60x_clkid_clk_crystal) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl60x_clkid_clk_crystal;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_PLL) {
		if (data->root.source == bl60x_clkid_clk_pll) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl60x_clkid_clk_pll;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static int clock_control_bl60x_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl60x_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;

	key = irq_lock();

	if ((enum bl60x_clkid)sys == bl60x_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = false;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->crystal_enabled = true;
			}
		}
	} else if ((enum bl60x_clkid)sys == bl60x_clkid_clk_pll) {
		if (!data->pll_enabled) {
			ret = 0;
		} else {
			data->pll_enabled = false;
			ret = clock_control_bl60x_update_root(dev);
			if (ret < 0) {
				data->pll_enabled = true;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static enum clock_control_status clock_control_bl60x_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	struct clock_control_bl60x_data *data = dev->data;

	switch ((enum bl60x_clkid)sys) {
	case bl60x_clkid_clk_root:
	case bl60x_clkid_clk_bclk:
	case bl60x_clkid_clk_rc32m:
		return CLOCK_CONTROL_STATUS_ON;
	case bl60x_clkid_clk_crystal:
		if (data->crystal_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
	case bl60x_clkid_clk_pll:
		if (data->pll_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static int clock_control_bl60x_get_rate(const struct device *dev, clock_control_subsys_t sys,
					uint32_t *rate)
{
	if ((enum bl60x_clkid)sys == bl60x_clkid_clk_root) {
		*rate = clock_control_bl60x_get_clk(dev);
	} else if ((enum bl60x_clkid)sys == bl60x_clkid_clk_bclk) {
		*rate = clock_control_bl60x_get_bclk(dev);
	} else if ((enum bl60x_clkid)sys == bl60x_clkid_clk_crystal) {
		*rate = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else if ((enum bl60x_clkid)sys == bl60x_clkid_clk_rc32m) {
		*rate = BFLB_RC32M_FREQUENCY;
	} else {
		return -EINVAL;
	}
	return 0;
}

static int clock_control_bl60x_init(const struct device *dev)
{
	int ret;
	uint32_t key;

	key = irq_lock();

	ret = clock_control_bl60x_update_root(dev);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	clock_control_bl60x_peripheral_clock_init();

	clock_bflb_settle();

	irq_unlock(key);

	return 0;
}

static DEVICE_API(clock_control, clock_control_bl60x_api) = {
	.on = clock_control_bl60x_on,
	.off = clock_control_bl60x_off,
	.get_rate = clock_control_bl60x_get_rate,
	.get_status = clock_control_bl60x_get_status,
};

static const struct clock_control_bl60x_config clock_control_bl60x_config = {
	.crystal_id = CRYSTAL_FREQ_TO_ID(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal),
						 clock_frequency)),
};

static struct clock_control_bl60x_data clock_control_bl60x_data = {
	.crystal_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)),
	.pll_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_192)),

	.root = {
#if CLK_SRC_IS(root, pll_192)
			.source = bl60x_clkid_clk_pll,
			.pll_select = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select),
#elif CLK_SRC_IS(root, crystal)
			.source = bl60x_clkid_clk_crystal,
#else
			.source = bl60x_clkid_clk_rc32m,
#endif
			.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), divider),
		},

	.pll = {
#if CLK_SRC_IS(pll_192, crystal)
			.source = bl60x_clkid_clk_crystal,
#else
			.source = bl60x_clkid_clk_rc32m,
#endif
		},

	.bclk = {
			.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, bclk), divider),
		},
};

BUILD_ASSERT(CLK_SRC_IS(pll_192, crystal) || CLK_SRC_IS(root, crystal)
		     ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal))
		     : 1,
	     "Crystal must be enabled to use it");

BUILD_ASSERT(CLK_SRC_IS(root, pll_192) ?
	DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_192)) : 1,
	"PLL must be enabled to use it");

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m)), "RC32M is always on");

BUILD_ASSERT(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m), clock_frequency)
	== BFLB_RC32M_FREQUENCY, "RC32M must be 32M");

DEVICE_DT_INST_DEFINE(0, clock_control_bl60x_init, NULL, &clock_control_bl60x_data,
		      &clock_control_bl60x_config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &clock_control_bl60x_api);
