/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DPHY_DW_H_
#define _DPHY_DW_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>

/* DPHY TX Regs */
#define dphy4txtester_DIG_RDWR_TX_SYS_3            0x004
#define dphy4txtester_DIG_RD_TX_SYS_0              0x01e
#define dphy4txtester_DIG_RD_TX_SYS_1              0x01f
#define dphy4txtester_DIG_RDWR_TX_CB_0             0x1aa
#define dphy4txtester_DIG_RDWR_TX_CB_1             0x1ab
#define dphy4txtester_DIG_RDWR_TX_CB_2             0x1ac
#define dphy4txtester_DIG_RDWR_TX_CB_3             0x1ad
#define dphy4txtester_DIG_RDWR_TX_DAC_0            0x1da
#define dphy4txtester_DIG_RD_TX_DAC_0              0x1f2
#define dphy4txtester_DIG_RDWR_TX_SLEW_0           0x26b
#define dphy4txtester_DIG_RDWR_TX_SLEW_5           0x270
#define dphy4txtester_DIG_RDWR_TX_SLEW_6           0x271
#define dphy4txtester_DIG_RDWR_TX_SLEW_7           0x272
#define dphy4txtester_DIG_RDWR_TX_CLK_TERMLOWCAP   0x402
#define dphy4txtester_DIG_RDWR_TX_LANE0_LANE_0     0x501
#define dphy4txtester_DIG_RDWR_TX_LANE1_LANE_0     0x701
#define dphy4txtester_DIG_RDWR_TX_LANE1_SLEWRATE_0 0x70b
#define dphy4txtester_DIG_RDWR_TX_LANE2_SLEWRATE_0 0x90b
#define dphy4txtester_DIG_RDWR_TX_LANE3_SLEWRATE_0 0xb0b

/* TX PLL register index */
#define dphy4txtester_DIG_RDWR_TX_PLL_0  0x15d
#define dphy4txtester_DIG_RDWR_TX_PLL_1  0x15e
#define dphy4txtester_DIG_RDWR_TX_PLL_5  0x162
#define dphy4txtester_DIG_RDWR_TX_PLL_9  0x166
#define dphy4txtester_DIG_RDWR_TX_PLL_10 0x167
#define dphy4txtester_DIG_RDWR_TX_PLL_13 0x16a
#define dphy4txtester_DIG_RDWR_TX_PLL_17 0x16e
#define dphy4txtester_DIG_RDWR_TX_PLL_27 0x178
#define dphy4txtester_DIG_RDWR_TX_PLL_28 0x179
#define dphy4txtester_DIG_RDWR_TX_PLL_29 0x17a
#define dphy4txtester_DIG_RDWR_TX_PLL_30 0x17b
#define dphy4txtester_DIG_RD_TX_PLL_0    0x191

/* DPHY RX Regs */
#define dphy4rxtester_DIG_RD_RX_SYS_1                0x01f
#define dphy4rxtester_DIG_RDWR_RX_RX_STARTUP_OVR_2   0x0e2
#define dphy4rxtester_DIG_RDWR_RX_RX_STARTUP_OVR_3   0x0e3
#define dphy4rxtester_DIG_RDWR_RX_RX_STARTUP_OVR_4   0x0e4
#define dphy4rxtester_DIG_RDWR_RX_RX_STARTUP_OVR_17  0x0f1
#define dphy4rxtester_DIG_RDWR_RX_BIST_3             0x10a
#define dphy4rxtester_DIG_RDWR_RX_CLKLANE_LANE_6     0x307
#define dphy4rxtester_DIG_RD_RX_CLKLANE_OFFSET_CAL_3 0x39c
#define dphy2rxtester_DIG_RD_RX_CLKLANE_OFFSET_CAL_0 0x39d
#define dphy4rxtester_DIG_RDWR_RX_LANE0_LANE_9       0x50a
#define dphy4rxtester_DIG_RDWR_RX_LANE0_LANE_12      0x50d
#define dphy4rxtester_DIG_RD_RX_LANE0_LANE_7         0x532
#define dphy4rxtester_DIG_RD_RX_LANE0_LANE_8         0x533
#define dphy2rxtester_DIG_RD_RX_LANE0_OFFSET_CAL_0   0x58d
#define dphy2rxtester_DIG_RD_RX_LANE0_OFFSET_CAL_2   0x5a1
#define dphy2rxtester_DIG_RD_RX_LANE0_DDL_0          0x5e0
#define dphy2rxtester_DIG_RD_RX_LANE0_DDL_5          0x5e5
#define dphy4rxtester_DIG_RDWR_RX_LANE1_LANE_9       0x70a
#define dphy4rxtester_DIG_RDWR_RX_LANE1_LANE_12      0x70d
#define dphy4rxtester_DIG_RD_RX_LANE1_LANE_7         0x732
#define dphy4rxtester_DIG_RD_RX_LANE1_LANE_8         0x733
#define dphy2rxtester_DIG_RD_RX_LANE1_OFFSET_CAL_0   0x79f
#define dphy2rxtester_DIG_RD_RX_LANE1_OFFSET_CAL_2   0x7a1
#define dphy4rxtester_DIG_RD_RX_LANE1_DDL_0          0x7e0
#define dphy2rxtester_DIG_RD_RX_LANE1_DDL_5          0x7e5

/* D-PHY SoC Registers */
#define DPHY_PLL_CTRL0 0x10
#define DPHY_PLL_CTRL1 0x14
#define DPHY_PLL_CTRL2 0x18
#define DPHY_PLL_STAT0 0x20
#define DPHY_PLL_STAT1 0x24
#define TX_DPHY_CTRL0  0x30
#define TX_DPHY_CTRL1  0x34
#define RX_DPHY_CTRL0  0x38
#define RX_DPHY_CTRL1  0x3c
#define DSI_CTRL       0x44

/* D-PHY DSI Registers */
#define DSI_PHY_RSTZ      0xA0
#define DSI_PHY_IF_CFG    0xA4
#define DSI_PHY_STATUS    0xB0
#define DSI_PHY_TST_CTRL0 0xB4
#define DSI_PHY_TST_CTRL1 0xB8

/* D-PHY CSI-2 Registers */
#define CSI_PHY_SHUTDOWNZ  0x40
#define CSI_DPHY_RSTZ      0x44
#define CSI_PHY_RX         0x48
#define CSI_PHY_STOPSTATE  0x4C
#define CSI_PHY_TEST_CTRL0 0x50
#define CSI_PHY_TEST_CTRL1 0x54

/* D-PHY mux bits. */
#define DSI_CTRL_CAM2_EN BIT(9)

/* D-PHY DSI Registers bit-field */
#define DSI_PHY_RSTZ_PHY_FORCEPLL               BIT(3)
#define DSI_PHY_RSTZ_PHY_ENABLECLK              BIT(2)
#define DSI_PHY_RSTZ_PHY_RSTZ                   BIT(1)
#define DSI_PHY_RSTZ_PHY_SHUTDOWNZ              BIT(0)
#define DSI_PHY_IF_CFG_PHY_STOP_WAIT_TIME_MASK  GENMASK(7, 0)
#define DSI_PHY_IF_CFG_PHY_STOP_WAIT_TIME_SHIFT 0
#define DSI_PHY_IF_CFG_PHY_N_LANES_MASK         GENMASK(1, 0)
#define DSI_PHY_IF_CFG_PHY_N_LANES_SHIFT        0
#define DSI_PHY_STATUS_ULPSACTIVENOT1LANE       BIT(8)
#define DSI_PHY_STATUS_STOPSTATE1LANE           BIT(7)
#define DSI_PHY_STATUS_ULPSESC0LANE             BIT(6)
#define DSI_PHY_STATUS_ULPSACTIVENOT0LANE       BIT(5)
#define DSI_PHY_STATUS_STOPSTATE0LANE           BIT(4)
#define DSI_PHY_STATUS_ULPSACTIVENOTCLK         BIT(3)
#define DSI_PHY_STATUS_STOPSTATECLKLANE         BIT(2)
#define DSI_PHY_STATUS_PHY_DIRECTION            BIT(1)
#define DSI_PHY_STATUS_PHY_LOCK                 BIT(0)

/* D-PHY CSI-2 Registers bit-field */
#define CSI_PHY_SHUTDOWNZ_PHY_SHUTDOWNZ       BIT(0)
#define CSI_DPHY_RSTZ_DPHY_RSTZ               BIT(0)
#define CSI_PHY_RX_PHY_RXCLKACTIVEHS          BIT(17)
#define CSI_PHY_RX_PHY_RXULPSCLKNOT           BIT(16)
#define CSI_PHY_RX_PHY_RXULPSESC_1            BIT(1)
#define CSI_PHY_RX_PHY_RXULPSESC_0            BIT(0)
#define CSI_PHY_STOPSTATE_PHY_STOPSTATECLK    BIT(16)
#define CSI_PHY_STOPSTATE_PHY_STOPSTATEDATA_1 BIT(1)
#define CSI_PHY_STOPSTATE_PHY_STOPSTATEDATA_0 BIT(0)

/* D-PHY SoC Registers bit-field */
/* D-PHY PLL Control Register 0 */
#define DPHY_PLL_CTRL0_GP_CLK_EN      BIT(24)
#define DPHY_PLL_CTRL0_CLKSEL_MASK    GENMASK(1, 0)
#define DPHY_PLL_CTRL0_CLKSEL_SHIFT   20
#define DPHY_PLL_CTRL0_GMP_CTRL_MASK  GENMASK(1, 0)
#define DPHY_PLL_CTRL0_GMP_CTRL_SHIFT 16
#define DPHY_PLL_CTRL0_SHADOW_CLR     BIT(12)
#define DPHY_PLL_CTRL0_UPDATE_PLL     BIT(8)
#define DPHY_PLL_CTRL0_SHADOW_CONTROL BIT(4)
#define DPHY_PLL_CTRL0_FORCE_LOCK     BIT(0)

/* D-PHY PLL Control Register 1 */
#define DPHY_PLL_CTRL1_PLL_N_MASK  GENMASK(3, 0)
#define DPHY_PLL_CTRL1_PLL_N_SHIFT 12
#define DPHY_PLL_CTRL1_PLL_M_MASK  GENMASK(9, 0)
#define DPHY_PLL_CTRL1_PLL_M_SHIFT 0

/* D-PHY PLL Control Register 2 */
#define DPHY_PLL_CTRL2_VCO_CTRL_MASK     GENMASK(5, 0)
#define DPHY_PLL_CTRL2_VCO_CTRL_SHIFT    24
#define DPHY_PLL_CTRL2_PROP_CTRL_MASK    GENMASK(5, 0)
#define DPHY_PLL_CTRL2_PROP_CTRL_SHIFT   16
#define DPHY_PLL_CTRL2_INT_CTRL_MASK     GENMASK(5, 0)
#define DPHY_PLL_CTRL2_INT_CTRL_SHIFT    8
#define DPHY_PLL_CTRL2_CPBIAS_CTRL_MASK  GENMASK(6, 0)
#define DPHY_PLL_CTRL2_CPBIAS_CTRL_SHIFT 0

/* D-PHY Control Register 0 */
#define DPHY_CTRL0_CFG_CLK_FREQ_RANGE_MASK  GENMASK(7, 0)
#define DPHY_CTRL0_CFG_CLK_FREQ_RANGE_SHIFT 24
#define DPHY_CTRL0_HS_FREQ_RANGE_MASK       GENMASK(6, 0)
#define DPHY_CTRL0_HS_FREQ_RANGE_SHIFT      16
#define DPHY_CTRL0_BASE_DIR_MASK            GENMASK(1, 0)
#define DPHY_CTRL0_BASE_DIR_SHIFT           12
#define DPHY_CTRL0_TXRXZ                    BIT(8)
#define DPHY_CTRL0_TESTPORT_SEL             BIT(4)
#define DPHY_CTRL0_CONT_EN                  BIT(3)
#define DPHY_CTRL0_BIST_OK                  BIT(2)
#define DPHY_CTRL0_BIST_DONE                BIT(1)
#define DPHY_CTRL0_BIST_ON                  BIT(0)

/* D-PHY Control Register 1 */
#define DPHY_CTRL1_TURN_REQ_MASK            GENMASK(1, 0)
#define DPHY_CTRL1_TURN_REQ_SHIFT           6
#define DPHY_CTRL1_TURN_DIS_MASK            GENMASK(1, 0)
#define DPHY_CTRL1_TURN_DIS_SHIFT           4
#define DPHY_CTRL1_FORCE_TX_STOP_MODE_MASK  GENMASK(1, 0)
#define DPHY_CTRL1_FORCE_TX_STOP_MODE_SHIFT 2
#define DPHY_CTRL1_FORCE_RX_MODE_MASK       GENMASK(1, 0)
#define DPHY_CTRL1_FORCE_RX_MODE_SHIFT      0

/* PHY Test Interface Control Reg 1 bit-field */
#define PHY_TST_CTRL0_CLK BIT(1)
#define PHY_TST_CTRL0_CLR BIT(0)

/* PHY Test Interface Control Reg 1 bit-field */
#define PHY_TST_CTRL1_TESTEN         BIT(16)
#define PHY_TST_CTRL1_TESTDOUT_MASK  GENMASK(7, 0)
#define PHY_TST_CTRL1_TESTDOUT_SHIFT 8
#define PHY_TST_CTRL1_TESTDIN_MASK   GENMASK(7, 0)
#define PHY_TST_CTRL1_TESTDIN_SHIFT  0

/* Charge-pump Programmability */
#define DPHY_CPBIAS_CNTRL            0
#define DPHY_GMP_CNTRL               0x01
#define DPHY_INT_CNTRL               0x04
#define DPHY_PROP_CNTRL              0x10
#define DPHY_CB_VREF_MPLL_REG_REL_RW 0x02

/* Oscillation target for slew rate calibration */
#define DPHY_LESS_THEN_1GBPS_SR_OSC_FREQ_TARGET 657UL
#define DPHY_MORE_THEN_1GBPS_SR_OSC_FREQ_TARGET 920UL

struct dphy_freq_range {
	uint16_t bitrate_in_mbps;
	uint8_t hsfreqrange;
	uint16_t osc_freq_target;
	uint16_t clk_lp2hs;
	uint16_t clk_hs2lp;
	uint16_t lane_lp2hs;
	uint16_t lane_hs2lp;
};

struct dphy_pll_vco_ctrl {
	float min_freq;
	float max_freq;
	uint8_t vco_ctrl;
};

/*
 * vco_cntrl[5:3] dictate the output division factor
 *	vco_cntrl[5:3]	Output Division Factor
 *	3b'000			2
 *	3b'001			4
 *	3b'010			8
 *	3b'011			16
 *	3b'100			32
 *	3b'101			64
 */
#define PLL_OUT_DIV_FACTOR(vco_cntrl) (1 << ((0x7 & (vco_cntrl >> 3)) + 1))

/* Utility functions. */
#define CEIL(x)  (((x) - (uint32_t)(x)) > 0 ? ((uint32_t)(x) + 1) : (uint32_t)(x))
#define ROUND(x) (uint32_t)((double)(x) + 0.5)

struct dphy_dsi_settings {
	/* Number of lanes in D-PHY. */
	uint8_t num_lanes;
	/* Output PLL frequency. */
	uint32_t pll_fout;
	/* Clk Lane HS->LP and LP->HS timings. */
	uint16_t clk_hs2lp;
	uint16_t clk_lp2hs;
	/* Data Lane HS->LP and LP->HS timings. */
	uint16_t lane_hs2lp;
	uint16_t lane_lp2hs;

	uint16_t pll_m;
	uint16_t pll_n;
	uint8_t pll_p;
	uint8_t vco_cntrl;
};

struct dphy_csi2_settings {
	/* Number of lanes in D-PHY. */
	uint8_t num_lanes;
	/* Input PLL frequency. */
	uint32_t pll_fin;
};

/*
 * Setup the D-PHY as TX-PHY. dev is the DSI host controller device that
 * owns the D-PHY.
 */
int dphy_dw_master_setup(const struct device *dev, struct dphy_dsi_settings *phy);

/*
 * Setup the D-PHY as RX-PHY.
 */
int dphy_dw_slave_setup(const struct device *dev, struct dphy_csi2_settings *phy, uint8_t dphy_id);

int dphy_dw_slave_select(const struct device *dev, uint8_t slave_id);

#endif /* _DPHY_DW_H_ */
