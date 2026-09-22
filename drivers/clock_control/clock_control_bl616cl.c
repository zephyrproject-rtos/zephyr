/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl616cl_clock_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/bflb_bl616cl_clock.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_bl616cl, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <soc.h>
#include <bouffalolab/bl616cl/bflb_soc.h>
#include <bouffalolab/bl616cl/aon_reg.h>
#include <bouffalolab/bl616cl/glb_reg.h>
#include <bouffalolab/bl616cl/hbn_reg.h>
#include <bouffalolab/bl616cl/mcu_misc_reg.h>
#include <bouffalolab/bl616cl/pds_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

#define CLOCK_TIMEOUT			1024
#define EFUSE_RC32M_TRIM_OFFSET		0x64
#define EFUSE_RC32M_TRIM_EN_POS		31U
#define EFUSE_RC32M_TRIM_PARITY_POS	30U
#define EFUSE_RC32M_TRIM_POS		22U
#define EFUSE_RC32M_TRIM_MSK		(0xff << EFUSE_RC32M_TRIM_POS)
#define EFUSE_RC32K_TRIM_OFFSET		0x60
#define EFUSE_RC32K_TRIM_EN_POS		5U
#define EFUSE_RC32K_TRIM_PARITY_POS	4U
#define EFUSE_RC32K_TRIM_POS		0
#define EFUSE_RC32K_TRIM_MSK		(0xf << EFUSE_RC32K_TRIM_POS)

#define CRYSTAL_ID_FREQ_32000000	0
#define CRYSTAL_ID_FREQ_24000000	1
#define CRYSTAL_ID_FREQ_38400000	2
#define CRYSTAL_ID_FREQ_40000000	3
#define CRYSTAL_ID_FREQ_26000000	4
#define CRYSTAL_VALUES_CNT		5


/* 0x108 : HBN_RSV3 */
#define HBN_XTAL_TYPE			HBN_XTAL_TYPE
#define HBN_XTAL_TYPE_POS		(0U)
#define HBN_XTAL_TYPE_LEN		(4U)
#define HBN_XTAL_TYPE_MSK		(((1U << HBN_XTAL_TYPE_LEN) - 1) << HBN_XTAL_TYPE_POS)
#define HBN_XTAL_TYPE_UMSK		(~(((1U << HBN_XTAL_TYPE_LEN) - 1) << HBN_XTAL_TYPE_POS))
#define HBN_XTAL_STS			HBN_XTAL_STS
#define HBN_XTAL_STS_POS		(4U)
#define HBN_XTAL_STS_LEN		(4U)
#define HBN_XTAL_STS_MSK		(((1U << HBN_XTAL_STS_LEN) - 1) << HBN_XTAL_STS_POS)
#define HBN_XTAL_STS_UMSK		(~(((1U << HBN_XTAL_STS_LEN) - 1) << HBN_XTAL_STS_POS))
#define HBN_FLASH_POWER_DLY		HBN_FLASH_POWER_DLY
#define HBN_FLASH_POWER_DLY_POS		(8U)
#define HBN_FLASH_POWER_DLY_LEN		(8U)
#define HBN_FLASH_POWER_DLY_MSK		(((1U << HBN_FLASH_POWER_DLY_LEN) - 1) \
	<< HBN_FLASH_POWER_DLY_POS)
#define HBN_FLASH_POWER_DLY_UMSK	(~(((1U << HBN_FLASH_POWER_DLY_LEN) - 1) \
	<< HBN_FLASH_POWER_DLY_POS))
#define HBN_FLASH_POWER_STS		HBN_FLASH_POWER_STS
#define HBN_FLASH_POWER_STS_POS		(16U)
#define HBN_FLASH_POWER_STS_LEN		(4U)
#define HBN_FLASH_POWER_STS_MSK		(((1U << HBN_FLASH_POWER_STS_LEN) - 1) \
	<< HBN_FLASH_POWER_STS_POS)
#define HBN_FLASH_POWER_STS_UMSK	(~(((1U << HBN_FLASH_POWER_STS_LEN) - 1) \
	<< HBN_FLASH_POWER_STS_POS))
#define HBN_XTAL_FLAG_VALUE		0x8

#define BL616CL_TARGET_BASIC_CLOCK	MHZ(40)

#define CRYSTAL_FREQ_TO_ID(freq) CONCAT(CRYSTAL_ID_FREQ_, freq)

#if CLK_SRC_IS(root, pll_top)
#define CLK_AT_LEAST_MUL (BFLB_MUL_CLK(32,					\
	DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_top), top_frequency),	\
		BL616CL_PLL_TOP_FREQ))
#else
#define CLK_AT_LEAST_MUL 32
#endif

enum bl616cl_clkid {
	bl616cl_clkid_clk_root = BL616CL_CLKID_CLK_ROOT,
	bl616cl_clkid_clk_rc32m = BL616CL_CLKID_CLK_RC32M,
	bl616cl_clkid_clk_crystal = BL616CL_CLKID_CLK_CRYSTAL,
	bl616cl_clkid_clk_pll = BL616CL_CLKID_CLK_PLL,
	bl616cl_clkid_clk_bclk = BL616CL_CLKID_CLK_BCLK,
	bl616cl_clkid_clk_160mux = BL616CL_CLKID_CLK_160M,
	bl616cl_clkid_clk_f32k = BL616CL_CLKID_CLK_F32K,
	bl616cl_clkid_clk_xtal32k = BL616CL_CLKID_CLK_XTAL32K,
	bl616cl_clkid_clk_rc32k = BL616CL_CLKID_CLK_RC32K,
};

struct clock_control_bl616cl_pll_config {
	enum bl616cl_clkid	source;
	uint32_t		top_frequency;
	bool			enabled;
};

struct clock_control_bl616cl_root_config {
	enum bl616cl_clkid	source;
	uint8_t			pll_select;
	uint8_t			divider;
};

struct clock_control_bl616cl_bclk_config {
	uint8_t	divider;
};

struct clock_control_bl616cl_flashclk_config {
	enum bl616cl_clkid	source;
	uint8_t			divider;
};

struct clock_control_bl616cl_config {
	uint32_t		crystal_id;
};

struct clock_control_bl616cl_f32k_config {
	enum bl616cl_clkid	source;
	bool			xtal_enabled;
};

struct clock_control_bl616cl_data {
	bool	crystal_enabled;
	struct clock_control_bl616cl_pll_config		pll;
	struct clock_control_bl616cl_root_config	root;
	struct clock_control_bl616cl_bclk_config	bclk;
	struct clock_control_bl616cl_flashclk_config	flashclk;
	struct clock_control_bl616cl_f32k_config	f32k;
};

typedef struct {
/* Refclk divide ratio, Fref = Fxtal / wifipll_refclk_div_ratio */
	uint8_t refdivRatio;
/* 5:825M~1.2GHz, 3: Low-power */
	uint8_t vcoSpeed;
/* Unknown, 2: Normal, 0: Low-Power */
	uint8_t vcoIdacExtra;
/* 1: Enable low power mode, 0: Normal */
	uint8_t vco480mEn;
/* 1 : integer-N PLL, 0: Fractional */
	uint8_t sdmBypass;
/* Resolution select, 0: 36.4p~58.8p (Normal) , 1:43.57p~70.56p, 3: Low-Power */
	uint8_t dtcRSel;
/* "Change the alpha by 2^alpha_base_sel, could be 0, 1" */
	uint8_t lfAlphaBase;
/* "Change the alpha by 2^(2*alpha_exp_sel),could be 0, 1, 2, 3, 4, 5" */
	uint8_t lfAlphaExp;
/* "In fast lock state, alpha can be enlarge by 2^(alpha_fast_sel), could be 0, 1, 2, 3" */
	uint8_t lfAlphaFast;
/* "Could be 0.5, 0.625, 0.75, 0.875" */
	uint8_t lfBetaBase;
/* "Change the beta by 2^beta_exp_sel, could be 0, 1, 2, 3, 4, 5" */
	uint8_t lfBetaExp;
/* "In fast lock state, could be 0, 1, enlarge beta by 2^beta_exp_sel" */
	uint8_t lfBetaFast;
/* "0,1,2,3 for 2/2^6, 3/2^6, 4/2^6, 5/2^6, it is the TDC gain" */
	uint8_t spdGain;
} bl616cl_pll_internal_config;

typedef struct {
	bl616cl_pll_internal_config internal;
	uint32_t sdmin;
} bl616cl_pll_config;

static const bl616cl_pll_internal_config pll_internal_32_38_4_40 = {
	.refdivRatio = 2,
	.vcoSpeed = 5,
	.vcoIdacExtra = 2,
	.vco480mEn = 0,
	.sdmBypass = 1,
	.dtcRSel = 0,
	.lfAlphaBase = 1,
	.lfAlphaExp = 2,
	.lfAlphaFast = 3,
	.lfBetaBase = 0,
	.lfBetaExp = 2,
	.lfBetaFast = 1,
	.spdGain = 1,
};

static const bl616cl_pll_internal_config pll_internal_24 = {
	.refdivRatio = 1,
	.vcoSpeed = 5,
	.vcoIdacExtra = 2,
	.vco480mEn = 0,
	.sdmBypass = 1,
	.dtcRSel = 0,
	.lfAlphaBase = 1,
	.lfAlphaExp = 2,
	.lfAlphaFast = 3,
	.lfBetaBase = 0,
	.lfBetaExp = 2,
	.lfBetaFast = 1,
	.spdGain = 1,
};

static const bl616cl_pll_internal_config pll_internal_26 = {
	.refdivRatio = 1,
	.vcoSpeed = 5,
	.vcoIdacExtra = 2,
	.vco480mEn = 0,
	.sdmBypass = 0,
	.dtcRSel = 0,
	.lfAlphaBase = 1,
	.lfAlphaExp = 2,
	.lfAlphaFast = 3,
	.lfBetaBase = 0,
	.lfBetaExp = 2,
	.lfBetaFast = 1,
	.spdGain = 1,
};

static const bl616cl_pll_config pll_32M = {
	.internal = pll_internal_32_38_4_40,
	.sdmin = 0x780000,
};

static const bl616cl_pll_config pll_24M = {
	.internal = pll_internal_24,
	.sdmin = 0x500000,
};

static const bl616cl_pll_config pll_38P4M = {
	.internal = pll_internal_32_38_4_40,
	.sdmin = 0x640000,
};

static const bl616cl_pll_config pll_40M = {
	.internal = pll_internal_32_38_4_40,
	.sdmin = 0x600000,
};

static const bl616cl_pll_config pll_26M = {
	.internal = pll_internal_26,
	.sdmin = 0x49D89D,
};

static const bl616cl_pll_config *const bl616cl_pll_configs[CRYSTAL_VALUES_CNT] = {
&pll_32M, &pll_24M, &pll_38P4M, &pll_40M, &pll_26M
};

static void clock_control_bl616cl_clock_at_least_us(uint32_t us)
{
	for (uint32_t i = 0; i < us * CLK_AT_LEAST_MUL; i++) {
		clock_bflb_settle();
	}
}

/* 0: rc32k
 * 1: xtal32k
 * 3: dig32k
 */
static void clock_control_bl616cl_set_f32k_src(uint8_t src)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_F32K_SEL_UMSK;
	tmp |= src << HBN_F32K_SEL_POS;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);
}

static int clock_control_bl616cl_deinit_crystal(void)
{
	uint32_t tmp;

	/* power crystal */
	tmp = sys_read32(AON_BASE + AON_XTAL_PU_OFFSET);
	tmp = tmp & AON_PU_XTAL_AON_UMSK;
	sys_write32(tmp, AON_BASE + AON_XTAL_PU_OFFSET);

	clock_bflb_settle();
	return 0;
}

/* 0: None
 * 1: 24M
 * 2: 32M
 * 3: 38P4M
 * 4: 40M
 * 5: 26M
 * 6: RC32M
 */
static int clock_control_bl616cl_set_crystal_type(uint32_t type)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_RSV3_OFFSET);
	tmp = (tmp & HBN_XTAL_STS_UMSK) | (HBN_XTAL_FLAG_VALUE << HBN_XTAL_STS_POS);
	tmp = (tmp & HBN_XTAL_TYPE_UMSK) | (type << HBN_XTAL_TYPE_POS);
	sys_write32(tmp, HBN_BASE + HBN_RSV3_OFFSET);

	return 0;
}

static int clock_control_bl616cl_init_crystal(void)
{
	uint32_t tmp;
	int count = CLOCK_TIMEOUT;

	/* power crystal */
	tmp = sys_read32(AON_BASE + AON_XTAL_PU_OFFSET);
	tmp = (tmp & AON_PU_XTAL_AON_UMSK) | (1U << AON_PU_XTAL_AON_POS);
	sys_write32(tmp, AON_BASE + AON_XTAL_PU_OFFSET);

	/* wait for crystal to be powered on */
	do {
		clock_bflb_settle();
		tmp = sys_read32(AON_BASE + AON_XTAL_PU_OFFSET);
		count--;
	} while (!(tmp & AON_XTAL_RDY_MSK) && count > 0);

	clock_bflb_settle();
	if (count < 1) {
		return -1;
	}
	return 0;
}

static __bflb_critfunc int clock_bflb_set_root_clock_dividers(uint32_t hclk_div, uint32_t bclk_div)
{
	uint32_t tmp;
	int count = CLOCK_TIMEOUT;

	/* set dividers */
	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_UMSK) | (hclk_div << GLB_REG_HCLK_DIV_POS);
	tmp = (tmp & GLB_REG_BCLK_DIV_UMSK) | (bclk_div << GLB_REG_BCLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG1_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_ACT_PULSE_UMSK) | (1U << GLB_REG_BCLK_DIV_ACT_PULSE_POS);
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG1_OFFSET);

	do {
		tmp = sys_read32(GLB_BASE + GLB_SYS_CFG1_OFFSET);
		tmp &= GLB_STS_BCLK_PROT_DONE_MSK;
		tmp = tmp >> GLB_STS_BCLK_PROT_DONE_POS;
		count--;
	} while (count > 0 && tmp == 0);

	clock_bflb_settle();

	if (count < 1) {
		return -EIO;
	}

	return 0;
}

static void clock_control_bl616cl_set_machine_timer_clock_enable(bool enable)
{
	uint32_t tmp;

	tmp = sys_read32(MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);
	if (enable) {
		tmp = (tmp & MCU_MISC_REG_MCU_RTC_EN_UMSK) | (1U << MCU_MISC_REG_MCU_RTC_EN_POS);
	} else {
		tmp = (tmp & MCU_MISC_REG_MCU_RTC_EN_UMSK) | (0U << MCU_MISC_REG_MCU_RTC_EN_POS);
	}
	sys_write32(tmp, MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);
}

/* source_clock:
 * 0: XCLK (RC32M or XTAL)
 * 1: Root Clock (FCLK: RC32M, XTAL or PLLs)
 */
static void clock_control_bl616cl_set_machine_timer_clock(bool enable, uint32_t source_clock,
							uint32_t divider)
{
	uint32_t tmp;

	if (source_clock > 1) {
		source_clock = 0;
	}

	tmp = sys_read32(MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);
	tmp = (tmp & MCU_MISC_REG_MCU_RTC_CLK_SEL_UMSK)
		| (source_clock << MCU_MISC_REG_MCU_RTC_CLK_SEL_POS);
	sys_write32(tmp, MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);

	/* disable first, then set div */
	clock_control_bl616cl_set_machine_timer_clock_enable(false);

	tmp = sys_read32(MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);
	tmp = (tmp & MCU_MISC_REG_MCU_RTC_DIV_UMSK)
		| ((divider & 0x3FF) << MCU_MISC_REG_MCU_RTC_DIV_POS);
	sys_write32(tmp, MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);

	clock_control_bl616cl_set_machine_timer_clock_enable(enable);
}

static void clock_control_bl616cl_deinit_pll(void)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	tmp &= GLB_PU_WIFIPLL_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
}

/* XCLK XTAL / SOC XTAL : 0
 * XTAL : 1
 */
static void clock_control_bl616cl_set_pll_source(uint32_t source)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	if (source == 1) {
		tmp = (tmp & GLB_WIFIPLL_REFCLK_SEL_UMSK) | (1U << GLB_WIFIPLL_REFCLK_SEL_POS);
	} else {
		tmp &= GLB_WIFIPLL_REFCLK_SEL_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
}

static void clock_control_bl616cl_init_pll_setup(const bl616cl_pll_config *const config,
						 uint32_t top_frequency)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_REFCLK_DIV_RATIO_UMSK)
		| (config->internal.refdivRatio << GLB_WIFIPLL_REFCLK_DIV_RATIO_POS);
	tmp = (tmp & GLB_WIFIPLL_VCO_480M_EN_UMSK)
		| (config->internal.vco480mEn << GLB_WIFIPLL_VCO_480M_EN_POS);
	tmp = (tmp & GLB_WIFIPLL_VCO_SPEED_UMSK)
		| (config->internal.vcoSpeed << GLB_WIFIPLL_VCO_SPEED_POS);
	tmp = (tmp & GLB_WIFIPLL_VCO_IDAC_EXTRA_UMSK)
		| (config->internal.vcoIdacExtra << GLB_WIFIPLL_VCO_IDAC_EXTRA_POS);
	tmp = (tmp & GLB_WIFIPLL_DTC_R_SEL_UMSK)
		| (config->internal.dtcRSel << GLB_WIFIPLL_DTC_R_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_SPD_FCAL_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SPD_GAIN_UMSK)
		| (config->internal.spdGain << GLB_WIFIPLL_SPD_GAIN_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_SPD_FCAL_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_LF_VCTRL_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_LF_ALPHA_BASE_UMSK)
		| (config->internal.lfAlphaBase << GLB_WIFIPLL_LF_ALPHA_BASE_POS);
	tmp = (tmp & GLB_WIFIPLL_LF_ALPHA_EXP_UMSK)
		| (config->internal.lfAlphaExp << GLB_WIFIPLL_LF_ALPHA_EXP_POS);
	tmp = (tmp & GLB_WIFIPLL_LF_ALPHA_FAST_UMSK)
		| (config->internal.lfAlphaFast << GLB_WIFIPLL_LF_ALPHA_FAST_POS);
	tmp = (tmp & GLB_WIFIPLL_LF_BETA_BASE_UMSK)
		| (config->internal.lfBetaBase << GLB_WIFIPLL_LF_BETA_BASE_POS);
	tmp = (tmp & GLB_WIFIPLL_LF_BETA_EXP_UMSK)
		| (config->internal.lfBetaExp << GLB_WIFIPLL_LF_BETA_EXP_POS);
	tmp = (tmp & GLB_WIFIPLL_LF_BETA_FAST_UMSK)
		| (config->internal.lfBetaFast << GLB_WIFIPLL_LF_BETA_FAST_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_LF_VCTRL_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_SDMIN_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_IN_UMSK)
		| (BFLB_MUL_CLK(config->sdmin, top_frequency, BL616CL_PLL_TOP_FREQ)
		<< GLB_WIFIPLL_SDM_IN_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_SDMIN_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_PI_SDM_LMS_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_BYPASS_UMSK)
		| (config->internal.sdmBypass << GLB_WIFIPLL_SDM_BYPASS_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_PI_SDM_LMS_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	tmp |= GLB_PU_WIFIPLL_MSK;
	tmp |= GLB_WIFIPLL_RSTB_MSK;
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);

	clock_control_bl616cl_clock_at_least_us(6);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	tmp &= GLB_WIFIPLL_RSTB_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);

	clock_control_bl616cl_clock_at_least_us(2);

	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);
	tmp |= GLB_WIFIPLL_RSTB_MSK;
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_ANA_CTRL_OFFSET);

	/* enable PLL outputs */
	tmp = sys_read32(GLB_BASE + GLB_WIFIPLL_CLKTREE_DIG_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV1_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV1_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV2_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV2_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV3_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV3_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV4_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV4_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV5_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV5_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV6_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV6_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV8_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV8_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV10_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV10_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV12_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV12_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV20_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV20_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV30_UMSK)
		| (1U << GLB_WIFIPLL_EN_DIV30_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_USB2_DIV48_UMSK)
		| (1U << GLB_WIFIPLL_EN_USB2_DIV48_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_USB2_DIV2_UMSK)
		| (1U << GLB_WIFIPLL_EN_USB2_DIV2_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFIPLL_CLKTREE_DIG_OFFSET);

	clock_control_bl616cl_clock_at_least_us(280);
}

static void clock_control_bl616cl_init_pll(const bl616cl_pll_config *const *config,
					   enum bl616cl_clkid source, uint32_t crystal_id,
					   uint32_t top_frequency)
{
	uint32_t tmp;

	clock_control_bl616cl_deinit_pll();

	/* Always use XCLK XTAL or RF will be sad and not work*/
	clock_control_bl616cl_set_pll_source(0);

	if (source == BL616CL_CLKID_CLK_CRYSTAL) {
		clock_control_bl616cl_init_pll_setup(config[crystal_id], top_frequency);
	} else {
		clock_control_bl616cl_init_pll_setup(config[CRYSTAL_ID_FREQ_32000000],
						     top_frequency);
	}

	/* enable PLL clock */
	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp |= GLB_REG_PLL_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG0_OFFSET);

	clock_bflb_settle();
}

/*
 * PLL 96MHz: 0
 * PLL 192MHz: 1
 * PLL 240MHz: 2
 * PLL 320MHz: 3
 */
static void clock_control_bl616cl_select_PLL(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
	tmp = (tmp & PDS_REG_PLL_SEL_UMSK) | (pll << PDS_REG_PLL_SEL_POS);
	sys_write32(tmp, PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
}

/* Control PLL outputs gating at undocumented places
 * ISP PLL 80M : 2
 * ISP PLL 120M : 3
 * ISP PLL 96M : 4
 * TOP AUPLL DIV5 : 5
 * TOP AUPLL DIV6 : 6
 * PSRAMB PLL 320M : 7
 * PSRAMB PLL 240M: 8
 * TOP PLL 240M : 13
 * TOP PLL 320M : 14
 * TOP AUPLL DIV2 : 15
 * TOP AUPLL DIV1 : 16
 */
static void clock_control_bl616cl_ungate_pll(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG3_OFFSET);
	tmp |= (1U << pll);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG3_OFFSET);
}

static void clock_control_bl616cl_gate_pll(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG3_OFFSET);
	tmp &= ~(1U << pll);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG3_OFFSET);
}

static int clock_control_bl616cl_clock_trim_32M(void)
{
	uint32_t tmp;
	uint32_t trim, parity;
	int err;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);


	err = otp_read(efuse, EFUSE_RC32M_TRIM_OFFSET, &trim, sizeof(uint32_t));
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (!((trim >> EFUSE_RC32M_TRIM_EN_POS) & 1)) {
		LOG_ERR("RC32M trim disabled!");
		return -EINVAL;
	}

	parity = (trim >> EFUSE_RC32M_TRIM_PARITY_POS) & 1;

	trim = (trim & EFUSE_RC32M_TRIM_MSK) >> EFUSE_RC32M_TRIM_POS;

	if (parity != (POPCOUNT(trim) & 1)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

	tmp = sys_read32(HBN_BASE + HBN_RC32M_CTRL0_OFFSET);
	tmp = (tmp & HBN_RC32M_EXT_CODE_EN_UMSK) | 1U << HBN_RC32M_EXT_CODE_EN_POS;
	sys_write32(tmp, HBN_BASE + HBN_RC32M_CTRL0_OFFSET);
	clock_bflb_settle();

	tmp = sys_read32(HBN_BASE + HBN_RC32M_CTRL0_OFFSET);
	tmp = (tmp & HBN_RC32M_CODE_FR_EXT2_UMSK) | trim << HBN_RC32M_CODE_FR_EXT2_POS;
	sys_write32(tmp, HBN_BASE + HBN_RC32M_CTRL0_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_RC32M_CTRL0_OFFSET);
	tmp = (tmp & HBN_RC32M_EXT_CODE_SEL_UMSK) | 1U << HBN_RC32M_EXT_CODE_SEL_POS;
	sys_write32(tmp, HBN_BASE + HBN_RC32M_CTRL0_OFFSET);
	clock_bflb_settle();

	return 0;
}

static __ramfunc uint32_t clock_control_bl616cl_get_rc32m_speed(const struct device *dev)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_RC32M_CTRL0_OFFSET);

	switch (tmp & (HBN_RCHF_16M_SEL_MSK | HBN_RCHF_CLKX2_EN_MSK)) {
	case 0:
		return BFLB_RC32M_FREQUENCY / 4U;
	case HBN_RCHF_CLKX2_EN_MSK:
		return BFLB_RC32M_FREQUENCY / 2U;
	case HBN_RCHF_16M_SEL_MSK:
		return BFLB_RC32M_FREQUENCY / 2U;
	case (HBN_RCHF_16M_SEL_MSK | HBN_RCHF_CLKX2_EN_MSK):
	default:
		return BFLB_RC32M_FREQUENCY;
	};
}

/* source for most clocks, either XTAL or RC32M */
static __ramfunc uint32_t clock_control_bl616cl_get_xclk(const struct device *dev)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = tmp >> HBN_ROOT_CLK_SEL_POS;
	tmp &= 1;
	if (tmp == 0) {
		return clock_control_bl616cl_get_rc32m_speed(dev);
	} else if (tmp == 1) {
		return DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else {
		return 0;
	}
}

static uint32_t clock_control_bl616cl_mtimer_get_xclk_src_div(const struct device *dev)
{
	return (clock_control_bl616cl_get_xclk(dev) / 1000 / 1000 - 1);
}

/* Almost always CPU, AXI bus, SRAM Memory, Cache, use HCLK query instead */
static __ramfunc uint32_t clock_control_bl616cl_get_fclk(const struct device *dev)
{
	struct clock_control_bl616cl_data *data = dev->data;
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = (tmp >> HBN_ROOT_CLK_SEL_POS) >> 1;
	tmp &= 1;

	if (tmp == 0) {
		return clock_control_bl616cl_get_xclk(dev);
	}
	tmp = sys_read32(PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
	tmp = (tmp & PDS_REG_PLL_SEL_MSK) >> PDS_REG_PLL_SEL_POS;
	if (tmp == BL616CL_PLL_ID_DIV1) {
		return BFLB_MUL_CLK(MHZ(320), data->pll.top_frequency, BL616CL_PLL_TOP_FREQ);
	} else if (tmp == BL616CL_PLL_ID_DIV3_4) {
		return BFLB_MUL_CLK(MHZ(240), data->pll.top_frequency, BL616CL_PLL_TOP_FREQ);
	} else if (tmp == BL616CL_PLL_ID_DIV3_5) {
		return BFLB_MUL_CLK(MHZ(192), data->pll.top_frequency, BL616CL_PLL_TOP_FREQ);
	} else if (tmp == BL616CL_PLL_ID_DIV3_10) {
		return BFLB_MUL_CLK(MHZ(96), data->pll.top_frequency, BL616CL_PLL_TOP_FREQ);
	} else {
		return 0;
	}
	return 0;
}

/* CLIC, should be same as FCLK ideally */
static __ramfunc uint32_t clock_control_bl616cl_get_hclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t clock_f;

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_MSK) >> GLB_REG_HCLK_DIV_POS;
	clock_f = clock_control_bl616cl_get_fclk(dev);
	return clock_f / (tmp + 1);
}

/* most peripherals clock */
static __ramfunc uint32_t clock_control_bl616cl_get_bclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t source_clock;

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_MSK) >> GLB_REG_BCLK_DIV_POS;
	source_clock = clock_control_bl616cl_get_hclk(dev);
	return source_clock / (tmp + 1);
}

/* Alternative clock for SPI, DBI, UART, PKA peripherals */
static uint32_t clock_control_bl616cl_get_160m(const struct device *dev)
{
	uint32_t tmp;
	uint32_t source_clock;

	tmp = sys_read32(GLB_BASE + GLB_DIG_CLK_CFG1_OFFSET);
	tmp = (tmp & GLB_REG_TOP_MUXPLL_160M_SEL_MSK) >> GLB_REG_TOP_MUXPLL_160M_SEL_POS;
	source_clock = clock_control_bl616cl_get_fclk(dev);
	switch (tmp) {
	default:
	case 0:
		return source_clock / 2;
	case 1:
		return source_clock * 3 / 4;
	case 2:
		return source_clock * 3 / 8;
	case 3:
		return source_clock * 3 / 10;
	}
	return 0;
}

static void clock_control_bl616cl_setup_pll(const struct device *dev)
{
	struct clock_control_bl616cl_data *data = dev->data;
	const struct clock_control_bl616cl_config *config = dev->config;
	const bl616cl_pll_config *const *pll_configs = bl616cl_pll_configs;

	clock_control_bl616cl_init_pll(pll_configs, data->pll.source,
				       config->crystal_id, data->pll.top_frequency);

	clock_control_bl616cl_ungate_pll(GLB_CGEN_ISP_WIFIPLL_80M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_ISP_WIFIPLL_120M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_ISP_WIFIPLL_96M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_PSRAMB_WIFIPLL_320M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_PSRAMB_WIFIPLL_240M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_TOP_WIFIPLL_240M_POS);
	clock_control_bl616cl_ungate_pll(GLB_CGEN_TOP_WIFIPLL_320M_POS);
}

static __bflb_critfunc void clock_control_bl616cl_init_root_as_pll(const struct device *dev)
{
	struct clock_control_bl616cl_data *data = dev->data;

	clock_control_bl616cl_select_PLL(data->root.pll_select);

	/* 2T rom access goes here */

	if (data->pll.source == bl616cl_clkid_clk_crystal) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_XTAL);
	} else {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_RC32M);
	}
}

static __bflb_critfunc void clock_control_bl616cl_init_root_as_crystal(const struct device *dev)
{
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_XTAL);
}

static __ramfunc void clock_control_bl616cl_update_flash_clk(const struct device *dev)
{
	struct clock_control_bl616cl_data *data = dev->data;
	volatile uint32_t tmp;
	uint32_t clk;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp &= GLB_SF_CLK_DIV_UMSK;
	tmp &= GLB_SF_CLK_EN_UMSK;
	*(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp &= GLB_SF_CLK_SEL_UMSK;
	tmp &= GLB_SF_CLK_SEL2_UMSK;
	if (data->flashclk.source == bl616cl_clkid_clk_pll) {
		clk = clock_control_bl616cl_get_hclk(dev);
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 0U << GLB_SF_CLK_SEL2_POS;
	} else if (data->flashclk.source == bl616cl_clkid_clk_crystal) {
		clk = clock_control_bl616cl_get_xclk(dev);
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 1U << GLB_SF_CLK_SEL2_POS;
	} else {
		clk = clock_control_bl616cl_get_bclk(dev);
		/* If using RC32M or BCLK, use BCLK */
		tmp |= 2U << GLB_SF_CLK_SEL_POS;
	}

	/* If flash controller will manage flash, set to standard speed
	 * and let it set the divider.
	 */
#if defined(CONFIG_SOC_FLASH_BFLB)
	clk = DIV_ROUND_CLOSEST(clk, BL616CL_TARGET_BASIC_CLOCK);
	tmp |= clamp(clk - 1, 0x0, 0x7) << GLB_SF_CLK_DIV_POS;
#else
	tmp |= (data->flashclk.divider - 1) << GLB_SF_CLK_DIV_POS;
#endif

	*(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp |= GLB_SF_CLK_EN_MSK;
	*(volatile uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	clock_bflb_settle();
}

static int clock_control_bl616cl_clock_trim_32K(void)
{
	uint32_t tmp;
	int err;
	uint32_t trim, trim_parity;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);

	err = otp_read(efuse, EFUSE_RC32K_TRIM_OFFSET, &trim, sizeof(uint32_t));
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (((trim >> EFUSE_RC32K_TRIM_EN_POS) & 1) == 0) {
		LOG_ERR("RC32K trim disabled!");
		return -EINVAL;
	}

	trim_parity = (trim >> EFUSE_RC32K_TRIM_PARITY_POS) & 1;
	trim = (trim & EFUSE_RC32K_TRIM_MSK) >> EFUSE_RC32K_TRIM_POS;

	if (trim_parity != (POPCOUNT(trim) & 1)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

	tmp = sys_read32(HBN_BASE + HBN_RC32K_0_OFFSET);
	tmp = (tmp & HBN_RC32K_CAP_SEL_AON_UMSK) | trim << HBN_RC32K_CAP_SEL_AON_POS;
	sys_write32(tmp, HBN_BASE + HBN_RC32K_0_OFFSET);

	clock_control_bl616cl_clock_at_least_us(2);

	tmp = sys_read32(HBN_BASE + HBN_RC32K_1_OFFSET);
	tmp |= HBN_RC32K_EXT_CODE_EN_MSK;
	sys_write32(tmp, HBN_BASE + HBN_RC32K_1_OFFSET);

	clock_control_bl616cl_clock_at_least_us(2);

	return 0;
}

static int clock_control_bl616cl_update_f32k(const struct device *dev)
{
	struct clock_control_bl616cl_data *data = dev->data;
	uint32_t tmp, tmpold;
	int count = CLOCK_TIMEOUT;
	int ret = 0;

	if (data->f32k.source != bl616cl_clkid_clk_xtal32k
		&& data->f32k.source != bl616cl_clkid_clk_rc32k) {
		return -EINVAL;
	}

	/* Reset to RC32K for safety */
	clock_control_bl616cl_set_f32k_src(0);

	if (data->f32k.xtal_enabled) {
		/* Ensure XTAL32K muxing is enabled (GPIO4 and 5) */
		tmp = sys_read32(HBN_BASE + HBN_PAD_CTRL_0_OFFSET);
		tmp |= (0x30 << HBN_REG_EN_AON_CTRL_GPIO_POS);
		sys_write32(tmp, HBN_BASE + HBN_PAD_CTRL_0_OFFSET);

		/* Disable HBN pull up */
		tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
		tmp &= HBN_REG_EN_HW_PU_PD_UMSK;
		sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

		tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
		tmpold = tmp;
		tmp &= HBN_XTAL32K_HIZ_EN_AON_UMSK;
		tmp |= HBN_PU_XTAL32K_AON_MSK;
		tmp |= HBN_XTAL32K_EN_VCLAMP_AON_MSK;
		tmp |= HBN_XTAL32K_RDY_RSTB_AON_MSK;
		tmp |= HBN_XTAL32K_FAST_STARTUP_AON_MSK;
		if (tmpold != tmp) {
			sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);
			clock_control_bl616cl_clock_at_least_us(2);

			tmp &= HBN_XTAL32K_RDY_RSTB_AON_UMSK;
			sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);
			clock_control_bl616cl_clock_at_least_us(2);

			tmp |= HBN_XTAL32K_RDY_RSTB_AON_MSK;
			sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);

			clock_control_bl616cl_clock_at_least_us(500);

			tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
			tmp &= HBN_XTAL32K_FAST_STARTUP_AON_UMSK;
			sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);

			do {
				clock_control_bl616cl_clock_at_least_us(25);
				tmp = sys_read32(AON_BASE + HBN_XTAL32K_OFFSET);
				count--;
			} while (!(tmp & HBN_XTAL32K_CLK_RDY_MSK) && count > 0);

			tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
			if ((tmp & HBN_XTAL32K_CLK_RDY_MSK) == 0) {
				data->f32k.source = bl616cl_clkid_clk_rc32k;
				LOG_ERR("Failed to initialize 32KHz crystal");
				ret = -EIO;
			}
		}
	}
	if (ret != 0 || !data->f32k.xtal_enabled) {
		tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
		tmp |= HBN_XTAL32K_HIZ_EN_AON_MSK;
		tmp &= HBN_PU_XTAL32K_AON_UMSK;
		tmp &= HBN_XTAL32K_EN_VCLAMP_AON_UMSK;
		tmp |= HBN_XTAL32K_RDY_RSTB_AON_MSK;
		sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);

		clock_control_bl616cl_clock_at_least_us(2);

		tmp &= HBN_XTAL32K_RDY_RSTB_AON_UMSK;
		sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);

		clock_control_bl616cl_clock_at_least_us(2);

		tmp |= HBN_XTAL32K_RDY_RSTB_AON_MSK;
		sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);
	}

	if (data->f32k.source == bl616cl_clkid_clk_rc32k) {
		ret = clock_control_bl616cl_clock_trim_32K();
		if (ret < 0) {
			return ret;
		}
		clock_control_bl616cl_set_f32k_src(0);
	} else {
		clock_control_bl616cl_set_f32k_src(1);
	}

	return 0;
}

static __bflb_critfunc int clock_control_bl616cl_update_clocks(const struct device *dev)
{
	const struct clock_control_bl616cl_config *config = dev->config;
	struct clock_control_bl616cl_data *data = dev->data;
	uint32_t tmp;
	int ret;

	/* make sure all clocks are enabled */
	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK) | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK) | (1U << GLB_REG_HCLK_EN_POS);
	tmp = (tmp & GLB_REG_FCLK_EN_UMSK) | (1U << GLB_REG_FCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG0_OFFSET);

	/* set root clock to internal 32MHz Oscillator as failsafe */
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	if (clock_bflb_set_root_clock_dividers(0, 0) != 0) {
		return -EIO;
	}

	clock_control_bl616cl_set_crystal_type(6U);

	clock_control_bl616cl_update_flash_clk(dev);

	ret = clock_control_bl616cl_update_f32k(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->crystal_enabled) {
		if (clock_control_bl616cl_init_crystal() < 0) {
			return -EIO;
		}
	} else {
		clock_control_bl616cl_deinit_crystal();
	}

	switch (config->crystal_id) {
	case CRYSTAL_ID_FREQ_24000000:
		clock_control_bl616cl_set_crystal_type(1U);
		break;
	case CRYSTAL_ID_FREQ_32000000:
		clock_control_bl616cl_set_crystal_type(2U);
		break;
	case CRYSTAL_ID_FREQ_38400000:
		clock_control_bl616cl_set_crystal_type(3U);
		break;
	case CRYSTAL_ID_FREQ_40000000:
		clock_control_bl616cl_set_crystal_type(4U);
		break;
	case CRYSTAL_ID_FREQ_26000000:
		clock_control_bl616cl_set_crystal_type(5U);
		break;
	default:
		clock_control_bl616cl_set_crystal_type(6U);
	}

	ret = clock_bflb_set_root_clock_dividers(data->root.divider - 1, data->bclk.divider - 1);
	if (ret < 0) {
		return ret;
	}

	clock_control_bl616cl_gate_pll(GLB_CGEN_ISP_WIFIPLL_80M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_ISP_WIFIPLL_120M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_ISP_WIFIPLL_96M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_AUPLL_DIV5_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_AUPLL_DIV6_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_PSRAMB_WIFIPLL_320M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_PSRAMB_WIFIPLL_240M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_WIFIPLL_240M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_WIFIPLL_320M_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_AUPLL_DIV2_POS);
	clock_control_bl616cl_gate_pll(GLB_CGEN_TOP_AUPLL_DIV1_POS);

	if (data->pll.enabled) {
		clock_control_bl616cl_setup_pll(dev);
	} else {
		clock_control_bl616cl_deinit_pll();
	}

	if (data->root.source == bl616cl_clkid_clk_pll) {
		if (!data->pll.enabled) {
			return -EINVAL;
		}
		clock_control_bl616cl_init_root_as_pll(dev);
	} else if (data->root.source == bl616cl_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			return -EINVAL;
		}
		clock_control_bl616cl_init_root_as_crystal(dev);
	} else {
		/* Root clock already setup as RC32M */
	}

	ret = clock_control_bl616cl_clock_trim_32M();
	if (ret < 0) {
		return ret;
	}
	clock_control_bl616cl_set_machine_timer_clock(
		1, 0, clock_control_bl616cl_mtimer_get_xclk_src_div(dev));

	clock_bflb_settle();

	return ret;
}

static void clock_control_bl616cl_uart_set_clock_enable(bool enable)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_UART_CFG0_OFFSET);
	if (enable) {
		tmp = (tmp & GLB_UART_CLK_EN_UMSK) | (1U << GLB_UART_CLK_EN_POS);
	} else {
		tmp = (tmp & GLB_UART_CLK_EN_UMSK) | (0U << GLB_UART_CLK_EN_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_UART_CFG0_OFFSET);
}

/* Clock:
 * BCLK: 0
 * 160 Mhz PLL: 1
 * XCLK: 2
 */
static void clock_control_bl616cl_uart_set_clock(bool enable, uint32_t source_clock,
						 uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x7) {
		divider = 0x7;
	}
	if (source_clock > 2) {
		source_clock = 2;
	}
	/* disable uart clock */
	clock_control_bl616cl_uart_set_clock_enable(false);


	tmp = sys_read32(GLB_BASE + GLB_UART_CFG0_OFFSET);
	tmp = (tmp & GLB_UART_CLK_DIV_UMSK) | (divider << GLB_UART_CLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_UART_CFG0_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	if (source_clock < 2) {
		tmp = (tmp & HBN_UART_CLK_SEL_UMSK) | (source_clock << HBN_UART_CLK_SEL_POS);
		tmp = (tmp & HBN_UART_CLK_SEL2_UMSK) | (0U << HBN_UART_CLK_SEL2_POS);
	} else {
		tmp = (tmp & HBN_UART_CLK_SEL_UMSK) | (0U << HBN_UART_CLK_SEL_POS);
		tmp = (tmp & HBN_UART_CLK_SEL2_UMSK) | (1U << HBN_UART_CLK_SEL2_POS);
	}
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	clock_control_bl616cl_uart_set_clock_enable(enable);
}

/* Leave only minimal peripherals on */
static void clock_control_bl616cl_gate_all_peripherals(void)
{
	uint32_t tmp = 0;

	/* Enable CPU clock routing */
	tmp |= (1U << 0);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG0_OFFSET);

	tmp = 0;
	/* Enable SEC clock routing */
	tmp |= (1U << 3);
	tmp |= (1U << 4);
	/* Enable EF CTRL (Efuses) clock routing */
	tmp |= (1U << 7);
	/* Enable SF CTRL (Flash) clock routing */
	tmp |= (1U << 11);
	/* enable UART0 clock routing (in case of log during init) */
	tmp |= (1U << 16);
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	tmp = 0;
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	/* CFG3 is left alone and set when updating root as bootrom typically boots to PLL */
}

/* Simple function to enable all peripherals for now */
static void clock_control_bl616cl_peripheral_clock_init(void)
{
	uint32_t regval;

	regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);
	/* Enable WO clock routing */
	regval |= (1U << 0);
	/* enable RF TOP clock routing */
	regval |= (1U << 1);
	/* enable ADC / GPIP clock routing */
	regval |= (1U << 2);
	/* enable SEC clock routing */
	regval |= (1U << 3);
	regval |= (1U << 4);
	/* enable DMA clock routing */
	regval |= (1U << 12);
	/* enable SDU clock routing */
	regval |= (1U << 13);
	/* enable USB clock routing */
	regval |= (1U << 14);
	/* enable ADC / GPIP2 clock routing */
	regval |= (1U << 15);
	/* enable UART0 clock routing */
	regval |= (1U << 16);
	/* enable UART1 clock routing */
	regval |= (1U << 17);
	/* enable SPI0 clock routing */
	regval |= (1U << 18);
	/* enable I2C0 clock routing */
	regval |= (1U << 19);
	/* enable PWM clock routing */
	regval |= (1U << 20);
	/* enable Timers clock routing */
	regval |= (1U << 21);
	/* enable IR clock routing */
	regval |= (1U << 22);
	/* enable DBI clock routing */
	regval |= (1U << 24);
	/* enable I2C1 clock routing */
	regval |= (1U << 25);
	/* enable UART2 clock routing */
	regval |= (1U << 26);
	/* enable UART3 clock routing */
	regval |= (1U << 29);
	/* enable SPI1 clock routing */
	regval |= (1U << 30);
	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	regval = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	/* enable PSRAM clock routing */
	regval |= (1U << 17);
	regval |= (1U << 18);
	/* enable SDH clock routing */
	regval |= (1U << 22);
	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	clock_control_bl616cl_uart_set_clock(true, 0, 2);
}

static int clock_control_bl616cl_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl616cl_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;
	enum bl616cl_clkid oldroot;

	key = irq_lock();

	if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = true;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->crystal_enabled = false;
			}
		}
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_pll) {
		if (data->pll.enabled) {
			ret = 0;
		} else {
			data->pll.enabled = true;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->pll.enabled = false;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_RC32M) {
		if (data->root.source == bl616cl_clkid_clk_rc32m) {
			ret = 0;
		} else {
			/* Cannot fail to set root to rc32m */
			data->root.source = bl616cl_clkid_clk_rc32m;
			ret = clock_control_bl616cl_update_clocks(dev);
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_CRYSTAL) {
		if (data->root.source == bl616cl_clkid_clk_crystal) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl616cl_clkid_clk_crystal;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_PLL) {
		if (data->root.source == bl616cl_clkid_clk_pll) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl616cl_clkid_clk_pll;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static int clock_control_bl616cl_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl616cl_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;

	key = irq_lock();

	if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = false;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->crystal_enabled = true;
			}
		}
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_pll) {
		if (!data->pll.enabled) {
			ret = 0;
		} else {
			data->pll.enabled = false;
			ret = clock_control_bl616cl_update_clocks(dev);
			if (ret < 0) {
				data->pll.enabled = true;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static enum clock_control_status clock_control_bl616cl_get_status(const struct device *dev,
								   clock_control_subsys_t sys)
{
	struct clock_control_bl616cl_data *data = dev->data;

	if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_root) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_bclk) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_rc32m) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_pll) {
		if (data->pll.enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}
	return -EINVAL;
}

static int clock_control_bl616cl_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	struct clock_control_bl616cl_data *data = dev->data;

	if ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_root) {
		*rate = clock_control_bl616cl_get_hclk(dev);
	} else if  ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_bclk) {
		*rate = clock_control_bl616cl_get_bclk(dev);
	} else if  ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_crystal) {
		*rate = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else if  ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_160mux) {
		if (data->pll.enabled) {
			*rate = clock_control_bl616cl_get_160m(dev);
		} else {
			return -EINVAL;
		}
	} else if  ((enum bl616cl_clkid)sys == bl616cl_clkid_clk_rc32m) {
		*rate = BFLB_RC32M_FREQUENCY;
	} else {
		return -EINVAL;
	}
	return 0;
}

static int clock_control_bl616cl_init(const struct device *dev)
{
	int ret;
	uint32_t key;

	key = irq_lock();

	clock_control_bl616cl_gate_all_peripherals();

	ret = clock_control_bl616cl_update_clocks(dev);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	clock_control_bl616cl_peripheral_clock_init();

	clock_bflb_settle();

	irq_unlock(key);

	return 0;
}

static DEVICE_API(clock_control, clock_control_bl616cl_api) = {
	.on = clock_control_bl616cl_on,
	.off = clock_control_bl616cl_off,
	.get_rate = clock_control_bl616cl_get_rate,
	.get_status = clock_control_bl616cl_get_status,
};

static const struct clock_control_bl616cl_config clock_control_bl616cl_config = {
	.crystal_id = CRYSTAL_FREQ_TO_ID(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal),
						 clock_frequency)),
};

static struct clock_control_bl616cl_data clock_control_bl616cl_data = {
	.crystal_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)),

	.root = {
#if CLK_SRC_IS(root, pll_top)
		.pll_select = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select),
		.source = bl616cl_clkid_clk_pll,
#elif CLK_SRC_IS(root, crystal)
		.source = bl616cl_clkid_clk_crystal,
#else
		.source = bl616cl_clkid_clk_rc32m,
#endif
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), divider),
	},

	.pll = {
#if CLK_SRC_IS(pll_top, crystal)
		.source = bl616cl_clkid_clk_crystal,
#else
		.source = bl616cl_clkid_clk_rc32m,
#endif
		.top_frequency = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_top),
					 top_frequency),
		.enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_top)),
	},

	.bclk = {
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, bclk), divider),
	},

	.flashclk = {
#if CLK_SRC_IS(flash, crystal)
		.source = bl616cl_clkid_clk_crystal,
#elif CLK_SRC_IS(flash, bclk)
		.source = bl616cl_clkid_clk_bclk,
#elif CLK_SRC_IS(flash, pll_top)
		.source = bl616cl_clkid_clk_pll,
#else
		.source = bl616cl_clkid_clk_rc32m,
#endif
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), divider),
	},

	.f32k = {
#if CLK_SRC_IS(f32k, xtal32k)
		.source = bl616cl_clkid_clk_xtal32k,
#else
		.source = bl616cl_clkid_clk_rc32k,
#endif
		.xtal_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, xtal32k)),
	},
};

BUILD_ASSERT((CLK_SRC_IS(aupll_top, crystal)
	|| CLK_SRC_IS(pll_top, crystal)
	|| CLK_SRC_IS(root, crystal)
	) ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)) : 1,
		"Crystal must be enabled to use it");

BUILD_ASSERT((CLK_SRC_IS(root, pll_top)
	|| CLK_SRC_IS(flash, pll_top)
	) ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll_top)) : 1,
		"PLL must be enabled to use it");


BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m)), "RC32M is always on");
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32k)), "RC32K is always on");

BUILD_ASSERT(CLK_SRC_IS(f32k, xtal32k)
	? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, xtal32k)) : 1,
	"XTAL32K must be enabled to use it");

BUILD_ASSERT(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m),
		     clock_frequency) == BFLB_RC32M_FREQUENCY, "RC32M must be 32M");

DEVICE_DT_INST_DEFINE(0, clock_control_bl616cl_init, NULL, &clock_control_bl616cl_data,
		      &clock_control_bl616cl_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_bl616cl_api);
