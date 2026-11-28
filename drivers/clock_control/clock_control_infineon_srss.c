/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_clk_srss

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one SRSS clock controller is supported");

#define IFX_SRSS_BASE		DT_INST_REG_ADDR(0)
#define IFX_OSC_NODE		DT_CHILD(DT_DRV_INST(0), oscillators)
#define IFX_PLL_NODE		DT_CHILD(DT_DRV_INST(0), pll)
#define IFX_ROOT_NODE		DT_CHILD(DT_DRV_INST(0), rootclocks)
#define IFX_ECO_NODE		DT_CHILD(IFX_OSC_NODE, eco_2)
#define IFX_HAS_CPUSS		DT_INST_REG_HAS_NAME(0, cpuss)
#define IFX_HAS_BACKUP		DT_INST_REG_HAS_NAME(0, backup)
#define IFX_BACKUP_BASE							\
	COND_CODE_1(IFX_HAS_BACKUP, (DT_INST_REG_ADDR_BY_NAME(0, backup)), (0))

#define SRSS_PATH_SELECT(p)	(0x1200U + ((p) * 4U))
#define SRSS_ROOT_SELECT(r)	(0x1240U + ((r) * 4U))
#define SRSS_DIRECT_SELECT(r)	(0x1280U + ((r) * 4U))
#define SRSS_PILO_CONFIG	0x1370U
#define SRSS_PILO_STATUS	0x1374U
#define SRSS_ECO_CONFIG		0x151CU
#define SRSS_ECO_STATUS		0x1524U
#define SRSS_ECO_CONFIG2	0x1544U
#define SRSS_FLL_CONFIG		0x1530U
#define SRSS_FLL_CONFIG2	0x1534U
#define SRSS_FLL_CONFIG3	0x1538U
#define SRSS_FLL_CONFIG4	0x153CU
#define SRSS_FLL_STATUS		0x1540U
#define SRSS_IHO_CONFIG		0x1560U
#define SRSS_PLL200_CONFIG(n)	(0x1600U + ((n) * 4U))
#define SRSS_PLL200_STATUS(n)	(0x1640U + ((n) * 4U))
#define SRSS_PLL400_CONFIG(n)	(0x1900U + ((n) * 16U))
#define SRSS_PLL400_CONFIG2(n)	(0x1904U + ((n) * 16U))
#define SRSS_PLL400_CONFIG3(n)	(0x1908U + ((n) * 16U))
#define SRSS_PLL400_STATUS(n)	(0x190CU + ((n) * 16U))
#define SRSS_DPLL_LP(n, off)	(SRSS_DPLL_LP_BASE + ((n) * 32U) + (off))
#define SRSS_DPLL_LP_CONFIG(n)	SRSS_DPLL_LP(n, 0x00U)
#define SRSS_DPLL_LP_CONFIG2(n)	SRSS_DPLL_LP(n, 0x04U)
#define SRSS_DPLL_LP_CONFIG3(n)	SRSS_DPLL_LP(n, 0x08U)
#define SRSS_DPLL_LP_CONFIG4(n)	SRSS_DPLL_LP(n, 0x0CU)
#define SRSS_DPLL_LP_CONFIG5(n)	SRSS_DPLL_LP(n, 0x10U)
#define SRSS_DPLL_LP_CONFIG6(n)	SRSS_DPLL_LP(n, 0x14U)
#define SRSS_DPLL_LP_CONFIG7(n)	SRSS_DPLL_LP(n, 0x18U)
#define SRSS_DPLL_LP_STATUS(n)	SRSS_DPLL_LP(n, 0x1CU)
#define SRSS_DPLL_HP_CONFIG(n)	(0x1800U + ((n) * 64U))
#define SRSS_DPLL_HP_CONFIG2(n)	(0x1804U + ((n) * 64U))
#define SRSS_DPLL_HP_CONFIG3(n)	(0x1808U + ((n) * 64U))
#define SRSS_DPLL_HP_CONFIG4(n)	(0x180CU + ((n) * 64U))
#define SRSS_DPLL_HP_CONFIG5(n)	(0x1810U + ((n) * 64U))
#define SRSS_DPLL_HP_TRIGMOD2(n) (0x1818U + ((n) * 64U))
#define SRSS_DPLL_HP_STATUS(n)	(0x181CU + ((n) * 64U))
#define SRSS_DPLL_HP_DUTYCAL(n)	(0x1820U + ((n) * 64U))
#define SRSS_PLL_CONFIG2_OFF	0x4U

#define PATH_MUX		GENMASK(2, 0)
#define ROOT_MUX		GENMASK(3, 0)
#define ROOT_DIV_POW2		GENMASK(5, 4)
#define ROOT_DIV_INT		GENMASK(11, 8)
#define ROOT_DIRECT_PATH	BIT(8)
#define ROOT_ENABLE		BIT(31)

#define BACKUP_CTL		0x0000U
#define BACKUP_WCO_STATUS	0x0090U

#define CPUSS_SLOW_CLOCK_CTL	0x0008U
#define CPUSS_MEM_CLOCK_CTL	0x0010U
#define CPUSS_INT_DIV		GENMASK(15, 8)

#define PILO_ENABLE		BIT(31)
#define PILO_OK			BIT(0)
#define WCO_EN			BIT(3)
#define WCO_OK			BIT(2)

#define ECO_AGC_EN		BIT(1)
#define ECO_EN			BIT(31)
#define ECO_OK			BIT(0)
#define ECO_READY		BIT(1)
#define ECO_WDTRIM		GENMASK(2, 0)
#define ECO_ATRIM		GENMASK(7, 4)
#define ECO_FTRIM		GENMASK(9, 8)
#define ECO_RTRIM		GENMASK(11, 10)
#define ECO_GTRIM		GENMASK(14, 12)

#define PLL200_FEEDBACK		GENMASK(6, 0)
#define PLL400_FEEDBACK		GENMASK(7, 0)
#define PLL_REFERENCE		GENMASK(12, 8)
#define PLL_OUTPUT		GENMASK(20, 16)
#define PLL_LOCK_DELAY		GENMASK(26, 25)
#define PLL_BYPASS		GENMASK(29, 28)
#define PLL_ENABLE		BIT(31)
#define PLL_FRAC_DIV		GENMASK(23, 0)
#define PLL_FRAC_EN		BIT(31)
#define PLL_LOCKED		BIT(0)
#define PLL_UNLOCK_OCCURRED	BIT(1)

#define FLL_MULT		GENMASK(17, 0)
#define FLL_OUTPUT_DIV		BIT(24)
#define FLL_ENABLE		BIT(31)
#define FLL_REF_DIV		GENMASK(12, 0)
#define FLL_LOCK_TOL		GENMASK(23, 16)
#define FLL_IGAIN		GENMASK(3, 0)
#define FLL_PGAIN		GENMASK(7, 4)
#define FLL_SETTLING_COUNT	GENMASK(20, 8)
#define FLL_CCO_RANGE		GENMASK(10, 8)
#define FLL_CCO_FREQ		GENMASK(24, 16)
#define FLL_CCO_ENABLE		BIT(31)
#define FLL_LOCKED		BIT(0)
#define FLL_UNLOCK_OCCURRED	BIT(1)
#define FLL_CCO_READY		BIT(2)

#define IHO_ENABLE		BIT(31)

#define DPLL_LP_DCO_MULT	BIT(27)
#define DPLL_LP_ACC_MODE	GENMASK(17, 16)
#define DPLL_LP_TDC_MODE	GENMASK(19, 18)
#define DPLL_LP_DCO_CODE	GENMASK(10, 0)

#define DPLL_HP_NDIV		GENMASK(7, 0)
#define DPLL_HP_PDIV		GENMASK(10, 8)
#define DPLL_HP_KDIV		GENMASK(14, 12)
#define DPLL_HP_FRAC_DIV	GENMASK(20, 0)
#define DPLL_HP_FREQ_MODE	GENMASK(23, 21)
#define DPLL_HP_IVR_TRIM	GENMASK(27, 24)
#define DPLL_HP_CLKR_SEL	BIT(0)
#define DPLL_HP_LC_ALPHA	GENMASK(4, 0)
#define DPLL_HP_LC_BETA		GENMASK(9, 5)
#define DPLL_HP_FLOCK_EN_THRESH	GENMASK(13, 11)
#define DPLL_HP_FLOCK_WAITPER	GENMASK(19, 16)
#define DPLL_HP_FLOCK_LK_THRESH	GENMASK(22, 20)
#define DPLL_HP_FLOCK_LK_WAITPER GENMASK(26, 23)
#define DPLL_HP_LF_ALPHA	GENMASK(4, 0)
#define DPLL_HP_LF_BETA		GENMASK(9, 5)
#define DPLL_HP_LF_SET_PARAMS	BIT(12)
#define DPLL_HP_DUTYCAL_EN	BIT(31)

#define PLL_BYPASS_AUTO		0
#define PLL_BYPASS_INPUT	2U
#define PLL_BYPASS_OUTPUT	3U

#define IFX_PLL_LOCK_DELAY_FRAC	1U
#define IFX_PLL400_FRAC_BITS	POPCOUNT(PLL_FRAC_DIV)
#define IFX_DPLL_LP_FRAC_BITS	24U
#define IFX_DPLL_HP_FRAC_BITS	21U

#define IFX_HAS_FLL							\
	!IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS)
#define IFX_FLL_PATH		0
#define IFX_FLL_REF_SCALE	250U
#define IFX_FLL_LOCK_TOL_NUM	18939U
#define IFX_FLL_LOCK_TOL_DEN	1000000U
#define IFX_FLL_INT_COEF	327680000ULL
#define IFX_FLL_KI_SCALE	850ULL
#define IFX_FLL_GAIN_IDX	11U
#define IFX_FLL_GAIN_VAL	((uint32_t)(8ULL * IFX_FLL_INT_COEF))
#define IFX_FLL_TRIM_SCALE	100000000ULL
#define IFX_FLL_MARGIN_SCALE	100000ULL
#define IFX_FLL_SETTLE_SCALE	6000ULL
#define IFX_FLL_CCO_RANGE4_HZ	150339200U
#define IFX_FLL_CCO_RANGE3_HZ	113009380U
#define IFX_FLL_CCO_RANGE2_HZ	84948700U
#define IFX_FLL_CCO_RANGE1_HZ	63855600U
#define IFX_FLL_TRIM_STEP_0	110340U
#define IFX_FLL_TRIM_STEP_1	110200U
#define IFX_FLL_TRIM_STEP_2	110000U
#define IFX_FLL_TRIM_STEP_3	110000U
#define IFX_FLL_TRIM_STEP_4	117062U
#define IFX_FLL_MARGIN_0	436U
#define IFX_FLL_MARGIN_1	581U
#define IFX_FLL_MARGIN_2	772U
#define IFX_FLL_MARGIN_3	1030U
#define IFX_FLL_MARGIN_4	1320U

#define IFX_MUX_IHO		0x6U

#define IFX_HAS_PLL200							\
	IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SRSS)
#define IFX_HAS_PLL400							\
	IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SRSS)
#define IFX_HAS_DPLL_LP							\
	(IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SSRSS) ||	\
	 IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS))
#define IFX_HAS_DPLL_HP							\
	IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS)
#define IFX_DPLL_LP_DCO_MULT_HZ	MHZ(310)

#define IFX_DPLL_HP_IVR_TRIM	0x8U
#define IFX_DPLL_HP_LC_ALPHA	0xCU
#define IFX_DPLL_HP_LC_BETA	0x5U
#define IFX_DPLL_HP_FLOCK_NDIV	20U
#define IFX_DPLL_HP_FLOCK_LOW	0x3U
#define IFX_DPLL_HP_FLOCK_HIGH	0x7U
#define IFX_DPLL_HP_FLOCK_WAIT	0x6U
#define IFX_DPLL_HP_LK_THRESH	0x7U
#define IFX_DPLL_HP_LK_WAIT	0x4U
#define IFX_DPLL_HP_LF_ALPHA	0x14U
#define IFX_DPLL_HP_PFD_HZ	MHZ(25)
#define IFX_DPLL_HP_LF_BETA_LOW	0x10U
#define IFX_DPLL_HP_LF_BETA_HIGH 0x14U

#define IFX_DPLL_HP_CONFIG4						\
	(FIELD_PREP(DPLL_HP_LC_ALPHA, IFX_DPLL_HP_LC_ALPHA) |		\
	 FIELD_PREP(DPLL_HP_LC_BETA, IFX_DPLL_HP_LC_BETA) |		\
	 FIELD_PREP(DPLL_HP_FLOCK_WAITPER, IFX_DPLL_HP_FLOCK_WAIT) |	\
	 FIELD_PREP(DPLL_HP_FLOCK_LK_THRESH, IFX_DPLL_HP_LK_THRESH) |	\
	 FIELD_PREP(DPLL_HP_FLOCK_LK_WAITPER, IFX_DPLL_HP_LK_WAIT))

#define IFX_DPLL_HP_CONFIG5						\
	(FIELD_PREP(DPLL_HP_LF_ALPHA, IFX_DPLL_HP_LF_ALPHA) |		\
	 DPLL_HP_LF_SET_PARAMS)

#define IFX_DPLL_LP_GAINS(ki, kp, ki_acc, kp_acc)			\
	(FIELD_PREP(DPLL_LP_KI, ki) | FIELD_PREP(DPLL_LP_KP, kp) |	\
	 FIELD_PREP(DPLL_LP_KI_ACC, ki_acc) |				\
	 FIELD_PREP(DPLL_LP_KP_ACC, kp_acc))

#define IFX_MUX_DIRECT		0x0U
#define IFX_MUX_ECO		0x2U
#define IFX_MUX_WCO		0x111U
#define IFX_MUX_PILO		0x113U

#define IFX_HAS_PILO							\
	IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS)

#if defined(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SRSS)
#define IFX_ROOT_DIV_MAX	8U
#else
#define IFX_ROOT_DIV_MAX	16U
#endif /* CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SRSS */

#if defined(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS)
#define SRSS_DPLL_LP_BASE	0x1600U
#define DPLL_LP_KI		GENMASK(5, 0)
#define DPLL_LP_KP		GENMASK(12, 7)
#define DPLL_LP_KI_ACC		GENMASK(19, 14)
#define DPLL_LP_KP_ACC		GENMASK(26, 21)
#define IFX_DPLL_LP_CONFIG4	FIELD_PREP(DPLL_LP_DCO_CODE, 0xFU)
#define IFX_DPLL_LP_KI_INT	0x28U
#define IFX_DPLL_LP_KP_INT	0x20U
#define IFX_DPLL_LP_KI_ACC_INT	0x27U
#define IFX_DPLL_LP_KP_ACC_INT	0x23U
#define IFX_DPLL_LP_KI_FRAC	0x2CU
#define IFX_DPLL_LP_KP_FRAC	0x24U
#define IFX_DPLL_LP_KI_ACC_FRAC	0x27U
#define IFX_DPLL_LP_KP_ACC_FRAC	0x1CU
#define IFX_DPLL_LP_KI_SSCG	0x1CU
#define IFX_DPLL_LP_KP_SSCG	0x1CU
#define IFX_DPLL_LP_KI_ACC_SSCG	0x27U
#define IFX_DPLL_LP_KP_ACC_SSCG	0x23U
#else
#define SRSS_DPLL_LP_BASE	0x1A00U
#define DPLL_LP_KI		GENMASK(6, 0)
#define DPLL_LP_KP		GENMASK(14, 8)
#define DPLL_LP_KI_ACC		GENMASK(22, 16)
#define DPLL_LP_KP_ACC		GENMASK(30, 24)
#define IFX_DPLL_LP_CONFIG4						\
	(FIELD_PREP(DPLL_LP_ACC_MODE, 1U) | FIELD_PREP(DPLL_LP_TDC_MODE, 1U))
#define IFX_DPLL_LP_KI_INT	0x24U
#define IFX_DPLL_LP_KP_INT	0x1CU
#define IFX_DPLL_LP_KI_ACC_INT	0x23U
#define IFX_DPLL_LP_KP_ACC_INT	0x1AU
#define IFX_DPLL_LP_KI_FRAC	0x24U
#define IFX_DPLL_LP_KP_FRAC	0x20U
#define IFX_DPLL_LP_KI_ACC_FRAC	0x23U
#define IFX_DPLL_LP_KP_ACC_FRAC	0x1AU
#define IFX_DPLL_LP_KI_SSCG	0x18U
#define IFX_DPLL_LP_KP_SSCG	0x18U
#define IFX_DPLL_LP_KI_ACC_SSCG	0x16U
#define IFX_DPLL_LP_KP_ACC_SSCG	0x14U
#endif /* CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS */

#define IFX_ROOT_DIV_LEGACY						\
	IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SRSS)

#define IFX_ECO_FTRIM		3U
#define IFX_ECO_AMP_SCALE	159155ULL
#define IFX_ECO_DRIVE_SCALE	2000000U
#define IFX_ECO_AMP_MIN_MV	500U
#define IFX_ECO_AMP_AGC_MV	1100U
#define IFX_ECO_ATRIM_BASE	4U
#define IFX_ECO_ATRIM_STEP_MV	50U
#define IFX_ECO_WDTRIM_BASE	2U
#define IFX_ECO_WDTRIM_STEP_MV	100U
#define IFX_ECO_WDTRIM_MAX	FIELD_GET(ECO_WDTRIM, ECO_WDTRIM)
#define IFX_ECO_GM_SCALE	157914ULL
#define IFX_ECO_GM_MARGIN	5U
#define IFX_ECO_GM_FREQ_DIV	1000U
#define IFX_ECO_GM_DIV		1000000000000ULL
#define IFX_ECO_GM_MAX		15400U
#define IFX_ECO_GTRIM_BASE	1U
#define IFX_ECO_GTRIM_STEP	2200U
#define IFX_ECO_RTRIM_FREQ_0	KHZ(28600)
#define IFX_ECO_RTRIM_FREQ_1	KHZ(23330)
#define IFX_ECO_RTRIM_FREQ_2	KHZ(16500)
#define IFX_ECO_RTRIM_0		0
#define IFX_ECO_RTRIM_1		1U
#define IFX_ECO_RTRIM_2		2U
#define IFX_ECO_RTRIM_3		3U

enum ifx_pll_type {
	IFX_PLL_FLL,
	IFX_PLL_200M,
	IFX_PLL_400M,
	IFX_PLL_DPLL_LP,
	IFX_PLL_DPLL_HP,
};

struct ifx_pll_div {
	uint32_t fraction;
	uint16_t feedback;
	uint8_t reference;
	uint8_t output;
};

struct ifx_fll_cfg {
	uint32_t config;
	uint32_t config2;
	uint32_t config3;
	uint32_t config4;
};

struct ifx_osc {
	uint16_t mux;
	uint32_t freq;
};

struct ifx_pll {
	uint8_t path;
	uint8_t type;
	uint16_t src;
	uint32_t freq;
	struct ifx_pll_div div;
};

struct ifx_root {
	uint8_t root;
	bool direct;
	uint16_t parent;
	uint32_t div;
};

#define IFX_PLL_TYPE(n)		DT_ENUM_IDX(n, infineon_pll_type)

#define IFX_OSC_ENTRY(n)						\
	{								\
		.mux = DT_REG_ADDR(n),					\
		.freq = DT_PROP_OR(n, clock_frequency, 0),		\
	},

#define IFX_PLL_ENTRY(n)						\
	{								\
		.path = DT_REG_ADDR(n),					\
		.type = IFX_PLL_TYPE(n),				\
		.src = DT_REG_ADDR(DT_CLOCKS_CTLR(n)),			\
		.freq = DT_PROP_OR(n, clock_frequency, 0),		\
		.div = {						\
			.fraction = DT_PROP_OR(n, fraction_div, 0),	\
			.feedback = DT_PROP_OR(n, feedback_div, 0),	\
			.reference = DT_PROP_OR(n, reference_div, 0),	\
			.output = DT_PROP_OR(n, output_div, 0),		\
		},							\
	},

#define IFX_ROOT_ENTRY(n)						\
	{								\
		.root = DT_REG_ADDR(n),					\
		.direct = DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR(n),		\
					     infineon_clk_osc),		\
		.parent = DT_REG_ADDR(DT_CLOCKS_CTLR(n)),		\
		.div = DT_PROP_OR(n, clock_div, 1),			\
	},

static const struct ifx_osc ifx_oscs[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(IFX_OSC_NODE, IFX_OSC_ENTRY)
};

static const struct ifx_pll ifx_plls[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(IFX_PLL_NODE, IFX_PLL_ENTRY)
};

static const struct ifx_root ifx_roots[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(IFX_ROOT_NODE, IFX_ROOT_ENTRY)
};

static inline void srss_update(uint32_t off, uint32_t mask, uint32_t val)
{
	mem_addr_t reg = IFX_SRSS_BASE + off;

	sys_write32((sys_read32(reg) & ~mask) | (val & mask), reg);
}

static uint32_t ifx_osc_freq(uint16_t mux)
{
	for (size_t i = 0; i < ARRAY_SIZE(ifx_oscs); i++) {
		if (ifx_oscs[i].mux == mux) {
			return ifx_oscs[i].freq;
		}
	}

	return 0;
}

static const struct ifx_pll *ifx_pll_find(uint8_t path)
{
	for (size_t i = 0; i < ARRAY_SIZE(ifx_plls); i++) {
		if (ifx_plls[i].path == path) {
			return &ifx_plls[i];
		}
	}

	return NULL;
}

#define IFX_PLL_TYPE_ENTRY(n)	[DT_REG_ADDR(n)] = IFX_PLL_TYPE(n),

static uint8_t ifx_pll_inst(uint8_t type, uint8_t path)
{
	const uint8_t types[] = {
		DT_FOREACH_CHILD(IFX_PLL_NODE, IFX_PLL_TYPE_ENTRY)
	};
	uint8_t inst = 0;

	for (uint8_t i = 0; i < path; i++) {
		if (types[i] == type) {
			inst++;
		}
	}

	return inst;
}

static bool ifx_eco_ready(void)
{
	uint32_t sts = sys_read32(IFX_SRSS_BASE + SRSS_ECO_STATUS);

	return (sts & (ECO_OK | ECO_READY)) == (ECO_OK | ECO_READY);
}

static bool ifx_wco_ok(void)
{
	if (!IFX_HAS_BACKUP) {
		return false;
	}

	return (sys_read32(IFX_BACKUP_BASE + BACKUP_WCO_STATUS) & WCO_OK) != 0;
}

static bool ifx_pilo_enabled(void)
{
	uint32_t cfg = sys_read32(IFX_SRSS_BASE + SRSS_PILO_CONFIG);

	return (cfg & PILO_ENABLE) != 0;
}

static bool ifx_osc_running(uint16_t mux)
{
	switch (mux) {
	case IFX_MUX_ECO:
		return IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS) ||
		       ifx_eco_ready();
	case IFX_MUX_PILO:
		return IFX_HAS_PILO && ifx_pilo_enabled();
	case IFX_MUX_WCO:
		return ifx_wco_ok();
	default:
		return true;
	}
}

static uint32_t ifx_osc_rate(uint16_t mux)
{
	return ifx_osc_running(mux) ? ifx_osc_freq(mux) : 0;
}

static uint16_t ifx_path_src(uint8_t path)
{
	uint32_t sel = sys_read32(IFX_SRSS_BASE + SRSS_PATH_SELECT(path));

	return FIELD_GET(PATH_MUX, sel);
}

static inline uint8_t ifx_pll_frac_bits(uint8_t type)
{
	switch (type) {
	case IFX_PLL_400M:
		return IFX_PLL400_FRAC_BITS;
	case IFX_PLL_DPLL_LP:
		return IFX_DPLL_LP_FRAC_BITS;
	case IFX_PLL_DPLL_HP:
		return IFX_DPLL_HP_FRAC_BITS;
	default:
		return 0;
	}
}

static uint32_t ifx_pll_calc(uint32_t in, const struct ifx_pll_div *d,
			     uint8_t frac_bits)
{
	uint64_t num, den;

	den = (uint64_t)d->reference * d->output;
	if (den == 0) {
		return 0;
	}

	num = (uint64_t)in * (((uint64_t)d->feedback << frac_bits) +
			      d->fraction);

	return (uint32_t)((num / den) >> frac_bits);
}

static int ifx_pll_regs(uint8_t type, uint8_t inst, uint32_t *cfg,
			uint32_t *sts)
{
	switch (type) {
	case IFX_PLL_200M:
		if (!IFX_HAS_PLL200) {
			return -ENOTSUP;
		}
		*cfg = SRSS_PLL200_CONFIG(inst);
		*sts = SRSS_PLL200_STATUS(inst);
		return 0;
	case IFX_PLL_400M:
		if (!IFX_HAS_PLL400) {
			return -ENOTSUP;
		}
		*cfg = SRSS_PLL400_CONFIG(inst);
		*sts = SRSS_PLL400_STATUS(inst);
		return 0;
	case IFX_PLL_DPLL_LP:
		if (!IFX_HAS_DPLL_LP) {
			return -ENOTSUP;
		}
		*cfg = SRSS_DPLL_LP_CONFIG(inst);
		*sts = SRSS_DPLL_LP_STATUS(inst);
		return 0;
	case IFX_PLL_DPLL_HP:
		if (!IFX_HAS_DPLL_HP) {
			return -ENOTSUP;
		}
		*cfg = SRSS_DPLL_HP_CONFIG(inst);
		*sts = SRSS_DPLL_HP_STATUS(inst);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void ifx_pll_read(uint8_t type, uint32_t cfg, struct ifx_pll_div *d)
{
	uint32_t val = sys_read32(IFX_SRSS_BASE + cfg);
	uint32_t val2;

	*d = (struct ifx_pll_div){0};

	switch (type) {
	case IFX_PLL_200M:
		if (!IFX_HAS_PLL200) {
			return;
		}
		d->feedback = FIELD_GET(PLL200_FEEDBACK, val);
		break;
	case IFX_PLL_400M:
	case IFX_PLL_DPLL_LP:
		if (!IFX_HAS_PLL400 && !IFX_HAS_DPLL_LP) {
			return;
		}
		val2 = sys_read32(IFX_SRSS_BASE + cfg + SRSS_PLL_CONFIG2_OFF);
		d->feedback = FIELD_GET(PLL400_FEEDBACK, val);
		if ((val2 & PLL_FRAC_EN) != 0) {
			d->fraction = FIELD_GET(PLL_FRAC_DIV, val2);
		}
		break;
	case IFX_PLL_DPLL_HP:
		if (!IFX_HAS_DPLL_HP) {
			return;
		}
		val2 = sys_read32(IFX_SRSS_BASE + cfg + SRSS_PLL_CONFIG2_OFF);
		d->feedback = FIELD_GET(DPLL_HP_NDIV, val) + 1U;
		d->reference = FIELD_GET(DPLL_HP_PDIV, val) + 1U;
		d->output = FIELD_GET(DPLL_HP_KDIV, val) + 1U;
		d->fraction = FIELD_GET(DPLL_HP_FRAC_DIV, val2);
		return;
	default:
		return;
	}

	d->reference = FIELD_GET(PLL_REFERENCE, val);
	d->output = FIELD_GET(PLL_OUTPUT, val);
}

static bool ifx_pll_running(uint32_t cfg, uint32_t sts)
{
	return (sys_read32(IFX_SRSS_BASE + cfg) & PLL_ENABLE) != 0 &&
	       (sys_read32(IFX_SRSS_BASE + sts) & PLL_LOCKED) != 0;
}

static uint32_t ifx_fll_rate(uint32_t in)
{
	uint32_t cfg = sys_read32(IFX_SRSS_BASE + SRSS_FLL_CONFIG);
	uint32_t cfg2 = sys_read32(IFX_SRSS_BASE + SRSS_FLL_CONFIG2);
	uint32_t cfg3 = sys_read32(IFX_SRSS_BASE + SRSS_FLL_CONFIG3);
	uint32_t ref = FIELD_GET(FLL_REF_DIV, cfg2);
	uint32_t bypass = FIELD_GET(PLL_BYPASS, cfg3);

	if (bypass == PLL_BYPASS_INPUT) {
		return in;
	}

	if ((cfg & FLL_ENABLE) == 0 ||
	    (sys_read32(IFX_SRSS_BASE + SRSS_FLL_STATUS) & FLL_LOCKED) == 0 ||
	    ref == 0) {
		return (bypass == PLL_BYPASS_OUTPUT) ? 0 : in;
	}

	uint64_t rate = (uint64_t)in * FIELD_GET(FLL_MULT, cfg) / ref;

	if ((cfg & FLL_OUTPUT_DIV) != 0) {
		rate /= 2U;
	}

	return (uint32_t)rate;
}

static uint32_t ifx_pll_rate(uint8_t type, uint8_t inst, uint32_t in)
{
	struct ifx_pll_div d;
	uint32_t cfg, sts, bypass;

	if (type == IFX_PLL_FLL) {
		if (!IFX_HAS_FLL) {
			return 0;
		}
		return ifx_fll_rate(in);
	}

	if (ifx_pll_regs(type, inst, &cfg, &sts) != 0) {
		return 0;
	}

	bypass = FIELD_GET(PLL_BYPASS, sys_read32(IFX_SRSS_BASE + cfg));
	if (bypass == PLL_BYPASS_INPUT) {
		return in;
	}

	if (!ifx_pll_running(cfg, sts)) {
		return (bypass == PLL_BYPASS_OUTPUT) ? 0 : in;
	}

	ifx_pll_read(type, cfg, &d);

	return ifx_pll_calc(in, &d, ifx_pll_frac_bits(type));
}

static uint32_t ifx_path_rate(uint8_t path)
{
	const struct ifx_pll *p = ifx_pll_find(path);

	if (p == NULL) {
		return 0;
	}

	return ifx_pll_rate(p->type, ifx_pll_inst(p->type, path),
			    ifx_osc_rate(ifx_path_src(path)));
}

static uint32_t ifx_root_div_get(uint32_t reg)
{
	if (IFX_ROOT_DIV_LEGACY) {
		return BIT(FIELD_GET(ROOT_DIV_POW2, reg));
	}

	return FIELD_GET(ROOT_DIV_INT, reg) + 1U;
}

static uint32_t ifx_root_direct_reg(uint8_t root)
{
	return IFX_ROOT_DIV_LEGACY ? SRSS_ROOT_SELECT(root) :
	       SRSS_DIRECT_SELECT(root);
}

static bool ifx_root_is_direct(uint8_t root)
{
	return (sys_read32(IFX_SRSS_BASE + ifx_root_direct_reg(root)) &
		ROOT_DIRECT_PATH) == 0;
}

static uint32_t ifx_root_rate(uint8_t root)
{
	uint32_t reg = sys_read32(IFX_SRSS_BASE + SRSS_ROOT_SELECT(root));
	uint32_t rate;

	if ((reg & ROOT_ENABLE) == 0) {
		return 0;
	}

	if (ifx_root_is_direct(root)) {
		rate = ifx_osc_freq(IFX_MUX_DIRECT);
	} else {
		rate = ifx_path_rate(FIELD_GET(ROOT_MUX, reg));
	}

	return rate / ifx_root_div_get(reg);
}

static int ifx_srss_get_rate(const struct device *dev,
			     clock_control_subsys_t sys, uint32_t *rate)
{
	const struct ifx_clk *clk = sys;

	ARG_UNUSED(dev);

	if (clk == NULL || rate == NULL) {
		return -EINVAL;
	}

	switch (clk->clk) {
	case IFX_CLK_HF:
		*rate = ifx_root_rate(clk->clk_id);
		break;
	case IFX_CLK_PLL:
		*rate = ifx_path_rate(clk->clk_id);
		break;
	case IFX_CLK_OSC:
		*rate = ifx_osc_rate(clk->clk_id);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if DT_NODE_HAS_STATUS_OKAY(IFX_ECO_NODE) &&				\
	!defined(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS22SRSS)

#define IFX_ECO_FREQ		DT_PROP(IFX_ECO_NODE, clock_frequency)
#define IFX_ECO_FREQ_KHZ	DIV_ROUND_CLOSEST(IFX_ECO_FREQ, KHZ(1))
#define IFX_ECO_ESR		DT_PROP_OR(IFX_ECO_NODE, eco_esr_ohms, 0)
#define IFX_ECO_CAP							\
	DT_PROP_OR(IFX_ECO_NODE, eco_load_capacitance_pf, 0)
#define IFX_ECO_DRIVE							\
	DT_PROP_OR(IFX_ECO_NODE, eco_drive_level_uw, 0)

#define IFX_ECO_GM							\
	((uint32_t)(IFX_ECO_GM_SCALE * IFX_ECO_GM_MARGIN *		\
		    IFX_ECO_ESR * IFX_ECO_CAP * IFX_ECO_CAP *		\
		    ((uint64_t)IFX_ECO_FREQ_KHZ * IFX_ECO_FREQ_KHZ /	\
		     IFX_ECO_GM_FREQ_DIV) / IFX_ECO_GM_DIV))

#define IFX_ECO_GTRIM							\
	(IFX_ECO_GM / IFX_ECO_GTRIM_STEP + IFX_ECO_GTRIM_BASE)

#define IFX_ECO_RTRIM							\
	((IFX_ECO_FREQ > IFX_ECO_RTRIM_FREQ_0) ? IFX_ECO_RTRIM_0 :	\
	 (IFX_ECO_FREQ > IFX_ECO_RTRIM_FREQ_1) ? IFX_ECO_RTRIM_1 :	\
	 (IFX_ECO_FREQ > IFX_ECO_RTRIM_FREQ_2) ? IFX_ECO_RTRIM_2 :	\
	 IFX_ECO_RTRIM_3)

static uint32_t ifx_isqrt(uint32_t x)
{
	uint32_t res = 0;

	for (uint32_t bit = BIT(15); bit != 0; bit >>= 1) {
		uint32_t tmp = res | bit;

		if (x >= tmp * tmp) {
			res = tmp;
		}
	}

	return res;
}

static int ifx_eco_trims(uint32_t *cfg2, bool *agc)
{
	uint32_t esr = IFX_ECO_ESR;
	uint32_t cap = IFX_ECO_CAP;
	uint32_t freq_khz = IFX_ECO_FREQ_KHZ;
	uint32_t peak_ua, amp_mv, atrim, wdtrim;

	if (esr == 0 || cap == 0 || freq_khz == 0 ||
	    IFX_ECO_GM >= IFX_ECO_GM_MAX) {
		return -EINVAL;
	}

	peak_ua = ifx_isqrt(DIV_ROUND_CLOSEST(IFX_ECO_DRIVE_SCALE *
					      IFX_ECO_DRIVE, esr));
	amp_mv = (uint32_t)DIV_ROUND_CLOSEST(IFX_ECO_AMP_SCALE * peak_ua,
					     (uint64_t)freq_khz * cap);
	if (amp_mv < IFX_ECO_AMP_MIN_MV) {
		return -EINVAL;
	}

	if (amp_mv >= IFX_ECO_AMP_AGC_MV) {
		atrim = 0;
		*agc = false;
	} else {
		atrim = IFX_ECO_ATRIM_BASE +
			(amp_mv - IFX_ECO_AMP_MIN_MV) / IFX_ECO_ATRIM_STEP_MV;
		*agc = true;
	}

	wdtrim = MIN(IFX_ECO_WDTRIM_BASE +
		     (amp_mv - IFX_ECO_AMP_MIN_MV) / IFX_ECO_WDTRIM_STEP_MV,
		     IFX_ECO_WDTRIM_MAX);

	*cfg2 = FIELD_PREP(ECO_WDTRIM, wdtrim) | FIELD_PREP(ECO_ATRIM, atrim) |
		FIELD_PREP(ECO_FTRIM, IFX_ECO_FTRIM) |
		FIELD_PREP(ECO_RTRIM, IFX_ECO_RTRIM) |
		FIELD_PREP(ECO_GTRIM, IFX_ECO_GTRIM);

	return 0;
}

static int ifx_eco_enable(void)
{
	uint32_t cfg2;
	bool agc;
	int ret;

	if ((sys_read32(IFX_SRSS_BASE + SRSS_ECO_CONFIG) & ECO_EN) == 0) {
		ret = ifx_eco_trims(&cfg2, &agc);
		if (ret != 0) {
			return ret;
		}

		sys_write32(cfg2, IFX_SRSS_BASE + SRSS_ECO_CONFIG2);
		sys_write32(agc ? ECO_AGC_EN : 0,
			    IFX_SRSS_BASE + SRSS_ECO_CONFIG);
		srss_update(SRSS_ECO_CONFIG, ECO_EN, ECO_EN);
	}

	while (!ifx_eco_ready()) {
	}

	return 0;
}
#elif DT_NODE_HAS_STATUS_OKAY(IFX_ECO_NODE)
static int ifx_eco_enable(void)
{
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(IFX_ECO_NODE) */

#if IFX_HAS_CPUSS
#define IFX_CPUSS_BASE		DT_INST_REG_ADDR_BY_NAME(0, cpuss)

#define IFX_CPUSS_DIV(lbl)						\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lbl)),		\
		    (DT_PROP(DT_NODELABEL(lbl), clock_div)), (0))

static void ifx_cpuss_set_div(uint32_t ctl, uint32_t div)
{
	mem_addr_t reg = IFX_CPUSS_BASE + ctl;
	uint32_t val;

	if (div == 0) {
		return;
	}

	val = sys_read32(reg);
	val &= ~CPUSS_INT_DIV;
	val |= FIELD_PREP(CPUSS_INT_DIV, div - 1U);
	sys_write32(val, reg);
}

static void ifx_cpuss_init(void)
{
	ifx_cpuss_set_div(CPUSS_MEM_CLOCK_CTL, IFX_CPUSS_DIV(clk_mem));
	ifx_cpuss_set_div(CPUSS_SLOW_CLOCK_CTL, IFX_CPUSS_DIV(clk_slow));
}
#endif /* IFX_HAS_CPUSS */

static int ifx_osc_enable(uint16_t mux)
{
	if (ifx_osc_running(mux)) {
		return 0;
	}

	switch (mux) {
	case IFX_MUX_PILO:
		srss_update(SRSS_PILO_CONFIG, PILO_ENABLE, PILO_ENABLE);
		while ((sys_read32(IFX_SRSS_BASE + SRSS_PILO_STATUS) &
			PILO_OK) == 0) {
		}
		return 0;
	case IFX_MUX_WCO:
		if (IFX_HAS_BACKUP) {
			sys_set_bits(IFX_BACKUP_BASE + BACKUP_CTL, WCO_EN);
		}
		return 0;
	default:
		return 0;
	}
}

static void ifx_path_set_src(uint8_t path, uint16_t mux)
{
	sys_write32(FIELD_PREP(PATH_MUX, mux),
		    IFX_SRSS_BASE + SRSS_PATH_SELECT(path));
}

static uint32_t ifx_dpll_hp_mode(uint32_t in)
{
	if (in <= MHZ(6)) {
		return 0;
	}

	if (in <= MHZ(10)) {
		return 1U;
	}

	if (in <= MHZ(16)) {
		return 2U;
	}

	if (in <= MHZ(25)) {
		return 3U;
	}

	if (in <= MHZ(32)) {
		return 4U;
	}

	if (in <= MHZ(40)) {
		return 5U;
	}

	if (in <= MHZ(45)) {
		return 6U;
	}

	return 7U;
}

static uint32_t ifx_pll_frac(const struct ifx_pll_div *d)
{
	if (d->fraction == 0) {
		return 0;
	}

	return FIELD_PREP(PLL_FRAC_DIV, d->fraction) | PLL_FRAC_EN;
}

static void ifx_pll_write(uint8_t type, uint8_t inst, uint32_t in,
			  const struct ifx_pll_div *d)
{
	uint32_t val = FIELD_PREP(PLL_REFERENCE, d->reference) |
		       FIELD_PREP(PLL_OUTPUT, d->output);

	switch (type) {
	case IFX_PLL_200M:
		if (!IFX_HAS_PLL200) {
			break;
		}
		val |= FIELD_PREP(PLL200_FEEDBACK, d->feedback);
		sys_write32(val, IFX_SRSS_BASE + SRSS_PLL200_CONFIG(inst));
		break;
	case IFX_PLL_400M:
		if (!IFX_HAS_PLL400) {
			break;
		}
		val |= FIELD_PREP(PLL400_FEEDBACK, d->feedback);
		if (d->fraction != 0) {
			val |= FIELD_PREP(PLL_LOCK_DELAY,
					  IFX_PLL_LOCK_DELAY_FRAC);
		}
		sys_write32(val, IFX_SRSS_BASE + SRSS_PLL400_CONFIG(inst));
		sys_write32(ifx_pll_frac(d),
			    IFX_SRSS_BASE + SRSS_PLL400_CONFIG2(inst));
		sys_write32(0, IFX_SRSS_BASE + SRSS_PLL400_CONFIG3(inst));
		break;
	case IFX_PLL_DPLL_LP:
		if (!IFX_HAS_DPLL_LP) {
			break;
		}
		val |= FIELD_PREP(PLL400_FEEDBACK, d->feedback);
		if ((in / d->reference) * d->feedback >=
		    IFX_DPLL_LP_DCO_MULT_HZ) {
			val |= DPLL_LP_DCO_MULT;
		}
		sys_write32(val, IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG(inst));
		sys_write32(ifx_pll_frac(d),
			    IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG2(inst));
		sys_write32(0, IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG3(inst));
		sys_write32(IFX_DPLL_LP_CONFIG4,
			    IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG4(inst));
		sys_write32(IFX_DPLL_LP_GAINS(IFX_DPLL_LP_KI_INT,
					      IFX_DPLL_LP_KP_INT,
					      IFX_DPLL_LP_KI_ACC_INT,
					      IFX_DPLL_LP_KP_ACC_INT),
			    IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG5(inst));
		sys_write32(IFX_DPLL_LP_GAINS(IFX_DPLL_LP_KI_FRAC,
					      IFX_DPLL_LP_KP_FRAC,
					      IFX_DPLL_LP_KI_ACC_FRAC,
					      IFX_DPLL_LP_KP_ACC_FRAC),
			    IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG6(inst));
		sys_write32(IFX_DPLL_LP_GAINS(IFX_DPLL_LP_KI_SSCG,
					      IFX_DPLL_LP_KP_SSCG,
					      IFX_DPLL_LP_KI_ACC_SSCG,
					      IFX_DPLL_LP_KP_ACC_SSCG),
			    IFX_SRSS_BASE + SRSS_DPLL_LP_CONFIG7(inst));
		break;
	case IFX_PLL_DPLL_HP:
		if (!IFX_HAS_DPLL_HP) {
			break;
		}
		sys_write32(FIELD_PREP(DPLL_HP_NDIV, d->feedback - 1U) |
			    FIELD_PREP(DPLL_HP_PDIV, d->reference - 1U) |
			    FIELD_PREP(DPLL_HP_KDIV, d->output - 1U),
			    IFX_SRSS_BASE + SRSS_DPLL_HP_CONFIG(inst));
		sys_write32(FIELD_PREP(DPLL_HP_FRAC_DIV, d->fraction) |
			    FIELD_PREP(DPLL_HP_FREQ_MODE,
				       ifx_dpll_hp_mode(in)) |
			    FIELD_PREP(DPLL_HP_IVR_TRIM, IFX_DPLL_HP_IVR_TRIM),
			    IFX_SRSS_BASE + SRSS_DPLL_HP_CONFIG2(inst));
		sys_write32(DPLL_HP_CLKR_SEL,
			    IFX_SRSS_BASE + SRSS_DPLL_HP_CONFIG3(inst));

		val = IFX_DPLL_HP_CONFIG4;
		if (d->feedback <= IFX_DPLL_HP_FLOCK_NDIV) {
			val |= FIELD_PREP(DPLL_HP_FLOCK_EN_THRESH,
					  IFX_DPLL_HP_FLOCK_LOW);
		} else {
			val |= FIELD_PREP(DPLL_HP_FLOCK_EN_THRESH,
					  IFX_DPLL_HP_FLOCK_HIGH);
		}
		sys_write32(val, IFX_SRSS_BASE + SRSS_DPLL_HP_CONFIG4(inst));

		val = IFX_DPLL_HP_CONFIG5;
		if (in / d->reference < IFX_DPLL_HP_PFD_HZ) {
			val |= FIELD_PREP(DPLL_HP_LF_BETA,
					  IFX_DPLL_HP_LF_BETA_LOW);
		} else {
			val |= FIELD_PREP(DPLL_HP_LF_BETA,
					  IFX_DPLL_HP_LF_BETA_HIGH);
		}
		sys_write32(val, IFX_SRSS_BASE + SRSS_DPLL_HP_CONFIG5(inst));

		sys_write32(0, IFX_SRSS_BASE + SRSS_DPLL_HP_TRIGMOD2(inst));
		sys_write32(DPLL_HP_DUTYCAL_EN,
			    IFX_SRSS_BASE + SRSS_DPLL_HP_DUTYCAL(inst));
		break;
	default:
		break;
	}
}

static void ifx_pll_disable(uint32_t cfg)
{
	srss_update(cfg, PLL_BYPASS, FIELD_PREP(PLL_BYPASS, PLL_BYPASS_INPUT));
	srss_update(cfg, PLL_ENABLE, 0);
}

static void ifx_pll_enable(uint32_t cfg, uint32_t sts)
{
	srss_update(cfg, PLL_BYPASS, FIELD_PREP(PLL_BYPASS, PLL_BYPASS_AUTO));
	srss_update(cfg, PLL_ENABLE, PLL_ENABLE);

	while ((sys_read32(IFX_SRSS_BASE + sts) & PLL_LOCKED) == 0) {
	}

	sys_write32(PLL_UNLOCK_OCCURRED, IFX_SRSS_BASE + sts);
}

static int ifx_pll_setup(const struct ifx_pll *p)
{
	uint8_t inst = ifx_pll_inst(p->type, p->path);
	uint32_t cfg, sts;
	int ret;

	ret = ifx_pll_regs(p->type, inst, &cfg, &sts);
	if (ret != 0) {
		return ret;
	}

	ifx_pll_disable(cfg);
	ifx_path_set_src(p->path, p->src);
	ifx_pll_write(p->type, inst, ifx_osc_freq(p->src), &p->div);
	ifx_pll_enable(cfg, sts);

	return 0;
}

static uint32_t ifx_fll_range(uint32_t cco, uint32_t *trim_step,
			      uint32_t *margin)
{
	if (cco >= IFX_FLL_CCO_RANGE4_HZ) {
		*trim_step = IFX_FLL_TRIM_STEP_4;
		*margin = IFX_FLL_MARGIN_4;
		return 4U;
	}
	if (cco >= IFX_FLL_CCO_RANGE3_HZ) {
		*trim_step = IFX_FLL_TRIM_STEP_3;
		*margin = IFX_FLL_MARGIN_3;
		return 3U;
	}
	if (cco >= IFX_FLL_CCO_RANGE2_HZ) {
		*trim_step = IFX_FLL_TRIM_STEP_2;
		*margin = IFX_FLL_MARGIN_2;
		return 2U;
	}
	if (cco >= IFX_FLL_CCO_RANGE1_HZ) {
		*trim_step = IFX_FLL_TRIM_STEP_1;
		*margin = IFX_FLL_MARGIN_1;
		return 1U;
	}

	*trim_step = IFX_FLL_TRIM_STEP_0;
	*margin = IFX_FLL_MARGIN_0;
	return 0;
}

static void ifx_fll_calc(uint32_t in, uint32_t out, struct ifx_fll_cfg *c)
{
	uint32_t cco = out * 2U;
	uint32_t ref = DIV_ROUND_UP((uint64_t)in * IFX_FLL_REF_SCALE, out);
	uint32_t mult = DIV_ROUND_UP((uint64_t)cco * ref, in);
	uint32_t lock_tol = DIV_ROUND_UP(mult * IFX_FLL_LOCK_TOL_NUM,
					 IFX_FLL_LOCK_TOL_DEN);
	uint32_t igain_val = IFX_FLL_GAIN_VAL;
	uint32_t pgain_val = IFX_FLL_GAIN_VAL;
	uint32_t range, trim_step, margin, gain, igain, pgain, cco_freq = 0;
	uint64_t cco_target, trim_ratio, ref_scaled;
	uint32_t settle_min, settle_ref, settle;

	range = ifx_fll_range(cco, &trim_step, &margin);

	gain = DIV_ROUND_CLOSEST(IFX_FLL_KI_SCALE * IFX_FLL_INT_COEF * in,
				 (uint64_t)trim_step * margin * ref);

	for (igain = IFX_FLL_GAIN_IDX;
	     igain > 0 && igain_val > gain; igain--) {
		igain_val >>= 1;
	}

	for (pgain = IFX_FLL_GAIN_IDX;
	     pgain > 0 && pgain_val > (gain - igain_val); pgain--) {
		pgain_val >>= 1;
	}

	cco_target = DIV_ROUND_CLOSEST((IFX_FLL_TRIM_SCALE /
					IFX_FLL_MARGIN_SCALE) * cco, margin);
	trim_ratio = IFX_FLL_TRIM_SCALE + trim_step;
	for (uint64_t trim = trim_ratio; trim < cco_target;
	     trim = trim * trim_ratio / IFX_FLL_TRIM_SCALE) {
		cco_freq++;
	}

	ref_scaled = DIV_ROUND_CLOSEST(IFX_FLL_SETTLE_SCALE * in, ref);
	settle_min = DIV_ROUND_UP(in, MHZ(1));
	settle_ref = DIV_ROUND_UP((uint64_t)settle_min * ref_scaled,
				  IFX_FLL_SETTLE_SCALE * MHZ(1)) + 1U;
	settle = (out < ref_scaled) ? settle_min : MAX(settle_min, settle_ref);

	c->config = FIELD_PREP(FLL_MULT, mult) | FLL_OUTPUT_DIV;
	c->config2 = FIELD_PREP(FLL_REF_DIV, ref) |
		     FIELD_PREP(FLL_LOCK_TOL, lock_tol);
	c->config3 = FIELD_PREP(FLL_IGAIN, igain) |
		     FIELD_PREP(FLL_PGAIN, pgain) |
		     FIELD_PREP(FLL_SETTLING_COUNT, settle) |
		     FIELD_PREP(PLL_BYPASS, PLL_BYPASS_INPUT);
	c->config4 = FIELD_PREP(FLL_CCO_RANGE, range) |
		     FIELD_PREP(FLL_CCO_FREQ, cco_freq);
}

static void ifx_fll_disable(void)
{
	srss_update(SRSS_FLL_CONFIG3, PLL_BYPASS,
		    FIELD_PREP(PLL_BYPASS, PLL_BYPASS_INPUT));
	while (FIELD_GET(PLL_BYPASS,
			 sys_read32(IFX_SRSS_BASE + SRSS_FLL_CONFIG3)) !=
	       PLL_BYPASS_INPUT) {
	}
	srss_update(SRSS_FLL_CONFIG, FLL_ENABLE, 0);
	srss_update(SRSS_FLL_CONFIG4, FLL_CCO_ENABLE, 0);
}

static int ifx_fll_setup(uint16_t src, uint32_t out)
{
	struct ifx_fll_cfg c;

	if (!IFX_HAS_FLL) {
		return -ENOTSUP;
	}

	ifx_fll_calc(ifx_osc_freq(src), out, &c);

	ifx_fll_disable();
	ifx_path_set_src(IFX_FLL_PATH, src);
	sys_write32(c.config, IFX_SRSS_BASE + SRSS_FLL_CONFIG);
	sys_write32(c.config2, IFX_SRSS_BASE + SRSS_FLL_CONFIG2);
	sys_write32(c.config3, IFX_SRSS_BASE + SRSS_FLL_CONFIG3);
	srss_update(SRSS_FLL_CONFIG4, FLL_CCO_RANGE | FLL_CCO_FREQ, c.config4);

	srss_update(SRSS_FLL_CONFIG4, FLL_CCO_ENABLE, FLL_CCO_ENABLE);
	while ((sys_read32(IFX_SRSS_BASE + SRSS_FLL_STATUS) &
		FLL_CCO_READY) == 0) {
	}

	srss_update(SRSS_FLL_CONFIG, FLL_ENABLE, FLL_ENABLE);
	while ((sys_read32(IFX_SRSS_BASE + SRSS_FLL_STATUS) &
		FLL_LOCKED) == 0) {
	}

	srss_update(SRSS_FLL_CONFIG3, PLL_BYPASS,
		    FIELD_PREP(PLL_BYPASS, PLL_BYPASS_OUTPUT));
	sys_write32(FLL_UNLOCK_OCCURRED, IFX_SRSS_BASE + SRSS_FLL_STATUS);

	return 0;
}

static int ifx_fll_init(void)
{
	const struct ifx_pll *p = ifx_pll_find(IFX_FLL_PATH);

	if (p == NULL || p->type != IFX_PLL_FLL) {
		return 0;
	}

	return ifx_fll_setup(p->src, p->freq);
}

static uint32_t ifx_root_div_mask(void)
{
	return IFX_ROOT_DIV_LEGACY ? ROOT_DIV_POW2 :
	       (ROOT_DIV_POW2 | ROOT_DIV_INT);
}

static uint32_t ifx_root_div_prep(uint32_t div)
{
	if (IFX_ROOT_DIV_LEGACY) {
		return FIELD_PREP(ROOT_DIV_POW2, LOG2(div));
	}

	return FIELD_PREP(ROOT_DIV_INT, div - 1U);
}

static void ifx_root_set_direct(uint8_t root, bool direct)
{
	srss_update(ifx_root_direct_reg(root), ROOT_DIRECT_PATH,
		    direct ? 0 : ROOT_DIRECT_PATH);
}

static void ifx_root_set_div(uint8_t root, uint32_t div)
{
	srss_update(SRSS_ROOT_SELECT(root), ifx_root_div_mask(),
		    ifx_root_div_prep(div));
}

static void ifx_root_setup(const struct ifx_root *r)
{
	ifx_root_set_direct(r->root, true);
	ifx_root_set_div(r->root, IFX_ROOT_DIV_MAX);

	if (!r->direct) {
		srss_update(SRSS_ROOT_SELECT(r->root), ROOT_MUX,
			    FIELD_PREP(ROOT_MUX, r->parent));
		ifx_root_set_direct(r->root, false);
	}

	for (uint32_t d = IFX_ROOT_DIV_MAX / 2U; d > r->div; d /= 2U) {
		ifx_root_set_div(r->root, d);
	}

	ifx_root_set_div(r->root, r->div);
	srss_update(SRSS_ROOT_SELECT(r->root), ROOT_ENABLE, ROOT_ENABLE);
}

static int ifx_srss_init(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_CONFIGURE)) {
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ifx_roots); i++) {
		ifx_root_set_direct(ifx_roots[i].root, true);
	}

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_IFX_SRSS_MXS40SSRSS) &&
	    ifx_osc_freq(IFX_MUX_IHO) != 0) {
		srss_update(SRSS_IHO_CONFIG, IHO_ENABLE, IHO_ENABLE);
	}

#if DT_NODE_HAS_STATUS_OKAY(IFX_ECO_NODE)
	ret = ifx_eco_enable();
	if (ret != 0) {
		return ret;
	}
#endif /* DT_NODE_HAS_STATUS_OKAY(IFX_ECO_NODE) */

	for (size_t i = 0; i < ARRAY_SIZE(ifx_oscs); i++) {
		ret = ifx_osc_enable(ifx_oscs[i].mux);
		if (ret != 0) {
			return ret;
		}
	}

	ret = ifx_fll_init();
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ifx_plls); i++) {
		if (ifx_plls[i].type == IFX_PLL_FLL) {
			continue;
		}

		ret = ifx_pll_setup(&ifx_plls[i]);
		if (ret != 0) {
			return ret;
		}
	}

#if IFX_HAS_CPUSS
	ifx_cpuss_init();
#endif /* IFX_HAS_CPUSS */

	for (size_t i = 0; i < ARRAY_SIZE(ifx_roots); i++) {
		ifx_root_setup(&ifx_roots[i]);
	}

	return 0;
}

static DEVICE_API(clock_control, ifx_srss_api) = {
	.get_rate = ifx_srss_get_rate,
};

DEVICE_DT_INST_DEFINE(0, ifx_srss_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ifx_srss_api);
