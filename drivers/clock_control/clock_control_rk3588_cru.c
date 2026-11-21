/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 CRU clock controller.
 */

#define DT_DRV_COMPAT rockchip_rk3588_cru_clk

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rk3588-cru.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_rk3588_cru, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RK3588_CRU_PLL_CON(n)         ((n) * 4U)
#define RK3588_CRU_MODE_CON0          0x280U
#define RK3588_CRU_CLKSEL_CON(n)      ((n) * 4U + 0x300U)
#define RK3588_CRU_CLKGATE_CON(n)     ((n) * 4U + 0x800U)
#define RK3588_PMU1CRU_CLKGATE_CON(n) ((n) * 4U + 0x30800U)

/*
 * Every clock the driver knows is one row of rk3588_cru_nodes. A row tells how
 * its rate is computed (type) and which parts the hardware offers (features);
 * the fields of an absent part are zero and never read. A row corresponds to
 * one entry of the clock table of Linux's clk-rk3588.c: a COMPOSITE there is a
 * row with mux, divider and gate, a GATE only carries the gate fields and a
 * COMPOSITE_NODIV has no divider.
 */
enum rk3588_cru_node_type {
	RK3588_NODE_FIXED,  /* constant rate, fixed_hz */
	RK3588_NODE_PLL,    /* rate read from the PLL registers, rk3588_cru_plls */
	RK3588_NODE_CLKSEL, /* rate from the parent selected in a CLKSEL_CON register */
};

enum rk3588_cru_feature {
	RK3588_CLK_MUX  = BIT(0), /* parent selector in a CLKSEL_CON register */
	RK3588_CLK_DIV  = BIT(1), /* programmable divider in a CLKSEL_CON register */
	RK3588_CLK_GATE = BIT(2), /* gate in a CLKGATE_CON register */
};

#define RK3588_CRU_MAX_PARENTS 5
#define RK3588_CRU_MAX_DEPTH   3

/* 24 MHz oscillator node; not an identifier of the DT binding. */
#define RK3588_CRU_CLK_XIN24M 0U

struct rk3588_cru_node {
	uint16_t id;
	uint32_t gate_off;   /* CLKGATE_CON offset, set with RK3588_CLK_GATE */
	uint32_t fixed_hz;   /* set with RK3588_NODE_FIXED */
	uint8_t type;
	uint8_t features;
	uint8_t con;         /* CLKSEL_CON index of the mux and divider */
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
#define RK3588_GATE(_id, _gate_off, _gate_shift)                                      \
	{                                                                            \
		.id = (_id),                                                         \
		.type = RK3588_NODE_CLKSEL,                                           \
		.features = RK3588_CLK_GATE,                                          \
		.gate_off = (_gate_off),                                              \
		.gate_shift = (_gate_shift),                                          \
	}

/* A gate that passes the rate of its single parent through unchanged. */
#define RK3588_GATE_FROM(_id, _parent, _gate_off, _gate_shift)                        \
	{                                                                            \
		.id = (_id),                                                         \
		.type = RK3588_NODE_CLKSEL,                                           \
		.features = RK3588_CLK_GATE,                                          \
		.num_parents = 1,                                                     \
		.parents = {_parent},                                                 \
		.gate_off = (_gate_off),                                              \
		.gate_shift = (_gate_shift),                                          \
	}

#define RK3588_MUX_GATE(_id, _con, _sel_shift, _sel_width, _gate_off, _gate_shift, \
			...)                                                             \
	{                                                                            \
		.id = (_id),                                                         \
		.type = RK3588_NODE_CLKSEL,                                           \
		.features = RK3588_CLK_MUX | RK3588_CLK_GATE,                         \
		.con = (_con),                                                       \
		.sel_shift = (_sel_shift),                                            \
		.sel_width = (_sel_width),                                            \
		.num_parents = NUM_VA_ARGS(__VA_ARGS__),                       \
		.parents = {__VA_ARGS__},                                             \
		.gate_off = (_gate_off),                                              \
		.gate_shift = (_gate_shift),                                          \
	}

#define RK3588_MUX_DIV(_id, _con, _sel_shift, _sel_width, _div_shift, _div_width,  \
		      ...)                                                           \
	{                                                                            \
		.id = (_id),                                                         \
		.type = RK3588_NODE_CLKSEL,                                           \
		.features = RK3588_CLK_MUX | RK3588_CLK_DIV,                          \
		.con = (_con),                                                       \
		.sel_shift = (_sel_shift),                                            \
		.sel_width = (_sel_width),                                            \
		.div_shift = (_div_shift),                                            \
		.div_width = (_div_width),                                            \
		.num_parents = NUM_VA_ARGS(__VA_ARGS__),                       \
		.parents = {__VA_ARGS__},                                             \
	}

#define RK3588_MUX_DIV_GATE(_id, _con, _sel_shift, _sel_width, _div_shift,          \
			    _div_width, _gate_off, _gate_shift, ...)                  \
	{                                                                            \
		.id = (_id),                                                         \
		.type = RK3588_NODE_CLKSEL,                                           \
		.features = RK3588_CLK_MUX | RK3588_CLK_DIV | RK3588_CLK_GATE,        \
		.con = (_con),                                                       \
		.sel_shift = (_sel_shift),                                            \
		.sel_width = (_sel_width),                                            \
		.div_shift = (_div_shift),                                            \
		.div_width = (_div_width),                                            \
		.num_parents = NUM_VA_ARGS(__VA_ARGS__),                       \
		.parents = {__VA_ARGS__},                                             \
		.gate_off = (_gate_off),                                              \
		.gate_shift = (_gate_shift),                                          \
	}

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
#define RK3588_XIN32K_HZ       32768U

struct rk3588_cru_pll {
	uint16_t id;         /* identifier of the PLL node */
	uint8_t con;         /* index of its first PLL_CON register */
	uint8_t mode_shift;  /* its mode bits in MODE_CON0 */
};

static const struct rk3588_cru_pll rk3588_cru_plls[] = {
	{.id = PLL_GPLL, .con = 112, .mode_shift = 2},
	{.id = PLL_CPLL, .con = 104, .mode_shift = 8},
};

static const struct rk3588_cru_node rk3588_cru_nodes[] = {
	/* Rate sources */
	{.id = RK3588_CRU_CLK_XIN24M, .type = RK3588_NODE_FIXED, .fixed_hz = MHZ(24)},
	{.id = PLL_GPLL, .type = RK3588_NODE_PLL},
	{.id = PLL_CPLL, .type = RK3588_NODE_PLL},
	RK3588_MUX_DIV(CLK_150M_SRC, 1, 5, 1, 0, 5, PLL_GPLL, PLL_CPLL),
	RK3588_MUX_DIV(CLK_200M_SRC, 1, 11, 1, 6, 5, PLL_GPLL, PLL_CPLL),

	/* GPIO bank peripheral and debounce clocks */
	RK3588_GATE(PCLK_GPIO1, RK3588_CRU_CLKGATE_CON(16), 14),
	RK3588_GATE(DBCLK_GPIO1, RK3588_CRU_CLKGATE_CON(16), 15),
	RK3588_GATE(PCLK_GPIO2, RK3588_CRU_CLKGATE_CON(17), 0),
	RK3588_GATE(DBCLK_GPIO2, RK3588_CRU_CLKGATE_CON(17), 1),
	RK3588_GATE(PCLK_GPIO3, RK3588_CRU_CLKGATE_CON(17), 2),
	RK3588_GATE(DBCLK_GPIO3, RK3588_CRU_CLKGATE_CON(17), 3),
	RK3588_GATE(PCLK_GPIO4, RK3588_CRU_CLKGATE_CON(17), 4),
	RK3588_GATE(DBCLK_GPIO4, RK3588_CRU_CLKGATE_CON(17), 5),
	RK3588_GATE(PCLK_GPIO0, RK3588_PMU1CRU_CLKGATE_CON(5), 5),
	RK3588_GATE(DBCLK_GPIO0, RK3588_PMU1CRU_CLKGATE_CON(5), 6),

	/* SPI controllers: a mux over the fixed taps, without a divider */
	RK3588_GATE(PCLK_SPI0, RK3588_CRU_CLKGATE_CON(14), 6),
	RK3588_GATE(PCLK_SPI1, RK3588_CRU_CLKGATE_CON(14), 7),
	RK3588_GATE(PCLK_SPI2, RK3588_CRU_CLKGATE_CON(14), 8),
	RK3588_GATE(PCLK_SPI3, RK3588_CRU_CLKGATE_CON(14), 9),
	RK3588_GATE(PCLK_SPI4, RK3588_CRU_CLKGATE_CON(14), 10),
	RK3588_MUX_GATE(CLK_SPI0, 59, 2, 2, RK3588_CRU_CLKGATE_CON(14), 11,
			CLK_200M_SRC, CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI1, 59, 4, 2, RK3588_CRU_CLKGATE_CON(14), 12,
			CLK_200M_SRC, CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI2, 59, 6, 2, RK3588_CRU_CLKGATE_CON(14), 13,
			CLK_200M_SRC, CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI3, 59, 8, 2, RK3588_CRU_CLKGATE_CON(14), 14,
			CLK_200M_SRC, CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_GATE(CLK_SPI4, 59, 10, 2, RK3588_CRU_CLKGATE_CON(14), 15,
			CLK_200M_SRC, CLK_150M_SRC, RK3588_CRU_CLK_XIN24M),

	/* eMMC controller: HCLK and ACLK come from unmodeled bus roots */
	RK3588_GATE(HCLK_EMMC, RK3588_CRU_CLKGATE_CON(31), 4),
	RK3588_GATE(ACLK_EMMC, RK3588_CRU_CLKGATE_CON(31), 5),
	RK3588_MUX_DIV_GATE(CCLK_EMMC, 77, 14, 2, 8, 6, RK3588_CRU_CLKGATE_CON(31), 6,
			    PLL_GPLL, PLL_CPLL, RK3588_CRU_CLK_XIN24M),
	RK3588_MUX_DIV_GATE(BCLK_EMMC, 78, 5, 1, 0, 5, RK3588_CRU_CLKGATE_CON(31), 7,
			    PLL_GPLL, PLL_CPLL),
	RK3588_GATE_FROM(TMCLK_EMMC, RK3588_CRU_CLK_XIN24M, RK3588_CRU_CLKGATE_CON(31), 8),

	/*
	 * Ethernet controller. The PTP reference muxes also offer an external
	 * input, which the driver does not model, and the ACLK and PCLK of the
	 * controllers come from unmodeled bus roots.
	 */
	RK3588_MUX_DIV_GATE(MCLK_GMAC0_OUT, 15, 7, 1, 0, 7, RK3588_CRU_CLKGATE_CON(5), 3,
			    PLL_GPLL, PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC0_PTP_REF, 81, 6, 1, 0, 6, RK3588_CRU_CLKGATE_CON(34),
			    10, PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC1_PTP_REF, 81, 13, 1, 7, 6, RK3588_CRU_CLKGATE_CON(34),
			    11, PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC_125M, 83, 15, 1, 8, 7, RK3588_CRU_CLKGATE_CON(35), 5,
			    PLL_GPLL, PLL_CPLL),
	RK3588_MUX_DIV_GATE(CLK_GMAC_50M, 84, 7, 1, 0, 7, RK3588_CRU_CLKGATE_CON(35), 6,
			    PLL_GPLL, PLL_CPLL),
	RK3588_GATE(PCLK_GMAC0, RK3588_CRU_CLKGATE_CON(32), 3),
	RK3588_GATE(PCLK_GMAC1, RK3588_CRU_CLKGATE_CON(32), 4),
	RK3588_GATE(ACLK_GMAC0, RK3588_CRU_CLKGATE_CON(32), 10),
	RK3588_GATE(ACLK_GMAC1, RK3588_CRU_CLKGATE_CON(32), 11),

	/*
	 * USB 3.0 OTG0 controller and its USB 2.0 PHY: the suspend, reference
	 * and PHY clocks run from the oscillator, the ACLK from an unmodeled
	 * root.
	 */
	RK3588_GATE(ACLK_USB3OTG0, RK3588_CRU_CLKGATE_CON(42), 4),
	RK3588_GATE_FROM(SUSPEND_CLK_USB3OTG0, RK3588_CRU_CLK_XIN24M,
			 RK3588_CRU_CLKGATE_CON(42), 5),
	RK3588_GATE_FROM(REF_CLK_USB3OTG0, RK3588_CRU_CLK_XIN24M,
			 RK3588_CRU_CLKGATE_CON(42), 6),
	RK3588_GATE_FROM(CLK_USB2PHY_HDPTXRXPHY_REF, RK3588_CRU_CLK_XIN24M,
			 RK3588_PMU1CRU_CLKGATE_CON(4), 7),
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

static const struct rk3588_cru_pll *rk3588_cru_pll_lookup(uint16_t clk_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_plls); i++) {
		if (rk3588_cru_plls[i].id == clk_id) {
			return &rk3588_cru_plls[i];
		}
	}

	return NULL;
}

static uint32_t rk3588_cru_pll_rate(mm_reg_t base, const struct rk3588_cru_pll *pll)
{
	uint32_t mode =
		(sys_read32(base + RK3588_CRU_MODE_CON0) >> pll->mode_shift) & RK3588_PLL_MODE_MASK;
	uint32_t con1 = sys_read32(base + RK3588_CRU_PLL_CON(pll->con + 1U));
	uint32_t m = sys_read32(base + RK3588_CRU_PLL_CON(pll->con)) & RK3588_PLL_M_MASK;
	uint32_t p = con1 & RK3588_PLL_P_MASK;
	uint32_t s = (con1 >> RK3588_PLL_S_SHIFT) & RK3588_PLL_S_MASK;
	int16_t k =
		(int16_t)(sys_read32(base + RK3588_CRU_PLL_CON(pll->con + 2U)) & RK3588_PLL_K_MASK);
	int64_t vco;

	if (mode == RK3588_PLL_MODE_SLOW) {
		return MHZ(24);
	}
	if (mode != RK3588_PLL_MODE_NORMAL) {
		return RK3588_XIN32K_HZ;
	}
	if (p == 0U) {
		return 0U;
	}

	vco = (int64_t)MHZ(24) * ((int64_t)m * 65536 + k) / ((int64_t)p * 65536);

	return (uint32_t)(vco >> s);
}

/*
 * Rate of node, following its parent chain up to RK3588_CRU_MAX_DEPTH nodes.
 * Without a mux a node passes the rate of its single parent, if it has one,
 * through unchanged.
 */
static int rk3588_cru_node_rate(mm_reg_t base, const struct rk3588_cru_node *node,
				uint8_t depth, uint32_t *rate)
{
	const struct rk3588_cru_node *parent;
	const struct rk3588_cru_pll *pll;
	uint32_t con;
	uint32_t sel;
	uint32_t divisor = 1U;
	uint32_t parent_rate;
	int ret;

	if (depth >= RK3588_CRU_MAX_DEPTH) {
		return -EIO;
	}

	switch (node->type) {
	case RK3588_NODE_FIXED:
		*rate = node->fixed_hz;
		return 0;
	case RK3588_NODE_PLL:
		pll = rk3588_cru_pll_lookup(node->id);
		if (pll == NULL) {
			return -ENOTSUP;
		}
		*rate = rk3588_cru_pll_rate(base, pll);
		return 0;
	case RK3588_NODE_CLKSEL:
		break;
	}

	con = sys_read32(base + RK3588_CRU_CLKSEL_CON(node->con));

	if ((node->features & RK3588_CLK_MUX) != 0U) {
		sel = (con >> node->sel_shift) & GENMASK(node->sel_width - 1U, 0);
		if (sel >= node->num_parents) {
			return -EIO;
		}
	} else if (node->num_parents == 1U) {
		sel = 0U;
	} else {
		return -ENOTSUP;
	}

	if ((node->features & RK3588_CLK_DIV) != 0U) {
		divisor = ((con >> node->div_shift) & GENMASK(node->div_width - 1U, 0)) + 1U;
	}

	parent = rk3588_cru_node_lookup(node->parents[sel]);
	if (parent == NULL) {
		return -ENOTSUP;
	}

	ret = rk3588_cru_node_rate(base, parent, depth + 1U, &parent_rate);
	if (ret != 0) {
		return ret;
	}

	*rate = parent_rate / divisor;

	return 0;
}

/*
 * Set the parent and divider of node that come closest to rate without
 * exceeding it.
 */
static int rk3588_cru_node_set_rate(mm_reg_t base, const struct rk3588_cru_node *node,
				    uint32_t rate)
{
	uint32_t candidates;
	uint32_t best_rate = 0U;
	uint32_t best_sel = 0U;
	uint32_t best_div = 0U;
	uint32_t mask = 0U;
	uint32_t val = 0U;

	if (node->type != RK3588_NODE_CLKSEL) {
		return -ENOTSUP;
	}
	if ((node->features & (RK3588_CLK_MUX | RK3588_CLK_DIV)) == 0U) {
		return -ENOTSUP;
	}
	if (rate == 0U) {
		return -EINVAL;
	}

	candidates = ((node->features & RK3588_CLK_MUX) != 0U) ? node->num_parents : 1U;

	for (uint32_t sel = 0U; sel < candidates; sel++) {
		const struct rk3588_cru_node *parent = rk3588_cru_node_lookup(node->parents[sel]);
		uint32_t parent_rate;
		uint32_t div_max = 1U;
		uint32_t divisor;

		if (parent == NULL) {
			continue;
		}
		if (rk3588_cru_node_rate(base, parent, 0U, &parent_rate) != 0) {
			continue;
		}
		if ((node->features & RK3588_CLK_DIV) != 0U) {
			div_max = BIT(node->div_width);
		}

		divisor = DIV_ROUND_UP(parent_rate, rate);
		if (divisor == 0U || divisor > div_max) {
			continue;
		}
		if (parent_rate / divisor <= best_rate || parent_rate / divisor > rate) {
			continue;
		}

		best_rate = parent_rate / divisor;
		best_sel = sel;
		best_div = divisor;
	}

	if (best_rate == 0U) {
		return -EINVAL;
	}

	if ((node->features & RK3588_CLK_MUX) != 0U) {
		mask |= GENMASK(node->sel_width - 1U, 0) << node->sel_shift;
		val |= best_sel << node->sel_shift;
	}
	if ((node->features & RK3588_CLK_DIV) != 0U) {
		mask |= GENMASK(node->div_width - 1U, 0) << node->div_shift;
		val |= (best_div - 1U) << node->div_shift;
	}
	sys_write32((mask << 16) | val, base + RK3588_CRU_CLKSEL_CON(node->con));

	return 0;
}

static void rk3588_cru_gate_write(mm_reg_t base, uint32_t gate_off, uint8_t gate_shift, bool enable)
{
	uint32_t reg = sys_read32(base + gate_off);
	uint32_t val;

	if (enable) {
		val = (BIT(gate_shift) << 16) | (reg & ~BIT(gate_shift));
	} else {
		val = (BIT(gate_shift) << 16) | (reg | BIT(gate_shift));
	}

	sys_write32(val, base + gate_off);
}

/* Cross-check the features of every row against its fields and parents. */
static int rk3588_cru_verify_nodes(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(rk3588_cru_nodes); i++) {
		const struct rk3588_cru_node *node = &rk3588_cru_nodes[i];
		uint8_t referenced;
		bool valid = true;

		for (size_t j = 0U; j < i; j++) {
			if (rk3588_cru_nodes[j].id == node->id) {
				valid = false;
			}
		}

		switch (node->type) {
		case RK3588_NODE_FIXED:
			valid = valid && node->fixed_hz != 0U;
			referenced = 0U;
			break;
		case RK3588_NODE_PLL:
			valid = valid && rk3588_cru_pll_lookup(node->id) != NULL;
			referenced = 0U;
			break;
		case RK3588_NODE_CLKSEL:
			if ((node->features & RK3588_CLK_MUX) != 0U) {
				valid = valid && node->sel_width != 0U &&
					node->num_parents != 0U &&
					node->num_parents <= RK3588_CRU_MAX_PARENTS &&
					node->num_parents <= BIT(node->sel_width);
				referenced = node->num_parents;
			} else {
				valid = valid && node->sel_width == 0U &&
					node->num_parents <= 1U;
				referenced = node->num_parents;
			}
			if ((node->features & RK3588_CLK_DIV) != 0U) {
				valid = valid && node->div_width != 0U;
			} else {
				valid = valid && node->div_width == 0U;
			}
			if ((node->features & RK3588_CLK_GATE) != 0U) {
				valid = valid && node->gate_off != 0U;
			} else {
				valid = valid && node->gate_off == 0U;
			}
			valid = valid && node->features != 0U;
			break;
		default:
			valid = false;
			referenced = 0U;
			break;
		}

		for (uint8_t j = 0U; valid && j < referenced; j++) {
			const struct rk3588_cru_node *parent;

			parent = rk3588_cru_node_lookup(node->parents[j]);
			valid = parent != NULL && parent != node;
		}

		if (!valid) {
			LOG_ERR("invalid clock node %u", node->id);
			return -EINVAL;
		}
	}

	return 0;
}

static int rk3588_cru_clk_on_off(const struct device *dev, clock_control_subsys_t sys, bool enable)
{
	uint16_t clk_id = (uint16_t)(uintptr_t)sys;
	const struct rk3588_cru_node *node = rk3588_cru_node_lookup(clk_id);

	if (node == NULL) {
		LOG_ERR("unknown clock id %u", clk_id);
		return -EINVAL;
	}
	if ((node->features & RK3588_CLK_GATE) == 0U) {
		return -ENOTSUP;
	}

	rk3588_cru_gate_write(DEVICE_MMIO_GET(dev), node->gate_off, node->gate_shift, enable);

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
	return rk3588_cru_clk_on_off(dev, sys, true);
}

static int clock_control_rk3588_off(const struct device *dev, clock_control_subsys_t sys)
{
	return rk3588_cru_clk_on_off(dev, sys, false);
}

static int clock_control_rk3588_get_rate(const struct device *dev, clock_control_subsys_t sys,
					 uint32_t *rate)
{
	uint16_t clk_id = (uint16_t)(uintptr_t)sys;
	const struct rk3588_cru_node *node = rk3588_cru_node_lookup(clk_id);

	if (node == NULL) {
		return -ENOTSUP;
	}

	return rk3588_cru_node_rate(DEVICE_MMIO_GET(dev), node, 0U, rate);
}

static int clock_control_rk3588_set_rate(const struct device *dev, clock_control_subsys_t sys,
					 clock_control_subsys_rate_t rate)
{
	uint16_t clk_id = (uint16_t)(uintptr_t)sys;
	const struct rk3588_cru_node *node = rk3588_cru_node_lookup(clk_id);

	if (node == NULL) {
		return -ENOTSUP;
	}

	return rk3588_cru_node_set_rate(DEVICE_MMIO_GET(dev), node, (uint32_t)(uintptr_t)rate);
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
