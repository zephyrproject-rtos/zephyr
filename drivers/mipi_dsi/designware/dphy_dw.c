/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/device_mmio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>

#include "dphy_dw.h"
#include "dsi_dw_priv.h"
#include "zephyr/drivers/clock_control.h"

LOG_MODULE_REGISTER(dphy_dw, CONFIG_MIPI_DSI_LOG_LEVEL);

#define DEV_DATA(dev) ((struct dsi_dw_data *)((dev)->data))
#define DEV_CFG(dev)  ((struct dsi_dw_config *)((dev)->config))

static const struct dphy_freq_range frequency_range[] = {
	{80, 0x00, 0x1e9, 21, 17, 15, 10},      {90, 0x10, 0x1e9, 23, 17, 16, 10},
	{100, 0x20, 0x1e9, 22, 17, 16, 10},     {110, 0x30, 0x1e9, 25, 18, 17, 11},
	{120, 0x01, 0x1e9, 26, 20, 18, 11},     {130, 0x11, 0x1e9, 27, 19, 19, 11},
	{140, 0x21, 0x1e9, 27, 19, 19, 11},     {150, 0x31, 0x1e9, 28, 20, 20, 12},
	{160, 0x02, 0x1e9, 30, 21, 22, 13},     {170, 0x12, 0x1e9, 30, 21, 23, 13},
	{180, 0x22, 0x1e9, 31, 21, 23, 13},     {190, 0x32, 0x1e9, 32, 22, 24, 13},
	{205, 0x03, 0x1e9, 35, 22, 25, 13},     {220, 0x13, 0x1e9, 37, 26, 27, 15},
	{235, 0x23, 0x1e9, 38, 28, 27, 16},     {250, 0x33, 0x1e9, 41, 29, 30, 17},
	{275, 0x04, 0x1e9, 43, 29, 32, 18},     {300, 0x14, 0x1e9, 45, 32, 35, 19},
	{325, 0x25, 0x1e9, 48, 33, 36, 18},     {350, 0x35, 0x1e9, 51, 35, 40, 20},
	{400, 0x05, 0x1e9, 59, 37, 44, 21},     {450, 0x16, 0x1e9, 65, 40, 49, 23},
	{500, 0x26, 0x1e9, 71, 41, 54, 24},     {550, 0x37, 0x1e9, 77, 44, 57, 26},
	{600, 0x07, 0x1e9, 82, 46, 64, 27},     {650, 0x18, 0x1e9, 87, 48, 67, 28},
	{700, 0x28, 0x1e9, 94, 52, 71, 29},     {750, 0x39, 0x1e9, 99, 52, 75, 31},
	{800, 0x09, 0x1e9, 105, 55, 82, 32},    {850, 0x19, 0x1e9, 110, 58, 85, 32},
	{900, 0x29, 0x1e9, 115, 58, 88, 35},    {950, 0x3a, 0x1e9, 120, 62, 93, 36},
	{1000, 0x0a, 0x1e9, 128, 63, 99, 38},   {1050, 0x1a, 0x1e9, 132, 65, 102, 38},
	{1100, 0x2a, 0x1e9, 138, 67, 106, 39},  {1150, 0x3b, 0x1e9, 146, 69, 112, 42},
	{1200, 0x0b, 0x1e9, 151, 71, 117, 43},  {1250, 0x1b, 0x1e9, 153, 74, 120, 45},
	{1300, 0x2b, 0x1e9, 160, 73, 124, 46},  {1350, 0x3c, 0x1e9, 165, 76, 130, 47},
	{1400, 0x0c, 0x1e9, 172, 78, 134, 49},  {1450, 0x1c, 0x1e9, 177, 80, 138, 49},
	{1500, 0x2c, 0x1e9, 183, 81, 143, 52},  {1550, 0x3d, 0x10f, 191, 84, 147, 52},
	{1600, 0x0d, 0x118, 194, 85, 152, 52},  {1650, 0x1d, 0x121, 201, 86, 155, 53},
	{1700, 0x2e, 0x12a, 208, 88, 161, 53},  {1750, 0x3e, 0x132, 212, 89, 165, 53},
	{1800, 0x0e, 0x13b, 220, 90, 171, 54},  {1850, 0x1e, 0x144, 223, 92, 175, 54},
	{1900, 0x2f, 0x14d, 231, 91, 180, 55},  {1950, 0x3f, 0x155, 236, 95, 185, 56},
	{2000, 0x0f, 0x15e, 243, 97, 190, 56},  {2050, 0x40, 0x167, 248, 99, 194, 58},
	{2100, 0x41, 0x170, 252, 100, 199, 59}, {2150, 0x42, 0x178, 259, 102, 204, 61},
	{2200, 0x43, 0x181, 266, 105, 210, 62}, {2250, 0x44, 0x18a, 269, 109, 213, 63},
	{2300, 0x45, 0x193, 272, 109, 217, 65}, {2350, 0x46, 0x19b, 281, 112, 225, 66},
	{2400, 0x47, 0x1a4, 283, 115, 226, 66}, {2450, 0x48, 0x1ad, 282, 115, 226, 67},
	{2500, 0x49, 0x1e9, 281, 118, 227, 67},
};

static const struct dphy_pll_vco_ctrl vco_ctrl_range[] = {
	{MHZ(1170), MHZ(1250), 0x03},      {MHZ(975), MHZ(1230), 0x07},
	{MHZ(853.125), MHZ(1025.0), 0x08}, {MHZ(706.875), MHZ(896.875), 0x08},
	{MHZ(585), MHZ(743.125), 0x0b},    {MHZ(487.5), MHZ(615), 0x0f},
	{MHZ(426.56), MHZ(512.5), 0x10},   {MHZ(353.4), MHZ(484.4), 0x10},
	{MHZ(292.5), MHZ(371.5), 0x13},    {MHZ(243.75), MHZ(307.5), 0x17},
	{MHZ(213.3), MHZ(256.25), 0x18},   {MHZ(176.72), MHZ(224.2), 0x18},
	{MHZ(146.25), MHZ(185.78), 0x1b},  {MHZ(121.88), MHZ(153.75), 0x1f},
	{MHZ(106.64), MHZ(125.12), 0x20},  {MHZ(88.36), MHZ(112.1), 0x20},
	{MHZ(73.13), MHZ(92.9), 0x23},     {MHZ(60.93), MHZ(76.87), 0x27},
	{MHZ(53.32), MHZ(64), 0x28},       {MHZ(44.18), MHZ(56), 0x28},
	{MHZ(40), MHZ(46.44), 0x2b},
};

static void reg_write_part(uintptr_t reg, uint32_t data, uint32_t mask, uint8_t shift)
{
	uint32_t tmp = 0;

	tmp = sys_read32(reg);
	tmp &= ~(mask << shift);
	tmp |= (data & mask) << shift;
	sys_write32(tmp, reg);
}

static uint8_t mipi_dphy_read_register(uintptr_t test_ctrl0, uintptr_t test_ctrl1, uint16_t address)
{
	uint8_t ret = 0;

	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	reg_write_part(test_ctrl1, 0x0, PHY_TST_CTRL1_TESTDIN_MASK, PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	reg_write_part(test_ctrl1, address >> 8, PHY_TST_CTRL1_TESTDIN_MASK,
		       PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_set_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	reg_write_part(test_ctrl1, address, PHY_TST_CTRL1_TESTDIN_MASK,
		       PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	ret = (sys_read32(test_ctrl1) >> PHY_TST_CTRL1_TESTDOUT_SHIFT) &
	      PHY_TST_CTRL1_TESTDOUT_MASK;
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);

	return ret;
}

static void mipi_dphy_write_register(uintptr_t test_ctrl0, uintptr_t test_ctrl1, uint16_t address,
				     uint8_t data)
{
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	reg_write_part(test_ctrl1, 0x0, PHY_TST_CTRL1_TESTDIN_MASK, PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	reg_write_part(test_ctrl1, address >> 8, PHY_TST_CTRL1_TESTDIN_MASK,
		       PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_set_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	reg_write_part(test_ctrl1, address, PHY_TST_CTRL1_TESTDIN_MASK,
		       PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl1, PHY_TST_CTRL1_TESTEN);
	reg_write_part(test_ctrl1, data, PHY_TST_CTRL1_TESTDIN_MASK, PHY_TST_CTRL1_TESTDIN_SHIFT);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLK);
}

void mipi_dphy_mask_write_register(uintptr_t test_ctrl0, uintptr_t test_ctrl1, uint16_t address,
				   uint8_t data, uint8_t mask, uint8_t shift)
{
	uint8_t tmp = mipi_dphy_read_register(test_ctrl0, test_ctrl1, address);

	tmp &= ~(mask << shift);
	tmp |= (data & mask) << shift;
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, address, tmp);
}

static int dphy_pll_calculate_mnp(uint32_t pll_fout, uint16_t *pll_m, uint16_t *pll_n,
				  uint8_t *pll_p, uint8_t *vco_ctrl, uint32_t ref_clk_freq)
{
	uint32_t delta = UINT32_MAX;
	float freq_mhz = pll_fout / 1000000.0;

	if ((pll_fout > vco_ctrl_range[0].max_freq) ||
	    (pll_fout < vco_ctrl_range[ARRAY_SIZE(vco_ctrl_range) - 1].min_freq)) {
		LOG_ERR("Invalid Frequency requested.");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(vco_ctrl_range); i++) {
		uint16_t m, n;
		uint8_t p, vco;
		uint32_t calculated_freq;
		float tmp;

		if (pll_fout > vco_ctrl_range[i].max_freq) {
			break;
		}
		if (pll_fout < vco_ctrl_range[i].min_freq) {
			continue;
		}

		vco = vco_ctrl_range[i].vco_ctrl;
		p = PLL_OUT_DIV_FACTOR(vco);
		n = 3;

		if ((ref_clk_freq / n) > MHZ(24) || (ref_clk_freq / n) < MHZ(8)) {
			continue;
		}

		m = CEIL((freq_mhz * 2 * p * n) / (ref_clk_freq / 1000000.0f));

		tmp = ref_clk_freq / (n * (p << 1));
		tmp *= m;

		calculated_freq = (uint32_t)tmp;
		if ((pll_fout - calculated_freq) < delta) {
			delta = (pll_fout - calculated_freq);
			*pll_m = m;
			*pll_n = n;
			*pll_p = p;
			*vco_ctrl = vco;
		}
	}

	if (!(*pll_m)) {
		return -EINVAL;
	}
	return 0;
}

static int dphy_dw_config_pll(const struct device *dev, struct dphy_dsi_settings *phy,
			      uint32_t ref_clk_freq)
{
	uintptr_t dsi_regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, clkctl_regs);
	uintptr_t test_ctrl0 = dsi_regs + DSI_PHY_TST_CTRL0;
	uintptr_t test_ctrl1 = dsi_regs + DSI_PHY_TST_CTRL1;
	float temp_freq;
	uint8_t tmp = 0;

	/*
	 * PLL configuration mechanism:
	 *	0 - through SoC registers.
	 *	1 - through Test Control interface.
	 */
	/* Enable PLL shadow cntrl bit:config through test cntrl interface. */
	sys_set_bits(regs + DPHY_PLL_CTRL0, DPHY_PLL_CTRL0_SHADOW_CONTROL);

	/*
	 * PLL clksel[1:0]:
	 *	2b'00 - Clocks stopped
	 *	2b'01 - Clocks generated
	 *	2b'10 - Buffered clkext
	 *	2b'11 - Forbidden
	 *
	 * Select output clock to be generated by the PLL (clksel - 2b'01).
	 */
	reg_write_part(regs + DPHY_PLL_CTRL0, 1, DPHY_PLL_CTRL0_CLKSEL_MASK,
		       DPHY_PLL_CTRL0_CLKSEL_SHIFT);

	/* Set PLL m[9:0] to regs - 0x179, 0x17a */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_28,
				      (uint8_t)phy->pll_m, 0xff, 0x0);
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_29,
				      (uint8_t)(phy->pll_m >> 8), 0x3, 0x0);

	tmp = mipi_dphy_read_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_30);
	/* Override the feedback divider m with above written values. */
	tmp |= BIT(0);

	/* Set up vco_cntrl value. */
	tmp &= ~(0x3f << 1);
	tmp |= (phy->vco_cntrl & 0x3f) << 1;
	tmp |= BIT(7);
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_30, tmp);

	/* Set up n[3:0] */
	tmp = mipi_dphy_read_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_27);
	tmp &= ~(0xf << 3);
	tmp |= (((phy->pll_n - 1) & 0xf) << 3) | BIT(7);
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_27, tmp);

	/* Setup charge-pump bias */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_1,
				      DPHY_CPBIAS_CNTRL, 0x7f, 0);

	/* Setup Integral charge-pump co-efficient */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_5,
				      DPHY_INT_CNTRL, 0x3f, 2);

	/* Setup GMP Control register */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_5,
				      DPHY_GMP_CNTRL, 0x3, 0);

	/* Setup the PLL reference voltage control */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_CB_3, 0x02,
				      0x7, 0);

	/* Setup proportional charge-pump co-efficient */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_17,
				      DPHY_PROP_CNTRL, 0x3f, 0);

	/* Enable the PLL power. */
	mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_17, 3,
				      0x3, 6);

	/* Final PLL Frequency calibrated */
	temp_freq = ref_clk_freq;
	temp_freq /= (phy->pll_n * (phy->pll_p << 1));
	temp_freq *= phy->pll_m;

	phy->pll_fout = (uint32_t)temp_freq;
	return 0;
}

int dphy_dw_master_setup(const struct device *dev, struct dphy_dsi_settings *phy)
{
	uintptr_t dsi_regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, clkctl_regs);
	const struct dsi_dw_config *config = dev->config;
	uint32_t bitrate_mbps = (phy->pll_fout / MHZ(1)) << 1;
	uintptr_t test_ctrl0 = dsi_regs + DSI_PHY_TST_CTRL0;
	uintptr_t test_ctrl1 = dsi_regs + DSI_PHY_TST_CTRL1;
	uint8_t cfgclkfreqrange = 0;
	uint8_t hsfreq = 0;
	uint32_t tmp = 0;
	int ret = 0;
	uint32_t i;
	uint32_t cfg_clk_freq, ref_clk_freq;

	for (i = 0; ((i < ARRAY_SIZE(frequency_range) - 1) &&
		     (bitrate_mbps > frequency_range[i].bitrate_in_mbps));
	     i++) {
	}

	hsfreq = frequency_range[i].hsfreqrange;
	LOG_DBG("hsfrequency - %d", hsfreq);

	/* Return the HS->LP and LP->HS timings to the calling driver. */
	phy->clk_hs2lp = frequency_range[i].clk_hs2lp;
	phy->clk_lp2hs = frequency_range[i].clk_lp2hs;
	phy->lane_hs2lp = frequency_range[i].lane_hs2lp;
	phy->lane_lp2hs = frequency_range[i].lane_lp2hs;

	LOG_DBG("CLK HS2LP: %d, CLK LP2HS: %d, LANE_HS2LP: %d, LANE_LP2HS: %d", phy->clk_hs2lp,
		phy->clk_lp2hs, phy->lane_hs2lp, phy->lane_lp2hs);

	/* Setup number of lanes. */
	reg_write_part(dsi_regs + DSI_PHY_IF_CFG, phy->num_lanes - 1,
		       DSI_PHY_IF_CFG_PHY_N_LANES_MASK, DSI_PHY_IF_CFG_PHY_N_LANES_SHIFT);

	/*
	 * Put D-PHY in shutdown mode prior to configuring the D-PHY.
	 * Set RSTZ = 0, SHUTDOWNZ = 0
	 */
	sys_clear_bits(dsi_regs + DSI_PHY_RSTZ, DSI_PHY_RSTZ_PHY_RSTZ | DSI_PHY_RSTZ_PHY_SHUTDOWNZ);

	/* Set txrxz = 1 to enable D-PHY master side calibration. */
	sys_set_bits(regs + TX_DPHY_CTRL0, DPHY_CTRL0_TXRXZ);

	/*
	 * Reset the Test Control interface.
	 */
	/* Select RX-Test Port. */
	sys_set_bits(regs + TX_DPHY_CTRL0, DPHY_CTRL0_TESTPORT_SEL);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLR);
	/* Select TX-Test Port. */
	sys_clear_bits(regs + TX_DPHY_CTRL0, DPHY_CTRL0_TESTPORT_SEL);
	sys_set_bits(test_ctrl0, PHY_TST_CTRL0_CLR);

	k_busy_wait(1);

	/* Select RX-Test Port. */
	sys_set_bits(regs + TX_DPHY_CTRL0, DPHY_CTRL0_TESTPORT_SEL);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLR);
	/* Select TX-Test Port. */
	sys_clear_bits(regs + TX_DPHY_CTRL0, DPHY_CTRL0_TESTPORT_SEL);
	sys_clear_bits(test_ctrl0, PHY_TST_CTRL0_CLR);

	/* Set-up HS-Frequency in TX_DPHY_CTRL0. */
	reg_write_part(regs + TX_DPHY_CTRL0, hsfreq, DPHY_CTRL0_HS_FREQ_RANGE_MASK,
		       DPHY_CTRL0_HS_FREQ_RANGE_SHIFT);

	/* Set TX register 0x16A: pll_mpll_prog_rw[1:0] */
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_PLL_13, 0x03);
	/*
	 * Set LP RX contention detector voltage ref as 325mV
	 * (reg 0x1ab[1:0] - 2b'10)
	 * and LPTX ref voltage selection as 1200mV
	 * (reg 0x1ab[2] = 1b'1)
	 */
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_CB_1, 0x6);

	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_CB_0, 0x53);

	/*
	 * For datarates < 450 Mbps or when in HS BIST modes,
	 * clkdiv_clk_en must be enabled. To do this, write 1'b1 in TX
	 * config register with address 0x1ac bit [4].
	 */
	if (bitrate_mbps < 450) {
		mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1,
					      dphy4txtester_DIG_RDWR_TX_CB_2, 1, 1, 4);
	}

	/*
	 * Set TX test ctrl register 0x402 txclk_term_lowcap_lp00_en_ovr_en_rw,
	 * txclk_term_lowcap_lp00_en_ovr_rw (bits [1:0]) to 2b'10)
	 */
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_CLK_TERMLOWCAP,
				 0x2);
	if (bitrate_mbps < 1000) {
		/* SR_OSC_FREQ_TARGET[11:0] - 657 = 0x291 */
		mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_SLEW_5,
					 0x91);
		mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_SLEW_6,
					 0x2);
		mipi_dphy_mask_write_register(test_ctrl0, test_ctrl1,
					      dphy4txtester_DIG_RDWR_TX_SLEW_7, 0x11, 0x11, 0);
	} else if (bitrate_mbps >= 1000 && bitrate_mbps < 1500) {
		/* SR_OSC_FREQ_TARGET[11:0] - 920 = 0x398 */
		mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_SLEW_5,
					 0x98);
		mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_SLEW_6,
					 0x3);
	}

	/* For disabled lanes set sr_finished_ovr_en = 1, sr_finished_ovr = 1,
	 * srcal_en_ovr_en = 1, through test control registers.
	 *	0x310 - Clock lane slew-rate calibration override.
	 *	0x50b - lane0 slew-rate calibration override.
	 *	0x70b - lane1 slew-rate calibration override.
	 *	0x90b - lane2 slew-rate calibration override.
	 *	0xb0b - lane3 slew-rate calibration override.
	 */
	if (phy->num_lanes == 1) {
		mipi_dphy_write_register(test_ctrl0, test_ctrl1,
					 dphy4txtester_DIG_RDWR_TX_LANE1_SLEWRATE_0, 0xe);
	}
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_LANE2_SLEWRATE_0,
				 0xe);
	mipi_dphy_write_register(test_ctrl0, test_ctrl1, dphy4txtester_DIG_RDWR_TX_LANE3_SLEWRATE_0,
				 0xe);

	ret = clock_control_get_rate(config->clk_dev, config->dphy_cfgclk, &cfg_clk_freq);
	if (ret < 0) {
		LOG_ERR("Failed to get D-PHY config clock rate: %d", ret);
		return ret;
	}

	cfgclkfreqrange = ((cfg_clk_freq / MHZ(1)) - 17) << 2;
	reg_write_part(regs + TX_DPHY_CTRL0, cfgclkfreqrange, DPHY_CTRL0_CFG_CLK_FREQ_RANGE_MASK,
		       DPHY_CTRL0_CFG_CLK_FREQ_RANGE_SHIFT);

	ret = clock_control_get_rate(config->clk_dev, config->dphy_clkin, &ref_clk_freq);
	if (ret < 0) {
		LOG_ERR("Failed to get D-PHY reference clock rate: %d", ret);
		return ret;
	}

	/*
	 * Find the parameters for the PLL to be written to DPLL config
	 */
	ret = dphy_pll_calculate_mnp(phy->pll_fout, &phy->pll_m, &phy->pll_n, &phy->pll_p,
				     &phy->vco_cntrl, ref_clk_freq);
	if (ret) {
		LOG_ERR("Failed to set the frequency!");
		return ret;
	}

	LOG_DBG("vco_cntrl: 0x%02x M: %d, N: %d P: %d", phy->vco_cntrl, phy->pll_m, phy->pll_n,
		phy->pll_p);

	/* Configure the PLL */
	ret = dphy_dw_config_pll(dev, phy, ref_clk_freq);
	if (ret) {
		LOG_ERR("PLL configuration failed.");
		return ret;
	}

	/* Set Base-direction of the master side D-PHY Lanes to TX. */
	tmp = (1U << phy->num_lanes) - 1;
	sys_clear_bits(regs + TX_DPHY_CTRL0, tmp << DPHY_CTRL0_BASE_DIR_SHIFT);

	/* Clear ForceRx bits to zero per lane. */
	sys_clear_bits(regs + TX_DPHY_CTRL1, tmp << DPHY_CTRL1_FORCE_RX_MODE_SHIFT);

	/*
	 * Enable clock and put D-PHY in start-up.
	 */
	k_busy_wait(1);
	sys_set_bits(dsi_regs + DSI_PHY_RSTZ, DSI_PHY_RSTZ_PHY_ENABLECLK);
	k_busy_wait(1);
	sys_set_bits(dsi_regs + DSI_PHY_RSTZ, DSI_PHY_RSTZ_PHY_SHUTDOWNZ);
	k_busy_wait(1);
	sys_set_bits(dsi_regs + DSI_PHY_RSTZ, DSI_PHY_RSTZ_PHY_RSTZ);

	/* Wait for PLL to lock to the desired frequency. */
	tmp = DSI_PHY_STATUS_PHY_LOCK;
	for (int i = 0; (i < 1000000) && (sys_read32(dsi_regs + DSI_PHY_STATUS) & tmp) != tmp;
	     i++) {
		k_busy_wait(1);
	}

	if ((sys_read32(dsi_regs + DSI_PHY_STATUS) & tmp) != tmp) {
		/* DPLL has still not locked. Return error. */
		LOG_ERR("D-PLL not locked to desired frequency. "
			"DSI Status reg - 0x%08x",
			sys_read32(dsi_regs + DSI_PHY_STATUS));
		return -ETIMEDOUT;
	}

	/* Wait for LP11 state to be driven. */
	tmp = DSI_PHY_STATUS_STOPSTATE0LANE | DSI_PHY_STATUS_STOPSTATECLKLANE;
	tmp = (phy->num_lanes == 2) ? DSI_PHY_STATUS_STOPSTATE1LANE : tmp;
	for (int i = 0; (i < 1000000) && (sys_read32(dsi_regs + DSI_PHY_STATUS) & tmp) != tmp;
	     i++) {
		k_busy_wait(1);
	}

	if ((sys_read32(dsi_regs + DSI_PHY_STATUS) & tmp) != tmp) {
		/* D-PHY has still not come up to Stop state. Return error. */
		LOG_ERR("D-PHY not locked to Stop-State. "
			"DSI Status reg - 0x%08x",
			sys_read32(dsi_regs + DSI_PHY_STATUS));
		return -ETIMEDOUT;
	}

	return 0;
}
