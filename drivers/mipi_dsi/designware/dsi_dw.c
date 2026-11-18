/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT snps_designware_dsi

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/device_mmio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include "dsi_dw_priv.h"

LOG_MODULE_REGISTER(dsi_dw, CONFIG_MIPI_DSI_LOG_LEVEL);

/* Utility functions. */
static int dsi_format_to_bpp(uint8_t color_coding)
{
	switch (color_coding) {
	case MIPI_DSI_PIXFMT_RGB565:
		return 16;
	case MIPI_DSI_PIXFMT_RGB666:
	case MIPI_DSI_PIXFMT_RGB888:
		return 24;
	case MIPI_DSI_PIXFMT_RGB666_PACKED:
		return 18;
	}
	return -EINVAL;
}

static void reg_write_part(uintptr_t reg, uint32_t data, uint32_t mask, uint8_t shift)
{
	uint32_t tmp = 0;

	tmp = sys_read32(reg);
	tmp &= ~(mask << shift);
	tmp |= (data & mask) << shift;
	sys_write32(tmp, reg);
}

static void dsi_dw_wait_2_frames(uint32_t pixclk, const struct mipi_dsi_timings *timings)
{
	uint32_t htotal = timings->hactive + timings->hfp + timings->hbp + timings->hsync;
	uint32_t vtotal = timings->vactive + timings->vfp + timings->vbp + timings->vsync;

	uint32_t den = htotal * vtotal;
	uint32_t tmp = DIV_ROUND_UP(pixclk, den) << 1;

	k_sleep(K_MSEC(tmp));
}

static void dsi_dw_intr_en(uintptr_t regs)
{
	sys_read32(regs + DSI_INT_ST0);
	sys_read32(regs + DSI_INT_ST1);

	sys_set_bits(regs + DSI_INT_MSK0,
		     DSI_INT_0_DPHY_ERR_4 | DSI_INT_0_DPHY_ERR_3 | DSI_INT_0_DPHY_ERR_2 |
			     DSI_INT_0_DPHY_ERR_1 | DSI_INT_0_DPHY_ERR_0 |
			     DSI_INT_0_ACK_WITH_ERR_15 | DSI_INT_0_ACK_WITH_ERR_14 |
			     DSI_INT_0_ACK_WITH_ERR_13 | DSI_INT_0_ACK_WITH_ERR_12 |
			     DSI_INT_0_ACK_WITH_ERR_11 | DSI_INT_0_ACK_WITH_ERR_10 |
			     DSI_INT_0_ACK_WITH_ERR_9 | DSI_INT_0_ACK_WITH_ERR_8 |
			     DSI_INT_0_ACK_WITH_ERR_7 | DSI_INT_0_ACK_WITH_ERR_6 |
			     DSI_INT_0_ACK_WITH_ERR_5 | DSI_INT_0_ACK_WITH_ERR_4 |
			     DSI_INT_0_ACK_WITH_ERR_3 | DSI_INT_0_ACK_WITH_ERR_2 |
			     DSI_INT_0_ACK_WITH_ERR_1 | DSI_INT_0_ACK_WITH_ERR_0);

	sys_set_bits(regs + DSI_INT_MSK1,
		     DSI_INT_1_DPI_BUFF_PLD_UNDER | DSI_INT_1_GEN_PLD_RECEV_ERR |
			     DSI_INT_1_GEN_PLD_SEND_ERR | DSI_INT_1_GEN_PLD_WR_ERR |
			     DSI_INT_1_GEN_CMD_WR_ERR | DSI_INT_1_DPI_PLD_WR_ERR |
			     DSI_INT_1_EOTP_ERR | DSI_INT_1_PKT_SIZE_ERR | DSI_INT_1_CRC_ERR |
			     DSI_INT_1_ECC_MULTI_ERR | DSI_INT_1_ECC_SINGLE_ERR |
			     DSI_INT_1_TO_LP_RX | DSI_INT_1_TO_HP_TX);
}

/* Setup functions */
static void dsi_dw_phy_clk_timer_setup(uintptr_t regs, struct dphy_dsi_settings *phy)
{
	reg_write_part(regs + DSI_PHY_TMR_LPCLK_CFG, phy->clk_hs2lp,
		       DSI_PHY_TMR_LPCLK_CFG_HS2LP_MASK, DSI_PHY_TMR_LPCLK_CFG_HS2LP_SHIFT);

	reg_write_part(regs + DSI_PHY_TMR_LPCLK_CFG, phy->clk_lp2hs,
		       DSI_PHY_TMR_LPCLK_CFG_LP2HS_MASK, DSI_PHY_TMR_LPCLK_CFG_LP2HS_SHIFT);
}

static void dsi_dw_phy_data_timer_setup(uintptr_t regs, struct dphy_dsi_settings *phy)
{
	reg_write_part(regs + DSI_PHY_TMR_CFG, phy->lane_hs2lp, DSI_PHY_TMR_CFG_HS2LP_MASK,
		       DSI_PHY_TMR_CFG_HS2LP_SHIFT);

	reg_write_part(regs + DSI_PHY_TMR_CFG, phy->lane_lp2hs, DSI_PHY_TMR_CFG_LP2HS_MASK,
		       DSI_PHY_TMR_CFG_LP2HS_SHIFT);
}

static void dsi_dw_setup_phy_timings(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	struct dphy_dsi_settings *phy = &data->phy;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	/* Setup D-PHY Clk and Data lane configuration. */
	dsi_dw_phy_clk_timer_setup(regs, phy);
	dsi_dw_phy_data_timer_setup(regs, phy);
}

static int dsi_dw_phy_config(const struct device *dev, const struct mipi_dsi_device *mdev)
{
	struct dsi_dw_data *data = dev->data;
	struct dphy_dsi_settings *phy = &data->phy;
	int ret;

	/* Do the D-PHY configuration here. */
	ret = dphy_dw_master_setup(dev, phy);
	if (ret) {
		LOG_ERR("Failed to set-up D-PHY TX");
		return ret;
	}

	dsi_dw_setup_phy_timings(dev);

	return 0;
}

static int dw_calc_clocks(const struct device *dev, const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_timings *timings = &mdev->timings;
	struct dsi_dw_data *data = dev->data;
	struct dphy_dsi_settings *phy = &data->phy;
	const struct dsi_dw_config *config = dev->config;

	uint32_t dpi_pix_clk;
	uint8_t esc_clk_div;
#if !DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	uint32_t htotal;
	uint32_t vtotal;
#endif /* !DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */
	float hs_bit_clk;
	int ret;

	ret = clock_control_get_rate(config->clk_dev, config->pixel_clk, &dpi_pix_clk);
	htotal = timings->hsync + timings->hbp + timings->hactive + timings->hfp;
	vtotal = timings->vsync + timings->vbp + timings->vactive + timings->vfp;
	dpi_pix_clk = htotal * vtotal * DPI_FRAME_RATE;

	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		LOG_DBG("Burst mode of clock calculation");
		/*
		 * We get 1 pixel in 1 pixel-clock cycle. Each pixel can be
		 * made up of 24/16/18 - bits, based on the encoding used. For
		 * bandwidth considerations, the DSI in HS mode should be able
		 * to support similar bandwidth as done by the DPI interface.
		 */
		hs_bit_clk = (dpi_pix_clk * dsi_format_to_bpp(mdev->pixfmt));
		hs_bit_clk /= mdev->data_lanes;

		/*
		 * We can run the PLL at 20% higher of the desired Bandwidth
		 * as per the above calculations.
		 */
		hs_bit_clk *= DSI_HS_CLK_SCALING_FACTOR;
		LOG_DBG("hs_bit_clk: %f", (double)hs_bit_clk);
	} else {
		float tmp;

		LOG_DBG("Non-Burst mode of clock calculation");

		hs_bit_clk = ((data->pkt_size * dsi_format_to_bpp(mdev->pixfmt)) / 8.0) + 12;
		hs_bit_clk = (hs_bit_clk / data->pkt_size) * dpi_pix_clk;
		hs_bit_clk *= (8.0f / mdev->data_lanes);

		if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			tmp = (64.0 / mdev->data_lanes) * dpi_pix_clk;
		} else {
			tmp = (32.0 / mdev->data_lanes) * dpi_pix_clk;
		}
		tmp /= timings->hactive;

		hs_bit_clk += tmp;
	}

	/* Clamp the Datarate to maximum panel supported. */
	if (hs_bit_clk > config->max_bit_clk) {
		hs_bit_clk = config->max_bit_clk;
	}

	phy->pll_fout = ((uint32_t)hs_bit_clk) >> 1;
	LOG_DBG("PLL Fout requested - %d", phy->pll_fout);

	/* Setup the PLL frequency. */
	ret = dsi_dw_phy_config(dev, mdev);
	if (ret) {
		LOG_ERR("Phy configuration failed.");
		return ret;
	}

	/*
	 * scale = hs_lane_byte_clk/dpi_pix_clk
	 *	 = hs_bit_clk/(8 * dpi_pix_clk)
	 */
	data->clk_scale = ((double)(phy->pll_fout >> 2)) / dpi_pix_clk;
	data->dpi_pix_clk = dpi_pix_clk;
	data->lane_byte_clk = phy->pll_fout >> 2;

	/* Calculate NULL-packet and Number of chunks size. */
	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)) {
		float tmp;

		tmp = data->clk_scale * phy->num_lanes;
		if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			tmp -= (8.0f / timings->hactive);
		} else {
			tmp -= (4.0f / timings->hactive);
		}
		tmp = tmp * data->pkt_size -
		      ((dsi_format_to_bpp(mdev->pixfmt) * data->pkt_size) / 8.0f);
		tmp -= 12;

		if (tmp < 0) {
			data->null_size = 0;
		} else {
			data->null_size = ROUND(tmp);
		}
		data->num_chunks = (timings->hactive / data->pkt_size);
		if (data->null_size == 0 && data->num_chunks == 1) {
			data->num_chunks = 0;
		}
	}

	/*
	 * Generate the TX-Escape Clock. Escape Clk Divider values 0/1 stops
	 * the escape clock generation.
	 */
	if (data->lane_byte_clk < MAX_ESC_CLK) {
		esc_clk_div = 2;
	} else {
		esc_clk_div = (data->lane_byte_clk / MAX_ESC_CLK) + 1;
	}
	data->esc_clk_div = esc_clk_div;

	LOG_DBG("Escape clock divider - %d", esc_clk_div);
	LOG_DBG("pixel clock calculated: %d", data->dpi_pix_clk);
	LOG_DBG("lane byte clock calculated: %d", data->lane_byte_clk);
	LOG_DBG("PLL Fout: %d", phy->pll_fout);
	LOG_DBG("Lane byte clock / pixel clock ratio: %f", data->clk_scale);
	LOG_DBG("Escape clk value: %d", data->lane_byte_clk / esc_clk_div);
	return 0;
}

static void dw_setup_txesc_clk(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	reg_write_part(regs + DSI_CLKMGR_CFG, data->esc_clk_div, DSI_CLKMGR_CFG_TX_ESC_CLK_DIV_MASK,
		       DSI_CLKMGR_CFG_TX_ESC_CLK_DIV_SHIFT);
}

static void dw_calc_lpcmd_time(const struct device *dev, const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_timings *timings = &mdev->timings;
	struct dsi_dw_data *data = dev->data;
	struct dphy_dsi_settings *phy = &data->phy;

	uint32_t max_rd_time;
	int outvact;
	int invact;

	/* Line time in number of Escape Clock cycles. */
	outvact =
		(timings->hsync + timings->hbp + timings->hactive + timings->hfp) * data->clk_scale;
	invact = outvact;

	if ((mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) ||
	    !(mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)) {
		/* HSS packet Transmission and EoTp if it is enabled. */
		outvact -= (4 + ((mdev->mode_flags & MIPI_DSI_MODE_EOT_PACKET) ? 4 : 0)) /
			   mdev->data_lanes;
	} else {
		/*
		 * Time for H-Sync active pulse in number of Escape Clock
		 * cycles.
		 */
		outvact -= (timings->hsync * data->clk_scale);
	}

	/* HS->LP and LP->HS transmit time in Escape Clock cycles. */
	outvact -= (phy->lane_hs2lp + phy->lane_lp2hs);
	outvact /= data->esc_clk_div;

	/* LPDT mode entry and DSI controller implemented delay.  */
	outvact -= (22 + 2);

	/*
	 * [MAX_RD_TIME] * LANEBYTECLK_period < [OUTVACT_LPCMD_TIME] *
	 *					16 * TXCLKESC_period
	 */
	max_rd_time = (outvact * data->esc_clk_div) - 1;

	/*
	 * OUTVACT LP-CMD Time is time available in bytes to transmit cmd in
	 * LP mode during VSA, VBP and VPF regions.
	 */
	outvact = outvact >> 4;

	invact -= ((timings->hsync + timings->hbp) * data->clk_scale);
	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		float tmp;

		tmp = timings->hactive * dsi_format_to_bpp(mdev->pixfmt);
		tmp /= (mdev->data_lanes << 3);
		invact -= (uint32_t)tmp;
	} else {
		invact -= (timings->hactive * data->clk_scale);
	}

	/* HS->LP and LP->HS transmit time in Escape Clock cycles. */
	invact -= (phy->lane_hs2lp + phy->lane_lp2hs);
	invact /= data->esc_clk_div;

	/* LPDT mode entry and DSI controller implemented delay.  */
	invact -= (22 + 2);

	/*
	 * OUTVACT LP-CMD Time is time available in bytes to transmit cmd in
	 * LP mode during VSA, VBP and VPF regions.
	 */
	invact = invact >> 4;

	data->outvact = (outvact > 0) ? outvact : 0;
	data->invact = (invact > 0) ? invact : 0;
	data->max_rd_time = max_rd_time;
	LOG_DBG("OUTVACT - %d INVACT - %d MAX_RD_TIME - %d", data->outvact, data->invact,
		data->max_rd_time);
}

static void dw_setup_timeout(const struct device *dev, const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_timings *timings = &mdev->timings;
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	uint32_t hstx_to;
	uint32_t tmp;

	/* Time in lanebyteclocks to send 1 line + 15% of pixel data */
	hstx_to = (timings->hsync + timings->hbp + timings->hfp + timings->hactive) * 1.15 *
		  data->clk_scale;

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)) {
		/*
		 * HS-TX timeout for non-burst mode is dependent on time to
		 * transmit 1 frame data.
		 */
		hstx_to *= timings->vactive;
	}
	hstx_to /= TO_CLK_DIV;

	reg_write_part(regs + DSI_CLKMGR_CFG, TO_CLK_DIV, DSI_CLKMGR_CFG_TO_CLK_DIV_MASK,
		       DSI_CLKMGR_CFG_TO_CLK_DIV_SHIFT);

	tmp = sys_read32(regs + DSI_TO_CNT_CFG);
	tmp = ((hstx_to & DSI_TO_CNT_CFG_HSTX_TO_CNT_MASK) << DSI_TO_CNT_CFG_HSTX_TO_CNT_SHIFT) |
	      ((LPRX_TO_CNT & DSI_TO_CNT_CFG_LPRX_TO_CNT_MASK) << DSI_TO_CNT_CFG_LPRX_TO_CNT_SHIFT);
	sys_write32(tmp, regs + DSI_TO_CNT_CFG);

	/*
	 * TODO: Find the values that need to be programmed for HS/LP RD/WR and
	 *  BTA Time-outs, as these values are dependent on the Peripheral
	 *  response time.
	 */
	sys_write32((BTA_TO_CNT & DSI_BTA_TO_CNT_MASK) << DSI_BTA_TO_CNT_SHIFT,
		    regs + DSI_BTA_TO_CNT);
}

static void dsi_dw_dpi_color_code(uintptr_t regs, uint32_t pixfmt)
{
	switch (pixfmt) {
	case MIPI_DSI_PIXFMT_RGB565:
		reg_write_part(regs + DSI_DPI_COLOR_CODING, DPI_COLOR_CODE_16B_CONFIG_2,
			       DSI_DPI_COLOR_CODING_CLR_MASK, DSI_DPI_COLOR_CODING_CLR_SHIFT);
		break;
	case MIPI_DSI_PIXFMT_RGB666:
		sys_set_bits(regs + DSI_DPI_COLOR_CODING, DSI_DPI_COLOR_CODING_LOOSELY_18_EN);
	case MIPI_DSI_PIXFMT_RGB666_PACKED:
		reg_write_part(regs + DSI_DPI_COLOR_CODING, DPI_COLOR_CODE_18B_CONFIG_2,
			       DSI_DPI_COLOR_CODING_CLR_MASK, DSI_DPI_COLOR_CODING_CLR_SHIFT);
		break;
	case MIPI_DSI_PIXFMT_RGB888:
		reg_write_part(regs + DSI_DPI_COLOR_CODING, DPI_COLOR_CODE_24B,
			       DSI_DPI_COLOR_CODING_CLR_MASK, DSI_DPI_COLOR_CODING_CLR_SHIFT);
		break;
	}
}

static int dsi_dw_burst_mode_setup(uintptr_t regs, const struct device *dev,
				   const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_timings *timings = &mdev->timings;
	struct dsi_dw_data *data = dev->data;
	uint32_t pkt_size;
	uint32_t num_chunks;
	uint32_t null_size;

	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		pkt_size = timings->hactive;
		num_chunks = 0;
		null_size = 0;

		reg_write_part(regs + DSI_VID_MODE_CFG, DPI_VID_MODE_BURST_0,
			       DSI_VID_MODE_CFG_MODE_TYPE_MASK, DSI_VID_MODE_CFG_MODE_TYPE_SHIFT);

	} else {
		pkt_size = data->pkt_size;
		num_chunks = data->num_chunks;
		null_size = data->null_size;

		if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			reg_write_part(regs + DSI_VID_MODE_CFG, DPI_VID_MODE_NON_BURST_SYNC_PULSES,
				       DSI_VID_MODE_CFG_MODE_TYPE_MASK,
				       DSI_VID_MODE_CFG_MODE_TYPE_SHIFT);
		} else {
			reg_write_part(regs + DSI_VID_MODE_CFG, DPI_VID_MODE_NON_BURST_SYNC_EVENTS,
				       DSI_VID_MODE_CFG_MODE_TYPE_MASK,
				       DSI_VID_MODE_CFG_MODE_TYPE_SHIFT);
		}
	}

	if ((mdev->pixfmt == MIPI_DSI_PIXFMT_RGB666_PACKED) && (pkt_size % 4)) {
		LOG_ERR("18-bit Loosely packed pixel format should "
			"have vid_pkt_size multiple of 4.");
		return -EINVAL;
	}

	LOG_DBG("PKT_SIZE:%d NUM_CHUNKS:%d NULL_SIZE:%d", pkt_size, num_chunks, null_size);
	reg_write_part(regs + DSI_VID_PKT_SIZE, pkt_size, DSI_VID_PKT_SIZE_MASK,
		       DSI_VID_PKT_SIZE_SHIFT);

	reg_write_part(regs + DSI_VID_NUM_CHUNKS, num_chunks, DSI_VID_NUM_CHUNKS_MASK,
		       DSI_VID_NUM_CHUNKS_SHIFT);

	reg_write_part(regs + DSI_VID_NULL_SIZE, null_size, DSI_VID_NULL_SIZE_MASK,
		       DSI_VID_NULL_SIZE_SHIFT);

	return 0;
}

static void dsi_dw_dpi_frame_ack(uintptr_t regs, uint8_t flag)
{
	uint32_t tmp = sys_read32(regs + DSI_VID_MODE_CFG);

	if (flag) {
		tmp |= DSI_VID_MODE_CFG_FRAME_BTA_ACK_EN;
	} else {
		tmp &= ~DSI_VID_MODE_CFG_FRAME_BTA_ACK_EN;
	}
	sys_write32(tmp, regs + DSI_VID_MODE_CFG);
}

static void dsi_dw_setup_lp_cmd(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	if (data->mode_flags & MIPI_DSI_MODE_LPM) {
		reg_write_part(regs + DSI_DPI_LP_CMD_TIM, data->outvact,
			       DSI_DPI_LP_CMD_TIM_OUTVACT_MASK, DSI_DPI_LP_CMD_TIM_OUTVACT_SHIFT);

		reg_write_part(regs + DSI_DPI_LP_CMD_TIM, data->invact,
			       DSI_DPI_LP_CMD_TIM_INVACT_MASK, DSI_DPI_LP_CMD_TIM_INVACT_SHIFT);

		reg_write_part(regs + DSI_PHY_TMR_RD_CFG, data->max_rd_time,
			       DSI_PHY_TMR_RD_CFG_MAX_RD_TIME_MASK,
			       DSI_PHY_TMR_RD_CFG_MAX_RD_TIME_SHIFT);

		sys_set_bits(regs + DSI_VID_MODE_CFG, DSI_VID_MODE_CFG_LP_CMD_EN);
	} else {
		sys_clear_bits(regs + DSI_VID_MODE_CFG, DSI_VID_MODE_CFG_LP_CMD_EN);
	}
}

static void dsi_dw_packet_handler_config(const struct device *dev)
{
	const struct dsi_dw_config *config = dev->config;
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uint32_t tmp;

	tmp = sys_read32(regs + DSI_PCKHDL_CFG);

	if (data->mode_flags & MIPI_DSI_MODE_EOT_PACKET) {
		tmp |= DSI_PCKHDL_CFG_EOTP_TX_EN;
	}
	if (config->eotp_lp_tx) {
		tmp |= DSI_PCKHDL_CFG_EOTP_TX_LP_EN;
	}
	if (config->eotp_rx) {
		tmp |= DSI_PCKHDL_CFG_EOTP_RX_EN;
	}
	if (config->frame_ack_en) {
		tmp |= DSI_PCKHDL_CFG_BTA_EN;
	}
	if (config->crc_recv_en) {
		tmp |= DSI_PCKHDL_CFG_CRC_RX_EN;
	}
	if (config->ecc_recv_en) {
		tmp |= DSI_PCKHDL_CFG_ECC_RX_EN;
	}

	sys_write32(tmp, regs + DSI_PCKHDL_CFG);
}

static void dsi_dw_panel_timings_setup(const struct device *dev,
				       const struct mipi_dsi_timings *timings)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uint32_t tmp = timings->hsync + timings->hbp + timings->hfp + timings->hactive;

	/* Setup Horizontal timings. */
	reg_write_part(regs + DSI_VID_HSA_TIME, ROUND(timings->hsync * data->clk_scale),
		       DSI_VID_HSA_TIME_MASK, DSI_VID_HSA_TIME_SHIFT);
	reg_write_part(regs + DSI_VID_HBP_TIME, ROUND(timings->hbp * data->clk_scale),
		       DSI_VID_HBP_TIME_MASK, DSI_VID_HBP_TIME_SHIFT);
	reg_write_part(regs + DSI_VID_HLINE_TIME, ROUND(tmp * data->clk_scale),
		       DSI_VID_HLINE_TIME_MASK, DSI_VID_HLINE_TIME_SHIFT);

	/* Setup Vertical timings. */
	reg_write_part(regs + DSI_VID_VSA_LINES, timings->vsync, DSI_VID_VSA_LINES_MASK,
		       DSI_VID_VSA_LINES_SHIFT);
	reg_write_part(regs + DSI_VID_VBP_LINES, timings->vbp, DSI_VID_VBP_LINES_MASK,
		       DSI_VID_VBP_LINES_SHIFT);
	reg_write_part(regs + DSI_VID_VFP_LINES, timings->vfp, DSI_VID_VFP_LINES_MASK,
		       DSI_VID_VFP_LINES_SHIFT);
	reg_write_part(regs + DSI_VID_VACTIVE_LINES, timings->vactive, DSI_VID_VACTIVE_LINES_MASK,
		       DSI_VID_VACTIVE_LINES_SHIFT);
}

static int dsi_dw_dpi_config(const struct device *dev, uint8_t channel,
			     const struct mipi_dsi_device *mdev)
{
	const struct dsi_dw_config *config = dev->config;
	const struct dpi_config *dpi = &(config->dpi);
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	if (channel > 4) {
		LOG_ERR("VC-ID can be between 0-3 only.");
		return -EINVAL;
	}
	sys_write32(channel, regs + DSI_DPI_VCID);

	/* Setup the polarity of signals DPI signals. */
	sys_write32(dpi->polarity, regs + DSI_DPI_CFG_POL);

	/* Setup the color-coding. */
	dsi_dw_dpi_color_code(regs, mdev->pixfmt);

	/* Burst mode settings. */
	dsi_dw_burst_mode_setup(regs, dev, mdev);

	/* DPI panel timings setup. */
	dsi_dw_panel_timings_setup(dev, &mdev->timings);

	return 0;
}

static void dsi_dw_msg_config(uintptr_t regs, uint32_t mode_flags)
{
	uint32_t cmd_mode_cfg = 0;
	bool lpm = mode_flags & MIPI_DSI_MODE_LPM;

	if (lpm) {
		cmd_mode_cfg = DSI_CMD_MODE_CFG_MAX_RD_PKT_SIZE | DSI_CMD_MODE_CFG_DCS_LW_TX |
			       DSI_CMD_MODE_CFG_DCS_SR_0P_TX | DSI_CMD_MODE_CFG_DCS_SW_1P_TX |
			       DSI_CMD_MODE_CFG_DCS_SW_0P_TX | DSI_CMD_MODE_CFG_GEN_LW_TX |
			       DSI_CMD_MODE_CFG_GEN_SR_2P_TX | DSI_CMD_MODE_CFG_GEN_SR_1P_TX |
			       DSI_CMD_MODE_CFG_GEN_SR_0P_TX | DSI_CMD_MODE_CFG_GEN_SW_2P_TX |
			       DSI_CMD_MODE_CFG_GEN_SW_1P_TX | DSI_CMD_MODE_CFG_GEN_SW_0P_TX;
	}
	sys_set_bits(regs + DSI_CMD_MODE_CFG, cmd_mode_cfg);

	/* clear TXREQUESTCLKHS signal when sending commands in LP mode. */
	sys_write32((lpm ? 0 : DSI_LPCLK_CTRL_PHY_TXREQUESTCLKHS), regs + DSI_LPCLK_CTRL);
	if (mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) {
		sys_set_bits(regs + DSI_LPCLK_CTRL, DSI_LPCLK_CTRL_AUTO_CLKLN_CTRL);
	}
}

static void dsi_dw_cmd_mode_config(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	/* Enable Transmission of commands in LP mode. */
	dsi_dw_setup_lp_cmd(dev);

	dsi_dw_msg_config(regs, data->mode_flags);

	/* Setup the DSI as Command mode. */
	sys_write32(DSI_MODE_CFG_CMD_MODE, regs + DSI_MODE_CFG);
}

static void dsi_dw_video_mode_config(const struct device *dev)
{
	const struct dsi_dw_config *config = dev->config;
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uint32_t tmp;

	/* Setup return to Low-Power capability. */
	sys_set_bits(regs + DSI_VID_MODE_CFG,
		     DSI_VID_MODE_CFG_LP_HFP_EN | DSI_VID_MODE_CFG_LP_HBP_EN |
			     DSI_VID_MODE_CFG_LP_VACT_EN | DSI_VID_MODE_CFG_LP_VFP_EN |
			     DSI_VID_MODE_CFG_LP_VBP_EN | DSI_VID_MODE_CFG_LP_VSA_EN);

	/* Setup request for Peripheral ACK at the end of frame. */
	dsi_dw_dpi_frame_ack(regs, config->frame_ack_en);

	tmp = sys_read32(regs + DSI_LPCLK_CTRL);
	if (data->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) {
		tmp |= DSI_LPCLK_CTRL_AUTO_CLKLN_CTRL;
	} else {
		tmp &= ~DSI_LPCLK_CTRL_AUTO_CLKLN_CTRL;
	}
	tmp |= DSI_LPCLK_CTRL_PHY_TXREQUESTCLKHS;
	sys_write32(tmp, regs + DSI_LPCLK_CTRL);

	/* Setup the DSI as Video mode. */
	sys_write32(DSI_MODE_CFG_VID_MODE, regs + DSI_MODE_CFG);
}

/* Generic APIs */
int dsi_dw_attach(const struct device *dev, uint8_t channel, const struct mipi_dsi_device *mdev)
{
	const struct dsi_dw_config *config = dev->config;
	const struct dpi_config *dpi = &config->dpi;
	struct dsi_dw_data *data = dev->data;
	struct dphy_dsi_settings *phy = &data->phy;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	int ret;

	LOG_DBG("Attach called for channel: %d "
		"With parameters - htimings(%d, %d, %d, %d)\t"
		"vtimings(%d, %d, %d, %d)",
		channel, mdev->timings.hsync, mdev->timings.hbp, mdev->timings.hactive,
		mdev->timings.hfp, mdev->timings.vsync, mdev->timings.vbp, mdev->timings.vactive,
		mdev->timings.vfp);

	switch (mdev->pixfmt) {
	case MIPI_DSI_PIXFMT_RGB565:
		LOG_DBG("DSI Interface Format - RGB565");
		break;
	case MIPI_DSI_PIXFMT_RGB666:
		LOG_DBG("DSI Interface Format - RGB666");
		break;
	case MIPI_DSI_PIXFMT_RGB888:
		LOG_DBG("DSI Interface Format - RGB888");
		break;
	case MIPI_DSI_PIXFMT_RGB666_PACKED:
		LOG_DBG("DSI Interface Format - RGB66_PACKED");
		break;
	default:
		LOG_DBG("Unsupported DSI Interface Format");
		return -EINVAL;
	}

	phy->num_lanes = mdev->data_lanes;
	data->mode_flags = mdev->mode_flags;

	/* Fresh attach: allow each DSI error class to be reported once again. */
	atomic_clear(&data->err_logged);

	LOG_DBG("Number of lanes: %d", data->phy.num_lanes);
	LOG_DBG("DSI mode_flags: 0x%x", data->mode_flags);

	dsi_dw_pwr_down(regs);
	ret = dw_calc_clocks(dev, mdev);
	if (ret) {
		return ret;
	}
	dw_calc_lpcmd_time(dev, mdev);
	dw_setup_txesc_clk(dev);
	dw_setup_timeout(dev, mdev);
	ret = dsi_dw_dpi_config(dev, channel, mdev);
	if (ret) {
		return ret;
	}
	dsi_dw_packet_handler_config(dev);
	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		dsi_dw_video_mode_config(dev);
		data->curr_mode = DSI_DW_VIDEO_MODE;
	} else {
		dsi_dw_cmd_mode_config(dev);
		data->curr_mode = DSI_DW_COMMAND_MODE;
	}

	/*
	 * Setup as Video Mode at the end of attach only if VPG is active.
	 * In case of Video inputs from DPI, keep DSI controller in command
	 * mode.
	 */
	if (dpi->vpg_pattern != DPI_VID_PATTERN_GEN_NONE) {
		switch (dpi->vpg_pattern) {
		case DPI_VID_PATTERN_GEN_VERT_COLORBAR:
			/* Setup the DSI as Video mode. */
			sys_set_bits(regs + DSI_VID_MODE_CFG, DSI_VID_MODE_CFG_VPG_EN);
			break;
		case DPI_VID_PATTERN_GEN_HORIZ_COLORBAR:
			/* Setup the DSI as Video mode. */
			sys_set_bits(regs + DSI_VID_MODE_CFG,
				     DSI_VID_MODE_CFG_VPG_EN | DSI_VID_MODE_CFG_VPG_ORIENTATION);
			break;
		case DPI_VID_PATTERN_GEN_VERT_BER:
			/* Setup the DSI as Video mode. */
			sys_set_bits(regs + DSI_VID_MODE_CFG,
				     DSI_VID_MODE_CFG_VPG_EN | DSI_VID_MODE_CFG_VPG_MODE);
			break;
		case DPI_VID_PATTERN_GEN_NONE:
			break;
		default:
			LOG_ERR("Unknown Video Pattern Mode.");
			return -EINVAL;
		}
	}

	/* DSI must wait for 2 frames time after setup. */
	dsi_dw_wait_2_frames(data->dpi_pix_clk, &mdev->timings);
	dsi_dw_intr_en(regs);
	dsi_dw_pwr_up(regs);
	return 0;
}

#define HEADER(channel, type, data0, data1)                                                        \
	((((channel) & DSI_GEN_HDR_VC_MASK) << DSI_GEN_HDR_VC_SHIFT) |                             \
	 (((type) & DSI_GEN_HDR_DT_MASK) << DSI_GEN_HDR_DT_SHIFT) |                                \
	 (((data0) & DSI_GEN_HDR_WC_LSBYTE_MASK) << DSI_GEN_HDR_WC_LSBYTE_SHIFT) |                 \
	 (((data1) & DSI_GEN_HDR_WC_MSBYTE_MASK) << DSI_GEN_HDR_WC_MSBYTE_SHIFT))

static int dsi_dw_read_payload(uintptr_t regs, uint8_t *rx, ssize_t len)
{
	uint32_t tmp;
	int i;
	int j;

	/* Wait for 20ms at max to wait for Read operation to end. */
	for (j = 0; j < 20 &&
		    !(sys_read32(regs + DSI_CMD_PKT_STATUS) & DSI_CMD_PKT_STATUS_GEN_RD_CMD_BUSY);
	     j++) {
		k_busy_wait(1000);
	}

	if (sys_read32(regs + DSI_CMD_PKT_STATUS) & DSI_CMD_PKT_STATUS_GEN_RD_CMD_BUSY) {
		/* Timed-out during wait for Read operation to finish. */
		return -ETIMEDOUT;
	}

	/* Read from Read Payload FIFO. */
	for (i = 0; i < len; i += 4) {
		/*
		 * Wait till read response is reflected on
		 * Generic Read Payload FIFO.
		 */
		for (j = 0; j < 20 && !(sys_read32(regs + DSI_CMD_PKT_STATUS) &
					DSI_CMD_PKT_STATUS_GEN_PLD_R_EMPTY);
		     j++) {
			k_busy_wait(1000);
		}

		if (j == 20) {
			/* Read Payload FIFO is empty. */
			return -EIO;
		}

		tmp = sys_read32(regs + DSI_GEN_PLD_DATA);
		for (j = 0; j < sizeof(uint32_t) && (i + j < len); j++) {
			rx[i + j] = tmp >> (8 * j);
		}
	}
	return 0;
}

static int dsi_dw_write_payload(uintptr_t regs, uint8_t byte0, const uint8_t *tx, ssize_t len)
{
	uint8_t tmp = (len < 3) ? len : 3;
	uint32_t payload_word = 0;
	int i;

	payload_word = byte0 << DSI_GEN_PLD_DATA_B1_SHIFT;
	for (i = 0; i < tmp; i++) {
		payload_word |= tx[i] << (8 * (i + 1));
	}
	sys_write32(payload_word, regs + DSI_GEN_PLD_DATA);

	while (i < len) {
		payload_word = 0;
		for (tmp = 0; (tmp < 4) && (i < len); i++, tmp++) {
			payload_word |= tx[i] << (8 * tmp);
		}
		sys_write32(payload_word, regs + DSI_GEN_PLD_DATA);

		k_busy_wait(1000);
		if (sys_read32(regs + DSI_CMD_PKT_STATUS) & DSI_CMD_PKT_STATUS_GEN_PLD_W_FULL) {
			/* Generic Payload FIFO overflow occurred. */
			return -EMSGSIZE;
		}
	}
	return 0;
}

static int dsi_dw_write_hdr(uintptr_t regs, uint32_t header)
{
	uint32_t mask = 0;
	int j;
	uint32_t tmp;

	sys_write32(header, regs + DSI_GEN_HDR);

	j = 100;
	mask = DSI_CMD_PKT_STATUS_GEN_CMD_EMPTY | DSI_CMD_PKT_STATUS_GEN_PLD_W_EMPTY;
	do {
		tmp = sys_read32(regs + DSI_CMD_PKT_STATUS);
		if ((tmp & mask) == mask) {
			break;
		}

		k_usleep(1000);
	} while (j-- > 0);

	if ((mask & sys_read32(regs + DSI_CMD_PKT_STATUS)) != mask) {
		LOG_ERR("Failed to write command FIFO.");
		return -ETIMEDOUT;
	}

	return 0;
}

static int dsi_dw_send_max_return_packet_size(uintptr_t regs, uint8_t channel, uint16_t value)
{
	uint32_t mask;

	sys_write32(HEADER(channel, MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, (uint8_t)value & 0xff,
			   (uint8_t)value >> 8),
		    regs + DSI_GEN_HDR);

	mask = DSI_CMD_PKT_STATUS_GEN_CMD_EMPTY | DSI_CMD_PKT_STATUS_GEN_PLD_W_EMPTY;
	for (int j = 0; (j < 100) && (mask & sys_read32(regs + DSI_CMD_PKT_STATUS)) != mask; j++) {
		k_usleep(1000);
	}

	if ((mask & sys_read32(regs + DSI_CMD_PKT_STATUS)) != mask) {
		LOG_ERR("Failed to write command FIFO.");
		return -ETIMEDOUT;
	}

	return 0;
}

ssize_t dsi_dw_transfer(const struct device *dev, uint8_t channel, struct mipi_dsi_msg *msg)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);

	const uint8_t *tx = msg->tx_buf;
	uint32_t header;
	uint8_t param0;
	uint8_t param1;
	uint32_t mask;
	int ret = 0;

	if (data->curr_mode == DSI_DW_VIDEO_MODE) {
		/* Switch to command mode for transfer */
		dsi_dw_cmd_mode_config(dev);
	}

	/* Wait till the Command FIFO has empty space or time-out. */
	mask = DSI_CMD_PKT_STATUS_GEN_CMD_FULL;
	for (int j = 0; (j < 100) && ((mask & sys_read32(regs + DSI_CMD_PKT_STATUS)) != 0); j++) {
		k_usleep(1000);
	}

	if ((mask & sys_read32(regs + DSI_CMD_PKT_STATUS)) != 0) {
		LOG_ERR("Timed-out waiting get available Command-FIFO.");
		ret = -ETIMEDOUT;
		goto out;
	}

	switch (msg->type) {
	case MIPI_DSI_DCS_READ:
		/*
		 * Send a maximum return packet size packet to
		 * configure the return response of peripheral.
		 */
		ret = dsi_dw_send_max_return_packet_size(regs, channel, msg->rx_len);
		if (ret) {
			goto out;
		}
		/* Now write the Read packet. */
		param0 = msg->cmd;
		param1 = 0;
		header = HEADER(channel, msg->type, param0, param1);
		ret = dsi_dw_write_hdr(regs, header);
		if (ret) {
			goto out;
		}
		break;
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		param0 = msg->cmd;
		param1 = (msg->tx_len > 0) ? tx[0] : 0;
		header = HEADER(channel, msg->type, param0, param1);
		ret = dsi_dw_write_hdr(regs, header);
		if (ret) {
			goto out;
		}
		break;
	case MIPI_DSI_DCS_LONG_WRITE:
		ret = dsi_dw_write_payload(regs, msg->cmd, tx, msg->tx_len);
		if (ret) {
			goto out;
		}
		param0 = msg->tx_len + 1;
		param1 = (msg->tx_len + 1) >> 8;

		header = HEADER(channel, msg->type, param0, param1);
		ret = dsi_dw_write_hdr(regs, header);
		if (ret) {
			goto out;
		}
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		/*
		 * Send a maximum return packet size packet to
		 * configure the return response of peripheral.
		 */
		ret = dsi_dw_send_max_return_packet_size(regs, channel, msg->rx_len);
		if (ret) {
			goto out;
		}
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		param0 = (msg->tx_len > 0) ? tx[0] : 0;
		param1 = (msg->tx_len > 1) ? tx[1] : 0;
		header = HEADER(channel, msg->type, param0, param1);
		ret = dsi_dw_write_hdr(regs, header);
		if (ret) {
			goto out;
		}
		break;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		if (msg->tx_len >= 1) {
			ret = dsi_dw_write_payload(regs, tx[0], tx + 1, msg->tx_len - 1);
			if (ret) {
				goto out;
			}
		}
		param0 = msg->tx_len;
		param1 = msg->tx_len >> 8;
		header = HEADER(channel, msg->type, param0, param1);
		ret = dsi_dw_write_hdr(regs, header);
		if (ret) {
			goto out;
		}
		break;
	default:
		LOG_ERR("Un-supported packet type!");
		ret = -EINVAL;
		goto out;
	}

	if (msg->rx_buf && msg->rx_len) {
		dsi_dw_read_payload(regs, msg->rx_buf, msg->rx_len);
	}

out:
	if (data->curr_mode == DSI_DW_VIDEO_MODE) {
		/* Switch back to video mode*/
		dsi_dw_video_mode_config(dev);
	}

	return ret;
}

/* One bit per error class reported by dsi_dw_irq(), tracked in
 * dsi_dw_data.err_logged so each class is logged only once. When no panel is
 * attached the controller raises the same error on every interrupt (typically
 * an LP-RX timeout, DSI_INT_1_TO_LP_RX), which otherwise floods the console
 * every few milliseconds.
 */
#define DSI_ERR_LOGGED_ACK  BIT(0)
#define DSI_ERR_LOGGED_DPHY BIT(1)
#define DSI_ERR_LOGGED_PKT  BIT(2)
#define DSI_ERR_LOGGED_DPI  BIT(3)

/* Log a DSI error class the first time it is seen, then suppress it. The first
 * error of any class also emits a single short advisory, since a missing or
 * unresponsive display raises the same interrupt continuously. Messages are
 * kept short and separate so each is a self-contained, prefixed log line.
 */
static void dsi_dw_log_err_once(struct dsi_dw_data *data, uint32_t bit, const char *what,
				const char *reg, uint32_t irq_st)
{
	atomic_val_t prev;

	if ((atomic_get(&data->err_logged) & (atomic_val_t)bit) != 0) {
		return;
	}

	/* atomic_or makes the ISR set and the dsi_dw_attach() clear race-free.
	 * prev holds the mask before this class was set: prev already having
	 * the bit means another context logged it first; prev == 0 means this
	 * is the very first error of any class, which gets the one advisory.
	 */
	prev = atomic_or(&data->err_logged, (atomic_val_t)bit);
	if ((prev & (atomic_val_t)bit) != 0) {
		return;
	}

	if (prev == 0) {
		LOG_ERR("DSI error detected; if no panel is attached, this is expected");
	}

	LOG_ERR("%s %s=0x%x (further occurrences suppressed)", what, reg, irq_st);
}

/* ISR Function */
void dsi_dw_irq(const struct device *dev)
{
	struct dsi_dw_data *data = dev->data;
	uintptr_t regs = DEVICE_MMIO_NAMED_GET(dev, dsi_regs);
	uint32_t irq_st0;
	uint32_t irq_st1;
	uint32_t mask;

	irq_st0 = sys_read32(regs + DSI_INT_ST0);
	irq_st1 = sys_read32(regs + DSI_INT_ST1);

	mask = DSI_INT_0_ACK_WITH_ERR_15 | DSI_INT_0_ACK_WITH_ERR_14 | DSI_INT_0_ACK_WITH_ERR_13 |
	       DSI_INT_0_ACK_WITH_ERR_12 | DSI_INT_0_ACK_WITH_ERR_11 | DSI_INT_0_ACK_WITH_ERR_10 |
	       DSI_INT_0_ACK_WITH_ERR_9 | DSI_INT_0_ACK_WITH_ERR_8 | DSI_INT_0_ACK_WITH_ERR_7 |
	       DSI_INT_0_ACK_WITH_ERR_6 | DSI_INT_0_ACK_WITH_ERR_5 | DSI_INT_0_ACK_WITH_ERR_4 |
	       DSI_INT_0_ACK_WITH_ERR_3 | DSI_INT_0_ACK_WITH_ERR_2 | DSI_INT_0_ACK_WITH_ERR_1 |
	       DSI_INT_0_ACK_WITH_ERR_0;
	if (irq_st0 & mask) {
		dsi_dw_log_err_once(data, DSI_ERR_LOGGED_ACK, "ACK Error.", "irq_st0", irq_st0);
	}

	mask = DSI_INT_0_DPHY_ERR_4 | DSI_INT_0_DPHY_ERR_3 | DSI_INT_0_DPHY_ERR_2 |
	       DSI_INT_0_DPHY_ERR_1 | DSI_INT_0_DPHY_ERR_0;
	if (irq_st0 & mask) {
		dsi_dw_log_err_once(data, DSI_ERR_LOGGED_DPHY, "D-PHY Error.", "irq_st0", irq_st0);
	}

	mask = DSI_INT_1_TO_HP_TX | DSI_INT_1_TO_LP_RX | DSI_INT_1_ECC_SINGLE_ERR |
	       DSI_INT_1_ECC_MULTI_ERR | DSI_INT_1_CRC_ERR | DSI_INT_1_PKT_SIZE_ERR |
	       DSI_INT_1_EOTP_ERR;
	if (irq_st1 & mask) {
		dsi_dw_log_err_once(data, DSI_ERR_LOGGED_PKT, "DSI PKT Error.", "irq_st1", irq_st1);
	}

	mask = DSI_INT_1_DPI_PLD_WR_ERR | DSI_INT_1_GEN_CMD_WR_ERR | DSI_INT_1_GEN_PLD_WR_ERR |
	       DSI_INT_1_GEN_PLD_SEND_ERR | DSI_INT_1_GEN_PLD_RD_ERR | DSI_INT_1_GEN_PLD_RECEV_ERR |
	       DSI_INT_1_DPI_BUFF_PLD_UNDER;
	if (irq_st1 & mask) {
		dsi_dw_log_err_once(data, DSI_ERR_LOGGED_DPI, "DSI DPI Error Event.", "irq_st1",
				    irq_st1);
	}
}
