/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 CRU clock controller.
 */

#define DT_DRV_COMPAT rockchip_rk3588_cru_clk

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rk3588-cru.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_rk3588_cru, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RK3588_CRU_PLL_CON(n)         ((n) * 4U)
#define RK3588_CRU_MODE_CON0          0x280U
#define RK3588_CRU_CLKSEL_CON(n)      (((n) * 4U) + 0x300U)
#define RK3588_CRU_CLKGATE_CON(n)     (((n) * 4U) + 0x800U)
#define RK3588_PMU1CRU_CLKGATE_CON(n) (((n) * 4U) + 0x30800U)

/*
 * CLKSEL_CON register index of a PMU1CRU node, for the row macros below: the
 * PMU1CRU is 0x30000 above the CRU, whose CLKSEL_CON registers are 32-bit.
 */
#define RK3588_PMU1CRU_CLKSEL_CON(n) ((n) + 0xC000U)

/*
 * Every clock the driver knows is one row of rk3588_cru_nodes. A row tells how
 * its rate is computed (type) and which parts or policies apply (flags);
 * the fields of an absent part are zero and never read. A row corresponds to
 * one entry of the clock table of Linux's clk-rk3588.c: a COMPOSITE there is a
 * row with mux, divider and gate, a GATE only carries the gate fields and a
 * COMPOSITE_NODIV has no divider.
 */
enum rk3588_cru_node_type {
	RK3588_NODE_FIXED,    /* constant rate, fixed_hz */
	RK3588_NODE_EXTERNAL, /* external input, optional DT rate in fixed_hz */
	RK3588_NODE_PLL,      /* rate read from the PLL registers, rk3588_cru_plls */
	RK3588_NODE_CLKSEL,   /* rate from the parent selected in a CLKSEL_CON register */
};

enum rk3588_cru_flag {
	RK3588_CLK_MUX = BIT(0),      /* parent selector in a CLKSEL_CON register */
	RK3588_CLK_DIV = BIT(1),      /* programmable divider in a CLKSEL_CON register */
	RK3588_CLK_GATE = BIT(2),     /* gate in a CLKGATE_CON register */
	RK3588_CLK_CRITICAL = BIT(3), /* shared root clock that must stay enabled */
};

#define RK3588_CRU_MAX_PARENTS 5U
#define RK3588_CRU_MAX_DEPTH   4U

/* Internal oscillator and external input nodes; not identifiers of the DT binding. */
#define RK3588_CRU_CLK_XIN24M          0xFFFFU
#define RK3588_CRU_CLK_XIN32K          0xFFFEU
#define RK3588_CRU_CLK_GMAC0_PTPREF_IO 0xFFFDU
#define RK3588_CRU_CLK_GMAC1_PTPREF_IO 0xFFFCU

#define RK3588_DT_EXT_CLK_HZ(name)                                                                 \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(0, name),                                              \
		    (COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, name)),    \
				 (DT_PROP_OR(DT_INST_CLOCKS_CTLR_BY_NAME(0, name),                 \
					     clock_frequency, 0U)),                                \
				 (0U))),                                                           \
		    (0U))

struct rk3588_cru_node {
	uint16_t id;
	uint32_t gate_off; /* CLKGATE_CON offset, set with RK3588_CLK_GATE */
	uint32_t fixed_hz; /* set with RK3588_NODE_FIXED or optional RK3588_NODE_EXTERNAL */
	uint8_t type;
	uint8_t flags;
	uint16_t con; /* CLKSEL_CON index of the mux and divider */
	uint8_t sel_shift;
	uint8_t sel_width;
	uint8_t div_shift;
	uint8_t div_width;
	uint8_t gate_shift;
	uint8_t num_parents; /* parents selectable by the mux, at most one without */
	uint16_t parents[RK3588_CRU_MAX_PARENTS];
};

/*
 * Initializers of the node rows, one per combination of parts a clock offers.
 * The trailing arguments of the macros that take a parent list name the parents
 * in the order the selector encodes them. The parameters are named unlike the
 * fields they initialize, which the preprocessor would substitute as well.
 */
#define RK3588_FIXED(_id, _rate)                                                                   \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_FIXED,                                                         \
		.fixed_hz = (_rate),                                                               \
	}

#define RK3588_EXTERNAL(_id, _rate)                                                                \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_EXTERNAL,                                                      \
		.fixed_hz = (_rate),                                                               \
	}

#define RK3588_PLL(_id)                                                                            \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_PLL,                                                           \
	}

#define RK3588_GATE(_id, _gate_off, _gate_shift)                                                   \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_CLKSEL,                                                        \
		.flags = RK3588_CLK_GATE,                                                          \
		.gate_off = (_gate_off),                                                           \
		.gate_shift = (_gate_shift),                                                       \
	}

/* A gate that passes the rate of its single parent through unchanged. */
#define RK3588_GATE_FROM(_id, _parent, _gate_off, _gate_shift)                                     \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_CLKSEL,                                                        \
		.flags = RK3588_CLK_GATE,                                                          \
		.num_parents = 1,                                                                  \
		.parents = {_parent},                                                              \
		.gate_off = (_gate_off),                                                           \
		.gate_shift = (_gate_shift),                                                       \
	}

#define RK3588_MUX_GATE(_id, _con, _sel_shift, _sel_width, _gate_off, _gate_shift, ...)            \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_CLKSEL,                                                        \
		.flags = RK3588_CLK_MUX | RK3588_CLK_GATE,                                         \
		.con = (_con),                                                                     \
		.sel_shift = (_sel_shift),                                                         \
		.sel_width = (_sel_width),                                                         \
		.num_parents = NUM_VA_ARGS(__VA_ARGS__),                                           \
		.parents = {__VA_ARGS__},                                                          \
		.gate_off = (_gate_off),                                                           \
		.gate_shift = (_gate_shift),                                                       \
	}

#define RK3588_MUX_DIV_GATE_FLAGS(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width,       \
				  _gate_off, _gate_shift, _extra_flags, ...)                       \
	{                                                                                          \
		.id = (_id),                                                                       \
		.type = RK3588_NODE_CLKSEL,                                                        \
		.flags = RK3588_CLK_MUX | RK3588_CLK_DIV | RK3588_CLK_GATE | (_extra_flags),       \
		.con = (_con),                                                                     \
		.sel_shift = (_sel_shift),                                                         \
		.sel_width = (_sel_width),                                                         \
		.div_shift = (_div_shift),                                                         \
		.div_width = (_div_width),                                                         \
		.num_parents = NUM_VA_ARGS(__VA_ARGS__),                                           \
		.parents = {__VA_ARGS__},                                                          \
		.gate_off = (_gate_off),                                                           \
		.gate_shift = (_gate_shift),                                                       \
	}

#define RK3588_MUX_DIV_GATE(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width, _gate_off,  \
			    _gate_shift, ...)                                                      \
	RK3588_MUX_DIV_GATE_FLAGS(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width,       \
				  _gate_off, _gate_shift, 0U, __VA_ARGS__)

#define RK3588_MUX_DIV_GATE_CRITICAL(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width,    \
				     _gate_off, _gate_shift, ...)                                  \
	RK3588_MUX_DIV_GATE_FLAGS(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width,       \
				  _gate_off, _gate_shift, RK3588_CLK_CRITICAL, __VA_ARGS__)

/*
 * A PLL runs at FIN * (m + k / 65536) / (p * 2^s), with m in PLL_CON0, p and s
 * in PLL_CON1 and the signed k in PLL_CON2. MODE_CON0 bypasses it to the
 * 24 MHz oscillator, or to the 32 kHz one in deep slow mode.
 */
#define RK3588_PLL_M_MASK      GENMASK(9, 0)
#define RK3588_PLL_P_MASK      GENMASK(5, 0)
#define RK3588_PLL_S_SHIFT     6U
#define RK3588_PLL_S_MASK      GENMASK(2, 0)
#define RK3588_PLL_K_MASK      GENMASK(15, 0)
#define RK3588_PLL_MODE_MASK   0x3U
#define RK3588_PLL_MODE_SLOW   0U
#define RK3588_PLL_MODE_NORMAL 1U
#define RK3588_XIN24M_HZ       MHZ(24)
#define RK3588_XIN32K_HZ       32768U

struct rk3588_cru_pll {
	uint16_t id;        /* identifier of the PLL node */
	uint16_t con;       /* index of its first PLL_CON register */
	uint8_t mode_shift; /* its mode bits in MODE_CON0 */
};

static const struct rk3588_cru_pll rk3588_cru_plls[] = {
	{.id = PLL_GPLL, .con = 112, .mode_shift = 2},
	{.id = PLL_CPLL, .con = 104, .mode_shift = 8},
	{.id = PLL_PPLL, .con = 0x2080, .mode_shift = 10}, /* PHP CRU, 0x8200 */
};

static const struct rk3588_cru_node rk3588_cru_nodes[] = {
	/*
	 * Rate sources. Linux registers the 150 MHz and 200 MHz taps as
	 * critical clocks: they feed many controllers and must stay enabled.
	 */
	RK3588_FIXED(RK3588_CRU_CLK_XIN24M, RK3588_XIN24M_HZ),
	RK3588_FIXED(RK3588_CRU_CLK_XIN32K, RK3588_XIN32K_HZ),
	RK3588_EXTERNAL(RK3588_CRU_CLK_GMAC0_PTPREF_IO, RK3588_DT_EXT_CLK_HZ(clk_gmac0_ptpref_io)),
	RK3588_EXTERNAL(RK3588_CRU_CLK_GMAC1_PTPREF_IO, RK3588_DT_EXT_CLK_HZ(clk_gmac1_ptpref_io)),
	RK3588_PLL(PLL_GPLL),
	RK3588_PLL(PLL_CPLL),
	RK3588_PLL(PLL_PPLL),
	RK3588_MUX_DIV_GATE_CRITICAL(CLK_150M_SRC, 1, 5, 1, 0, 5, RK3588_CRU_CLKGATE_CON(0), 2,
				     PLL_GPLL, PLL_CPLL),
	RK3588_MUX_DIV_GATE_CRITICAL(CLK_200M_SRC, 1, 11, 1, 6, 5, RK3588_CRU_CLKGATE_CON(0), 3,
				     PLL_GPLL, PLL_CPLL),

	/* GPIO bank peripheral clocks and 24 MHz or 32 kHz debounce clocks */
	RK3588_GATE(PCLK_GPIO1, RK3588_CRU_CLKGATE_CON(16), 14),
	RK3588_MUX_DIV_GATE(DBCLK_GPIO1, 60, 8, 1, 3, 5, RK3588_CRU_CLKGATE_CON(16), 15,
			    RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLK_XIN32K),
	RK3588_GATE(PCLK_GPIO2, RK3588_CRU_CLKGATE_CON(17), 0),
	RK3588_MUX_DIV_GATE(DBCLK_GPIO2, 60, 14, 1, 9, 5, RK3588_CRU_CLKGATE_CON(17), 1,
			    RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLK_XIN32K),
	RK3588_GATE(PCLK_GPIO3, RK3588_CRU_CLKGATE_CON(17), 2),
	RK3588_MUX_DIV_GATE(DBCLK_GPIO3, 61, 5, 1, 0, 5, RK3588_CRU_CLKGATE_CON(17), 3,
			    RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLK_XIN32K),
	RK3588_GATE(PCLK_GPIO4, RK3588_CRU_CLKGATE_CON(17), 4),
	RK3588_MUX_DIV_GATE(DBCLK_GPIO4, 61, 11, 1, 6, 5, RK3588_CRU_CLKGATE_CON(17), 5,
			    RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLK_XIN32K),
	RK3588_GATE(PCLK_GPIO0, RK3588_PMU1CRU_CLKGATE_CON(5), 5),
	RK3588_MUX_GATE(DBCLK_GPIO0, RK3588_PMU1CRU_CLKSEL_CON(17), 0, 1,
			RK3588_PMU1CRU_CLKGATE_CON(5), 6, RK3588_CRU_CLK_XIN24M,
			RK3588_CRU_CLK_XIN32K),

	/* SPI controllers: a mux over the fixed taps, without a divider */
	RK3588_GATE(PCLK_SPI0, RK3588_CRU_CLKGATE_CON(14), 6),
	RK3588_GATE(PCLK_SPI1, RK3588_CRU_CLKGATE_CON(14), 7),
	RK3588_GATE(PCLK_SPI2, RK3588_CRU_CLKGATE_CON(14), 8),
	RK3588_GATE(PCLK_SPI3, RK3588_CRU_CLKGATE_CON(14), 9),
	RK3588_GATE(PCLK_SPI4, RK3588_CRU_CLKGATE_CON(14), 10),
	RK3588_MUX_GATE(CLK_SPI0, 59, 2, 2, RK3588_CRU_CLKGATE_CON(14), 11, CLK_200M_SRC,
			CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI1, 59, 4, 2, RK3588_CRU_CLKGATE_CON(14), 12, CLK_200M_SRC,
			CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI2, 59, 6, 2, RK3588_CRU_CLKGATE_CON(14), 13, CLK_200M_SRC,
			CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI3, 59, 8, 2, RK3588_CRU_CLKGATE_CON(14), 14, CLK_200M_SRC,
			CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI4, 59, 10, 2, RK3588_CRU_CLKGATE_CON(14), 15, CLK_200M_SRC,
			CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),

	/* eMMC controller: HCLK and ACLK come from unmodeled bus roots */
	RK3588_GATE(HCLK_EMMC, RK3588_CRU_CLKGATE_CON(31), 4),
	RK3588_GATE(ACLK_EMMC, RK3588_CRU_CLKGATE_CON(31), 5),
	RK3588_MUX_DIV_GATE(CCLK_EMMC, 77, 14, 2, 8, 6, RK3588_CRU_CLKGATE_CON(31), 6, PLL_GPLL,
			    PLL_CPLL, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_DIV_GATE(BCLK_EMMC, 78, 5, 1, 0, 5, RK3588_CRU_CLKGATE_CON(31), 7, PLL_GPLL,
			    PLL_CPLL),
	RK3588_GATE_FROM(TMCLK_EMMC, RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLKGATE_CON(31), 8),

	/*
	 * Ethernet controller. Selector 1 of the PTP reference muxes is an
	 * external input whose frequency may be provided in devicetree, and the
	 * ACLK and PCLK of the controllers come from unmodeled bus roots.
	 */
	RK3588_MUX_DIV_GATE(MCLK_GMAC0_OUT, 15, 7, 1, 0, 7, RK3588_CRU_CLKGATE_CON(5), 3, PLL_GPLL,
			    PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC0_PTP_REF, 81, 6, 1, 0, 6, RK3588_CRU_CLKGATE_CON(34), 10,
			    PLL_CPLL, RK3588_CRU_CLK_GMAC0_PTPREF_IO),
	RK3588_MUX_DIV_GATE(CLK_GMAC1_PTP_REF, 81, 13, 1, 7, 6, RK3588_CRU_CLKGATE_CON(34), 11,
			    PLL_CPLL, RK3588_CRU_CLK_GMAC1_PTPREF_IO),
	RK3588_MUX_DIV_GATE(CLK_GMAC_125M, 83, 15, 1, 8, 7, RK3588_CRU_CLKGATE_CON(35), 5, PLL_GPLL,
			    PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC_50M, 84, 7, 1, 0, 7, RK3588_CRU_CLKGATE_CON(35), 6, PLL_GPLL,
			    PLL_CPLL),
	RK3588_GATE(PCLK_GMAC0, RK3588_CRU_CLKGATE_CON(32), 3),
	RK3588_GATE(PCLK_GMAC1, RK3588_CRU_CLKGATE_CON(32), 4),
	RK3588_GATE(ACLK_GMAC0, RK3588_CRU_CLKGATE_CON(32), 10),
	RK3588_GATE(ACLK_GMAC1, RK3588_CRU_CLKGATE_CON(32), 11),

	/*
	 * USB 3.0 OTG0 controller and its USB 2.0 PHY: the suspend and
	 * reference clocks run from the oscillator, the ACLK from an unmodeled
	 * root, and the PHY reference muxes the oscillator with PPLL.
	 */
	RK3588_GATE(ACLK_USB3OTG0, RK3588_CRU_CLKGATE_CON(42), 4),
	RK3588_GATE_FROM(SUSPEND_CLK_USB3OTG0, RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLKGATE_CON(42),
			 5),
	RK3588_GATE_FROM(REF_CLK_USB3OTG0, RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLKGATE_CON(42), 6),
	RK3588_MUX_DIV_GATE(CLK_USB2PHY_HDPTXRXPHY_REF, RK3588_PMU1CRU_CLKSEL_CON(14), 14, 1, 9, 5,
			    RK3588_PMU1CRU_CLKGATE_CON(4), 7, RK3588_CRU_CLK_XIN24M, PLL_PPLL),
};

struct clock_control_rk3588_config {
	DEVICE_MMIO_ROM;
};

struct clock_control_rk3588_data {
	DEVICE_MMIO_RAM;
};

static const struct rk3588_cru_node *rk3588_cru_node_lookup(uint16_t clk_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_nodes); i++) {
		if (rk3588_cru_nodes[i].id == clk_id) {
			return &rk3588_cru_nodes[i];
		}
	}

	return NULL;
}

static const struct rk3588_cru_node *rk3588_cru_subsys_lookup(clock_control_subsys_t sys)
{
	uintptr_t raw_id = (uintptr_t)sys;
	const struct rk3588_cru_node *node;

	if (raw_id > UINT16_MAX) {
		return NULL;
	}

	node = rk3588_cru_node_lookup((uint16_t)raw_id);
	if ((node == NULL) || (node->type == RK3588_NODE_FIXED) ||
	    (node->type == RK3588_NODE_EXTERNAL)) {
		return NULL;
	}

	return node;
}

static const struct rk3588_cru_pll *rk3588_cru_pll_lookup(uint16_t clk_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_plls); i++) {
		if (rk3588_cru_plls[i].id == clk_id) {
			return &rk3588_cru_plls[i];
		}
	}

	return NULL;
}

static void rk3588_cru_gate_write(mm_reg_t base, uint32_t gate_off, uint8_t gate_shift, bool enable)
{
	uint32_t val = BIT(gate_shift) << 16;

	if (!enable) {
		val |= BIT(gate_shift);
	}

	sys_write32(val, base + gate_off);
}

static bool rk3588_cru_gate_is_enabled(mm_reg_t base, const struct rk3588_cru_node *node)
{
	if ((node->flags & RK3588_CLK_GATE) == 0U) {
		return true;
	}

	return (sys_read32(base + node->gate_off) & BIT(node->gate_shift)) == 0U;
}

static uint32_t rk3588_cru_pll_get_rate(mm_reg_t base, const struct rk3588_cru_pll *pll)
{
	uint32_t mode =
		(sys_read32(base + RK3588_CRU_MODE_CON0) >> pll->mode_shift) & RK3588_PLL_MODE_MASK;
	uint32_t con1;
	uint32_t m;
	uint32_t p;
	uint32_t s;
	int16_t k;
	int64_t vco;

	if (mode == RK3588_PLL_MODE_SLOW) {
		return RK3588_XIN24M_HZ;
	}
	if (mode != RK3588_PLL_MODE_NORMAL) {
		return RK3588_XIN32K_HZ;
	}

	con1 = sys_read32(base + RK3588_CRU_PLL_CON(pll->con + 1U));
	p = con1 & RK3588_PLL_P_MASK;
	if (p == 0U) {
		return 0U;
	}

	m = sys_read32(base + RK3588_CRU_PLL_CON(pll->con)) & RK3588_PLL_M_MASK;
	s = (con1 >> RK3588_PLL_S_SHIFT) & RK3588_PLL_S_MASK;
	k = (int16_t)(sys_read32(base + RK3588_CRU_PLL_CON(pll->con + 2U)) & RK3588_PLL_K_MASK);

	vco = ((int64_t)RK3588_XIN24M_HZ * (((int64_t)m * 65536LL) + k)) / ((int64_t)p * 65536LL);
	if (vco <= 0) {
		return 0U;
	}

	return (uint32_t)((uint64_t)vco >> s);
}

/*
 * Resolve the currently selected parent of a CLKSEL node. Sets *parent to NULL
 * when the node has no modeled parent (a pure gate). Returns -EIO if the
 * hardware mux selector points outside the modeled parent list.
 */
static int rk3588_cru_node_get_parent(const struct rk3588_cru_node *node, uint32_t con,
				      const struct rk3588_cru_node **parent)
{
	uint32_t sel;

	if ((node->flags & RK3588_CLK_MUX) != 0U) {
		sel = (con >> node->sel_shift) & GENMASK(node->sel_width - 1U, 0);
		if (sel >= node->num_parents) {
			return -EIO;
		}
	} else if (node->num_parents == 1U) {
		sel = 0U;
	} else {
		*parent = NULL;
		return 0;
	}

	*parent = rk3588_cru_node_lookup(node->parents[sel]);
	if (*parent == NULL) {
		return -ENOTSUP;
	}

	return 0;
}

/*
 * Enable any gated parents along the currently selected parent chain before
 * enabling node itself. Returns an error if the selected mux input is invalid.
 */
static int rk3588_cru_node_enable(mm_reg_t base, const struct rk3588_cru_node *node)
{
	const struct rk3588_cru_node *chain[RK3588_CRU_MAX_DEPTH];
	uint8_t count = 0U;

	while (node->type == RK3588_NODE_CLKSEL) {
		const struct rk3588_cru_node *parent = NULL;
		uint32_t con = 0U;
		int ret;

		if (count >= RK3588_CRU_MAX_DEPTH) {
			return -EIO;
		}

		if ((node->flags & RK3588_CLK_MUX) != 0U) {
			con = sys_read32(base + RK3588_CRU_CLKSEL_CON(node->con));
		}

		ret = rk3588_cru_node_get_parent(node, con, &parent);
		if (ret != 0) {
			return ret;
		}

		chain[count] = node;
		count++;
		if (parent == NULL) {
			break;
		}
		node = parent;
	}

	while (count > 0U) {
		count--;
		if ((chain[count]->flags & RK3588_CLK_GATE) != 0U) {
			rk3588_cru_gate_write(base, chain[count]->gate_off,
					      chain[count]->gate_shift, true);
		}
	}

	return 0;
}

/*
 * Rate of node, following its parent chain up to RK3588_CRU_MAX_DEPTH nodes.
 * Without a mux a node passes the rate of its single parent, if it has one,
 * through unchanged.
 */
static int rk3588_cru_node_get_rate(mm_reg_t base, const struct rk3588_cru_node *node,
				    uint32_t *rate)
{
	uint32_t divs[RK3588_CRU_MAX_DEPTH];
	const struct rk3588_cru_pll *pll;
	uint8_t depth = 0U;
	uint32_t cur_rate = 0U;
	int ret = 0;

	while (node->type == RK3588_NODE_CLKSEL) {
		const struct rk3588_cru_node *parent = NULL;
		uint32_t con = 0U;
		uint32_t divisor = 1U;

		if (depth >= RK3588_CRU_MAX_DEPTH) {
			return -EIO;
		}

		if ((node->flags & (RK3588_CLK_MUX | RK3588_CLK_DIV)) != 0U) {
			con = sys_read32(base + RK3588_CRU_CLKSEL_CON(node->con));
		}

		ret = rk3588_cru_node_get_parent(node, con, &parent);
		if (ret != 0) {
			return ret;
		}
		if (parent == NULL) {
			return -ENOTSUP;
		}

		if ((node->flags & RK3588_CLK_DIV) != 0U) {
			uint32_t div_mask = GENMASK(node->div_width - 1U, 0);

			divisor = ((con >> node->div_shift) & div_mask) + 1U;
		}

		divs[depth] = divisor;
		depth++;
		node = parent;
	}

	switch (node->type) {
	case RK3588_NODE_FIXED:
		cur_rate = node->fixed_hz;
		break;
	case RK3588_NODE_EXTERNAL:
		if (node->fixed_hz == 0U) {
			ret = -ENOTSUP;
		} else {
			cur_rate = node->fixed_hz;
		}
		break;
	case RK3588_NODE_PLL:
		pll = rk3588_cru_pll_lookup(node->id);
		if (pll == NULL) {
			ret = -ENOTSUP;
		} else {
			cur_rate = rk3588_cru_pll_get_rate(base, pll);
			if (cur_rate == 0U) {
				ret = -EIO;
			}
		}
		break;
	default:
		ret = -EIO;
		break;
	}

	if (ret != 0) {
		return ret;
	}

	while (depth > 0U) {
		depth--;
		cur_rate /= divs[depth];
	}

	*rate = cur_rate;

	return 0;
}

/*
 * Set the parent and divider of node that come closest to rate without
 * exceeding it.
 */
static int rk3588_cru_node_set_rate(mm_reg_t base, const struct rk3588_cru_node *node,
				    uint32_t rate)
{
	const struct rk3588_cru_node *best_parent = NULL;
	uint32_t best_rate = 0U;
	uint32_t best_sel = 0U;
	uint32_t best_div = 0U;
	uint32_t mask = 0U;
	uint32_t val = 0U;

	if ((node->type != RK3588_NODE_CLKSEL) ||
	    ((node->flags & (RK3588_CLK_MUX | RK3588_CLK_DIV)) == 0U)) {
		return -ENOTSUP;
	}

	for (uint32_t sel = 0U; sel < node->num_parents; sel++) {
		const struct rk3588_cru_node *parent = rk3588_cru_node_lookup(node->parents[sel]);
		uint32_t parent_rate;
		uint32_t div_max = 1U;
		uint64_t divisor;
		uint32_t candidate_rate;

		if (parent == NULL) {
			continue;
		}
		if ((rk3588_cru_node_get_rate(base, parent, &parent_rate) != 0) ||
		    (parent_rate == 0U)) {
			continue;
		}
		if ((node->flags & RK3588_CLK_DIV) != 0U) {
			div_max = BIT(node->div_width);
		}

		divisor = DIV_ROUND_UP((uint64_t)parent_rate, (uint64_t)rate);
		if (divisor > div_max) {
			continue;
		}

		candidate_rate = (uint32_t)(parent_rate / divisor);
		if ((candidate_rate <= best_rate) || (candidate_rate > rate)) {
			continue;
		}

		best_rate = candidate_rate;
		best_sel = sel;
		best_div = (uint32_t)divisor;
		best_parent = parent;
	}

	if (best_parent == NULL) {
		return -EINVAL;
	}

	if (rk3588_cru_gate_is_enabled(base, node)) {
		int ret = rk3588_cru_node_enable(base, best_parent);

		if (ret != 0) {
			return ret;
		}
	}

	if ((node->flags & RK3588_CLK_MUX) != 0U) {
		mask |= GENMASK(node->sel_width - 1U, 0) << node->sel_shift;
		val |= best_sel << node->sel_shift;
	}
	if ((node->flags & RK3588_CLK_DIV) != 0U) {
		mask |= GENMASK(node->div_width - 1U, 0) << node->div_shift;
		val |= (best_div - 1U) << node->div_shift;
	}
	sys_write32((mask << 16) | val, base + RK3588_CRU_CLKSEL_CON(node->con));

	return 0;
}

static bool rk3588_cru_node_parents_resolved(const struct rk3588_cru_node *node,
					     const bool *resolved)
{
	if (node->type != RK3588_NODE_CLKSEL) {
		return true;
	}

	for (uint8_t i = 0U; i < node->num_parents; i++) {
		const struct rk3588_cru_node *parent = rk3588_cru_node_lookup(node->parents[i]);

		if (parent == NULL) {
			return false;
		}
		if ((parent->type == RK3588_NODE_CLKSEL) &&
		    (!resolved[(size_t)(parent - rk3588_cru_nodes)])) {
			return false;
		}
	}

	return true;
}

/*
 * Verify that every parent ID exists and every parent chain reaches a root
 * within RK3588_CRU_MAX_DEPTH CLKSEL nodes (which also rules out cycles).
 */
static bool rk3588_cru_verify_parents(void)
{
	bool resolved[ARRAY_SIZE(rk3588_cru_nodes)] = {false};
	bool all_resolved = false;

	for (uint8_t step = 0U; step < RK3588_CRU_MAX_DEPTH; step++) {
		bool next[ARRAY_SIZE(rk3588_cru_nodes)];

		all_resolved = true;
		for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_nodes); i++) {
			next[i] = rk3588_cru_node_parents_resolved(&rk3588_cru_nodes[i], resolved);
			if (!next[i]) {
				all_resolved = false;
			}
		}
		if (all_resolved) {
			break;
		}
		(void)memcpy(resolved, next, sizeof(resolved));
	}

	return all_resolved;
}

static bool rk3588_cru_verify_clksel_node(const struct rk3588_cru_node *node)
{
	if (node->flags == 0U) {
		return false;
	}

	if ((node->flags & RK3588_CLK_MUX) != 0U) {
		if ((node->sel_width == 0U) || ((node->sel_shift + node->sel_width) > 16U) ||
		    (node->num_parents == 0U) || (node->num_parents > RK3588_CRU_MAX_PARENTS) ||
		    (node->num_parents > BIT(node->sel_width))) {
			return false;
		}
	} else {
		if ((node->sel_width != 0U) || (node->num_parents > 1U)) {
			return false;
		}
	}

	if ((node->flags & RK3588_CLK_DIV) != 0U) {
		if ((node->div_width == 0U) || ((node->div_shift + node->div_width) > 16U)) {
			return false;
		}
	} else {
		if (node->div_width != 0U) {
			return false;
		}
	}

	if (((node->flags & RK3588_CLK_MUX) != 0U) && ((node->flags & RK3588_CLK_DIV) != 0U)) {
		uint32_t sel_mask = GENMASK(node->sel_width - 1U, 0) << node->sel_shift;
		uint32_t div_mask = GENMASK(node->div_width - 1U, 0) << node->div_shift;

		if ((sel_mask & div_mask) != 0U) {
			return false;
		}
	}

	if ((node->flags & RK3588_CLK_GATE) != 0U) {
		if ((node->gate_off == 0U) || (node->gate_shift >= 16U)) {
			return false;
		}
	} else {
		if ((node->gate_off != 0U) || ((node->flags & RK3588_CLK_CRITICAL) != 0U)) {
			return false;
		}
	}

	return true;
}

/* Cross-check the flags of every row against its fields and parents. */
static int rk3588_cru_verify_nodes(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_nodes); i++) {
		const struct rk3588_cru_node *node = &rk3588_cru_nodes[i];
		bool valid = true;

		for (size_t j = 0U; j < i; j++) {
			if (rk3588_cru_nodes[j].id == node->id) {
				valid = false;
			}
		}

		switch (node->type) {
		case RK3588_NODE_FIXED:
			valid = valid && (node->fixed_hz != 0U) && (node->flags == 0U);
			break;
		case RK3588_NODE_EXTERNAL:
			valid = valid && (node->flags == 0U);
			break;
		case RK3588_NODE_PLL:
			valid = valid && (rk3588_cru_pll_lookup(node->id) != NULL) &&
				(node->flags == 0U);
			break;
		case RK3588_NODE_CLKSEL:
			valid = valid && rk3588_cru_verify_clksel_node(node);
			break;
		default:
			valid = false;
			break;
		}

		if (!valid) {
			LOG_ERR("invalid clock node %u", node->id);
			return -EINVAL;
		}
	}

	if (!rk3588_cru_verify_parents()) {
		LOG_ERR("invalid clock parent chain");
		return -EINVAL;
	}

	return 0;
}

static int clock_control_rk3588_init(const struct device *dev)
{
	int ret = rk3588_cru_verify_nodes();

	if (ret != 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static int clock_control_rk3588_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct rk3588_cru_node *node = rk3588_cru_subsys_lookup(sys);

	if (node == NULL) {
		LOG_ERR("unknown clock id %lu", (unsigned long)(uintptr_t)sys);
		return -EINVAL;
	}
	if ((node->flags & RK3588_CLK_GATE) == 0U) {
		return -ENOTSUP;
	}

	return rk3588_cru_node_enable(DEVICE_MMIO_GET(dev), node);
}

static int clock_control_rk3588_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct rk3588_cru_node *node = rk3588_cru_subsys_lookup(sys);

	if (node == NULL) {
		LOG_ERR("unknown clock id %lu", (unsigned long)(uintptr_t)sys);
		return -EINVAL;
	}
	if ((node->flags & RK3588_CLK_GATE) == 0U) {
		return -ENOTSUP;
	}
	if ((node->flags & RK3588_CLK_CRITICAL) != 0U) {
		return -EPERM;
	}

	/* Only gate the requested leaf; parents may be shared by other consumers. */
	rk3588_cru_gate_write(DEVICE_MMIO_GET(dev), node->gate_off, node->gate_shift, false);

	return 0;
}

static int clock_control_rk3588_get_rate(const struct device *dev, clock_control_subsys_t sys,
					 uint32_t *rate)
{
	const struct rk3588_cru_node *node = rk3588_cru_subsys_lookup(sys);

	if (rate == NULL) {
		return -EINVAL;
	}
	if (node == NULL) {
		return -ENOTSUP;
	}

	return rk3588_cru_node_get_rate(DEVICE_MMIO_GET(dev), node, rate);
}

static int clock_control_rk3588_set_rate(const struct device *dev, clock_control_subsys_t sys,
					 clock_control_subsys_rate_t rate)
{
	const struct rk3588_cru_node *node = rk3588_cru_subsys_lookup(sys);
	uintptr_t req_rate = (uintptr_t)rate;

	if (node == NULL) {
		return -ENOTSUP;
	}
	if ((req_rate == 0U) || (req_rate > UINT32_MAX)) {
		return -EINVAL;
	}

	return rk3588_cru_node_set_rate(DEVICE_MMIO_GET(dev), node, (uint32_t)req_rate);
}

static DEVICE_API(clock_control, clock_control_rk3588_api) = {
	.on = clock_control_rk3588_on,
	.off = clock_control_rk3588_off,
	.get_rate = clock_control_rk3588_get_rate,
	.set_rate = clock_control_rk3588_set_rate,
};

static struct clock_control_rk3588_data rk3588_cru_data;

static const struct clock_control_rk3588_config clock_control_rk3588_config = {
	DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, clock_control_rk3588_init, NULL, &rk3588_cru_data,
		      &clock_control_rk3588_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_rk3588_api);
