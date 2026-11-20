/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_MIPI_DSI_DESIGNWARE_DSI_DW_PRIV_H_
#define _ZEPHYR_DRIVERS_MIPI_DSI_DESIGNWARE_DSI_DW_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/sys/atomic.h>

#include "dphy_dw.h"

#define DSI_VERSION            0x00  /* HW Version Register */
#define DSI_PWR_UP             0x04  /* Power-up Control Register */
#define DSI_CLKMGR_CFG         0x08  /* Clock Control Register */
#define DSI_DPI_VCID           0x0C  /* VC ID Config Register */
#define DSI_DPI_COLOR_CODING   0x10  /* DPI Color Coding Register */
#define DSI_DPI_CFG_POL        0x14  /* DPI Polarity Config Register */
#define DSI_DPI_LP_CMD_TIM     0x18  /* DPI Low-Power Mode Config Register */
#define DSI_PCKHDL_CFG         0x2C  /* Protocol Config Register */
#define DSI_GEN_VCID           0x30  /* Generic VC-ID Config Register */
#define DSI_MODE_CFG           0x34  /* Mode Config Register */
#define DSI_VID_MODE_CFG       0x38  /* Video Mode Config Register */
#define DSI_VID_PKT_SIZE       0x3C  /* Video Packet Size Register */
#define DSI_VID_NUM_CHUNKS     0x40  /* Video Chunks Config Register */
#define DSI_VID_NULL_SIZE      0x44  /* Video Null Packet Config Register */
#define DSI_VID_HSA_TIME       0x48  /* Video HSA Config Register */
#define DSI_VID_HBP_TIME       0x4C  /* Video HBP Config Register */
#define DSI_VID_HLINE_TIME     0x50  /* Video Line Config Register */
#define DSI_VID_VSA_LINES      0x54  /* Video VSA Config Register */
#define DSI_VID_VBP_LINES      0x58  /* Video VBP Config Register */
#define DSI_VID_VFP_LINES      0x5C  /* Video VFP Config Register */
#define DSI_VID_VACTIVE_LINES  0x60  /* Video VA Config Register */
#define DSI_CMD_MODE_CFG       0x68  /* Generic Packet-cmd Config Register */
#define DSI_GEN_HDR            0x6C  /* Generic Header Config Register */
#define DSI_GEN_PLD_DATA       0x70  /* Generic Payload Data Register */
#define DSI_CMD_PKT_STATUS     0x74  /* Generic Packet Status Register */
#define DSI_TO_CNT_CFG         0x78  /* Timeout Counter Config Register */
#define DSI_HS_RD_TO_CNT       0x7C  /* HS Read Timeout Config Register */
#define DSI_LP_RD_TO_CNT       0x80  /* LP Read Timeout Config Register */
#define DSI_HS_WR_TO_CNT       0x84  /* HS Write Timeout Config Register */
#define DSI_LP_WR_TO_CNT       0x88  /* LP Write Timeout Config Register */
#define DSI_BTA_TO_CNT         0x8C  /* BTA Timeout Config Register */
#define DSI_SDF_3D             0x90  /* 3D Control Register */
#define DSI_LPCLK_CTRL         0x94  /* Clock Lane Power Control Register */
#define DSI_PHY_TMR_LPCLK_CFG  0x98  /* Clock Lane Timer Config Register */
#define DSI_PHY_TMR_CFG        0x9C  /* Data Lane Timer Config Register */
#define DSI_PHY_RSTZ           0xA0  /* PHY Control Register */
#define DSI_PHY_IF_CFG         0xA4  /* PHY Config Register */
#define DSI_PHY_ULPS_CTRL      0xA8  /* PHY ULPS Control Register */
#define DSI_PHY_TX_TRIGGERS    0xAC  /* PHY TX Trigger Config Register */
#define DSI_PHY_STATUS         0xB0  /* PHY Status Register */
#define DSI_PHY_TST_CTRL0      0xB4  /* PHY Test Interface Control Reg 0 */
#define DSI_PHY_TST_CTRL1      0xB8  /* PHY Test Interface Control Reg 1 */
#define DSI_INT_ST0            0xBC  /* Interrupt Status Register 0 */
#define DSI_INT_ST1            0xC0  /* Interrupt Status Register 1 */
#define DSI_INT_MSK0           0xC4  /* Interrupt Mask Register 0 */
#define DSI_INT_MSK1           0xC8  /* Interrupt Mask Register 1 */
#define DSI_PHY_CAL            0xCC  /* PHY Skew Calibration Control */
#define DSI_INT_FORCE0         0xD8  /* Force Interrupt Register 0 */
#define DSI_INT_FORCE1         0xDC  /* Force Interrupt Register 1 */
#define DSI_PHY_TMR_RD_CFG     0xF4  /* Data Lane Timer Read Config Register */
#define DSI_VID_SHADOW_CTRL    0x100 /* Video Shadow Control Register */
#define DSI_DPI_VCID_ACT       0x10C /* Current VC ID Register */
#define DSI_DPI_CLR_CODING_ACT 0x110 /* Current Color Coding Reg */
#define DSI_DPI_LP_CMD_TIM_ACT 0x118 /* LP Mode Current Config Register */
#define DSI_VID_MODE_CFG_ACT   0x138 /* Video Mode Current Config Register */
#define DSI_VID_PKT_SIZE_ACT   0x13C /* Video Pkt Size Current Config Reg */
#define DSI_VID_NUM_CHUNKS_ACT 0x140 /* Video Chunks Current Config Reg */
#define DSI_VID_NULL_SIZE_ACT  0x144 /* Video NULL-PKT Size Curr Config Reg */
#define DSI_VID_HSA_TIME_ACT   0x148 /* Video HSA Current Config Register */
#define DSI_VID_HBP_TIME_ACT   0x14C /* Video HBP Current Config Register */
#define DSI_VID_HLINE_TIME_ACT 0x150 /* Video Line Current Config Register */
#define DSI_VID_VSA_LINES_ACT  0x154 /* Video VSA Current Config Register */
#define DSI_VID_VBP_LINES_ACT  0x158 /* Video VBP Current Config Register */
#define DSI_VID_VFP_LINES_ACT  0x15C /* Video VFP Current Config Register */
#define DSI_VID_VACT_LINES_ACT 0x160 /* Video VA Current Config Register */
#define DSI_VID_PKT_STATUS     0x168 /* Video Pkt Status Register */
#define DSI_SDF_3D_ACT         0x190 /* 3D Current Config Register */

/* Registers Bit-fields */

/* Power-up Control Register */
#define DSI_PWR_UP_SHUTDOWNZ BIT(0)

/* Clock Control Register */
#define DSI_CLKMGR_CFG_TO_CLK_DIV_MASK      GENMASK(7, 0)
#define DSI_CLKMGR_CFG_TO_CLK_DIV_SHIFT     8
#define DSI_CLKMGR_CFG_TX_ESC_CLK_DIV_MASK  GENMASK(7, 0)
#define DSI_CLKMGR_CFG_TX_ESC_CLK_DIV_SHIFT 0

/* VC ID Config Register */
#define DSI_DPI_VCID_VCID_MASK  GENMASK(1, 0)
#define DSI_DPI_VCID_VCID_SHIFT 0

/* DPI Color Coding Register */
#define DSI_DPI_COLOR_CODING_LOOSELY_18_EN BIT(8)
#define DSI_DPI_COLOR_CODING_CLR_MASK      GENMASK(3, 0)
#define DSI_DPI_COLOR_CODING_CLR_SHIFT     0

/* DPI Polarity Config Register */
#define DSI_DPI_CFG_POL_COLM_ACTIVE_LOW   BIT(4)
#define DSI_DPI_CFG_POL_SHUTD_ACTIVE_LOW  BIT(3)
#define DSI_DPI_CFG_POL_HSYNC_ACTIVE_LOW  BIT(2)
#define DSI_DPI_CFG_POL_VSYNC_ACTIVE_LOW  BIT(1)
#define DSI_DPI_CFG_POL_DATAEN_ACTIVE_LOW BIT(0)

/* DPI Low-Power Mode Config Register */
#define DSI_DPI_LP_CMD_TIM_OUTVACT_MASK  GENMASK(7, 0)
#define DSI_DPI_LP_CMD_TIM_OUTVACT_SHIFT 16
#define DSI_DPI_LP_CMD_TIM_INVACT_MASK   GENMASK(7, 0)
#define DSI_DPI_LP_CMD_TIM_INVACT_SHIFT  0

/* Protocol Config Register */
#define DSI_PCKHDL_CFG_EOTP_TX_LP_EN BIT(5)
#define DSI_PCKHDL_CFG_CRC_RX_EN     BIT(4)
#define DSI_PCKHDL_CFG_ECC_RX_EN     BIT(3)
#define DSI_PCKHDL_CFG_BTA_EN        BIT(2)
#define DSI_PCKHDL_CFG_EOTP_RX_EN    BIT(1)
#define DSI_PCKHDL_CFG_EOTP_TX_EN    BIT(0)

/* Generic VC-ID Config Register */
#define DSI_GEN_VCID_TX_AUTO_MASK    GENMASK(1, 0)
#define DSI_GEN_VCID_TX_AUTO_SHIFT   16
#define DSI_GEN_VCID_TEAR_AUTO_MASK  GENMASK(1, 0)
#define DSI_GEN_VCID_TEAR_AUTO_SHIFT 8
#define DSI_GEN_VCID_RX_MASK         GENMASK(1, 0)
#define DSI_GEN_VCID_RX_SHIFT        0

/* Mode Config Register */
/*
 * Video Mode = 0 (only mode supported for this controller)
 * Command Mode = 1
 */
#define DSI_MODE_CFG_VID_MODE 0
#define DSI_MODE_CFG_CMD_MODE BIT(0)

/* Video Mode Config Register */
#define DSI_VID_MODE_CFG_VPG_ORIENTATION  BIT(24)
#define DSI_VID_MODE_CFG_VPG_MODE         BIT(20)
#define DSI_VID_MODE_CFG_VPG_EN           BIT(16)
#define DSI_VID_MODE_CFG_LP_CMD_EN        BIT(15)
#define DSI_VID_MODE_CFG_FRAME_BTA_ACK_EN BIT(14)
#define DSI_VID_MODE_CFG_LP_HFP_EN        BIT(13)
#define DSI_VID_MODE_CFG_LP_HBP_EN        BIT(12)
#define DSI_VID_MODE_CFG_LP_VACT_EN       BIT(11)
#define DSI_VID_MODE_CFG_LP_VFP_EN        BIT(10)
#define DSI_VID_MODE_CFG_LP_VBP_EN        BIT(9)
#define DSI_VID_MODE_CFG_LP_VSA_EN        BIT(8)
#define DSI_VID_MODE_CFG_MODE_TYPE_MASK   GENMASK(1, 0)
#define DSI_VID_MODE_CFG_MODE_TYPE_SHIFT  0

/* Video Packet Size Register */
#define DSI_VID_PKT_SIZE_MASK  GENMASK(13, 0)
#define DSI_VID_PKT_SIZE_SHIFT 0

/* Video Chunks Config Register */
#define DSI_VID_NUM_CHUNKS_MASK  GENMASK(12, 0)
#define DSI_VID_NUM_CHUNKS_SHIFT 0

/* Video Null Packet Config Register */
#define DSI_VID_NULL_SIZE_MASK  GENMASK(12, 0)
#define DSI_VID_NULL_SIZE_SHIFT 0

/* Video HSA Config Register */
#define DSI_VID_HSA_TIME_MASK  GENMASK(11, 0)
#define DSI_VID_HSA_TIME_SHIFT 0

/* Video HBP Config Register */
#define DSI_VID_HBP_TIME_MASK  GENMASK(11, 0)
#define DSI_VID_HBP_TIME_SHIFT 0

/* Video Line Config Register */
#define DSI_VID_HLINE_TIME_MASK  GENMASK(14, 0)
#define DSI_VID_HLINE_TIME_SHIFT 0

/* Video VSA Config Register */
#define DSI_VID_VSA_LINES_MASK  GENMASK(9, 0)
#define DSI_VID_VSA_LINES_SHIFT 0

/* Video VBP Config Register */
#define DSI_VID_VBP_LINES_MASK  GENMASK(9, 0)
#define DSI_VID_VBP_LINES_SHIFT 0

/* Video VFP Config Register */
#define DSI_VID_VFP_LINES_MASK  GENMASK(9, 0)
#define DSI_VID_VFP_LINES_SHIFT 0

/* Video VA Config Register */
#define DSI_VID_VACTIVE_LINES_MASK  GENMASK(13, 0)
#define DSI_VID_VACTIVE_LINES_SHIFT 0

/* Generic Packet-cmd Config Register */
#define DSI_CMD_MODE_CFG_MAX_RD_PKT_SIZE BIT(24)
#define DSI_CMD_MODE_CFG_DCS_LW_TX       BIT(19)
#define DSI_CMD_MODE_CFG_DCS_SR_0P_TX    BIT(18)
#define DSI_CMD_MODE_CFG_DCS_SW_1P_TX    BIT(17)
#define DSI_CMD_MODE_CFG_DCS_SW_0P_TX    BIT(16)
#define DSI_CMD_MODE_CFG_GEN_LW_TX       BIT(14)
#define DSI_CMD_MODE_CFG_GEN_SR_2P_TX    BIT(13)
#define DSI_CMD_MODE_CFG_GEN_SR_1P_TX    BIT(12)
#define DSI_CMD_MODE_CFG_GEN_SR_0P_TX    BIT(11)
#define DSI_CMD_MODE_CFG_GEN_SW_2P_TX    BIT(10)
#define DSI_CMD_MODE_CFG_GEN_SW_1P_TX    BIT(9)
#define DSI_CMD_MODE_CFG_GEN_SW_0P_TX    BIT(8)
#define DSI_CMD_MODE_CFG_ACK_RQST_EN     BIT(1)

/* Generic Header Config Register */
#define DSI_GEN_HDR_WC_MSBYTE_MASK  GENMASK(7, 0)
#define DSI_GEN_HDR_WC_MSBYTE_SHIFT 16
#define DSI_GEN_HDR_WC_LSBYTE_MASK  GENMASK(7, 0)
#define DSI_GEN_HDR_WC_LSBYTE_SHIFT 8
#define DSI_GEN_HDR_VC_MASK         GENMASK(1, 0)
#define DSI_GEN_HDR_VC_SHIFT        6
#define DSI_GEN_HDR_DT_MASK         GENMASK(5, 0)
#define DSI_GEN_HDR_DT_SHIFT        0

/* Generic Payload Data Register */
#define DSI_GEN_PLD_DATA_B4_MASK  GENMASK(7, 0)
#define DSI_GEN_PLD_DATA_B4_SHIFT 24
#define DSI_GEN_PLD_DATA_B3_MASK  GENMASK(7, 0)
#define DSI_GEN_PLD_DATA_B3_SHIFT 16
#define DSI_GEN_PLD_DATA_B2_MASK  GENMASK(7, 0)
#define DSI_GEN_PLD_DATA_B2_SHIFT 8
#define DSI_GEN_PLD_DATA_B1_MASK  GENMASK(7, 0)
#define DSI_GEN_PLD_DATA_B1_SHIFT 0

/* Generic Packet Status Register */
#define DSI_CMD_PKT_STATUS_GEN_BUFF_PLD_FULL  BIT(19)
#define DSI_CMD_PKT_STATUS_GEN_BUFF_PLD_EMPTY BIT(18)
#define DSI_CMD_PKT_STATUS_GEN_BUFF_CMD_FULL  BIT(17)
#define DSI_CMD_PKT_STATUS_GEN_BUFF_CMD_EMPTY BIT(16)
#define DSI_CMD_PKT_STATUS_GEN_RD_CMD_BUSY    BIT(6)
#define DSI_CMD_PKT_STATUS_GEN_PLD_R_FULL     BIT(5)
#define DSI_CMD_PKT_STATUS_GEN_PLD_R_EMPTY    BIT(4)
#define DSI_CMD_PKT_STATUS_GEN_PLD_W_FULL     BIT(3)
#define DSI_CMD_PKT_STATUS_GEN_PLD_W_EMPTY    BIT(2)
#define DSI_CMD_PKT_STATUS_GEN_CMD_FULL       BIT(1)
#define DSI_CMD_PKT_STATUS_GEN_CMD_EMPTY      BIT(0)

/* Timeout Counter Config Register */
#define DSI_TO_CNT_CFG_HSTX_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_TO_CNT_CFG_HSTX_TO_CNT_SHIFT 16
#define DSI_TO_CNT_CFG_LPRX_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_TO_CNT_CFG_LPRX_TO_CNT_SHIFT 0

/* HS Read Timeout Config Register */
#define DSI_HS_RD_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_HS_RD_TO_CNT_SHIFT 0

/* LP Read Timeout Config Register */
#define DSI_LP_RD_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_LP_RD_TO_CNT_SHIFT 0

/* HS Write Timeout Config Register */
#define DSI_HS_WR_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_HS_WR_TO_CNT_SHIFT 0

/* LP Write Timeout Config Register */
#define DSI_LP_WR_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_LP_WR_TO_CNT_SHIFT 0

/* BTA Timeout Config Register */
#define DSI_BTA_TO_CNT_MASK  GENMASK(15, 0)
#define DSI_BTA_TO_CNT_SHIFT 0

/* 3D Control Register */
#define DSI_SDF_3D_SEND_3D_CFG  BIT(16)
#define DSI_SDF_3D_RIGHT_FIRST  BIT(5)
#define DSI_SDF_3D_SECOND_VSYNC BIT(4)
#define DSI_SDF_3D_FORMAT_MASK  GENMASK(1, 0)
#define DSI_SDF_3D_FORMAT_SHIFT 2
#define DSI_SDF_3D_MODE_MASK    GENMASK(1, 0)
#define DSI_SDF_3D_MODE_SHIFT   0

/* Clock Lane Power Control Register */
#define DSI_LPCLK_CTRL_AUTO_CLKLN_CTRL    BIT(1)
#define DSI_LPCLK_CTRL_PHY_TXREQUESTCLKHS BIT(0)

/* Clock Lane Timer Config Register */
#define DSI_PHY_TMR_LPCLK_CFG_HS2LP_MASK  GENMASK(9, 0)
#define DSI_PHY_TMR_LPCLK_CFG_HS2LP_SHIFT 16
#define DSI_PHY_TMR_LPCLK_CFG_LP2HS_MASK  GENMASK(9, 0)
#define DSI_PHY_TMR_LPCLK_CFG_LP2HS_SHIFT 0

/* Data Lane Timer Config Register */
#define DSI_PHY_TMR_CFG_HS2LP_MASK  GENMASK(9, 0)
#define DSI_PHY_TMR_CFG_HS2LP_SHIFT 16
#define DSI_PHY_TMR_CFG_LP2HS_MASK  GENMASK(9, 0)
#define DSI_PHY_TMR_CFG_LP2HS_SHIFT 0

/* PHY Control Register */
#define DSI_PHY_RSTZ_FORCEPLL  BIT(3)
#define DSI_PHY_RSTZ_ENABLECLK BIT(2)
#define DSI_PHY_RSTZ_RSTZ      BIT(1)
#define DSI_PHY_RSTZ_SHUTDOWNZ BIT(0)

/* PHY Config Register */
#define DSI_PHY_IF_CFG_STOP_WAIT_TIME_MASK  GENMASK(7, 0)
#define DSI_PHY_IF_CFG_STOP_WAIT_TIME_SHIFT 8
#define DSI_PHY_IF_CFG_N_LANES_MASK         GENMASK(1, 0)
#define DSI_PHY_IF_CFG_N_LANES_SHIFT        0

/* PHY ULPS Control Register */
#define DSI_PHY_ULPS_CTRL_EXIT_DATA BIT(3)
#define DSI_PHY_ULPS_CTRL_REQ_DATA  BIT(2)
#define DSI_PHY_ULPS_CTRL_EXIT_CLK  BIT(1)
#define DSI_PHY_ULPS_CTRL_REQ_CLK   BIT(0)

/* PHY TX Trigger Config Register */
#define DSI_PHY_TX_TRIGGERS_MASK  GENMASK(3, 0)
#define DSI_PHY_TX_TRIGGERS_SHIFT 0

/* PHY Status Register */
#define DSI_PHY_STATUS_PHY_ULPSACTIVENOT1LANE BIT(8)
#define DSI_PHY_STATUS_PHY_STOPSTATE1LANE     BIT(7)
#define DSI_PHY_STATUS_PHY_RXULPSESC0LANE     BIT(6)
#define DSI_PHY_STATUS_PHY_ULPSACTIVENOT0LANE BIT(5)
#define DSI_PHY_STATUS_PHY_STOPSTATE0LANE     BIT(4)
#define DSI_PHY_STATUS_PHY_ULPSACTIVENOTCLK   BIT(3)
#define DSI_PHY_STATUS_PHY_STOPSTATECLKLANE   BIT(2)
#define DSI_PHY_STATUS_PHY_PHY_DIRECTION      BIT(1)
#define DSI_PHY_STATUS_PHY_PHY_LOCK           BIT(0)

/* PHY Test Interface Control Reg 0 */
#define DSI_PHY_TST_CTRL0_CLK BIT(1)
#define DSI_PHY_TST_CTRL0_CLR BIT(0)

/* PHY Test Interface Control Reg 1 */
#define DSI_PHY_TST_CTRL1_TESTEN         BIT(16)
#define DSI_PHY_TST_CTRL1_TESTDOUT_MASK  GENMASK(7, 0)
#define DSI_PHY_TST_CTRL1_TESTDOUT_SHIFT 8
#define DSI_PHY_TST_CTRL1_TESTDIN_MASK   GENMASK(7, 0)
#define DSI_PHY_TST_CTRL1_TESTDIN_SHIFT  0

/* Interrupt Mask, Status and Force 0 Reg Bit-fields */
#define DSI_INT_0_DPHY_ERR_4      BIT(20)
#define DSI_INT_0_DPHY_ERR_3      BIT(19)
#define DSI_INT_0_DPHY_ERR_2      BIT(18)
#define DSI_INT_0_DPHY_ERR_1      BIT(17)
#define DSI_INT_0_DPHY_ERR_0      BIT(16)
#define DSI_INT_0_ACK_WITH_ERR_15 BIT(15)
#define DSI_INT_0_ACK_WITH_ERR_14 BIT(14)
#define DSI_INT_0_ACK_WITH_ERR_13 BIT(13)
#define DSI_INT_0_ACK_WITH_ERR_12 BIT(12)
#define DSI_INT_0_ACK_WITH_ERR_11 BIT(11)
#define DSI_INT_0_ACK_WITH_ERR_10 BIT(10)
#define DSI_INT_0_ACK_WITH_ERR_9  BIT(9)
#define DSI_INT_0_ACK_WITH_ERR_8  BIT(8)
#define DSI_INT_0_ACK_WITH_ERR_7  BIT(7)
#define DSI_INT_0_ACK_WITH_ERR_6  BIT(6)
#define DSI_INT_0_ACK_WITH_ERR_5  BIT(5)
#define DSI_INT_0_ACK_WITH_ERR_4  BIT(4)
#define DSI_INT_0_ACK_WITH_ERR_3  BIT(3)
#define DSI_INT_0_ACK_WITH_ERR_2  BIT(2)
#define DSI_INT_0_ACK_WITH_ERR_1  BIT(1)
#define DSI_INT_0_ACK_WITH_ERR_0  BIT(0)

/* Interrupt Mask, Status and Force 1 Reg Bit-fields */
#define DSI_INT_1_DPI_BUFF_PLD_UNDER BIT(19)
#define DSI_INT_1_GEN_PLD_RECEV_ERR  BIT(12)
#define DSI_INT_1_GEN_PLD_RD_ERR     BIT(11)
#define DSI_INT_1_GEN_PLD_SEND_ERR   BIT(10)
#define DSI_INT_1_GEN_PLD_WR_ERR     BIT(9)
#define DSI_INT_1_GEN_CMD_WR_ERR     BIT(8)
#define DSI_INT_1_DPI_PLD_WR_ERR     BIT(7)
#define DSI_INT_1_EOTP_ERR           BIT(6)
#define DSI_INT_1_PKT_SIZE_ERR       BIT(5)
#define DSI_INT_1_CRC_ERR            BIT(4)
#define DSI_INT_1_ECC_MULTI_ERR      BIT(3)
#define DSI_INT_1_ECC_SINGLE_ERR     BIT(2)
#define DSI_INT_1_TO_LP_RX           BIT(1)
#define DSI_INT_1_TO_HP_TX           BIT(0)

/* PHY Skew Calibration Control */
#define DSI_PHY_CAL_TX_SKEW_CAL_HS BIT(0)

/* Data Lane Timer Read Config Register */
#define DSI_PHY_TMR_RD_CFG_MAX_RD_TIME_MASK  GENMASK(14, 0)
#define DSI_PHY_TMR_RD_CFG_MAX_RD_TIME_SHIFT 0

/* Video Shadow Control Register */
#define DSI_VID_SHADOW_CTRL_REQ BIT(8)
#define DSI_VID_SHADOW_CTRL_EN  BIT(0)

/*
 * MIPI-DSI Host controller configurations
 */
/* PHY parameters. */
#define PHY_STOP_WAIT_TIME 0x20

/* Time to return read data packet from peripheral. */
#define TO_CLK_DIV  10
#define LPRX_TO_CNT 1000
#define BTA_TO_CNT  0xd00

/* Read-only Frame-rate value to be used for clock calculations. */
#define DPI_FRAME_RATE            60
#define DSI_HS_CLK_SCALING_FACTOR (4.0f / 3.0f)
#define MAX_ESC_CLK               MHZ(20)
enum sdf_format {
	SDF_FORMAT_MUXED_LINES = 0x0,
	SDF_FORMAT_MUXED_FRAMES = 0x1,
	SDF_FORMAT_MUXED_PIXELS = 0x2,
	SDF_FORMAT_MUXED_RESERVED = 0x3,
};

enum sdf_mode {
	SDF_MODE_2D = 0x0,
	SDF_MODE_3D_PORTRAIT = 0x1,
	SDF_MODE_3D_LANDSCAPE = 0x2,
	SDF_MODE_3D_RESERVED = 0x3,
};

enum dpi_color_code {
	DPI_COLOR_CODE_16B_CONFIG_2 = 0x1,
	DPI_COLOR_CODE_18B_CONFIG_2 = 0x4,
	DPI_COLOR_CODE_24B = 0x5,
	DPI_COLOR_CODE_UNDEFINED = 0x10,
};

enum dpi_vid_pattern_gen {
	DPI_VID_PATTERN_GEN_VERT_COLORBAR = 0,
	DPI_VID_PATTERN_GEN_HORIZ_COLORBAR = 1,
	DPI_VID_PATTERN_GEN_VERT_BER = 2,
	DPI_VID_PATTERN_GEN_NONE = 3,
};

enum dpi_vid_mode_type {
	DPI_VID_MODE_NON_BURST_SYNC_PULSES = 0,
	DPI_VID_MODE_NON_BURST_SYNC_EVENTS = 1,
	DPI_VID_MODE_BURST_0 = 2,
	DPI_VID_MODE_BURST_1 = 3,
};

enum dsi_dw_mode {
	DSI_DW_VIDEO_MODE = 0,
	DSI_DW_COMMAND_MODE = 1
};

struct dpi_config {
	/* Video signals polarity */
	uint32_t polarity;

	/* Video pattern generator status. */
	enum dpi_vid_pattern_gen vpg_pattern;
};

struct dsi_dw_config {
	DEVICE_MMIO_NAMED_ROM(dsi_regs);
	DEVICE_MMIO_NAMED_ROM(clkctl_regs);

	void (*irq_config_func)(const struct device *dev);

	uint32_t irq;
	uint32_t max_bit_clk;
	struct dpi_config dpi;
	const struct device *clk_dev;
	clock_control_subsys_t pclk;
	clock_control_subsys_t pixel_clk;
	clock_control_subsys_t dphy_clkin;
	clock_control_subsys_t dphy_clkext;
	clock_control_subsys_t dphy_cfgclk;

	/* Allow EoTp Transmission/Reception in HS/LP mode. */
	uint32_t eotp_lp_tx: 1;
	uint32_t eotp_rx: 1;

	uint32_t ecc_recv_en: 1;
	uint32_t crc_recv_en: 1;

	/* Request ACK at the end of a frame. */
	uint32_t frame_ack_en: 1;
};

struct dsi_dw_data {
	DEVICE_MMIO_NAMED_RAM(dsi_regs);
	DEVICE_MMIO_NAMED_RAM(clkctl_regs);
	struct dphy_dsi_settings phy;

	/* Cached Clock values. */
	double clk_scale;
	uint32_t dpi_pix_clk;
	uint32_t lane_byte_clk;
	uint8_t esc_clk_div;

	/* calculated OUTVACT, INVACT and MAX_RD_TIME */
	uint32_t outvact;
	uint32_t invact;
	uint32_t max_rd_time;
	enum dsi_dw_mode curr_mode;

	/* null packet config */
	uint32_t num_chunks;
	uint32_t null_size;
	uint32_t pkt_size;

	uint32_t mode_flags;

	/* Bitmask of DSI error classes already reported by dsi_dw_irq(). A
	 * missing or unresponsive panel raises the same error every interrupt
	 * (e.g. an LP-RX timeout every few milliseconds), so each class is
	 * logged once instead of flooding the console. Set from the ISR and
	 * cleared from dsi_dw_attach() (thread context) so a fresh attach
	 * reports again; atomic_t makes that cross-context access well-defined.
	 * See DSI_ERR_LOGGED_* in dsi_dw.c.
	 */
	atomic_t err_logged;
};

/*
 * ISR handler. Should be installed from platform-specific glue code
 */
void dsi_dw_irq(const struct device *dev);

/*
 * Attach implementation for the DSI DW driver.
 */
int dsi_dw_attach(const struct device *dev, uint8_t channel, const struct mipi_dsi_device *mdev);

/*
 * Transfer implementation for the DSI DW driver.
 */
ssize_t dsi_dw_transfer(const struct device *dev, uint8_t channel, struct mipi_dsi_msg *msg);

#define DEV_DATA(dev) ((struct dsi_dw_data *)((dev)->data))
#define DEV_CFG(dev)  ((struct dsi_dw_config *)((dev)->config))

/* Helper functions */
static inline void dsi_dw_pwr_down(uintptr_t regs)
{
	sys_clear_bits(regs + DSI_PWR_UP, DSI_PWR_UP_SHUTDOWNZ);
}

static inline void dsi_dw_pwr_up(uintptr_t regs)
{
	sys_set_bits(regs + DSI_PWR_UP, DSI_PWR_UP_SHUTDOWNZ);
}

#endif /* _ZEPHYR_DRIVERS_MIPI_DSI_DESIGNWARE_DSI_DW_PRIV_H_ */
