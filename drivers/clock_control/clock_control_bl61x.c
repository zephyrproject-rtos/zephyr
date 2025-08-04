/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl61x_clock_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/bflb_bl61x_clock.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_bl61x, CONFIG_CLOCK_CONTROL_LOG_LEVEL);


#include <bouffalolab/bl61x/bflb_soc.h>
#include <bouffalolab/bl61x/aon_reg.h>
#include <bouffalolab/bl61x/glb_reg.h>
#include <bouffalolab/bl61x/hbn_reg.h>
#include <bouffalolab/bl61x/mcu_misc_reg.h>
#include <bouffalolab/bl61x/pds_reg.h>
#include <bouffalolab/bl61x/sf_ctrl_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

#define CLOCK_TIMEOUT                  1024
#define EFUSE_RC32M_TRIM_OFFSET        0x7C
#define EFUSE_RC32M_TRIM_EP_OFFSET     0x78
#define EFUSE_RC32M_TRIM_EP_EN_POS     1
#define EFUSE_RC32M_TRIM_EP_PARITY_POS 0
#define EFUSE_RC32M_TRIM_POS           4
#define EFUSE_RC32M_TRIM_MSK           0xFF0

#define CRYSTAL_ID_FREQ_32000000 0
#define CRYSTAL_ID_FREQ_24000000 1
#define CRYSTAL_ID_FREQ_38400000 2
#define CRYSTAL_ID_FREQ_40000000 3
#define CRYSTAL_ID_FREQ_26000000 4

#define CRYSTAL_FREQ_TO_ID(freq) CONCAT(CRYSTAL_ID_FREQ_, freq)

enum bl61x_clkid {
	bl61x_clkid_clk_root = BL61X_CLKID_CLK_ROOT,
	bl61x_clkid_clk_rc32m = BL61X_CLKID_CLK_RC32M,
	bl61x_clkid_clk_crystal = BL61X_CLKID_CLK_CRYSTAL,
	bl61x_clkid_clk_wifipll = BL61X_CLKID_CLK_WIFIPLL,
	bl61x_clkid_clk_aupll = BL61X_CLKID_CLK_AUPLL,
	bl61x_clkid_clk_bclk = BL61X_CLKID_CLK_BCLK,
};

struct clock_control_bl61x_pll_config {
	enum bl61x_clkid	source;
	bool			overclock;
};

struct clock_control_bl61x_root_config {
	enum bl61x_clkid	source;
	uint8_t			pll_select;
	uint8_t			divider;
};

struct clock_control_bl61x_bclk_config {
	uint8_t	divider;
};

struct clock_control_bl61x_flashclk_config {
	enum bl61x_clkid	source;
	uint8_t			divider;
	uint8_t			bank1_read_delay;
	bool			bank1_clock_invert;
	bool			bank1_rx_clock_invert;
};

struct clock_control_bl61x_config {
	uint32_t	crystal_id;
};

struct clock_control_bl61x_data {
	bool	crystal_enabled;
	bool	wifipll_enabled;
	bool	aupll_enabled;
	struct clock_control_bl61x_pll_config		wifipll;
	struct clock_control_bl61x_pll_config		aupll;
	struct clock_control_bl61x_root_config		root;
	struct clock_control_bl61x_bclk_config		bclk;
	struct clock_control_bl61x_flashclk_config	flashclk;
};

typedef struct {
	uint8_t		pllRefdivRatio;
	uint8_t		pllIntFracSw;
	uint8_t		pllIcp1u;
	uint8_t		pllIcp5u;
	uint8_t		pllRz;
	uint8_t		pllCz;
	uint8_t		pllC3;
	uint8_t		pllR4Short;
	uint8_t		pllC4En;
	uint8_t		pllSelSampleClk;
	uint8_t		pllVcoSpeed;
	uint8_t		pllSdmCtrlHw;
	uint8_t		pllSdmBypass;
	uint32_t	pllSdmin;
	uint8_t		aupllPostDiv;
} bl61x_pll_config;

/* XCLK is 32M */
static const bl61x_pll_config wifipll_32M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x1E00000,
	.aupllPostDiv = 0,
};

/* XCLK is 38.4M */
static const bl61x_pll_config wifipll_38P4M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x1900000,
	.aupllPostDiv = 0,
};

/* XCLK is 40M */
static const bl61x_pll_config wifipll_40M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x1800000,
	.aupllPostDiv = 0,
};

/* XCLK is 24M */
static const bl61x_pll_config wifipll_24M = {
	.pllRefdivRatio = 1,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x1400000,
	.aupllPostDiv = 0,
};

/* XCLK is 26M */
static const bl61x_pll_config wifipll_26M = {
	.pllRefdivRatio = 1,
	.pllIntFracSw = 1,
	.pllIcp1u = 1,
	.pllIcp5u = 0,
	.pllRz = 5,
	.pllCz = 2,
	.pllC3 = 2,
	.pllR4Short = 0,
	.pllC4En = 1,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 0,
	.pllSdmBypass = 0,
	.pllSdmin = 0x1276276,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config wifipll_32M_O480M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x2D00000,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config wifipll_40M_O480M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x2400000,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config wifipll_38P4M_O480M = {
	.pllRefdivRatio = 2,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x2580000,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config wifipll_24M_O480M = {
	.pllRefdivRatio = 1,
	.pllIntFracSw = 0,
	.pllIcp1u = 0,
	.pllIcp5u = 2,
	.pllRz = 3,
	.pllCz = 1,
	.pllC3 = 2,
	.pllR4Short = 1,
	.pllC4En = 0,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 1,
	.pllSdmBypass = 1,
	.pllSdmin = 0x1E00000,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config wifipll_26M_O480M = {
	.pllRefdivRatio = 1,
	.pllIntFracSw = 1,
	.pllIcp1u = 1,
	.pllIcp5u = 0,
	.pllRz = 5,
	.pllCz = 2,
	.pllC3 = 2,
	.pllR4Short = 0,
	.pllC4En = 1,
	.pllSelSampleClk = 1,
	.pllVcoSpeed = 5,
	.pllSdmCtrlHw = 0,
	.pllSdmBypass = 0,
	.pllSdmin = 0x1BB13B1,
	.aupllPostDiv = 0,
};

static const bl61x_pll_config *const bl61x_pll_configs[6] = {
&wifipll_32M, &wifipll_24M, &wifipll_38P4M, &wifipll_40M, &wifipll_26M
};


static const bl61x_pll_config *const bl61x_pll_configs_O480M[6] = {
&wifipll_32M_O480M, &wifipll_24M_O480M, &wifipll_38P4M_O480M, &wifipll_40M_O480M, &wifipll_26M_O480M
};

/* this imagines we are at 320M clock */
static void clock_control_bl61x_clock_at_least_us(uint32_t us)
{
	for (uint32_t i = 0; i < us * 32; i++) {
		clock_bflb_settle();
	}
}

static int clock_control_bl61x_deinit_crystal(void)
{
	uint32_t tmp;

	/* power crystal */
	tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	tmp = tmp & AON_PU_XTAL_AON_UMSK;
	tmp = tmp & AON_PU_XTAL_BUF_AON_UMSK;
	sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

	clock_bflb_settle();
	return 0;
}

static int clock_control_bl61x_init_crystal(void)
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

/* /!\ on bl61x hclk is only for CLIC
 * FCLK is the core clock
 */
static int clock_bflb_set_root_clock_dividers(uint32_t hclk_div, uint32_t bclk_div)
{
	uint32_t tmp;
	uint32_t old_rootclk;
	int count = CLOCK_TIMEOUT;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}

	/* set dividers */
	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_UMSK) | (hclk_div << GLB_REG_HCLK_DIV_POS);
	tmp = (tmp & GLB_REG_BCLK_DIV_UMSK) | (bclk_div << GLB_REG_BCLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG1_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_ACT_PULSE_UMSK) | (1 << GLB_REG_BCLK_DIV_ACT_PULSE_POS);
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG1_OFFSET);


	do {
		tmp = sys_read32(GLB_BASE + GLB_SYS_CFG1_OFFSET);
		tmp &= GLB_STS_BCLK_PROT_DONE_MSK;
		tmp = tmp >> GLB_STS_BCLK_PROT_DONE_POS;
		count--;
	} while (count > 0 && tmp == 0);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();

	if (count < 1) {
		return -EIO;
	}

	return 0;
}

static void clock_control_bl61x_set_machine_timer_clock_enable(bool enable)
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
static void clock_control_bl61x_set_machine_timer_clock(bool enable, uint32_t source_clock,
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
	clock_control_bl61x_set_machine_timer_clock_enable(false);

	tmp = sys_read32(MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);
	tmp = (tmp & MCU_MISC_REG_MCU_RTC_DIV_UMSK)
		| ((divider & 0x3FF) << MCU_MISC_REG_MCU_RTC_DIV_POS);
	sys_write32(tmp, MCU_MISC_BASE + MCU_MISC_MCU_E907_RTC_OFFSET);

	clock_control_bl61x_set_machine_timer_clock_enable(enable);
}

static void clock_control_bl61x_deinit_wifipll(void)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp &= GLB_PU_WIFIPLL_UMSK;
	tmp &= GLB_PU_WIFIPLL_SFREG_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
}

/* RC32M : 0
 * XTAL : 1
 */
static void clock_control_bl61x_set_wifipll_source(uint32_t source)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG1_OFFSET);
	if (source == 1) {
		tmp = (tmp & GLB_WIFIPLL_REFCLK_SEL_UMSK) | (1U << GLB_WIFIPLL_REFCLK_SEL_POS);
	} else {
		tmp = (tmp & GLB_WIFIPLL_REFCLK_SEL_UMSK) | (3U << GLB_WIFIPLL_REFCLK_SEL_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG1_OFFSET);
}

static void clock_control_bl61x_init_wifipll_setup(const bl61x_pll_config *const config,
						   bool overclock)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG1_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_REFDIV_RATIO_UMSK)
		| (config->pllRefdivRatio << GLB_WIFIPLL_REFDIV_RATIO_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG1_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG2_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_INT_FRAC_SW_UMSK)
		| (config->pllIntFracSw << GLB_WIFIPLL_INT_FRAC_SW_POS);
	tmp = (tmp & GLB_WIFIPLL_ICP_1U_UMSK)
		| (config->pllIcp1u << GLB_WIFIPLL_ICP_1U_POS);
	tmp = (tmp & GLB_WIFIPLL_ICP_5U_UMSK)
		| (config->pllIcp5u << GLB_WIFIPLL_ICP_5U_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG2_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG3_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_RZ_UMSK)
		| (config->pllRz << GLB_WIFIPLL_RZ_POS);
	tmp = (tmp & GLB_WIFIPLL_CZ_UMSK)
		| (config->pllCz << GLB_WIFIPLL_CZ_POS);
	tmp = (tmp & GLB_WIFIPLL_C3_UMSK)
		| (config->pllC3 << GLB_WIFIPLL_C3_POS);
	tmp = (tmp & GLB_WIFIPLL_R4_SHORT_UMSK)
		| (config->pllR4Short << GLB_WIFIPLL_R4_SHORT_POS);
	tmp = (tmp & GLB_WIFIPLL_C4_EN_UMSK)
		| (config->pllC4En << GLB_WIFIPLL_C4_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG3_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG4_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SEL_SAMPLE_CLK_UMSK)
		| (config->pllSelSampleClk << GLB_WIFIPLL_SEL_SAMPLE_CLK_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG4_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG5_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_VCO_SPEED_UMSK)
		| (config->pllVcoSpeed << GLB_WIFIPLL_VCO_SPEED_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG5_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG6_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_CTRL_HW_UMSK)
		| (config->pllSdmCtrlHw << GLB_WIFIPLL_SDM_CTRL_HW_POS);
	tmp = (tmp & GLB_WIFIPLL_SDM_BYPASS_UMSK)
		| (config->pllSdmBypass << GLB_WIFIPLL_SDM_BYPASS_POS);
	tmp = (tmp & GLB_WIFIPLL_SDMIN_UMSK)
		| (config->pllSdmin << GLB_WIFIPLL_SDMIN_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG6_OFFSET);

	/* We need to overclock those as well for USB to work for some reason */
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG10_OFFSET);
	if (overclock) {
		tmp = (tmp & GLB_USBPLL_SDMIN_UMSK)
			| (0x3C000 << GLB_USBPLL_SDMIN_POS);
	} else {
		tmp = (tmp & GLB_USBPLL_SDMIN_UMSK)
			| (0x28000 << GLB_USBPLL_SDMIN_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG10_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG12_OFFSET);
	if (overclock) {
		tmp = (tmp & GLB_SSCDIV_SDMIN_UMSK)
			| (0x3C000 << GLB_SSCDIV_SDMIN_POS);
	} else {
		tmp = (tmp & GLB_SSCDIV_SDMIN_UMSK)
			| (0x28000 << GLB_SSCDIV_SDMIN_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG12_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_PU_WIFIPLL_SFREG_UMSK)
		| (1 << GLB_PU_WIFIPLL_SFREG_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);

	clock_control_bl61x_clock_at_least_us(8);

	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_PU_WIFIPLL_UMSK)
		| (1 << GLB_PU_WIFIPLL_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);

	clock_control_bl61x_clock_at_least_us(8);

	/* 'SDM reset' */
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_RSTB_UMSK)
		| (1 << GLB_WIFIPLL_SDM_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	clock_control_bl61x_clock_at_least_us(8);
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_RSTB_UMSK)
		| (0 << GLB_WIFIPLL_SDM_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	clock_control_bl61x_clock_at_least_us(8);
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_SDM_RSTB_UMSK)
		| (1 << GLB_WIFIPLL_SDM_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);

	/* 'pll reset' */
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_FBDV_RSTB_UMSK)
		| (1 << GLB_WIFIPLL_FBDV_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	clock_control_bl61x_clock_at_least_us(8);
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_FBDV_RSTB_UMSK)
		| (0 << GLB_WIFIPLL_FBDV_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	clock_control_bl61x_clock_at_least_us(8);
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_FBDV_RSTB_UMSK)
		| (1 << GLB_WIFIPLL_FBDV_RSTB_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG0_OFFSET);

	/* enable PLL outputs */
	tmp = sys_read32(GLB_BASE + GLB_WIFI_PLL_CFG8_OFFSET);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV3_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV3_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV4_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV4_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV5_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV5_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV6_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV6_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV8_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV8_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV10_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV10_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV12_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV12_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV20_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV20_POS);
	tmp = (tmp & GLB_WIFIPLL_EN_DIV30_UMSK)
		| (1 << GLB_WIFIPLL_EN_DIV30_POS);
	sys_write32(tmp, GLB_BASE + GLB_WIFI_PLL_CFG8_OFFSET);

	clock_control_bl61x_clock_at_least_us(50);
}

static void clock_control_bl61x_init_wifipll(const bl61x_pll_config *const *config,
					     enum bl61x_clkid source, uint32_t crystal_id)
{
	uint32_t tmp;
	uint32_t old_rootclk = 0;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}


	clock_control_bl61x_deinit_wifipll();

	if (source == BL61X_CLKID_CLK_CRYSTAL) {
		clock_control_bl61x_set_wifipll_source(1);
		if (config == bl61x_pll_configs_O480M) {
			clock_control_bl61x_init_wifipll_setup(config[crystal_id], true);
		} else {
			clock_control_bl61x_init_wifipll_setup(config[crystal_id], false);
		}
	} else {
		clock_control_bl61x_set_wifipll_source(0);
		clock_control_bl61x_init_wifipll_setup(config[CRYSTAL_ID_FREQ_32000000], false);
	}

	/* enable PLL clock */
	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp |= GLB_REG_PLL_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_SYS_CFG0_OFFSET);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();
}

/*
 * AUPLL   DIV1: 1
 * AUPLL   DIV2: 0
 * WIFIPLL 240Mhz: 2
 * WIFIPLL 320Mhz: 3
 */
static void clock_control_bl61x_select_PLL(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
	tmp = (tmp & PDS_REG_PLL_SEL_UMSK) | (pll << PDS_REG_PLL_SEL_POS);
	sys_write32(tmp, PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
}

/* 'just for safe'
 * ISP WIFIPLL 80M : 2
 * ISP AUPLL DIV5 : 3
 * ISP AUPLL DIV6 : 4
 * TOP AUPLL DIV5 : 5
 * TOP AUPLL DIV6 : 6
 * PSRAMB WIFIPLL 320M : 7
 * PSRAMB AUPLL DIV1 : 8
 * TOP WIFIPLL 240M : 13
 * TOP WIFIPLL 320M : 14
 * TOP AUPLL DIV2 : 15
 * TOP AUPLL DIV1 : 16
 */
static void clock_control_bl61x_ungate_pll(uint8_t pll)
{
	uint32_t tmp;

	tmp = sys_read32(PDS_BASE + GLB_CGEN_CFG3_OFFSET);
	tmp |= (1 << pll);
	sys_write32(tmp, PDS_BASE + GLB_CGEN_CFG3_OFFSET);
}

static int clock_control_bl61x_clock_trim_32M(void)
{
	uint32_t tmp;
	uint32_t trim, trim_ep;
	int err;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);


	err = syscon_read_reg(efuse, EFUSE_RC32M_TRIM_OFFSET, &trim);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	err = syscon_read_reg(efuse, EFUSE_RC32M_TRIM_EP_OFFSET, &trim_ep);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (!((trim_ep >> EFUSE_RC32M_TRIM_EP_EN_POS) & 1)) {
		LOG_ERR("RC32M trim disabled!");
		return -EINVAL;
	}

	trim = (trim & EFUSE_RC32M_TRIM_MSK) >> EFUSE_RC32M_TRIM_POS;

	if (((trim_ep >> EFUSE_RC32M_TRIM_EP_PARITY_POS) & 1) != (POPCOUNT(trim) & 1)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	tmp = (tmp & PDS_RC32M_EXT_CODE_EN_UMSK) | 1 << PDS_RC32M_EXT_CODE_EN_POS;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	clock_bflb_settle();

	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	tmp = (tmp & PDS_RC32M_CODE_FR_EXT2_UMSK) | trim << PDS_RC32M_CODE_FR_EXT2_POS;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL2_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	tmp = (tmp & PDS_RC32M_EXT_CODE_SEL_UMSK) | 1 << PDS_RC32M_EXT_CODE_SEL_POS;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	clock_bflb_settle();

	return 0;
}

/* source for most clocks, either XTAL or RC32M */
static uint32_t clock_control_bl61x_get_xclk(const struct device *dev)
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

static uint32_t clock_control_bl61x_mtimer_get_xclk_src_div(const struct device *dev)
{
	return (clock_control_bl61x_get_xclk(dev) / 1000 / 1000 - 1);
}

/* Almost always CPU, AXI bus, SRAM Memory, Cache, use HCLK query instead */
static uint32_t clock_control_bl61x_get_fclk(const struct device *dev)
{
	struct clock_control_bl61x_data *data = dev->data;
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = (tmp >> HBN_ROOT_CLK_SEL_POS) >> 1;
	tmp &= 1;

	if (tmp == 0) {
		return clock_control_bl61x_get_xclk(dev);
	}
	tmp = sys_read32(PDS_BASE + PDS_CPU_CORE_CFG1_OFFSET);
	tmp = (tmp & PDS_REG_PLL_SEL_MSK) >> PDS_REG_PLL_SEL_POS;
	if (tmp == 3) {
		if (data->wifipll.overclock) {
			return MHZ(480);
		} else {
			return MHZ(320);
		}
	} else if (tmp == 2) {
		if (data->wifipll.overclock) {
			return MHZ(360);
		} else {
			return MHZ(240);
		}
	} else if (tmp == 1) {
		/* TODO AUPLL DIV 1 */
	} else if (tmp == 0) {
		/* TODO AUPLL DIV 2 */
	} else {
		return 0;
	}
	return 0;
}

/* CLIC, should be same as FCLK ideally */
static uint32_t clock_control_bl61x_get_hclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t clock_f;

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_MSK) >> GLB_REG_HCLK_DIV_POS;
	clock_f = clock_control_bl61x_get_fclk(dev);
	return clock_f / (tmp + 1);
}

/* most peripherals clock */
static uint32_t clock_control_bl61x_get_bclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t source_clock;

	tmp = sys_read32(GLB_BASE + GLB_SYS_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_MSK) >> GLB_REG_BCLK_DIV_POS;
	source_clock = clock_control_bl61x_get_hclk(dev);
	return source_clock / (tmp + 1);
}

static void clock_control_bl61x_init_root_as_wifipll(const struct device *dev)
{
	struct clock_control_bl61x_data *data = dev->data;
	const struct clock_control_bl61x_config *config = dev->config;

	if (data->wifipll.overclock) {
		clock_control_bl61x_init_wifipll(bl61x_pll_configs_O480M,
						 data->wifipll.source, config->crystal_id);
	} else {
		clock_control_bl61x_init_wifipll(bl61x_pll_configs,
						 data->wifipll.source, config->crystal_id);
	}

	clock_control_bl61x_select_PLL(data->root.pll_select);

	/* 2T rom access goes here */

	if (data->root.pll_select == 1) {
		clock_control_bl61x_ungate_pll(14);
	} else if (data->root.pll_select == 2) {
		clock_control_bl61x_ungate_pll(13);
	}

	if (data->wifipll.source == bl61x_clkid_clk_crystal) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_XTAL);
	} else {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_RC32M);
	}
}

static void clock_control_bl61x_init_root_as_crystal(const struct device *dev)
{
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_XTAL);
}

static __ramfunc void clock_control_bl61x_update_flash_clk(const struct device *dev)
{
	struct clock_control_bl61x_data *data = dev->data;
	uint32_t tmp;

	tmp = *(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp &= GLB_SF_CLK_DIV_UMSK;
	tmp &= GLB_SF_CLK_EN_UMSK;
	tmp |= (data->flashclk.divider - 1) << GLB_SF_CLK_DIV_POS;
	*(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	tmp = *(uint32_t *)(SF_CTRL_BASE + SF_CTRL_0_OFFSET);
	tmp |= SF_CTRL_SF_IF_READ_DLY_EN_MSK;
	tmp &= ~SF_CTRL_SF_IF_READ_DLY_N_MSK;
	tmp |= (data->flashclk.bank1_read_delay << SF_CTRL_SF_IF_READ_DLY_N_POS);
	if (data->flashclk.bank1_clock_invert) {
		tmp &= ~SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
	} else {
		tmp |= SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
	}
	if (data->flashclk.bank1_rx_clock_invert) {
		tmp |= SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
	}
	*(uint32_t *)(SF_CTRL_BASE + SF_CTRL_0_OFFSET) = tmp;

	tmp = *(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp &= GLB_SF_CLK_SEL_UMSK;
	tmp &= GLB_SF_CLK_SEL2_UMSK;
	if (data->flashclk.source == bl61x_clkid_clk_wifipll) {
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
	} else if (data->flashclk.source == bl61x_clkid_clk_crystal) {
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 1U << GLB_SF_CLK_SEL2_POS;
	} else {
		/* If using RC32M or BCLK, use BCLK */
		tmp |= 2U << GLB_SF_CLK_SEL_POS;
	}

	*(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	tmp = *(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET);
	tmp |= GLB_SF_CLK_EN_MSK;
	*(uint32_t *)(GLB_BASE + GLB_SF_CFG0_OFFSET) = tmp;

	clock_bflb_settle();
}

static int clock_control_bl61x_update_root(const struct device *dev)
{
	struct clock_control_bl61x_data *data = dev->data;
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

	if (data->crystal_enabled) {
		if (clock_control_bl61x_init_crystal() < 0) {
			return -EIO;
		}
	} else {
		clock_control_bl61x_deinit_crystal();
	}

	ret = clock_bflb_set_root_clock_dividers(data->root.divider - 1, data->bclk.divider - 1);
	if (ret < 0) {
		return ret;
	}

	if (data->root.source == bl61x_clkid_clk_wifipll) {
		clock_control_bl61x_init_root_as_wifipll(dev);
	} else if (data->root.source == bl61x_clkid_clk_crystal) {
		clock_control_bl61x_init_root_as_crystal(dev);
		clock_control_bl61x_deinit_wifipll();
	} else {
		clock_control_bl61x_deinit_wifipll();
	}

	ret = clock_control_bl61x_clock_trim_32M();
	if (ret < 0) {
		return ret;
	}
	clock_control_bl61x_set_machine_timer_clock(
		1, 0, clock_control_bl61x_mtimer_get_xclk_src_div(dev));

	clock_bflb_settle();

	return ret;
}

static void clock_control_bl61x_uart_set_clock_enable(bool enable)
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
static void clock_control_bl61x_uart_set_clock(bool enable, uint32_t source_clock, uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x7) {
		divider = 0x7;
	}
	if (source_clock > 2) {
		source_clock = 2;
	}
	/* disable uart clock */
	clock_control_bl61x_uart_set_clock_enable(false);


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

	clock_control_bl61x_uart_set_clock_enable(enable);
}

/* Simple function to enable all peripherals for now */
static void clock_control_bl61x_peripheral_clock_init(void)
{
	uint32_t regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	/* enable ADC clock routing */
	regval |= (1 << 2);
	/* enable UART0 clock routing */
	regval |= (1 << 16);
	/* enable UART1 clock routing */
	regval |= (1 << 17);
	/* enable I2C0 clock routing */
	regval |= (1 << 19);
	/* enable I2C1 clock routing */
	regval |= (1 << 25);
	/* enable SPI0 clock routing */
	regval |= (1 << 18);
	/* enable USB clock routing */
	regval |= (1 << 13);

	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	clock_control_bl61x_uart_set_clock(1, 0, 2);
}

static int clock_control_bl61x_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl61x_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;
	enum bl61x_clkid oldroot;

	key = irq_lock();

	if ((enum bl61x_clkid)sys == bl61x_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = true;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->crystal_enabled = false;
			}
		}
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_wifipll) {
		if (data->wifipll_enabled) {
			ret = 0;
		} else {
			data->wifipll_enabled = true;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->wifipll_enabled = false;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_RC32M) {
		if (data->root.source == bl61x_clkid_clk_rc32m) {
			ret = 0;
		} else {
			/* Cannot fail to set root to rc32m */
			data->root.source = bl61x_clkid_clk_rc32m;
			ret = clock_control_bl61x_update_root(dev);
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_CRYSTAL) {
		if (data->root.source == bl61x_clkid_clk_crystal) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl61x_clkid_clk_crystal;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_PLL) {
		if (data->root.source == bl61x_clkid_clk_wifipll) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl61x_clkid_clk_wifipll;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static int clock_control_bl61x_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl61x_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;

	key = irq_lock();

	if ((enum bl61x_clkid)sys == bl61x_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = false;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->crystal_enabled = true;
			}
		}
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_wifipll) {
		if (!data->wifipll_enabled) {
			ret = 0;
		} else {
			data->wifipll_enabled = false;
			ret = clock_control_bl61x_update_root(dev);
			if (ret < 0) {
				data->wifipll_enabled = true;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static enum clock_control_status clock_control_bl61x_get_status(const struct device *dev,
								   clock_control_subsys_t sys)
{
	struct clock_control_bl61x_data *data = dev->data;

	if ((enum bl61x_clkid)sys == bl61x_clkid_clk_root) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_bclk) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_rc32m) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_wifipll) {
		if (data->wifipll_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else if ((enum bl61x_clkid)sys == bl61x_clkid_clk_aupll) {
		if (data->aupll_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}
	return -EINVAL;
}

static int clock_control_bl61x_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	if ((enum bl61x_clkid)sys == bl61x_clkid_clk_root) {
		*rate = clock_control_bl61x_get_hclk(dev);
	} else if  ((enum bl61x_clkid)sys == bl61x_clkid_clk_bclk) {
		*rate = clock_control_bl61x_get_bclk(dev);
	} else if  ((enum bl61x_clkid)sys == bl61x_clkid_clk_crystal) {
		*rate = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else if  ((enum bl61x_clkid)sys == bl61x_clkid_clk_rc32m) {
		*rate = BFLB_RC32M_FREQUENCY;
	} else {
		return -EINVAL;
	}
	return 0;
}

static int clock_control_bl61x_init(const struct device *dev)
{
	int ret;
	uint32_t key;

	key = irq_lock();

	ret = clock_control_bl61x_update_root(dev);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	clock_control_bl61x_peripheral_clock_init();

	clock_bflb_settle();

	clock_control_bl61x_update_flash_clk(dev);

	irq_unlock(key);

	return 0;
}

static DEVICE_API(clock_control, clock_control_bl61x_api) = {
	.on = clock_control_bl61x_on,
	.off = clock_control_bl61x_off,
	.get_rate = clock_control_bl61x_get_rate,
	.get_status = clock_control_bl61x_get_status,
};

static const struct clock_control_bl61x_config clock_control_bl61x_config = {
	.crystal_id = CRYSTAL_FREQ_TO_ID(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal),
						 clock_frequency)),
};

static struct clock_control_bl61x_data clock_control_bl61x_data = {
	.crystal_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)),
	.wifipll_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, wifipll_320)),
	.aupll_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, aupll_div1)),

	.root = {
#if CLK_SRC_IS(root, wifipll_320)
		.pll_select = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select) & 0xF,
		.source = bl61x_clkid_clk_wifipll,
#elif CLK_SRC_IS(root, aupll_div1)
		.pll_select = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select) & 0xF,
		.source = bl61x_clkid_clk_aupll,
#elif CLK_SRC_IS(root, crystal)
		.source = bl61x_clkid_clk_crystal,
#else
		.source = bl61x_clkid_clk_rc32m,
#endif
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), divider),
	},

	.wifipll = {
#if CLK_SRC_IS(wifipll_320, crystal)
		.source = bl61x_clkid_clk_crystal,
#else
		.source = bl61x_clkid_clk_rc32m,
#endif
#if CLK_SRC_IS(root, wifipll_320)
		.overclock = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select) & 0x10,
#endif
	},

	.aupll = {
#if CLK_SRC_IS(aupll_div1, crystal)
		.source = bl61x_clkid_clk_crystal,
#else
		.source = bl61x_clkid_clk_rc32m,
#endif
	},

	.bclk = {
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, bclk), divider),
	},

	.flashclk = {
#if CLK_SRC_IS(flash, crystal)
		.source = bl61x_clkid_clk_crystal,
#elif CLK_SRC_IS(flash, bclk)
		.source = bl61x_clkid_clk_bclk,
#elif CLK_SRC_IS(flash, wifipll_320)
		.source = bl61x_clkid_clk_wifipll,
#elif CLK_SRC_IS(flash, aupll_div1)
		.source = bl61x_clkid_clk_aupll,
#else
		.source = bl61x_clkid_clk_rc32m,
#endif
		.bank1_read_delay = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), read_delay),
		.bank1_clock_invert = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), clock_invert),
		.bank1_rx_clock_invert =
			DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), rx_clock_invert),
		.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), divider),
	},
};

BUILD_ASSERT(CLK_SRC_IS(aupll_div1, crystal)
	|| CLK_SRC_IS(wifipll_320, crystal)
	|| CLK_SRC_IS(root, crystal)
	? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)) : 1,
	     "Crystal must be enabled to use it");

BUILD_ASSERT(CLK_SRC_IS(root, wifipll_320)
	? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, wifipll_320)) : 1,
	     "Wifi PLL must be enabled to use it");

BUILD_ASSERT(CLK_SRC_IS(root, aupll_div1)
	? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, aupll_div1)) : 1,
	     "Audio PLL must be enabled to use it");

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m)),
	     "RC32M is always on");

BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, aupll_div1)),
	     "Audio PLL is unsupported");

BUILD_ASSERT(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m),
		     clock_frequency) == BFLB_RC32M_FREQUENCY, "RC32M must be 32M");

DEVICE_DT_INST_DEFINE(0, clock_control_bl61x_init, NULL, &clock_control_bl61x_data,
		      &clock_control_bl61x_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_bl61x_api);
