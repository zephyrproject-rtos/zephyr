/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl70x_l_clock_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_bl70x_l, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_BL70XL)
#include <zephyr/dt-bindings/clock/bflb_bl70xl_clock.h>
#include <bouffalolab/bl70xl/bflb_soc.h>
#include <bouffalolab/bl70xl/aon_reg.h>
#include <bouffalolab/bl70xl/glb_reg.h>
#include <bouffalolab/bl70xl/hbn_reg.h>
#include <bouffalolab/bl70xl/l1c_reg.h>
#include <bouffalolab/bl70xl/extra_defines.h>
#include <bouffalolab/bl70xl/sf_ctrl_reg.h>
#else
#include <zephyr/dt-bindings/clock/bflb_bl70x_clock.h>
#include <bouffalolab/bl70x/bflb_soc.h>
#include <bouffalolab/bl70x/aon_reg.h>
#include <bouffalolab/bl70x/glb_reg.h>
#include <bouffalolab/bl70x/hbn_reg.h>
#include <bouffalolab/bl70x/pds_reg.h>
#include <bouffalolab/bl70x/l1c_reg.h>
#include <bouffalolab/bl70x/extra_defines.h>
#include <bouffalolab/bl70x/sf_ctrl_reg.h>
#endif
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define CLK_SRC_IS(clk, src)                                                                       \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

/* Primary DLL output clock-name differs between BL70x (dll_144) and BL70xL (dll_128)
 * RC32K trim location and size also differs.
 */
#if defined(CONFIG_SOC_SERIES_BL70XL)

#define DLL_PRIMARY_NAME            dll_128
#define EFUSE_RC32K_TRIM_OFFSET     0x0c
#define EFUSE_RC32K_TRIM_EN_POS     25
#define EFUSE_RC32K_TRIM_PARITY_POS 24
#define EFUSE_RC32K_TRIM_POS        20
#define EFUSE_RC32K_TRIM_MSK        0xf00000

#else

#define DLL_PRIMARY_NAME            dll_144
#define EFUSE_RC32K_TRIM_OFFSET     0x0c
#define EFUSE_RC32K_TRIM_EN_POS     31
#define EFUSE_RC32K_TRIM_PARITY_POS 30
#define EFUSE_RC32K_TRIM_POS        20
#define EFUSE_RC32K_TRIM_MSK        0x3ff00000

#endif

#define CLOCK_TIMEOUT               1024
#define EFUSE_RC32M_TRIM_OFFSET     0x0c
#define EFUSE_RC32M_TRIM_EN_POS     19
#define EFUSE_RC32M_TRIM_PARITY_POS 18
#define EFUSE_RC32M_TRIM_POS        10
#define EFUSE_RC32M_TRIM_MSK        0x3fc00

#define BL70X_L_UNDOCUMENTED_BCLK_EN 0x40000ffc

enum bl70x_l_clkid {
#if defined(CONFIG_SOC_SERIES_BL70XL)
	bl70x_l_clkid_clk_root = BL70XL_CLKID_CLK_ROOT,
	bl70x_l_clkid_clk_rc32m = BL70XL_CLKID_CLK_RC32M,
	bl70x_l_clkid_clk_crystal = BL70XL_CLKID_CLK_CRYSTAL,
	bl70x_l_clkid_clk_bclk = BL70XL_CLKID_CLK_BCLK,
	bl70x_l_clkid_clk_f32k = BL70XL_CLKID_CLK_F32K,
	bl70x_l_clkid_clk_xtal32k = BL70XL_CLKID_CLK_XTAL32K,
	bl70x_l_clkid_clk_rc32k = BL70XL_CLKID_CLK_RC32K,
	bl70x_l_clkid_clk_dll = BL70XL_CLKID_CLK_DLL,
#else
	bl70x_l_clkid_clk_root = BL70X_CLKID_CLK_ROOT,
	bl70x_l_clkid_clk_rc32m = BL70X_CLKID_CLK_RC32M,
	bl70x_l_clkid_clk_crystal = BL70X_CLKID_CLK_CRYSTAL,
	bl70x_l_clkid_clk_bclk = BL70X_CLKID_CLK_BCLK,
	bl70x_l_clkid_clk_f32k = BL70X_CLKID_CLK_F32K,
	bl70x_l_clkid_clk_xtal32k = BL70X_CLKID_CLK_XTAL32K,
	bl70x_l_clkid_clk_rc32k = BL70X_CLKID_CLK_RC32K,
	bl70x_l_clkid_clk_dll = BL70X_CLKID_CLK_DLL,
#endif
};

struct clock_control_bl70x_l_dll_config {
	enum bl70x_l_clkid source;
	bool enabled;
};

struct clock_control_bl70x_l_root_config {
	enum bl70x_l_clkid source;
	uint8_t dll_select;
	uint8_t divider;
};

struct clock_control_bl70x_l_bclk_config {
	uint8_t divider;
};

struct clock_control_bl70x_l_flashclk_config {
	enum bl70x_l_clkid source;
	uint8_t divider;
	uint8_t read_delay;
	bool clock_invert;
	bool rx_clock_invert;
};

struct clock_control_bl70x_l_f32k_config {
	enum bl70x_l_clkid source;
	bool xtal_enabled;
};

struct clock_control_bl70x_l_data {
	bool crystal_enabled;
	struct clock_control_bl70x_l_dll_config dll;
	struct clock_control_bl70x_l_root_config root;
	struct clock_control_bl70x_l_bclk_config bclk;
	struct clock_control_bl70x_l_flashclk_config flashclk;
	struct clock_control_bl70x_l_f32k_config f32k;
};

static void clock_control_bl70x_l_clock_at_least_us(uint32_t us)
{
	for (uint32_t i = 0; i < us * 8; i++) {
		clock_bflb_settle();
	}
}

/* 0: rc32k
 * 1: xtal32k
 * 3: dig32k
 */
static void clock_control_bl70x_l_set_f32k_src(uint8_t src)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_F32K_SEL_UMSK;
	tmp |= (uint32_t)src << HBN_F32K_SEL_POS;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);
}

static void clock_control_bl70x_l_rc32k_enabled(bool yes)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_PU_RC32K_UMSK;
	if (yes) {
		tmp |= HBN_PU_RC32K_MSK;
	}
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);
}

static bool clock_control_bl70x_l_rc32k_is_enabled(void)
{
	return (sys_read32(HBN_BASE + HBN_GLB_OFFSET) & HBN_PU_RC32K_MSK) != 0;
}

static int clock_control_bl70x_l_deinit_crystal(void)
{
	uint32_t tmp;

	/* unpower crystal */
	tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	tmp = tmp & AON_PU_XTAL_AON_UMSK;
#if defined(CONFIG_SOC_SERIES_BL70XL)
	tmp = tmp & AON_PU_XTAL_HF_RC32M_AON_UMSK;
#else
	tmp = tmp & AON_PU_XTAL_BUF_AON_UMSK;
#endif
	sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

	clock_bflb_settle();
	return 0;
}

static int clock_control_bl70x_l_init_crystal(void)
{
	uint32_t tmp;
	int count = CLOCK_TIMEOUT;

	/* power crystal */
	tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	tmp = (tmp & AON_PU_XTAL_AON_UMSK) | (1U << AON_PU_XTAL_AON_POS);
#if defined(CONFIG_SOC_SERIES_BL70XL)
	tmp = (tmp & AON_PU_XTAL_HF_RC32M_AON_UMSK) | (1U << AON_PU_XTAL_HF_RC32M_AON_POS);
#else
	tmp = (tmp & AON_PU_XTAL_BUF_AON_UMSK) | (1U << AON_PU_XTAL_BUF_AON_POS);
#endif
	sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

	/* wait for crystal to be powered on */
	do {
		clock_bflb_settle();
		tmp = sys_read32(AON_BASE + AON_TSEN_OFFSET);
		count--;
	} while (((tmp & AON_XTAL_RDY_MSK) == 0U) && count > 0);

	clock_bflb_settle();
	if (count < 1) {
		return -ETIMEDOUT;
	}
	return 0;
}

/* HCLK is the core clock */
static void clock_control_bl70x_l_set_root_clock_dividers(uint32_t hclk_div, uint32_t bclk_div)
{
	uint32_t tmp;
	uint32_t old_rootclk;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1U) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}

	/* set dividers */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_UMSK) | (hclk_div << GLB_REG_HCLK_DIV_POS);
	tmp = (tmp & GLB_REG_BCLK_DIV_UMSK) | (bclk_div << GLB_REG_BCLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* Disable then reenable bclk */
	sys_write32(0x00000001, BL70X_L_UNDOCUMENTED_BCLK_EN);
	sys_write32(0x00000000, BL70X_L_UNDOCUMENTED_BCLK_EN);

	clock_bflb_settle();

	/* enable clocks */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK) | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK) | (1U << GLB_REG_HCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();
}

static void clock_control_bl70x_l_set_machine_timer_clock_enable(bool enable)
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
 * 0: BCLK (BL70x) or XCLK (BL70xL)
 * 1: 32Khz Oscillator (RC32*K*)
 */
static void clock_control_bl70x_l_set_machine_timer_clock(bool enable, uint32_t clock,
							  uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x1ffffU) {
		divider = 0x1ffffU;
	}
	if (clock > 1U) {
		clock = 1U;
	}

	/* disable first, then set div */
	clock_control_bl70x_l_set_machine_timer_clock_enable(false);

	tmp = sys_read32(GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
	tmp = (tmp & GLB_CPU_RTC_SEL_UMSK) | (clock << GLB_CPU_RTC_SEL_POS);
	tmp = (tmp & GLB_CPU_RTC_DIV_UMSK) | (divider << GLB_CPU_RTC_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);

	clock_control_bl70x_l_set_machine_timer_clock_enable(enable);
}

static void clock_control_bl70x_l_deinit_dll(void)
{
	uint32_t tmp;

	/* DLL Off */
	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_PPU_DLL_UMSK) | (0U << GLB_PPU_DLL_POS);
	tmp = (tmp & GLB_PU_DLL_UMSK) | (0U << GLB_PU_DLL_POS);
	tmp = (tmp & GLB_DLL_RESET_UMSK) | (1U << GLB_DLL_RESET_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);
}

/* XTAL : 0
 * RC32M : 1
 */
static void clock_control_bl70x_l_set_dll_source(uint32_t source)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	if (source > 0U) {
		tmp = (tmp & GLB_DLL_REFCLK_SEL_UMSK) | (1U << GLB_DLL_REFCLK_SEL_POS);
	} else {
		tmp = (tmp & GLB_DLL_REFCLK_SEL_UMSK) | (0U << GLB_DLL_REFCLK_SEL_POS);
	}
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);
}

static void clock_control_bl70x_l_init_dll(enum bl70x_l_clkid source)
{
	uint32_t tmp;
	uint32_t old_rootclk;

	old_rootclk = clock_bflb_get_root_clock();

	/* security RC32M */
	if (old_rootclk > 1U) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_RC32M);
	}

	clock_control_bl70x_l_deinit_dll();

	if (source == bl70x_l_clkid_clk_crystal) {
		clock_control_bl70x_l_set_dll_source(0);
	} else {
		clock_control_bl70x_l_set_dll_source(1);
	}

	/* init sequence */
	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_DLL_PRECHG_SEL_UMSK) | (1U << GLB_DLL_PRECHG_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_PPU_DLL_UMSK) | (1U << GLB_PPU_DLL_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);

	clock_control_bl70x_l_clock_at_least_us(2);

	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_PU_DLL_UMSK) | (1U << GLB_PU_DLL_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);

	clock_control_bl70x_l_clock_at_least_us(2);

	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_DLL_RESET_UMSK) | (0U << GLB_DLL_RESET_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);

	clock_control_bl70x_l_clock_at_least_us(5);

	clock_bflb_set_root_clock(old_rootclk);
	clock_bflb_settle();
}

/*
 * 0: BL70X_L_DLL_0_FREQ
 * 1: BL70X_L_DLL_1_FREQ
 * 2: BL70X_L_DLL_2_FREQ
 * 3: BL70X_L_DLL_3_FREQ (Do not use on BL70x)
 */
static void clock_control_bl70x_l_select_DLL(uint8_t dll)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_SEL_UMSK) | ((uint32_t)dll << GLB_REG_PLL_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);
}

static int clock_control_bl70x_l_clock_trim_32M(void)
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
	if (((trim >> EFUSE_RC32M_TRIM_EN_POS) & 1U) == 0) {
		LOG_ERR("RC32M trim disabled!");
		return -EINVAL;
	}

	trim_parity = (trim >> EFUSE_RC32M_TRIM_PARITY_POS) & 1U;
	trim = (trim & EFUSE_RC32M_TRIM_MSK) >> EFUSE_RC32M_TRIM_POS;

	if (trim_parity != (POPCOUNT(trim) & 1U)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_BL70XL)
	tmp = sys_read32(GLB_BASE + GLB_RC32M_CTRL0_OFFSET);
	tmp = (tmp & GLB_RC32M_EXT_CODE_EN_UMSK) | (1U << GLB_RC32M_EXT_CODE_EN_POS);
	tmp = (tmp & GLB_RC32M_CODE_FR_EXT_UMSK) | (trim << GLB_RC32M_CODE_FR_EXT_POS);
	sys_write32(tmp, GLB_BASE + GLB_RC32M_CTRL0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_RC32M_CTRL1_OFFSET);
	tmp &= GLB_RC32M_EXT_CODE_SEL_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_RC32M_CTRL1_OFFSET);
#else
	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	tmp = (tmp & PDS_RC32M_EXT_CODE_EN_UMSK) | (1U << PDS_RC32M_EXT_CODE_EN_POS);
	tmp = (tmp & PDS_RC32M_CODE_FR_EXT_UMSK) | (trim << PDS_RC32M_CODE_FR_EXT_POS);
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
#endif

	clock_bflb_settle();

	return 0;
}

/* source for most clocks, either XTAL or RC32M */
static uint32_t clock_control_bl70x_l_get_xclk(const struct device *dev)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = tmp >> HBN_ROOT_CLK_SEL_POS;
	tmp &= 1U;
	/* on BL70x/L crystal can only be 32MHz */
	if (tmp == 0 || tmp == 1U) {
		return BFLB_RC32M_FREQUENCY;
	} else {
		return 0;
	}
}

static uint32_t clock_control_bl70x_l_get_clk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t hclk_div;

	hclk_div = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	hclk_div = (hclk_div & GLB_REG_HCLK_DIV_MSK) >> GLB_REG_HCLK_DIV_POS;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_ROOT_CLK_SEL_MSK;
	tmp = (tmp >> HBN_ROOT_CLK_SEL_POS) >> 1;
	tmp &= 1U;

	if (tmp == 0) {
		return clock_control_bl70x_l_get_xclk(dev) / (hclk_div + 1);
	}
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_SEL_MSK) >> GLB_REG_PLL_SEL_POS;
#if defined(CONFIG_SOC_SERIES_BL70XL)
	if (tmp == 3) {
		return MHZ(128) / (hclk_div + 1);
	} else if (tmp == 2) {
		return MHZ(64) / (hclk_div + 1);
	} else if (tmp == 1) {
		return MHZ(42.67) / (hclk_div + 1);
	} else if (tmp == 0) {
		return MHZ(25.6) / (hclk_div + 1);
	}
#else
	if (tmp == 3) {
		return MHZ(120) / (hclk_div + 1);
	} else if (tmp == 2) {
		return MHZ(144) / (hclk_div + 1);
	} else if (tmp == 1) {
		return MHZ(96) / (hclk_div + 1);
	} else if (tmp == 0) {
		return MHZ(57.6) / (hclk_div + 1);
	}
#endif
	return 0;
}

/* most peripherals clock */
static uint32_t clock_control_bl70x_l_get_bclk(const struct device *dev)
{
	uint32_t tmp;
	uint32_t hclk_freq;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_DIV_MSK) >> GLB_REG_BCLK_DIV_POS;
	hclk_freq = clock_control_bl70x_l_get_clk(dev);
	return hclk_freq / (tmp + 1);
}

static uint32_t clock_control_bl70x_l_mtimer_get_clk_src_div(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_BL70XL)
	/* BL70XL mtimer source 0 is XCLK, not BCLK */
	return clock_control_bl70x_l_get_xclk(dev) / 1000 / 1000 - 1;
#else
	return clock_control_bl70x_l_get_bclk(dev) / 1000 / 1000 - 1;
#endif
}

static void clock_control_bl70x_l_cache_2T(bool yes)
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

static void clock_control_bl70x_l_setup_dll(const struct device *dev)
{
	struct clock_control_bl70x_l_data *data = dev->data;
	uint32_t tmp;

	clock_control_bl70x_l_init_dll(data->dll.source);

	/* enable all DLL output dividers */
#if defined(CONFIG_SOC_SERIES_BL70XL)
	tmp = sys_read32(GLB_BASE + GLB_DLL2_OFFSET);
	tmp = (tmp & GLB_DLL_EN_DIV1_UMSK) | (1U << GLB_DLL_EN_DIV1_POS); /* 128 MHz */
	tmp = (tmp & GLB_DLL_EN_DIV2_UMSK) | (1U << GLB_DLL_EN_DIV2_POS); /* 64 MHz */
	tmp = (tmp & GLB_DLL_EN_DIV3_UMSK) | (1U << GLB_DLL_EN_DIV3_POS); /* 42.67 MHz */
	tmp = (tmp & GLB_DLL_EN_DIV5_UMSK) | (1U << GLB_DLL_EN_DIV5_POS); /* 25.6 MHz */
	tmp = (tmp & GLB_DLL_EN_DIV21_UMSK) | (1U << GLB_DLL_EN_DIV21_POS);
	tmp = (tmp & GLB_DLL_EN_DIV63_UMSK) | (1U << GLB_DLL_EN_DIV63_POS);
	tmp = (tmp & GLB_DLL_EN_DIV1_RF_UMSK) | (1U << GLB_DLL_EN_DIV1_RF_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL2_OFFSET);
#else
	tmp = sys_read32(GLB_BASE + GLB_DLL_OFFSET);
	tmp = (tmp & GLB_DLL_CLK_57P6M_EN_UMSK) | (1U << GLB_DLL_CLK_57P6M_EN_POS);
	tmp = (tmp & GLB_DLL_CLK_96M_EN_UMSK) | (1U << GLB_DLL_CLK_96M_EN_POS);
	tmp = (tmp & GLB_DLL_CLK_144M_EN_UMSK) | (1U << GLB_DLL_CLK_144M_EN_POS);
	tmp = (tmp & GLB_DLL_CLK_288M_EN_UMSK) | (1U << GLB_DLL_CLK_288M_EN_POS);
	tmp = (tmp & GLB_DLL_CLK_MMDIV_EN_UMSK) | (1U << GLB_DLL_CLK_MMDIV_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_DLL_OFFSET);
#endif

	/* glb enable dll actual? */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_EN_UMSK) | (1U << GLB_REG_PLL_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);
}

static void clock_control_bl70x_l_init_root_as_dll(const struct device *dev)
{
	struct clock_control_bl70x_l_data *data = dev->data;

	clock_control_bl70x_l_select_DLL(data->root.dll_select);

	if (data->dll.source == bl70x_l_clkid_clk_crystal) {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_XTAL);
	} else {
		clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_PLL_RC32M);
	}

	if (clock_control_bl70x_l_get_clk(dev) > MHZ(120)) {
		clock_control_bl70x_l_cache_2T(true);
	}

	sys_write32(clock_control_bl70x_l_get_clk(dev), CORECLOCKREGISTER);
}

static void clock_control_bl70x_l_init_root_as_crystal(const struct device *dev)
{
	clock_bflb_set_root_clock(BFLB_MAIN_CLOCK_XTAL);
	sys_write32(clock_control_bl70x_l_get_clk(dev), CORECLOCKREGISTER);
}

static __ramfunc void clock_control_bl70x_l_update_flash_clk(const struct device *dev)
{
	struct clock_control_bl70x_l_data *data = dev->data;
	volatile uint32_t tmp;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp &= GLB_SF_CLK_DIV_UMSK;
	tmp &= GLB_SF_CLK_EN_UMSK;
	tmp |= ((uint32_t)(data->flashclk.divider - 1U)) << GLB_SF_CLK_DIV_POS;
	*(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET) = tmp;

	tmp = *(volatile uint32_t *)(SF_CTRL_BASE + SF_CTRL_0_OFFSET);
	tmp |= SF_CTRL_SF_IF_READ_DLY_EN_MSK;
	tmp &= ~SF_CTRL_SF_IF_READ_DLY_N_MSK;
	tmp |= ((uint32_t)data->flashclk.read_delay << SF_CTRL_SF_IF_READ_DLY_N_POS);
	if (data->flashclk.clock_invert) {
		tmp &= ~SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
	} else {
		tmp |= SF_CTRL_SF_CLK_OUT_INV_SEL_MSK;
	}
	if (data->flashclk.rx_clock_invert) {
		tmp |= SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
	} else {
		tmp &= ~SF_CTRL_SF_CLK_SF_RX_INV_SEL_MSK;
	}
	*(volatile uint32_t *)(SF_CTRL_BASE + SF_CTRL_0_OFFSET) = tmp;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp &= GLB_SF_CLK_SEL_UMSK;
#if !defined(CONFIG_SOC_SERIES_BL70XL)
	tmp &= GLB_SF_CLK_SEL2_UMSK;
#endif
	if (data->flashclk.source == bl70x_l_clkid_clk_dll) {
#if defined(CONFIG_SOC_SERIES_BL70XL)
		tmp |= 1U << GLB_SF_CLK_SEL_POS;
#else
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 0U << GLB_SF_CLK_SEL2_POS;
#endif
	} else if (data->flashclk.source == bl70x_l_clkid_clk_crystal) {
#if defined(CONFIG_SOC_SERIES_BL70XL)
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
#else
		tmp |= 0U << GLB_SF_CLK_SEL_POS;
		tmp |= 1U << GLB_SF_CLK_SEL2_POS;
#endif
	} else {
		/* If using RC32M or BCLK, use BCLK */
		tmp |= 2U << GLB_SF_CLK_SEL_POS;
	}

	*(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET) = tmp;

	tmp = *(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp |= GLB_SF_CLK_EN_MSK;
	*(volatile uint32_t *)(GLB_BASE + GLB_CLK_CFG2_OFFSET) = tmp;

	clock_bflb_settle();
}

static int clock_control_bl70x_l_clock_trim_32K(void)
{
	uint32_t tmp;
	int err;
	uint32_t trim, trim_parity;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);

	err = syscon_read_reg(efuse, EFUSE_RC32K_TRIM_OFFSET, &trim);
	if (err < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", err);
		return err;
	}
	if (((trim >> EFUSE_RC32K_TRIM_EN_POS) & 1U) == 0) {
		LOG_ERR("RC32K trim disabled!");
		return -EINVAL;
	}

	trim_parity = (trim >> EFUSE_RC32K_TRIM_PARITY_POS) & 1U;
	trim = (trim & EFUSE_RC32K_TRIM_MSK) >> EFUSE_RC32K_TRIM_POS;

	if (trim_parity != (POPCOUNT(trim) & 1U)) {
		LOG_ERR("Bad trim parity");
		return -EINVAL;
	}

	tmp = sys_read32(HBN_BASE + HBN_RC32K_CTRL0_OFFSET);
#if defined(CONFIG_SOC_SERIES_BL70XL)
	tmp = (tmp & HBN_RC32K_CAP_SEL_UMSK) | (trim << HBN_RC32K_CAP_SEL_POS);
#else
	tmp |= HBN_RC32K_EXT_CODE_EN_MSK;
	tmp = (tmp & HBN_RC32K_CODE_FR_EXT_UMSK) | (trim << HBN_RC32K_CODE_FR_EXT_POS);
#endif
	sys_write32(tmp, HBN_BASE + HBN_RC32K_CTRL0_OFFSET);

	clock_bflb_settle();

	return 0;
}

static int clock_control_bl70x_l_update_f32k(const struct device *dev)
{
	struct clock_control_bl70x_l_data *data = dev->data;
	bool wait_change = false;
	uint32_t tmp, tmpold;
	int ret;

	if (data->f32k.source != bl70x_l_clkid_clk_xtal32k &&
	    data->f32k.source != bl70x_l_clkid_clk_rc32k) {
		return -EINVAL;
	}

	if (!clock_control_bl70x_l_rc32k_is_enabled()) {
		clock_control_bl70x_l_rc32k_enabled(true);
		wait_change = true;
	}

	if (data->f32k.xtal_enabled) {
		tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
		tmpold = tmp;
		tmp |= HBN_PU_XTAL32K_MSK;
		tmp |= HBN_PU_XTAL32K_BUF_MSK;
		if (tmpold != tmp) {
			sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);
			wait_change = true;
		}
	} else {
		tmp = sys_read32(HBN_BASE + HBN_XTAL32K_OFFSET);
		tmp &= HBN_PU_XTAL32K_UMSK;
		tmp &= HBN_PU_XTAL32K_BUF_UMSK;
		sys_write32(tmp, HBN_BASE + HBN_XTAL32K_OFFSET);
	}

	if (wait_change) {
		clock_control_bl70x_l_clock_at_least_us(1000);
	}

	if (data->f32k.source == bl70x_l_clkid_clk_rc32k) {
		ret = clock_control_bl70x_l_clock_trim_32K();
		if (ret < 0) {
			return ret;
		}
		clock_control_bl70x_l_set_f32k_src(0);
	} else {
		clock_control_bl70x_l_set_f32k_src(1);
	}

	return 0;
}

static int clock_control_bl70x_l_update_clocks(const struct device *dev)
{
	struct clock_control_bl70x_l_data *data = dev->data;
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
	clock_control_bl70x_l_set_root_clock_dividers(0, 0);
	sys_write32(BFLB_RC32M_FREQUENCY, CORECLOCKREGISTER);

	clock_control_bl70x_l_cache_2T(false);

	ret = clock_control_bl70x_l_update_f32k(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->crystal_enabled) {
		if (clock_control_bl70x_l_init_crystal() < 0) {
			return -EIO;
		}
	} else {
		clock_control_bl70x_l_deinit_crystal();
	}

	clock_control_bl70x_l_set_root_clock_dividers(data->root.divider - 1,
						      data->bclk.divider - 1);

	if (data->dll.enabled) {
		clock_control_bl70x_l_setup_dll(dev);
	} else {
		clock_control_bl70x_l_deinit_dll();
	}

	if (data->root.source == bl70x_l_clkid_clk_dll) {
		if (!data->dll.enabled) {
			return -EINVAL;
		}
		clock_control_bl70x_l_init_root_as_dll(dev);
	} else if (data->root.source == bl70x_l_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			return -EINVAL;
		}
		clock_control_bl70x_l_init_root_as_crystal(dev);
	} else {
		/* Root clock already setup as RC32M */
	}

	ret = clock_control_bl70x_l_clock_trim_32M();
	if (ret < 0) {
		return ret;
	}
	clock_control_bl70x_l_set_machine_timer_clock(
		true, 0, clock_control_bl70x_l_mtimer_get_clk_src_div(dev));

	clock_bflb_settle();

	return ret;
}

static void clock_control_bl70x_l_uart_set_clock_enable(bool enable)
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
 * DLL: 1
 * When using DLL root clock, we can use either setting, when using the 32Mhz Oscillator with a
 * uninitialized DLL, only FCLK will be available.
 */
static void clock_control_bl70x_l_uart_set_clock(bool enable, uint32_t clock, uint32_t divider)
{
	uint32_t tmp;

	if (divider > 0x7u) {
		divider = 0x7u;
	}
	if (clock > 1U) {
		clock = 1U;
	}
	/* disable uart clock */
	clock_control_bl70x_l_uart_set_clock_enable(false);

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp = (tmp & GLB_UART_CLK_DIV_UMSK) | (divider << GLB_UART_CLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG2_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp = (tmp & HBN_UART_CLK_SEL_UMSK) | (clock << HBN_UART_CLK_SEL_POS);
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	clock_control_bl70x_l_uart_set_clock_enable(enable);
}

/* Simple function to enable all peripherals for now */
static void clock_control_bl70x_l_peripheral_clock_init(void)
{
	uint32_t regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	/* enable ADC clock routing */
	regval |= (1U << 2);
	/* enable SEC_DBG clock routing */
	regval |= (1U << 3);
	/* enable SEC_ENG clock routing */
	regval |= (1U << 4);
	/* enable UART0 clock routing */
	regval |= (1U << 16);
	/* enable SPI0 clock routing */
	regval |= (1U << 18);
	/* enable I2C0 clock routing */
	regval |= (1U << 19);
	/* enable PWM clock routing */
	regval |= (1U << 20);
	/* enable DMA clock routing */
	regval |= (1U << 12);
	/* enable Timers clock routing */
	regval |= (1U << 21);
	/* enable IR clock routing */
	regval |= (1U << 22);

	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);

	clock_control_bl70x_l_uart_set_clock(true, 0, 0);
}

static int clock_control_bl70x_l_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl70x_l_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;
	enum bl70x_l_clkid oldroot;

	key = irq_lock();

	if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_crystal) {
		if (data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = true;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->crystal_enabled = false;
			}
		}
	} else if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_dll) {
		if (data->dll.enabled) {
			ret = 0;
		} else {
			data->dll.enabled = true;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->dll.enabled = false;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_RC32M) {
		if (data->root.source == bl70x_l_clkid_clk_rc32m) {
			ret = 0;
		} else {
			/* Cannot fail to set root to rc32m */
			data->root.source = bl70x_l_clkid_clk_rc32m;
			ret = clock_control_bl70x_l_update_clocks(dev);
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_CRYSTAL) {
		if (data->root.source == bl70x_l_clkid_clk_crystal) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl70x_l_clkid_clk_crystal;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	} else if ((int)sys == BFLB_FORCE_ROOT_PLL) {
		if (data->root.source == bl70x_l_clkid_clk_dll) {
			ret = 0;
		} else {
			oldroot = data->root.source;
			data->root.source = bl70x_l_clkid_clk_dll;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->root.source = oldroot;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static int clock_control_bl70x_l_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_control_bl70x_l_data *data = dev->data;
	int ret = -EINVAL;
	uint32_t key;

	key = irq_lock();

	if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_crystal) {
		if (!data->crystal_enabled) {
			ret = 0;
		} else {
			data->crystal_enabled = false;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->crystal_enabled = true;
			}
		}
	} else if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_dll) {
		if (!data->dll.enabled) {
			ret = 0;
		} else {
			data->dll.enabled = false;
			ret = clock_control_bl70x_l_update_clocks(dev);
			if (ret < 0) {
				data->dll.enabled = true;
			}
		}
	}

	irq_unlock(key);
	return ret;
}

static enum clock_control_status clock_control_bl70x_l_get_status(const struct device *dev,
								  clock_control_subsys_t sys)
{
	struct clock_control_bl70x_l_data *data = dev->data;

	switch ((enum bl70x_l_clkid)sys) {
	case bl70x_l_clkid_clk_root:
	case bl70x_l_clkid_clk_bclk:
	case bl70x_l_clkid_clk_rc32m:
		return CLOCK_CONTROL_STATUS_ON;
	case bl70x_l_clkid_clk_crystal:
		if (data->crystal_enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
	case bl70x_l_clkid_clk_dll:
		if (data->dll.enabled) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static int clock_control_bl70x_l_get_rate(const struct device *dev, clock_control_subsys_t sys,
					  uint32_t *rate)
{
	if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_root) {
		*rate = clock_control_bl70x_l_get_clk(dev);
	} else if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_bclk) {
		*rate = clock_control_bl70x_l_get_bclk(dev);
	} else if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_crystal) {
		*rate = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency);
	} else if ((enum bl70x_l_clkid)sys == bl70x_l_clkid_clk_rc32m) {
		*rate = BFLB_RC32M_FREQUENCY;
	} else {
		return -EINVAL;
	}
	return 0;
}

static int clock_control_bl70x_l_init(const struct device *dev)
{
	int ret;
	uint32_t key;

	key = irq_lock();

	ret = clock_control_bl70x_l_update_clocks(dev);
	if (ret < 0) {
		irq_unlock(key);
		return ret;
	}

	clock_control_bl70x_l_peripheral_clock_init();

	clock_bflb_settle();

	clock_control_bl70x_l_update_flash_clk(dev);

	irq_unlock(key);

	return 0;
}

static DEVICE_API(clock_control, clock_control_bl70x_l_api) = {
	.on = clock_control_bl70x_l_on,
	.off = clock_control_bl70x_l_off,
	.get_rate = clock_control_bl70x_l_get_rate,
	.get_status = clock_control_bl70x_l_get_status,
};

static struct clock_control_bl70x_l_data clock_control_bl70x_l_data = {
	.crystal_enabled = DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal)),

	.root = {
#if CLK_SRC_IS(root, DLL_PRIMARY_NAME)
			.source = bl70x_l_clkid_clk_dll,
			.dll_select = DT_CLOCKS_CELL(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), select),
#elif CLK_SRC_IS(root, crystal)
			.source = bl70x_l_clkid_clk_crystal,
#else
			.source = bl70x_l_clkid_clk_rc32m,
#endif
			.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, root), divider),
		},

	.dll = {
#if CLK_SRC_IS(DLL_PRIMARY_NAME, crystal)
			.source = bl70x_l_clkid_clk_crystal,
#else
			.source = bl70x_l_clkid_clk_rc32m,
#endif
			.enabled = DT_NODE_HAS_STATUS_OKAY(
				DT_INST_CLOCKS_CTLR_BY_NAME(0, DLL_PRIMARY_NAME)),
		},

	.bclk = {
			.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, bclk), divider),
		},

	.flashclk = {
#if CLK_SRC_IS(flash, crystal)
			.source = bl70x_l_clkid_clk_crystal,
#elif CLK_SRC_IS(flash, bclk)
			.source = bl70x_l_clkid_clk_bclk,
#elif CLK_SRC_IS(flash, DLL_PRIMARY_NAME)
			.source = bl70x_l_clkid_clk_dll,
#else
			.source = bl70x_l_clkid_clk_rc32m,
#endif
			.read_delay = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), read_delay),
			.clock_invert =
				DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), clock_invert),
			.rx_clock_invert =
				DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), rx_clock_invert),
			.divider = DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, flash), divider),
		},

	.f32k = {
#if CLK_SRC_IS(f32k, xtal32k)
			.source = bl70x_l_clkid_clk_xtal32k,
#else
			.source = bl70x_l_clkid_clk_rc32k,
#endif
			.xtal_enabled =
				DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, xtal32k)),
		},
};

BUILD_ASSERT((CLK_SRC_IS(DLL_PRIMARY_NAME, crystal) || CLK_SRC_IS(root, crystal))
		     ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal))
		     : 1,
	     "Crystal must be enabled to use it");

BUILD_ASSERT(CLK_SRC_IS(root, DLL_PRIMARY_NAME)
		     ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, DLL_PRIMARY_NAME))
		     : 1,
	     "DLL must be enabled to use it");

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m)), "RC32M is always on");
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32k)), "RC32K is always on");

BUILD_ASSERT(CLK_SRC_IS(f32k, xtal32k)
		     ? DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, xtal32k))
		     : 1,
	     "XTAL32K must be enabled to use it");

BUILD_ASSERT(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc32m), clock_frequency) ==
		     BFLB_RC32M_FREQUENCY,
	     "RC32M must be 32M");

BUILD_ASSERT(DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, crystal), clock_frequency) ==
		     BFLB_RC32M_FREQUENCY,
	     "Crystal must be 32M for BL70x/L");

DEVICE_DT_INST_DEFINE(0, clock_control_bl70x_l_init, NULL, &clock_control_bl70x_l_data, NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_bl70x_l_api);
