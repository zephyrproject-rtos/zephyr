/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The driver handles one camera: one GMSL input in pixel mode, video pipe 0 and one D-PHY CSI-2
 * output.
 */

#define DT_DRV_COMPAT adi_max96724

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/video/controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_max96724, CONFIG_VIDEO_LOG_LEVEL);

#define MAX96724_PROBE_TIMEOUT_MS  1000
#define MAX96724_PROBE_INTERVAL_MS 5

/* Link-lock verification at stream start. */
#define MAX96724_LINK_LOCK_TIMEOUT_MS 200
#define MAX96724_LINK_LOCK_POLL_MS    20

/* CSI PLL relock wait after reprogramming; lock is normally immediate. */
#define MAX96724_CSI_PLL_LOCK_TIMEOUT_MS 50
#define MAX96724_CSI_PLL_LOCK_ATTEMPTS   2

#define MAX96724_REG8(addr) ((addr) | VIDEO_REG_ADDR16_DATA8)

/* REG0 holds the device address; the probe reads it to see whether the device answers. */
#define MAX96724_REG_REG0 MAX96724_REG8(0x0000)

#define MAX96724_REG_REG6          MAX96724_REG8(0x0006)
#define MAX96724_REG6_LINK_EN_MASK GENMASK(3, 0)

/* REG26/REG27: RX (forward) and TX rate of links A/B and C/D, two bits each. */
#define MAX96724_REG_REG26(l)          MAX96724_REG8(0x0010 + (l) / 2)
#define MAX96724_REG26_RX_RATE_MASK(l) (GENMASK(1, 0) << (4 * ((l) % 2)))
#define MAX96724_RX_RATE_3GBPS         1
#define MAX96724_RX_RATE_6GBPS         2

/* CTRL1: one-shot reset of link A..D in bits 3:0, self-clearing. */
#define MAX96724_REG_CTRL1              MAX96724_REG8(0x0018)
#define MAX96724_CTRL1_RESET_ONESHOT(l) BIT(l)

/* RLMS49 of link A..D: errata item 5, error channel power control at 6 Gbps. */
#define MAX96724_REG_RLMS49(l)        MAX96724_REG8(0x1449 + (l) * 0x100)
#define MAX96724_RLMS49_ERR_CH_PWR_UP BIT(2)

#define MAX96724_REG_PWR1       MAX96724_REG8(0x0013)
#define MAX96724_PWR1_RESET_ALL BIT(6)

#define MAX96724_REG_REG13        MAX96724_REG8(0x000D)
#define MAX96724_REG_CTRL12       MAX96724_REG8(0x000A)
#define MAX96724_REG_CTRL13       MAX96724_REG8(0x000B)
#define MAX96724_REG_CTRL14       MAX96724_REG8(0x000C)
#define MAX96724_REG_CTRL3        MAX96724_REG8(0x001A)
#define MAX96724_CTRL_LOCKED      BIT(3)

/*
 * Error flags behind ERRB, all read-only. INTR3: DEC_ERR_FLAG_D..A [3:0]. INTR7: LCRC_ERR_FLAG [3],
 * VPRBS_ERR_FLAG [2], REM_ERR_FLAG [1], FSYNC_ERR_FLAG [0]. INTR9: IDLE_ERR_FLAG_D..A [3:0].
 * INTR11: RT_CNT_FLAG_D..A [7:4], MAX_RT_FLAG_D..A [3:0]. INTR5: EOM_ERR_FLAG_D..A [7:4],
 * LFLT_INT [2]. Register 0x45: VIDEO_MASKED_3..0 [7:4], VID_PXL_CRC_ERR_D..A [3:0], all enabled on
 * ERRB by default (0x44). A decode or idle flag clears when the error counter of its link is read;
 * the CSI PLL loss-of-lock flags clear when DPLL_ERRB_OEN is read; LFLT_INT clears when the
 * LFLT_INT_FLAG register is read. BACKTOP11: command FIFO overflow 3..0 [7:4] and line memory
 * overflow 3..0 [3:0], enabled on ERRB by default (0x454), cleared on read. VIDEO_RX8 per pipe:
 * VID_SEQ_ERR [4], on ERRB by default and cleared on read, next to the live status bits
 * VID_LOCK [6] and VID_PKT_DET [5].
 */
#define MAX96724_REG_INTR3          MAX96724_REG8(0x0026)
#define MAX96724_REG_BACKTOP11      MAX96724_REG8(0x040A)
#define MAX96724_REG_VIDEO_RX8(p)   MAX96724_REG8(0x0108 + (p) * 0x12)
#define MAX96724_REG_INTR5          MAX96724_REG8(0x0028)
#define MAX96724_REG_VID_ERR_FLAGS  MAX96724_REG8(0x0045)
#define MAX96724_REG_LFLT_INT_FLAG  MAX96724_REG8(0x00E5)
#define MAX96724_REG_INTR7          MAX96724_REG8(0x002A)
#define MAX96724_INTR7_REM_ERR_FLAG BIT(1)
#define MAX96724_REG_INTR9          MAX96724_REG8(0x002C)
#define MAX96724_REG_INTR11         MAX96724_REG8(0x002E)
#define MAX96724_REG_DEC_ERR(l)     MAX96724_REG8(0x0035 + (l))
#define MAX96724_REG_IDLE_ERR(l)    MAX96724_REG8(0x0039 + (l))
#define MAX96724_CTRL3_ERROR    BIT(2)
#define MAX96724_CTRL3_LOCK_PIN BIT(0)

/* Video pipe source selection: pipe 0 in bits 3:0, the GMSL link and the stream ID it takes. */
#define MAX96724_REG_VIDEO_PIPE_SEL_0       MAX96724_REG8(0x00F0)
#define MAX96724_VIDEO_PIPE_SEL_LINK_MASK   GENMASK(3, 2)
#define MAX96724_VIDEO_PIPE_SEL_STREAM_MASK GENMASK(1, 0)
/* Video pipe enable */
#define MAX96724_REG_VIDEO_PIPE_EN            MAX96724_REG8(0x00F4)
#define MAX96724_VIDEO_PIPE_EN_STREAM_SEL_ALL BIT(4)
#define MAX96724_VIDEO_PIPE_EN_MASK           GENMASK(3, 0)
#define MAX96724_VIDEO_PIPE_EN_PIPE(p)        BIT(p)

/* Per-pipe video receive configuration and status */
#define MAX96724_REG_VIDEO_RX0(p)      MAX96724_REG8(0x0100 + (p) * 0x12)
#define MAX96724_VIDEO_RX0_SEQ_MISS_EN BIT(4)

#define MAX96724_REG_VIDEO_RX6(p)    MAX96724_REG8(0x0106 + (p) * 0x12)
#define MAX96724_VIDEO_RX6_LIM_HEART BIT(3)

/* Video-lock timeout in ms while heartbeat is off (LIM_HEART = 1); reset 10 ms, max 127 ms. */
#define MAX96724_REG_LIM_HEART_TIMEOUT(p)  MAX96724_REG8(0x0160 + (p))
#define MAX96724_LIM_HEART_TIMEOUT_MASK    GENMASK(6, 0)
#define MAX96724_LIM_HEART_TIMEOUT_MAX_MS  127

/* Per-pipe tunnel mode registers. MIPI_TX54/57 addresses use the pipe index with stride 0x40. */
#define MAX96724_REG_MIPI_TX54(p) MAX96724_REG8(0x0936 + (p) * 0x40)
#define MAX96724_MIPI_TX54_TUN_EN BIT(0)

#define MAX96724_REG_MIPI_TX57(p)           MAX96724_REG8(0x0939 + (p) * 0x40)
#define MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET BIT(6)

/* Live CSI PLL lock status, one bit per PHY */
#define MAX96724_REG_BACKTOP1            MAX96724_REG8(0x0400)
#define MAX96724_BACKTOP1_CSIPLL_LOCK(x) BIT(4 + (x))

/* Sticky CSI PLL loss-of-lock flags [7:4]; routed to ERRB by the OEN bits [3:0], on by default */
#define MAX96724_REG_DPLL_ERRB_OEN MAX96724_REG8(0x0455)

/* CSI-2 output enable */
#define MAX96724_REG_BACKTOP12        MAX96724_REG8(0x040B)
#define MAX96724_BACKTOP12_CSI_OUT_EN BIT(1)

/*
 * DPLL frequency BACKTOP22 (0x415), BACKTOP25 (0x418), BACKTOP28 (0x41B), BACKTOP31 (0x41E),
 * one register per CSI TX PHY (0..3, stride 0x3).
 */
#define MAX96724_REG_BACKTOP22(x)         MAX96724_REG8(0x0415 + (x) * 0x3)
#define MAX96724_BACKTOP22_DPLL_FREQ_MASK GENMASK(4, 0)
#define MAX96724_BACKTOP22_PREDEF_FREQ_EN BIT(5)

/* MIPI PHY configuration */
#define MAX96724_REG_MIPI_PHY0           MAX96724_REG8(0x08A0)
#define MAX96724_MIPI_PHY0_PHY_CFG_MASK  GENMASK(4, 0)
#define MAX96724_MIPI_PHY0_PHY_4X2       BIT(0)
#define MAX96724_MIPI_PHY0_PHY_2X4       BIT(2)
#define MAX96724_MIPI_PHY0_FORCE_CSI_OUT BIT(7)

/* MIPI_PHY2 per-PHY enable + D-PHY timings. */
#define MAX96724_REG_MIPI_PHY2             MAX96724_REG8(0x08A2)
#define MAX96724_MIPI_PHY2_PHY_STDB_N_MASK GENMASK(7, 4)
/* 4x2 mode (<=2 lanes/PHY): one phy_Stdby_n bit per physical PHY */
#define MAX96724_MIPI_PHY2_PHY_STDB_N_2(x) BIT(4 + (x))
/* 2x4 mode (>2 lanes/PHY): each combined PHY owns two adjacent bits in 7:4 */
#define MAX96724_MIPI_PHY2_PHY_STDB_N_4(x) (GENMASK(5, 4) << ((x) / 2 * 2))

/*
 * Lane map, one register per PHY pair: two bits per lane give the physical lane of each logical
 * lane, a nibble per PHY in 4x2 mode and the whole byte in 2x4 mode.
 */
#define MAX96724_REG_MIPI_PHY3(x)        MAX96724_REG8(0x08A3 + (x) / 2)
#define MAX96724_MIPI_PHY3_LANE_MAP_2(x) (GENMASK(3, 0) << (4 * ((x) % 2)))
#define MAX96724_MIPI_PHY3_LANE_MAP_4    GENMASK(7, 0)

/* Lane polarity, one register per PHY pair: data lanes and clock, three bits per PHY in 4x2. */
#define MAX96724_REG_MIPI_PHY5(x)       MAX96724_REG8(0x08A5 + (x) / 2)
#define MAX96724_MIPI_PHY5_POL_MAP_2(x) (GENMASK(2, 0) << (3 * ((x) % 2)))
#define MAX96724_MIPI_PHY5_POL_MAP_4    (GENMASK(1, 0) | GENMASK(4, 3) | BIT(5))

/* Per-PHY MIPI D-PHY deskew calibration. */
#define MAX96724_REG_MIPI_TX3(p)            MAX96724_REG8(0x0903 + (p) * 0x40)
#define MAX96724_MIPI_TX3_DESKEW_INIT_AUTO  BIT(7)
#define MAX96724_MIPI_TX3_DESKEW_INIT_8X32K BIT(0)

#define MAX96724_REG_MIPI_TX4(p)          MAX96724_REG8(0x0904 + (p) * 0x40)
#define MAX96724_MIPI_TX4_DESKEW_PER_AUTO BIT(7)
#define MAX96724_MIPI_TX4_DESKEW_PER_2K   BIT(0)

/* Per-PHY CSI-2 lane count (MIPI_TX 0, MIPI_TX 1, MIPI_TX 2, MIPI_TX 3) */
#define MAX96724_REG_MIPI_TX10(x)             MAX96724_REG8(0x090A + (x) * 0x40)
#define MAX96724_MIPI_TX10_CSI2_LANE_CNT_MASK GENMASK(7, 6)
#define MAX96724_MIPI_TX10_CSI2_CPHY_EN       BIT(5)

/* Per-pipe MIPI TX mapping table: routes the pipe's data types to a CSI-2 controller. */
#define MAX96724_REG_MIPI_TX11(p)           MAX96724_REG8(0x090B + (p) * 0x40)
#define MAX96724_REG_MIPI_TX13(p, x)        MAX96724_REG8(0x090D + (p) * 0x40 + (x) * 0x2)
#define MAX96724_MIPI_TX13_MAP_SRC_DT_MASK  GENMASK(5, 0)
#define MAX96724_REG_MIPI_TX14(p, x)        MAX96724_REG8(0x090E + (p) * 0x40 + (x) * 0x2)
#define MAX96724_MIPI_TX14_MAP_DST_DT_MASK  GENMASK(5, 0)
#define MAX96724_REG_MIPI_TX45(p, x)        MAX96724_REG8(0x092D + (p) * 0x40 + (x) / 4)
/* Per-PHY alternate memory map, unpacks doubled 8/10/12-bit pixels on the CSI-2 output. */
#define MAX96724_REG_MIPI_TX51(p)           MAX96724_REG8(0x0933 + (p) * 0x40)
#define MAX96724_MIPI_TX51_ALT_MEM_MAP_10   BIT(2)
#define MAX96724_MIPI_TX45_MAP_DPHY_DEST(x) (GENMASK(1, 0) << (2 * ((x) % 4)))

/* MIPI CSI-2 short-packet data types. */
#define MIPI_CSI2_DT_FS 0x00
#define MIPI_CSI2_DT_FE 0x01

/* This register is not defined in the MAX96724 datasheet, but is used in the Linux driver. */
#define MAX96724_REG_DPLL_0(x)     MAX96724_REG8(0x1C00 + (x) * 0x100)
#define MAX96724_DPLL_0_SOFT_RST_N BIT(0)

/*
 * DPLL_AUTO_RST, set at power-on, resets a PHY's CSI PLL whenever its predefined frequency
 * changes and resets every CSI PLL on any write to MIPI_PHY0. The driver resets the DPLL
 * itself when it programs the rate, so the automatic resets are turned off at init: a
 * MIPI_PHY0 write must not reset the PLL under a running transmitter.
 */
#define MAX96724_REG_DP_ORSTB_CTL           MAX96724_REG8(0x1191)
#define MAX96724_DP_ORSTB_CTL_DPLL_AUTO_RST BIT(7)

/*
 * Global PCLK source for VPG.
 *
 * GMSL2 General User Guide, page 74.
 * Table 19. Video Pattern PCLK Selection
 * | DEBUG_EXTRA (0x09 [1:0]) | PATGEN_CLK_SRC (bit 7) | PCLK Frequency |
 * |--------------------------|------------------------|----------------|
 * | 00                       | X                      | 25MHz          |
 * | 01                       | X                      | 75MHz          |
 * | 1x                       | 0                      | 150MHz         |
 * | 1x                       | 1                      | 375MHz         |
 */
#define MAX96724_REG_DEBUG_EXTRA           MAX96724_REG8(0x0009)
#define MAX96724_DEBUG_EXTRA_PCLK_SRC_MASK GENMASK(1, 0)
#define MAX96724_DEBUG_EXTRA_PCLK_25MHZ    0x0
#define MAX96724_DEBUG_EXTRA_PCLK_75MHZ    0x1
#define MAX96724_DEBUG_EXTRA_PCLK_USE_PIPE 0x2

/* Per-pipe VPG clock select (VPRBS register) */
#define MAX96724_REG_VPRBS(p)         MAX96724_REG8(0x01DC + (p) * 0x20)
#define MAX96724_VPRBS_PATGEN_CLK_SRC BIT(7)

/* Pattern generator timing/colour registers */
#define MAX96724_REG_PATGEN_0                   MAX96724_REG8(0x1050)
#define MAX96724_PATGEN_0_VTG_MODE_MASK         GENMASK(1, 0)
#define MAX96724_PATGEN_0_VTG_MODE_FREE_RUNNING 0x3
#define MAX96724_PATGEN_0_GEN_VS                BIT(7)
#define MAX96724_PATGEN_0_GEN_HS                BIT(6)
#define MAX96724_PATGEN_0_GEN_DE                BIT(5)
#define MAX96724_PATGEN_0_VS_INV                BIT(4)
#define MAX96724_PATGEN_0_HS_INV                BIT(3)
#define MAX96724_PATGEN_0_DE_INV                BIT(2)

/* Pattern generator mode register. */
#define MAX96724_REG_PATGEN_1           MAX96724_REG8(0x1051)
#define MAX96724_PATGEN_1_MODE_MASK     GENMASK(5, 4)
#define MAX96724_PATGEN_1_MODE_DISABLED 0x0
/* Generate checkerboard pattern */
#define MAX96724_PATGEN_1_MODE_CHECKER  0x1
/* Generate gradient pattern */
#define MAX96724_PATGEN_1_MODE_GRADIENT 0x2
/* Gradient increment per pixel, in quarter steps: 4 adds 1 to each colour per pixel. */
#define MAX96724_REG_GRAD_INCR          MAX96724_REG8(0x106D)

/* Checkerboard-mode colour/repeat registers. */
#define MAX96724_REG_CHKR_COLOR_A_L MAX96724_REG8(0x106E) /* R */
#define MAX96724_REG_CHKR_COLOR_A_M MAX96724_REG8(0x106F) /* G */
#define MAX96724_REG_CHKR_COLOR_A_H MAX96724_REG8(0x1070) /* B */
#define MAX96724_REG_CHKR_COLOR_B_L MAX96724_REG8(0x1071) /* R */
#define MAX96724_REG_CHKR_COLOR_B_M MAX96724_REG8(0x1072) /* G */
#define MAX96724_REG_CHKR_COLOR_B_H MAX96724_REG8(0x1073) /* B */
#define MAX96724_REG_CHKR_RPT_A     MAX96724_REG8(0x1074)
#define MAX96724_REG_CHKR_RPT_B     MAX96724_REG8(0x1075)
#define MAX96724_REG_CHKR_ALT       MAX96724_REG8(0x1076)

/*
 * Checkerboard tile geometry. With 800x480 active area:
 *  - RPT_A=RPT_B=80  -> 10 horizontal tiles per line (800 / 80)
 *  - ALT=48          -> 10 vertical tile bands (480 / 48)
 * Colour A is blue, colour B is red, as in Figure 22 of the MAX96724 user guide (p. 57).
 */
#define MAX96724_VPG_CHKR_RPT_A 80U
#define MAX96724_VPG_CHKR_RPT_B 80U
#define MAX96724_VPG_CHKR_ALT   48U
#define MAX96724_VPG_CHKR_A_R   0x00U
#define MAX96724_VPG_CHKR_A_G   0x00U
#define MAX96724_VPG_CHKR_A_B   0xFFU
#define MAX96724_VPG_CHKR_B_R   0xFFU
#define MAX96724_VPG_CHKR_B_G   0x00U
#define MAX96724_VPG_CHKR_B_B   0x00U

/*
 * 800x480 @ 60 Hz, 25 MHz pixel clock.
 *
 *   hactive=800, hfp=4,  hsync=20, hbp=6   -> htotal=830
 *   vactive=480, vfp=2,  vsync=10, vbp=10  -> vtotal=502
 *   25 MHz / (830 * 502) = 60.001 Hz
 */
#define MAX96724_VPG_HACTIVE 800U
#define MAX96724_VPG_HFP     4U
#define MAX96724_VPG_HSYNC   20U
#define MAX96724_VPG_HBP     6U
#define MAX96724_VPG_HTOTAL                                                                        \
	(MAX96724_VPG_HACTIVE + MAX96724_VPG_HFP + MAX96724_VPG_HSYNC + MAX96724_VPG_HBP)

#define MAX96724_VPG_VACTIVE 480U
#define MAX96724_VPG_VFP     2U
#define MAX96724_VPG_VSYNC   10U
#define MAX96724_VPG_VBP     10U

/* Computed VPG timing fields */
#define MAX96724_VPG_VS_DLY  0U
#define MAX96724_VPG_V2H     0U
#define MAX96724_VPG_VS_HIGH (MAX96724_VPG_VSYNC * MAX96724_VPG_HTOTAL)
#define MAX96724_VPG_VS_LOW                                                                        \
	((MAX96724_VPG_VACTIVE + MAX96724_VPG_VFP + MAX96724_VPG_VBP) * MAX96724_VPG_HTOTAL)
#define MAX96724_VPG_HS_HIGH MAX96724_VPG_HSYNC
#define MAX96724_VPG_HS_LOW  (MAX96724_VPG_HACTIVE + MAX96724_VPG_HFP + MAX96724_VPG_HBP)
#define MAX96724_VPG_HS_CNT                                                                        \
	(MAX96724_VPG_VACTIVE + MAX96724_VPG_VFP + MAX96724_VPG_VBP + MAX96724_VPG_VSYNC)
#define MAX96724_VPG_V2D                                                                           \
	(MAX96724_VPG_HTOTAL * (MAX96724_VPG_VSYNC + MAX96724_VPG_VBP) +                           \
	 (MAX96724_VPG_HSYNC + MAX96724_VPG_HBP))
#define MAX96724_VPG_DE_HIGH MAX96724_VPG_HACTIVE
#define MAX96724_VPG_DE_LOW  (MAX96724_VPG_HFP + MAX96724_VPG_HSYNC + MAX96724_VPG_HBP)
#define MAX96724_VPG_DE_CNT  MAX96724_VPG_VACTIVE

static const char *max96724_variant_model(uint8_t id)
{
	if (id == 0xA2) {
		return "MAX96724";
	} else if (id == 0xA3) {
		return "MAX96724F";
	} else if (id == 0xA4) {
		return "MAX96724R";
	}

	return "unknown";
}

struct max96724_config {
	struct i2c_dt_spec i2c;
	const struct device *source_dev;
	uint8_t csi_tx_num_lanes;
	/* Physical lane of each logical lane, 1-based, from data-lanes. */
	uint8_t csi_tx_data_lanes[4];
	uint8_t csi_tx_phy_index;
	int64_t csi_tx_link_freq;
	uint8_t link_en_mask;
	/* GMSL link the video comes from, the index of its input port. */
	uint8_t gmsl_link;
	/* Links whose serializer runs at 3 Gbps; the others run at 6 Gbps. */
	uint8_t link_3gbps_mask;
};

/* VIDEO_CID_TEST_PATTERN: the pattern generator replaces the link video on pipe 0. */
enum max96724_test_pattern {
	MAX96724_TEST_PATTERN_OFF,
	MAX96724_TEST_PATTERN_GRADIENT,
	MAX96724_TEST_PATTERN_CHECKERBOARD,
};

static const char *const max96724_test_pattern_menu[] = {
	"Off",
	"Gradient",
	"Checkerboard",
	NULL,
};

struct max96724_data {
	struct video_format fmt;
	struct video_ctrl linkfreq;
	struct video_ctrl test_pattern;
	bool streaming;
};

static const struct video_format_cap max96724_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB24,
		.width_min = MAX96724_VPG_HACTIVE,
		.width_max = MAX96724_VPG_HACTIVE,
		.width_step = 0,
		.height_min = MAX96724_VPG_VACTIVE,
		.height_max = MAX96724_VPG_VACTIVE,
		.height_step = 0,
	},
	{0},
};

static const struct video_format max96724_vpg_fmt = {
	.type = VIDEO_BUF_TYPE_OUTPUT,
	.pixelformat = VIDEO_PIX_FMT_RGB24,
	.width = MAX96724_VPG_HACTIVE,
	.height = MAX96724_VPG_VACTIVE,
	.pitch = MAX96724_VPG_HACTIVE * 3,
	.size = MAX96724_VPG_HACTIVE * 3 * MAX96724_VPG_VACTIVE,
};

/*
 * Wait for the device to answer, with raw reads that neither retry nor log: after power-up and
 * after a reset the device stays silent for a while, which is expected.
 */
static int max96724_poll_device(const struct i2c_dt_spec *i2c)
{
	const uint16_t addr = MAX96724_REG_REG0 & VIDEO_REG_ADDR_MASK;
	const uint8_t reg[] = {addr >> 8, addr & 0xFF};
	uint8_t val;

	for (int ms = 0; ms < MAX96724_PROBE_TIMEOUT_MS; ms += MAX96724_PROBE_INTERVAL_MS) {
		if (i2c_write_read_dt(i2c, reg, sizeof(reg), &val, sizeof(val)) == 0 && val != 0) {
			LOG_DBG("Device answered after %d ms", ms);
			return 0;
		}

		k_msleep(MAX96724_PROBE_INTERVAL_MS);
	}

	LOG_ERR("No answer from %s@0x%02x within %d ms", i2c->bus->name, i2c->addr,
		MAX96724_PROBE_TIMEOUT_MS);

	return -ENODEV;
}

/* Full chip reset via PWR1.RESET_ALL. */
static int max96724_reset_device(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = max96724_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	/* This is equivalent to toggling the PWDNB pin. The bit is cleared when written. */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_PWR1, MAX96724_PWR1_RESET_ALL,
				   MAX96724_PWR1_RESET_ALL);
	if (ret < 0) {
		LOG_ERR("Failed to reset device (err %d)", ret);
		return ret;
	}

	ret = max96724_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void max96724_log_device_id(const struct i2c_dt_spec *i2c)
{
	uint32_t dev_id;
	int ret;

	ret = video_read_cci_reg(i2c, MAX96724_REG_REG13, &dev_id);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID (err %d)", ret);
		return;
	}

	LOG_DBG("Found %s (ID=0x%02X) on %s@0x%02x", max96724_variant_model(dev_id), dev_id,
		i2c->bus->name, i2c->addr);
}

/*
 * ERRB is the OR of the enabled error sources. Most are sticky flags that clear only when their
 * source register is read, and the driver's own link bring-up and CSI PLL reprogramming set some
 * of them. Read the flags and clear the ones whose source is read here, then look at the live pin
 * state again: only a source that is still active deserves a warning. REM_ERR mirrors the
 * serializer's ERRB and can only be cleared on the serializer.
 */
static void max96724_log_errb_status(const struct max96724_config *cfg)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	uint32_t ctrl3 = 0, intr3 = 0, intr5 = 0, intr7 = 0, intr9 = 0, intr11 = 0;
	uint32_t vid = 0, lflt = 0, lol = 0, ovfl = 0, rx8 = 0, cnt;

	if (video_read_cci_reg(i2c, MAX96724_REG_CTRL3, &ctrl3) < 0 ||
	    !(ctrl3 & MAX96724_CTRL3_ERROR)) {
		return;
	}

	(void)video_read_cci_reg(i2c, MAX96724_REG_INTR3, &intr3);
	(void)video_read_cci_reg(i2c, MAX96724_REG_INTR5, &intr5);
	(void)video_read_cci_reg(i2c, MAX96724_REG_INTR7, &intr7);
	(void)video_read_cci_reg(i2c, MAX96724_REG_INTR9, &intr9);
	(void)video_read_cci_reg(i2c, MAX96724_REG_INTR11, &intr11);
	(void)video_read_cci_reg(i2c, MAX96724_REG_DPLL_ERRB_OEN, &lol);
	(void)video_read_cci_reg(i2c, MAX96724_REG_VID_ERR_FLAGS, &vid);
	(void)video_read_cci_reg(i2c, MAX96724_REG_LFLT_INT_FLAG, &lflt);
	(void)video_read_cci_reg(i2c, MAX96724_REG_BACKTOP11, &ovfl);
	(void)video_read_cci_reg(i2c, MAX96724_REG_VIDEO_RX8(0), &rx8);

	for (int l = 0; l < 4; l++) {
		if (intr3 & BIT(l)) {
			(void)video_read_cci_reg(i2c, MAX96724_REG_DEC_ERR(l), &cnt);
		}
		if (intr9 & BIT(l)) {
			(void)video_read_cci_reg(i2c, MAX96724_REG_IDLE_ERR(l), &cnt);
		}
	}

	if (video_read_cci_reg(i2c, MAX96724_REG_CTRL3, &ctrl3) == 0 &&
	    (ctrl3 & MAX96724_CTRL3_ERROR)) {
		LOG_WRN("ERRB asserted: DEC_ERR=0x%X IDLE_ERR=0x%X INTR5=0x%02X INTR7=0x%02X%s "
			"RT=0x%02X CSIPLL_LOL=0x%X VIDMASK/PXLCRC=0x%02X LFLT=0x%X OVFL/LMO=0x%02X "
			"RX8=0x%02X",
			intr3 & 0xF, intr9 & 0xF, intr5, intr7,
			(intr7 & MAX96724_INTR7_REM_ERR_FLAG) ? " (serializer ERRB)" : "", intr11,
			(lol >> 4) & 0xF, vid, lflt & 0xF, ovfl, rx8);
	} else {
		LOG_INF("ERRB stale flags cleared: DEC_ERR=0x%X IDLE_ERR=0x%X INTR5=0x%02X "
			"INTR7=0x%02X RT=0x%02X CSIPLL_LOL=0x%X VIDMASK/PXLCRC=0x%02X LFLT=0x%X "
			"OVFL/LMO=0x%02X RX8=0x%02X",
			intr3 & 0xF, intr9 & 0xF, intr5, intr7, intr11, (lol >> 4) & 0xF, vid,
			lflt & 0xF, ovfl, rx8);
	}
}

/*
 * Verify that every GMSL link enabled in REG6.LINK_EN is locked. LOCK_PIN is the AND of the
 * enabled links, which is only meaningful because max96724_init() disables the unused ones.
 * If LOCK_PIN does not assert within the budget, the per-link LOCKED bits of the enabled
 * links are read so the failing link can be named.
 */
static int max96724_check_link_lock(const struct max96724_config *cfg)
{
	static const uint32_t link_ctrl_reg[] = {
		MAX96724_REG_CTRL3,
		MAX96724_REG_CTRL12,
		MAX96724_REG_CTRL13,
		MAX96724_REG_CTRL14,
	};
	bool all_locked = true;
	uint32_t val = 0;
	int ret;

	for (int i = 0; i < MAX96724_LINK_LOCK_TIMEOUT_MS / MAX96724_LINK_LOCK_POLL_MS; i++) {
		ret = video_read_cci_reg(&cfg->i2c, MAX96724_REG_CTRL3, &val);
		if (ret < 0) {
			LOG_ERR("Failed to read CTRL3 (err %d)", ret);
			return ret;
		}

		if (val & MAX96724_CTRL3_LOCK_PIN) {
			LOG_DBG("All enabled GMSL2 links locked (CTRL3=0x%02X)", val);
			return 0;
		}

		k_msleep(MAX96724_LINK_LOCK_POLL_MS);
	}

	for (int l = 0; l < ARRAY_SIZE(link_ctrl_reg); l++) {
		if (!(cfg->link_en_mask & BIT(l))) {
			continue;
		}

		ret = video_read_cci_reg(&cfg->i2c, link_ctrl_reg[l], &val);
		if (ret < 0) {
			LOG_ERR("Failed to read link %c status (err %d)", 'A' + l, ret);
			return ret;
		}

		if (val & MAX96724_CTRL_LOCKED) {
			LOG_DBG("GMSL2 link %c locked", 'A' + l);
		} else {
			LOG_ERR("GMSL2 link %c not locked after %d ms", 'A' + l,
				MAX96724_LINK_LOCK_TIMEOUT_MS);
			all_locked = false;
		}
	}

	return all_locked ? 0 : -ENOLINK;
}
static int max96724_write_pattern_timings(const struct i2c_dt_spec *i2c)
{
	/*
	 * VTG counters at 0x1052..0x106C. Multi-byte fields are MSB-first across consecutive 8-bit
	 * registers. The helpers below split a host value onto (base, base+1[, base+2]).
	 */
#define VTG_REG_3(base, value)                                                                     \
	{MAX96724_REG8((base) + 0), ((value) >> 16) & 0xFFU},                                      \
		{MAX96724_REG8((base) + 1), ((value) >> 8) & 0xFFU},                               \
		{MAX96724_REG8((base) + 2), ((value) >> 0) & 0xFFU}
#define VTG_REG_2(base, value)                                                                     \
	{MAX96724_REG8((base) + 0), ((value) >> 8) & 0xFFU},                                       \
		{MAX96724_REG8((base) + 1), ((value) >> 0) & 0xFFU}

	const struct video_reg regs[] = {
		VTG_REG_3(0x1052, MAX96724_VPG_VS_DLY),
		VTG_REG_3(0x1055, MAX96724_VPG_VS_HIGH),
		VTG_REG_3(0x1058, MAX96724_VPG_VS_LOW),
		VTG_REG_3(0x105B, MAX96724_VPG_V2H),

		VTG_REG_2(0x105E, MAX96724_VPG_HS_HIGH),
		VTG_REG_2(0x1060, MAX96724_VPG_HS_LOW),
		VTG_REG_2(0x1062, MAX96724_VPG_HS_CNT),

		VTG_REG_3(0x1064, MAX96724_VPG_V2D),

		VTG_REG_2(0x1067, MAX96724_VPG_DE_HIGH),
		VTG_REG_2(0x1069, MAX96724_VPG_DE_LOW),
		VTG_REG_2(0x106B, MAX96724_VPG_DE_CNT),

		{MAX96724_REG_PATGEN_0,
		 FIELD_PREP(MAX96724_PATGEN_0_VTG_MODE_MASK,
			    MAX96724_PATGEN_0_VTG_MODE_FREE_RUNNING) |
			 MAX96724_PATGEN_0_VS_INV | MAX96724_PATGEN_0_GEN_DE |
			 MAX96724_PATGEN_0_GEN_HS | MAX96724_PATGEN_0_GEN_VS},
	};

#undef VTG_REG_3
#undef VTG_REG_2

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96724_write_gradient_pattern(const struct i2c_dt_spec *i2c)
{
	return video_write_cci_reg(i2c, MAX96724_REG_GRAD_INCR, 8);
}

static int max96724_write_checkerboard_pattern(const struct i2c_dt_spec *i2c)
{
	const struct video_reg regs[] = {
		{MAX96724_REG_CHKR_COLOR_A_L, MAX96724_VPG_CHKR_A_R},
		{MAX96724_REG_CHKR_COLOR_A_M, MAX96724_VPG_CHKR_A_G},
		{MAX96724_REG_CHKR_COLOR_A_H, MAX96724_VPG_CHKR_A_B},
		{MAX96724_REG_CHKR_COLOR_B_L, MAX96724_VPG_CHKR_B_R},
		{MAX96724_REG_CHKR_COLOR_B_M, MAX96724_VPG_CHKR_B_G},
		{MAX96724_REG_CHKR_COLOR_B_H, MAX96724_VPG_CHKR_B_B},
		{MAX96724_REG_CHKR_RPT_A, MAX96724_VPG_CHKR_RPT_A},
		{MAX96724_REG_CHKR_RPT_B, MAX96724_VPG_CHKR_RPT_B},
		{MAX96724_REG_CHKR_ALT, MAX96724_VPG_CHKR_ALT},
	};

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96724_configure_vpg(const struct i2c_dt_spec *i2c, enum max96724_test_pattern pattern)
{
	int ret;

	/*
	 * VPRBS bit 7 (PATGEN_CLK_SRC) is the per-pipe clock selector, only consulted when the
	 * global PCLK_SRC = USE_PIPE. Irrelevant here, but clear it for determinism.
	 */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_VPRBS(0), MAX96724_VPRBS_PATGEN_CLK_SRC, 0);
	if (ret < 0) {
		return ret;
	}

	/* Pin the global VPG clock to the 25 MHz REFCLK: DEBUG_EXTRA[1:0] = 0. */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_DEBUG_EXTRA,
				   MAX96724_DEBUG_EXTRA_PCLK_SRC_MASK,
				   MAX96724_DEBUG_EXTRA_PCLK_25MHZ);
	if (ret < 0) {
		return ret;
	}

	ret = max96724_write_pattern_timings(i2c);
	if (ret < 0) {
		return ret;
	}

	if (pattern == MAX96724_TEST_PATTERN_CHECKERBOARD) {
		ret = max96724_write_checkerboard_pattern(i2c);
		if (ret < 0) {
			return ret;
		}

		return video_modify_cci_reg(
			i2c, MAX96724_REG_PATGEN_1, MAX96724_PATGEN_1_MODE_MASK,
			FIELD_PREP(MAX96724_PATGEN_1_MODE_MASK, MAX96724_PATGEN_1_MODE_CHECKER));
	}

	ret = max96724_write_gradient_pattern(i2c);
	if (ret < 0) {
		return ret;
	}

	return video_modify_cci_reg(
		i2c, MAX96724_REG_PATGEN_1, MAX96724_PATGEN_1_MODE_MASK,
		FIELD_PREP(MAX96724_PATGEN_1_MODE_MASK, MAX96724_PATGEN_1_MODE_GRADIENT));
}

/*
 * Lane map and polarity of the active PHY. The logical lanes take their physical lanes from
 * data-lanes; lanes the output does not use take the remaining physical lanes. The video
 * interfaces binding has no lane polarities, so every lane keeps the normal one.
 */
static int max96724_csi2_configure_lanes(const struct max96724_config *cfg)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	const uint8_t phy = cfg->csi_tx_phy_index;
	const uint8_t hw_lanes = (cfg->csi_tx_num_lanes <= 2) ? 2 : 4;
	const uint32_t map_mask = (hw_lanes == 4) ? MAX96724_MIPI_PHY3_LANE_MAP_4
						  : MAX96724_MIPI_PHY3_LANE_MAP_2(phy);
	const uint32_t pol_mask = (hw_lanes == 4) ? MAX96724_MIPI_PHY5_POL_MAP_4
						  : MAX96724_MIPI_PHY5_POL_MAP_2(phy);
	uint8_t used = 0;
	uint8_t map = 0;
	int ret;

	for (uint8_t i = 0; i < hw_lanes; i++) {
		uint8_t lane = 0;

		if (i < cfg->csi_tx_num_lanes) {
			lane = cfg->csi_tx_data_lanes[i] - 1;
		} else {
			while ((used & BIT(lane)) != 0) {
				lane++;
			}
		}

		if (lane >= hw_lanes || (used & BIT(lane)) != 0) {
			LOG_ERR("Invalid data-lanes for a %u-lane CSI-2 output", hw_lanes);
			return -EINVAL;
		}

		map |= lane << (2 * i);
		used |= BIT(lane);
	}

	ret = video_modify_cci_reg(i2c, MAX96724_REG_MIPI_PHY3(phy), map_mask,
				   FIELD_PREP(map_mask, map));
	if (ret < 0) {
		return ret;
	}

	return video_modify_cci_reg(i2c, MAX96724_REG_MIPI_PHY5(phy), pol_mask, 0);
}

/*
 * Lane count, lane map, deskew and DPLL rate of the output PHY. The DPLL is held in soft reset
 * while its rate is written; the caller waits for the PLL to lock before enabling the
 * transmitter.
 */
static int max96724_csi2_configure(const struct max96724_config *cfg)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	/*
	 * link-frequencies, published as VIDEO_CID_LINK_FREQ, is the D-PHY clock. Data moves on
	 * both clock edges, so each lane runs at twice that rate. The MAX96724 DPLL is configured
	 * with the lane data rate in units of 100 Mbps.
	 */
	const int64_t lane_rate = cfg->csi_tx_link_freq * 2;
	const uint32_t backtop22_freq = lane_rate / 100000000LL;
	int ret;

	/* Set the CSI-2 lane count on the output's MIPI TX controller. */
	ret = video_modify_cci_reg(
		i2c, MAX96724_REG_MIPI_TX10(cfg->csi_tx_phy_index),
		MAX96724_MIPI_TX10_CSI2_LANE_CNT_MASK,
		FIELD_PREP(MAX96724_MIPI_TX10_CSI2_LANE_CNT_MASK, cfg->csi_tx_num_lanes - 1U));
	if (ret < 0) {
		return ret;
	}

	/* D-PHY only: clear the C-PHY mode. */
	ret = video_modify_cci_reg(
		i2c, MAX96724_REG_MIPI_TX10(cfg->csi_tx_phy_index),
		MAX96724_MIPI_TX10_CSI2_CPHY_EN, 0);
	if (ret < 0) {
		return ret;
	}

	ret = max96724_csi2_configure_lanes(cfg);
	if (ret < 0) {
		return ret;
	}


	/*
	 * Program D-PHY deskew for the active PHY. D-PHY defines deskew only above 1.5 Gbps; at or
	 * below that the POR defaults (AUTO enabled) are out of spec and can glitch the host CSI
	 * receiver.
	 */
	if (lane_rate > 1500000000LL) {
		ret = video_write_cci_reg(i2c, MAX96724_REG_MIPI_TX3(cfg->csi_tx_phy_index),
					  MAX96724_MIPI_TX3_DESKEW_INIT_AUTO |
						  MAX96724_MIPI_TX3_DESKEW_INIT_8X32K);
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(i2c, MAX96724_REG_MIPI_TX4(cfg->csi_tx_phy_index),
					  MAX96724_MIPI_TX4_DESKEW_PER_AUTO |
						  MAX96724_MIPI_TX4_DESKEW_PER_2K);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = video_write_cci_reg(i2c, MAX96724_REG_MIPI_TX3(cfg->csi_tx_phy_index), 0);
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(i2c, MAX96724_REG_MIPI_TX4(cfg->csi_tx_phy_index), 0);
		if (ret < 0) {
			return ret;
		}
	}

	/* Assert SOFT_RST_N = 0 (bit 0). Put the DPLL in soft reset */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_DPLL_0(cfg->csi_tx_phy_index),
				   MAX96724_DPLL_0_SOFT_RST_N, 0);
	if (ret < 0) {
		return ret;
	}

	/* Set DPLL frequency for the chosen PHY: multiple of 100 MHz. */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_BACKTOP22(cfg->csi_tx_phy_index),
				   MAX96724_BACKTOP22_DPLL_FREQ_MASK,
				   FIELD_PREP(MAX96724_BACKTOP22_DPLL_FREQ_MASK, backtop22_freq));
	if (ret < 0) {
		return ret;
	}

	/* Disable software override for frequency fine tuning. */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_BACKTOP22(cfg->csi_tx_phy_index),
				   MAX96724_BACKTOP22_PREDEF_FREQ_EN,
				   MAX96724_BACKTOP22_PREDEF_FREQ_EN);
	if (ret < 0) {
		return ret;
	}

	/* Deassert SOFT_RST_N = 1 to take the DPLL out of reset and start the PHY. */
	ret = video_modify_cci_reg(i2c, MAX96724_REG_DPLL_0(cfg->csi_tx_phy_index),
				   MAX96724_DPLL_0_SOFT_RST_N, MAX96724_DPLL_0_SOFT_RST_N);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * Reading DPLL_ERRB_OEN clears the CSI PLL loss-of-lock sticky flags [7:4]. The driver sets them
 * itself whenever it pulses the CSI PLL resets, so it clears them after those steps.
 */
static void max96724_clear_csi_pll_lol(const struct i2c_dt_spec *i2c)
{
	uint32_t val;

	(void)video_read_cci_reg(i2c, MAX96724_REG_DPLL_ERRB_OEN, &val);
}

/*
 * With DP_ORSTB_CTL in its default auto-reset mode, a DPLL frequency change or any write to
 * MIPI_PHY0 pulses the CSI PLL reset. The CSI transmitter must not be enabled before the PLL of
 * the output PHY has relocked, otherwise the output stays dead with CSIPLL_LOL flagged on ERRB.
 * If it does not relock within the budget, pulse its soft reset once and wait again.
 */
static int max96724_wait_csi_pll_lock(const struct max96724_config *cfg)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	const unsigned int phy = cfg->csi_tx_phy_index;
	uint32_t val = 0;
	int ret;

	for (int attempt = 0; attempt < MAX96724_CSI_PLL_LOCK_ATTEMPTS; attempt++) {
		for (int ms = 0; ms < MAX96724_CSI_PLL_LOCK_TIMEOUT_MS; ms++) {
			ret = video_read_cci_reg(i2c, MAX96724_REG_BACKTOP1, &val);
			if (ret < 0) {
				return ret;
			}

			if (val & MAX96724_BACKTOP1_CSIPLL_LOCK(phy)) {
				if (attempt > 0) {
					LOG_WRN("CSI PLL %u locked after a reset pulse", phy);
				}
				return 0;
			}

			k_msleep(1);
		}

		if (attempt + 1 == MAX96724_CSI_PLL_LOCK_ATTEMPTS) {
			break;
		}

		LOG_WRN("CSI PLL %u not locked after %d ms, pulsing its reset", phy,
			MAX96724_CSI_PLL_LOCK_TIMEOUT_MS);

		ret = video_modify_cci_reg(i2c, MAX96724_REG_DPLL_0(phy),
					   MAX96724_DPLL_0_SOFT_RST_N, 0);
		if (ret < 0) {
			return ret;
		}

		ret = video_modify_cci_reg(i2c, MAX96724_REG_DPLL_0(phy),
					   MAX96724_DPLL_0_SOFT_RST_N, MAX96724_DPLL_0_SOFT_RST_N);
		if (ret < 0) {
			return ret;
		}
	}

	(void)video_read_cci_reg(i2c, MAX96724_REG_DPLL_ERRB_OEN, &val);
	LOG_ERR("CSI PLL %u not locked, CSI output cannot start (DPLL_ERRB_OEN=0x%02X)", phy, val);

	return -EIO;
}

/* fourcc -> CSI-2 DT byte for the MAX96724 pixel-mode packet mapper. */
struct max96724_csi_fmt {
	uint32_t pixelformat;
	uint8_t csi_dt;
};

#define INPUT_FMT(fmt, dt) {.pixelformat = VIDEO_PIX_FMT_##fmt, .csi_dt = VIDEO_MIPI_CSI2_DT_##dt}

static const struct max96724_csi_fmt max96724_csi_fmts[] = {
	/* RGB */
	INPUT_FMT(RGB565, RGB565),
	INPUT_FMT(RGB24, RGB888),
	/* RAW8 Bayer */
	INPUT_FMT(SBGGR8, RAW8),
	INPUT_FMT(SGBRG8, RAW8),
	INPUT_FMT(SGRBG8, RAW8),
	INPUT_FMT(SRGGB8, RAW8),
	/* RAW10 Bayer (packed + unpacked) */
	INPUT_FMT(SBGGR10P, RAW10),
	INPUT_FMT(SGBRG10P, RAW10),
	INPUT_FMT(SGRBG10P, RAW10),
	INPUT_FMT(SRGGB10P, RAW10),
	INPUT_FMT(SBGGR10, RAW10),
	INPUT_FMT(SGBRG10, RAW10),
	INPUT_FMT(SGRBG10, RAW10),
	INPUT_FMT(SRGGB10, RAW10),
};

static int max96724_pixfmt_to_csi2_dt(uint32_t pixelformat, uint8_t *dt)
{
	for (size_t i = 0; i < ARRAY_SIZE(max96724_csi_fmts); i++) {
		if (max96724_csi_fmts[i].pixelformat == pixelformat) {
			*dt = max96724_csi_fmts[i].csi_dt;
			return 0;
		}
	}
	return -ENOTSUP;
}

/*
 * Program pipe-0 pixel-mode mapper: m=0 FS, m=1 FE, m=2 active video DT (data_dt), all routed to
 * the same destination D-PHY. DPHY_DEST for maps 0..3 shares one byte at 0x092D (2 bits/map) so
 * the three RMWs accumulate into one final value.
 * The trailing TX11 write (MAP_EN = 0b111) activates all three.
 */
static int max96724_configure_map_table(const struct max96724_config *cfg, uint8_t data_dt)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	const uint8_t src_dts[3] = {MIPI_CSI2_DT_FS, MIPI_CSI2_DT_FE, data_dt};
	int ret;

	for (unsigned int m = 0; m < 3; m++) {
		ret = video_write_cci_reg(
			i2c, MAX96724_REG_MIPI_TX13(0, m),
			FIELD_PREP(MAX96724_MIPI_TX13_MAP_SRC_DT_MASK, src_dts[m]));
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(
			i2c, MAX96724_REG_MIPI_TX14(0, m),
			FIELD_PREP(MAX96724_MIPI_TX14_MAP_DST_DT_MASK, src_dts[m]));
		if (ret < 0) {
			return ret;
		}

		ret = video_modify_cci_reg(
			i2c, MAX96724_REG_MIPI_TX45(0, m), MAX96724_MIPI_TX45_MAP_DPHY_DEST(m),
			FIELD_PREP(MAX96724_MIPI_TX45_MAP_DPHY_DEST(m), cfg->csi_tx_phy_index));
		if (ret < 0) {
			return ret;
		}
	}

	return video_write_cci_reg(i2c, MAX96724_REG_MIPI_TX11(0), BIT(2) | BIT(1) | BIT(0));
}

/* Pipe 0 carries the link video in pixel mode, mapped to the output PHY. */
static int max96724_configure_link_video(const struct max96724_config *cfg,
					 const struct video_format *fmt)
{
	uint8_t data_dt;
	int ret;

	/* Clear tunnel enable for pipe 0. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_TX54(0), MAX96724_MIPI_TX54_TUN_EN,
				   0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Pipe 0 takes the link the serializer is on. Its stream select is 0, the ID the MAX96717
	 * sends (TX_STR_SEL = 0), but STREAM_SEL_ALL, set at stream start, overrides it and passes
	 * every stream of the link. One pipe serves the single camera.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_PIPE_SEL_0,
				   MAX96724_VIDEO_PIPE_SEL_LINK_MASK |
					   MAX96724_VIDEO_PIPE_SEL_STREAM_MASK,
				   FIELD_PREP(MAX96724_VIDEO_PIPE_SEL_LINK_MASK, cfg->gmsl_link));
	if (ret < 0) {
		return ret;
	}

	ret = max96724_pixfmt_to_csi2_dt(fmt->pixelformat, &data_dt);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format 0x%08x for MIPI map", fmt->pixelformat);
		return ret;
	}

	ret = max96724_configure_map_table(cfg, data_dt);
	if (ret < 0) {
		LOG_ERR("Map-table configure failed (err %d)", ret);
		return ret;
	}

	/*
	 * RAW10 arrives as 20-bit double pixels when the serializer doubles it
	 * (CONFIG_VIDEO_GMSL_RAW10_DOUBLE); the alternate memory map unpacks
	 * them back to 10-bit pixels on this PHY's CSI-2 output.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_TX51(cfg->csi_tx_phy_index),
				   MAX96724_MIPI_TX51_ALT_MEM_MAP_10,
				   (IS_ENABLED(CONFIG_VIDEO_GMSL_RAW10_DOUBLE) &&
				    data_dt == VIDEO_MIPI_CSI2_DT_RAW10)
					   ? MAX96724_MIPI_TX51_ALT_MEM_MAP_10
					   : 0);
	if (ret < 0) {
		LOG_ERR("Alternate memory map configure failed (err %d)", ret);
	}

	return ret;
}

static bool max96724_test_pattern_on(const struct max96724_data *data)
{
	return data->test_pattern.val != MAX96724_TEST_PATTERN_OFF;
}

static bool max96724_is_vpg_format(const struct video_format *fmt)
{
	return fmt->pixelformat == VIDEO_PIX_FMT_RGB24 && fmt->width == MAX96724_VPG_HACTIVE &&
	       fmt->height == MAX96724_VPG_VACTIVE;
}

static int max96724_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct max96724_config *cfg = dev->config;
	const struct max96724_data *data = dev->data;

	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	/* The link video is the source's; the pattern generator has its own format. */
	if (cfg->source_dev != NULL && !max96724_test_pattern_on(data)) {
		return video_get_caps(cfg->source_dev, caps);
	}

	caps->format_caps = max96724_fmts;
	/* The deserializer does not DMA; the buffers live downstream, in the CSI-2 receiver. */
	caps->min_vbuf_count = 1;
	return 0;
}

static int max96724_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;

	/* The pattern generator's format, whatever was set before the pattern was selected. */
	if (max96724_test_pattern_on(data)) {
		*fmt = max96724_vpg_fmt;
		return 0;
	}

	if (cfg->source_dev != NULL) {
		return video_get_format(cfg->source_dev, fmt);
	}

	*fmt = data->fmt;
	return 0;
}

static int max96724_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;
	int ret;

	if (max96724_test_pattern_on(data)) {
		/* The pattern generator emits its fixed format; the source is not involved. */
		if (!max96724_is_vpg_format(fmt)) {
			LOG_ERR("Format %s %ux%u not supported by the pattern generator",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
			return -ENOTSUP;
		}
	} else if (cfg->source_dev != NULL) {
		ret = video_set_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}

	data->fmt = *fmt;
	return 0;
}

/*
 * Turn the local video off: the pattern generator and its forced clocks, pipe 0 and the CSI-2
 * transmitter.
 */
static void max96724_stop_output(const struct max96724_config *cfg)
{
	video_modify_cci_reg(
		&cfg->i2c, MAX96724_REG_PATGEN_1, MAX96724_PATGEN_1_MODE_MASK,
		FIELD_PREP(MAX96724_PATGEN_1_MODE_MASK, MAX96724_PATGEN_1_MODE_DISABLED));

	video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_PHY0, MAX96724_MIPI_PHY0_FORCE_CSI_OUT,
			     0);

	video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_PIPE_EN, MAX96724_VIDEO_PIPE_EN_PIPE(0),
			     0);

	video_modify_cci_reg(&cfg->i2c, MAX96724_REG_BACKTOP12, MAX96724_BACKTOP12_CSI_OUT_EN, 0);
}

static int max96724_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;
	const bool vpg = max96724_test_pattern_on(data);
	const struct video_format *fmt;
	int ret;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

	if (enable == data->streaming) {
		return 0;
	}

	if (!enable) {
		/* Stop the source first, then the output it fed. */
		if (cfg->source_dev != NULL && !vpg) {
			video_stream_stop(cfg->source_dev, type);
		}

		max96724_stop_output(cfg);
		data->streaming = false;
		return 0;
	}

	if (!vpg) {
		/* Fail fast with a clear error instead of letting the CSI receiver time out. */
		ret = max96724_check_link_lock(cfg);
		if (ret < 0) {
			LOG_ERR("GMSL link not locked, not starting stream (err %d)", ret);
			return ret;
		}

		max96724_log_errb_status(cfg);

		/* Map the source's format, whatever was set while the pattern was on. */
		if (cfg->source_dev != NULL) {
			ret = video_get_format(cfg->source_dev, &data->fmt);
			if (ret < 0) {
				LOG_ERR("Failed to get the source format (err %d)", ret);
				return ret;
			}
		}
	}

	/* Enable stream autoselect. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_PIPE_EN,
				   MAX96724_VIDEO_PIPE_EN_STREAM_SEL_ALL,
				   MAX96724_VIDEO_PIPE_EN_STREAM_SEL_ALL);
	if (ret < 0) {
		goto err;
	}

	/* Disable the CSI-2 output before reconfiguring. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_BACKTOP12, MAX96724_BACKTOP12_CSI_OUT_EN,
				   0);
	if (ret < 0) {
		goto err;
	}

	/*
	 * FORCE_CSI_OUT keeps all MIPI clocks running without video lock, which the pattern
	 * generator needs and link video does not. It stays clear while the DPLL is reprogrammed
	 * and reset below, so the transmitter never runs through a PLL reset. The pattern path
	 * sets it once the PLL has locked.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_PHY0,
				   MAX96724_MIPI_PHY0_FORCE_CSI_OUT, 0);
	if (ret < 0) {
		goto err;
	}

	/* CSI-2 PHY/DPLL configuration */
	ret = max96724_csi2_configure(cfg);
	if (ret < 0) {
		LOG_ERR("CSI-2 configure failed (err %d)", ret);
		return ret;
	}

	/* Release standby on the output's PHY only; the other PHYs stay in standby. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_PHY2,
				   MAX96724_MIPI_PHY2_PHY_STDB_N_MASK,
				   (cfg->csi_tx_num_lanes <= 2)
					  ? MAX96724_MIPI_PHY2_PHY_STDB_N_2(cfg->csi_tx_phy_index)
					  : MAX96724_MIPI_PHY2_PHY_STDB_N_4(cfg->csi_tx_phy_index));
	if (ret < 0) {
		goto err;
	}

	/* Disable pipe 0 before configuring the map table and VPG. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_PIPE_EN,
				   MAX96724_VIDEO_PIPE_EN_PIPE(0), 0);
	if (ret < 0) {
		goto err;
	}

	ret = vpg ? max96724_configure_vpg(&cfg->i2c, data->test_pattern.val)
		  : max96724_configure_link_video(cfg, &data->fmt);
	if (ret < 0) {
		LOG_ERR("Pipe 0 configure failed (err %d)", ret);
		return ret;
	}

	/*
	 * Heartbeat off. A CSI-2 output has no use for the blanking timing, so the GMSL2 General
	 * User Guide (sections 0.7.2.1 and 5.4.2) suggests sending no packets during blanking.
	 * For camera video the serializer sets LIM_HEART in VIDEO_TX2. Here the packet sequence
	 * checker goes off (SEQ_MISS_EN = 0), the packet detector stays on for video lock
	 * (DIS_PKT_DET = 0), and LIM_HEART in VIDEO_RX6 replaces the 100 us packet timeout with the
	 * configurable one set below.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_RX0(0),
				   MAX96724_VIDEO_RX0_SEQ_MISS_EN, 0);
	if (ret < 0) {
		goto err;
	}

	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_RX6(0),
				   MAX96724_VIDEO_RX6_LIM_HEART, MAX96724_VIDEO_RX6_LIM_HEART);
	if (ret < 0) {
		goto err;
	}

	/*
	 * With heartbeat off the pipe drops video lock after LIM_HEART_TIMEOUT without
	 * video packets, 10 ms by default. A sensor's vertical blanking can be longer than
	 * that (the IMX219 at 30 fps with the frame length set from the frame rate blanks
	 * for about 15 ms), and the pipe then relocks every frame and never delivers one.
	 * Use the maximum, which covers blanking down to about 7 fps.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_LIM_HEART_TIMEOUT(0),
				   MAX96724_LIM_HEART_TIMEOUT_MASK,
				   FIELD_PREP(MAX96724_LIM_HEART_TIMEOUT_MASK,
					      MAX96724_LIM_HEART_TIMEOUT_MAX_MS));
	if (ret < 0) {
		goto err;
	}

	/* Enable pipe 0 only. From here on, a failure turns the output off again. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_VIDEO_PIPE_EN,
				   MAX96724_VIDEO_PIPE_EN_MASK, MAX96724_VIDEO_PIPE_EN_PIPE(0));
	if (ret < 0) {
		goto err_stop_output;
	}

	/*
	 * The DPLL was reset while its rate was written above. Make sure it relocked before the
	 * transmitter is enabled.
	 */
	ret = max96724_wait_csi_pll_lock(cfg);
	if (ret < 0) {
		goto err_stop_output;
	}

	max96724_clear_csi_pll_lol(&cfg->i2c);

	/*
	 * The pattern generator has no video lock to start the transmitter, so force the MIPI
	 * clocks now that the PLL is locked. With the automatic PLL resets off, this MIPI_PHY0
	 * write leaves the PLL alone. At 1.5 Gbps per lane the pattern never synchronized at the
	 * receiver when the bit was set before the DPLL reset.
	 */
	if (vpg) {
		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_PHY0,
					   MAX96724_MIPI_PHY0_FORCE_CSI_OUT,
					   MAX96724_MIPI_PHY0_FORCE_CSI_OUT);
		if (ret < 0) {
			goto err_stop_output;
		}
	}

	/* Re-enable the global CSI-2 transmitter */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_BACKTOP12, MAX96724_BACKTOP12_CSI_OUT_EN,
				   MAX96724_BACKTOP12_CSI_OUT_EN);
	if (ret < 0) {
		goto err_stop_output;
	}

	/* Start the source last, once the output is ready for its video. */
	if (cfg->source_dev != NULL && !vpg) {
		ret = video_stream_start(cfg->source_dev, type);
		if (ret < 0) {
			goto err_stop_output;
		}
	}

	data->streaming = true;

	fmt = vpg ? &max96724_vpg_fmt : &data->fmt;
	LOG_DBG("Streaming started: %s %ux%u, %s", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
		fmt->width, fmt->height, vpg ? "VPG" : "GMSL passthrough");

	return 0;

err_stop_output:
	max96724_stop_output(cfg);
err:
	LOG_ERR("Stream start failed (err %d)", ret);
	return ret;
}

static int max96724_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_set_frmival(cfg->source_dev, frmival);
}

static int max96724_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_get_frmival(cfg->source_dev, frmival);
}

static int max96724_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_enum_frmival(cfg->source_dev, fie);
}

static int max96724_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct max96724_data *data = dev->data;

	switch (id) {
	case VIDEO_CID_TEST_PATTERN:
		/* Applied at the next stream start, which sets the whole pipe up around it. */
		return data->streaming ? -EBUSY : 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, max96724_api) = {
	.set_ctrl = max96724_set_ctrl,
	.get_caps = max96724_get_caps,
	.get_format = max96724_get_format,
	.set_format = max96724_set_format,
	.set_stream = max96724_set_stream,
	.set_frmival = max96724_set_frmival,
	.get_frmival = max96724_get_frmival,
	.enum_frmival = max96724_enum_frmival,
};

/*
 * The forward rate of each link must match its serializer. Links already at the requested rate
 * are left alone; a change takes effect after a one-shot reset of the link.
 */
static int max96724_set_link_rates(const struct max96724_config *cfg)
{
	const struct i2c_dt_spec *i2c = &cfg->i2c;
	uint32_t val;
	int ret;

	for (int l = 0; l < 4; l++) {
		uint32_t mask = MAX96724_REG26_RX_RATE_MASK(l);
		bool is_3gbps = (cfg->link_3gbps_mask & BIT(l)) != 0;
		uint32_t rate = FIELD_PREP(mask, is_3gbps ? MAX96724_RX_RATE_3GBPS
							  : MAX96724_RX_RATE_6GBPS);

		if ((cfg->link_en_mask & BIT(l)) == 0) {
			continue;
		}

		ret = video_read_cci_reg(i2c, MAX96724_REG_REG26(l), &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == rate) {
			continue;
		}

		ret = video_write_cci_reg(i2c, MAX96724_REG_REG26(l), (val & ~mask) | rate);
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(i2c, MAX96724_REG_CTRL1, MAX96724_CTRL1_RESET_ONESHOT(l));
		if (ret < 0) {
			return ret;
		}

		LOG_INF("Link %c forward rate set to %u Gbps", 'A' + l, is_3gbps ? 3 : 6);
	}

	return 0;
}

/*
 * Errata item 5: at 6 Gbps the periodic power cycling of a link's error channel can degrade the
 * link, so keep the channel powered. Not needed at 3 Gbps. Cleared by a device reset.
 */
static int max96724_force_err_channel_on(const struct max96724_config *cfg)
{
	int ret;

	for (int l = 0; l < 4; l++) {
		if ((cfg->link_en_mask & BIT(l)) == 0 || (cfg->link_3gbps_mask & BIT(l)) != 0) {
			continue;
		}

		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_RLMS49(l),
					   MAX96724_RLMS49_ERR_CH_PWR_UP,
					   MAX96724_RLMS49_ERR_CH_PWR_UP);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int max96724_init(const struct device *dev)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->source_dev == NULL) {
		/* No source: the pattern generator's format until set_format() is called. */
		data->fmt = max96724_vpg_fmt;
	}

	ret = max96724_reset_device(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	max96724_log_device_id(&cfg->i2c);

	/*
	 * Enable only the GMSL links declared in the devicetree (REG6.LINK_EN_A..D, POR = all
	 * four). LOCK_PIN and the ERRB lock source consider every enabled link, so unused links
	 * left enabled keep LOCK deasserted and ERRB asserted. REG6[7:4] (GMSL1/GMSL2 per link)
	 * is left as latched from the CFG pins.
	 */
	if (cfg->link_en_mask == 0) {
		LOG_WRN("No GMSL link declared in devicetree, leaving all links enabled");
	} else {
		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_REG6, MAX96724_REG6_LINK_EN_MASK,
					   cfg->link_en_mask);
		if (ret < 0) {
			LOG_ERR("Failed to set LINK_EN (err %d)", ret);
			return ret;
		}
		LOG_DBG("LINK_EN=0x%02X (from devicetree)", cfg->link_en_mask);

		ret = max96724_set_link_rates(cfg);
		if (ret < 0) {
			LOG_ERR("Failed to set link rates (err %d)", ret);
			return ret;
		}

		ret = max96724_force_err_channel_on(cfg);
		if (ret < 0) {
			LOG_ERR("Failed to keep the error channels on (err %d)", ret);
			return ret;
		}
	}

	/*
	 * Link lock is verified at stream start rather than here: the serializer and camera may
	 * power up after this init runs, and failing the device at boot would leave no recovery.
	 */

	/* Pixel mode only: disable automatic tunnel mode detection on all pipes. */
	for (int i = 0; i < 4; i++) {
		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_TX57(i),
					   MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET,
					   MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET);
		if (ret < 0) {
			LOG_ERR("Failed to disable tunnel detection on pipe %d (err %d)", i, ret);
			return ret;
		}
	}

	/* Automatic CSI PLL resets off: the DPLL is reset explicitly when its rate is written. */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_DP_ORSTB_CTL,
				   MAX96724_DP_ORSTB_CTL_DPLL_AUTO_RST, 0);
	if (ret < 0) {
		LOG_ERR("Failed to disable the automatic CSI PLL reset (err %d)", ret);
		return ret;
	}

	/*
	 * CSI-2 PHY topology: select based on the number of CSI-2 data lanes.
	 *   <= 2 lanes -> 4x2 (four independent 2-lane PHYs)
	 *   >  2 lanes -> 2x4 (two combined 4-lane PHYs)
	 *
	 * FORCE_CSI_OUT stays clear; stream start sets it for the pattern generator once the
	 * CSI PLL has locked.
	 */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_PHY0,
				   MAX96724_MIPI_PHY0_PHY_CFG_MASK |
					   MAX96724_MIPI_PHY0_FORCE_CSI_OUT,
				   (cfg->csi_tx_num_lanes <= 2) ? MAX96724_MIPI_PHY0_PHY_4X2
								: MAX96724_MIPI_PHY0_PHY_2X4);
	if (ret < 0) {
		LOG_ERR("Failed to set the CSI-2 PHY topology (err %d)", ret);
		return ret;
	}

	max96724_clear_csi_pll_lol(&cfg->i2c);

	ret = video_init_int_menu_ctrl(&data->linkfreq, dev, VIDEO_CID_LINK_FREQ, 0,
				       &cfg->csi_tx_link_freq, 1);
	if (ret < 0) {
		LOG_ERR("Failed to init link-frequency control (err %d)", ret);
		return ret;
	}
	data->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return video_init_menu_ctrl(&data->test_pattern, dev, VIDEO_CID_TEST_PATTERN,
				    MAX96724_TEST_PATTERN_OFF, max96724_test_pattern_menu);
}

/*
 * Ports follow the Linux binding: ports 0 to 3 are the GMSL inputs of links A to D, ports 4 to 7
 * the CSI-2 outputs of PHYs 0 to 3. The driver uses one GMSL input and one CSI-2 output, the
 * first of each with an endpoint.
 */
#define MAX96724_EP(inst, port)     DT_INST_ENDPOINT_BY_ID(inst, port, 0)
#define MAX96724_HAS_EP(inst, port) DT_NODE_EXISTS(MAX96724_EP(inst, port))

#define MAX96724_GMSL_IN_EP(inst)                                                                  \
	COND_CODE_1(MAX96724_HAS_EP(inst, 0), (MAX96724_EP(inst, 0)),                              \
		(COND_CODE_1(MAX96724_HAS_EP(inst, 1), (MAX96724_EP(inst, 1)),                     \
			(COND_CODE_1(MAX96724_HAS_EP(inst, 2), (MAX96724_EP(inst, 2)),             \
				(MAX96724_EP(inst, 3)))))))

#define MAX96724_CSI_OUT_EP(inst)                                                                  \
	COND_CODE_1(MAX96724_HAS_EP(inst, 4), (MAX96724_EP(inst, 4)),                              \
		(COND_CODE_1(MAX96724_HAS_EP(inst, 5), (MAX96724_EP(inst, 5)),                     \
			(COND_CODE_1(MAX96724_HAS_EP(inst, 6), (MAX96724_EP(inst, 6)),             \
				(MAX96724_EP(inst, 7)))))))

/* The serializer on the GMSL input, when it is enabled. */
#define SOURCE_DEV(inst)                                                                           \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODE_REMOTE_DEVICE(MAX96724_GMSL_IN_EP(inst))),     \
		    (DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(MAX96724_GMSL_IN_EP(inst)))), (NULL))

/* GMSL link of the input: the number of its port, link A when there is no input. */
#define GMSL_IN_LINK(inst)                                                                         \
	COND_CODE_1(DT_NODE_EXISTS(MAX96724_GMSL_IN_EP(inst)),                                     \
		    (DT_REG_ADDR(DT_PARENT(MAX96724_GMSL_IN_EP(inst)))), (0))

/* PHY of the CSI-2 output port: the port number minus 4. */
#define CSI_TX_PORT_PHY(inst) (DT_REG_ADDR(DT_PARENT(MAX96724_CSI_OUT_EP(inst))) - 4)

/*
 * PHY whose registers serve the output: the port's own PHY, except that a 4-lane output on port 4
 * combines PHYs 0 and 1, whose master is PHY 1 (MAX96724 user guide).
 */
#define CSI_TX_PHY_INDEX(inst)                                                                     \
	((CSI_TX_PORT_PHY(inst) == 0 && CSI_TX_NUM_LANES(inst) > 2) ? 1 : CSI_TX_PORT_PHY(inst))

/* Number of CSI-2 TX data lanes from the CSI output endpoint's data-lanes property. */
#define CSI_TX_NUM_LANES(inst) DT_PROP_LEN_OR(MAX96724_CSI_OUT_EP(inst), data_lanes, 0)

/* CSI-2 TX D-PHY clock (Hz) from the CSI output endpoint's link-frequencies property. */
#define CSI_TX_LINK_FREQ(inst) DT_PROP_BY_IDX(MAX96724_CSI_OUT_EP(inst), link_frequencies, 0)

/* GMSL LINK_EN bitmap: one bit per GMSL input port, 0 to 3, with an endpoint. */
#define MAX96724_LINK_BIT(inst, l) (MAX96724_HAS_EP(inst, l) ? BIT(l) : 0)

#define MAX96724_LINK_EN_FROM_DT(inst)                                                             \
	(MAX96724_LINK_BIT(inst, 0) | MAX96724_LINK_BIT(inst, 1) | MAX96724_LINK_BIT(inst, 2) |    \
	 MAX96724_LINK_BIT(inst, 3))

/* Links whose input port declares link-rate-mbps = <3000>. */
#define MAX96724_LINK_3GBPS_BIT(inst, l)                                                           \
	(DT_PROP_OR(DT_INST_PORT_BY_ID(inst, l), link_rate_mbps, 6000) == 3000                     \
		 ? MAX96724_LINK_BIT(inst, l)                                                      \
		 : 0)

#define MAX96724_LINK_3GBPS_FROM_DT(inst)                                                          \
	(MAX96724_LINK_3GBPS_BIT(inst, 0) | MAX96724_LINK_3GBPS_BIT(inst, 1) |                     \
	 MAX96724_LINK_3GBPS_BIT(inst, 2) | MAX96724_LINK_3GBPS_BIT(inst, 3))

#define MAX96724_INIT(inst)                                                                        \
	BUILD_ASSERT(DT_NODE_EXISTS(MAX96724_CSI_OUT_EP(inst)),                                    \
		     "MAX96724 needs a CSI-2 output endpoint on one of ports 4 to 7");             \
	BUILD_ASSERT(IN_RANGE(CSI_TX_NUM_LANES(inst), 1, 4),                                       \
		     "MAX96724 CSI-2 output data-lanes must list 1 to 4 lanes");                   \
	BUILD_ASSERT(CSI_TX_NUM_LANES(inst) <= 2 || CSI_TX_PORT_PHY(inst) % 2 == 0,                \
		     "MAX96724 4-lane CSI-2 output must be port 4 (PHYs 0+1) or 6 (PHYs 2+3)");    \
	BUILD_ASSERT(DT_PROP_OR(MAX96724_CSI_OUT_EP(inst), bus_type, VIDEO_BUS_TYPE_CSI2_DPHY) ==  \
			     VIDEO_BUS_TYPE_CSI2_DPHY,                                             \
		     "MAX96724 CSI-2 output supports D-PHY only");                                 \
	BUILD_ASSERT(IN_RANGE(CSI_TX_LINK_FREQ(inst), MHZ(100), MHZ(1250)),                        \
		     "MAX96724 CSI-2 output link-frequencies must be 100 to 1250 MHz");            \
	BUILD_ASSERT(CSI_TX_LINK_FREQ(inst) % MHZ(50) == 0,                                        \
		     "MAX96724 CSI-2 output link-frequencies must be a multiple of 50 MHz");       \
                                                                                                   \
	static const struct max96724_config max96724_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.source_dev = SOURCE_DEV(inst),                                                    \
		.csi_tx_num_lanes = CSI_TX_NUM_LANES(inst),                                        \
		.csi_tx_data_lanes = DT_PROP_OR(MAX96724_CSI_OUT_EP(inst), data_lanes, {0}),       \
		.csi_tx_phy_index = CSI_TX_PHY_INDEX(inst),                                        \
		.csi_tx_link_freq = CSI_TX_LINK_FREQ(inst),                                        \
		.link_en_mask = MAX96724_LINK_EN_FROM_DT(inst),                                    \
		.gmsl_link = GMSL_IN_LINK(inst),                                                   \
		.link_3gbps_mask = MAX96724_LINK_3GBPS_FROM_DT(inst),                              \
	};                                                                                         \
	static struct max96724_data max96724_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max96724_init, NULL, &max96724_data_##inst,                    \
			      &max96724_config_##inst, POST_KERNEL,                                \
			      CONFIG_VIDEO_MAX96724_INIT_PRIORITY, &max96724_api);                 \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(max96724_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(MAX96724_INIT)
