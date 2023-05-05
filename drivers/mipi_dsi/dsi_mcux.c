/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_mipi_dsi

#include <zephyr/drivers/mipi_dsi.h>
#include <fsl_mipi_dsi.h>
#include <fsl_clock.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(dsi_mcux, CONFIG_MIPI_DSI_LOG_LEVEL);

#define MIPI_DPHY_REF_CLK DT_INST_PROP(0, dphy_ref_frequency)

/* Max output frequency of DPHY bit clock */
#define MIPI_DPHY_MAX_FREQ MHZ(800)

/* PLL CN should be in the range of 1 to 32. */
#define DSI_DPHY_PLL_CN_MIN 1U
#define DSI_DPHY_PLL_CN_MAX 32U

/* PLL refClk / CN should be in the range of 24M to 30M. */
#define DSI_DPHY_PLL_REFCLK_CN_MIN MHZ(24)
#define DSI_DPHY_PLL_REFCLK_CN_MAX MHZ(30)

/* PLL CM should be in the range of 16 to 255. */
#define DSI_DPHY_PLL_CM_MIN 16U
#define DSI_DPHY_PLL_CM_MAX 255U

/* PLL VCO output frequency max value is 1.5GHz, VCO output is (ref_clk / CN ) * CM. */
#define DSI_DPHY_PLL_VCO_MAX MHZ(1500)
#define DSI_DPHY_PLL_VCO_MIN (DSI_DPHY_PLL_REFCLK_CN_MIN * DSI_DPHY_PLL_CM_MIN)

#define DSI_DPHY_PLL_CO_MIN 0
#define DSI_DPHY_PLL_CO_MAX 3

/* MAX DSI TX payload */
#define DSI_TX_MAX_PAYLOAD_BYTE (64U * 4U)

struct display_mcux_mipi_dsi_config {
	MIPI_DSI_Type base;
	dsi_dpi_config_t dpi_config;
	bool auto_insert_eotp;
	uint32_t phy_clock;
};

struct display_mcux_mipi_dsi_data {
	const struct device *dev;
};

static uint32_t dsi_mcux_best_clock(uint32_t ref_clk, uint32_t target_freq)
{
	/*
	 * This function is intended to find the closest realizable DPHY
	 * bit clock for a given target frequency, such that the DPHY clock
	 * is faster than the target frequency. MCUX SDK implements a similar
	 * function with DSI_DphyGetPllDivider, but this function will
	 * configure the DPHY to output the closest realizable clock frequency
	 * to the requested value. This can cause dropped pixels if
	 * the output frequency is less than the requested one.
	 */
	uint32_t co_shift, cn, cm;
	uint32_t cand_freq, vco_freq, refclk_cn_freq;
	uint32_t best_pll_freq = 0U;
	uint32_t best_diff = UINT32_MAX;

	/*
	 * The formula for the DPHY output frequency is:
	 * ref_clk * (CM / (CN * (1 << CO)))
	 */

	/* Test all available CO shifts (1x, 2x, 4x, 8x) */
	for (co_shift = DSI_DPHY_PLL_CO_MIN; co_shift <= DSI_DPHY_PLL_CO_MAX; co_shift++) {
		/* Determine VCO output frequency before CO divider */
		vco_freq = target_freq << co_shift;

		/* If desired VCO output frequency is too low, try next CO shift */
		if (vco_freq < DSI_DPHY_PLL_VCO_MIN) {
			continue;
		}

		/* If desired VCO output frequency is too high, no point in
		 * searching further
		 */
		if (vco_freq > DSI_DPHY_PLL_VCO_MAX) {
			break;
		}

		/* Search the best CN and CM values for desired VCO frequency */
		for (cn = DSI_DPHY_PLL_CN_MIN; cn <= DSI_DPHY_PLL_CN_MAX; cn++) {
			refclk_cn_freq = ref_clk / cn;

			/* If the frequency after input divider is too high,
			 * try next CN value
			 */
			if (refclk_cn_freq > DSI_DPHY_PLL_REFCLK_CN_MAX) {
				continue;
			}

			/* If the frequency after input divider is too low,
			 * no point in trying higher dividers.
			 */
			if (refclk_cn_freq < DSI_DPHY_PLL_REFCLK_CN_MIN) {
				break;
			}

			/* Get the closest CM value for this vco frequency
			 * and input divider. Round up, to bias towards higher
			 * frequencies
			 * NOTE: we differ from the SDK algorithm here, which
			 * would round cm to the closest integer
			 */
			cm = (vco_freq + (refclk_cn_freq - 1)) / refclk_cn_freq;

			/* If CM was rounded up to one over valid range,
			 * round down
			 */
			if (cm == (DSI_DPHY_PLL_CM_MAX + 1)) {
				cm = DSI_DPHY_PLL_CM_MAX;
			}

			/* If CM value is still out of range, CN/CO setting won't work */
			if ((cm < DSI_DPHY_PLL_CM_MIN) || (cm > DSI_DPHY_PLL_CM_MAX)) {
				continue;
			}

			/* Calculate candidate frequency */
			cand_freq = (refclk_cn_freq * cm) >> co_shift;

			if (cand_freq < target_freq) {
				/* SKIP frequencies less than target frequency.
				 * this is where the algorithm differs from the
				 * SDK.
				 */
				continue;
			} else {
				if ((cand_freq - target_freq) < best_diff) {
					/* New best CN, CM, and CO found */
					best_diff = (cand_freq - target_freq);
					best_pll_freq = cand_freq;
				}
			}

			if (best_diff == 0U) {
				/* We have found exact match for CN, CM, CO.
				 * return now.
				 */
				return best_pll_freq;
			}
		}
	}
	return best_pll_freq;
}


static int dsi_mcux_attach(const struct device *dev,
			   uint8_t channel,
			   const struct mipi_dsi_device *mdev)
{
	const struct display_mcux_mipi_dsi_config *config = dev->config;
	dsi_dphy_config_t dphy_config;
	dsi_config_t dsi_config;
	uint32_t mipi_dsi_esc_clk_hz;
	uint32_t mipi_dsi_tx_esc_clk_hz;
	uint32_t mipi_dsi_dphy_ref_clk_hz = MIPI_DPHY_REF_CLK;

	DSI_GetDefaultConfig(&dsi_config);
	dsi_config.numLanes = mdev->data_lanes;
	dsi_config.autoInsertEoTp = config->auto_insert_eotp;

	/* Init the DSI module. */
	DSI_Init((MIPI_DSI_Type *)&config->base, &dsi_config);

	/* Init DPHY.
	 *
	 * The DPHY bit clock must be fast enough to send out the pixels, it should be
	 * larger than:
	 *
	 *         (Pixel clock * bit per output pixel) / number of MIPI data lane
	 */
	uint32_t mipi_dsi_dpi_clk_hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Lcdif);
	/* Find the best realizable clock value for the MIPI DSI */
	uint32_t mipi_dsi_dphy_bit_clk_hz =
		dsi_mcux_best_clock(mipi_dsi_dphy_ref_clk_hz, config->phy_clock);
	if (mipi_dsi_dphy_bit_clk_hz == 0) {
		LOG_ERR("DPHY cannot support requested PHY clock");
		return -ENOTSUP;
	}
	/* Cap clock value to max frequency */
	mipi_dsi_dphy_bit_clk_hz = MIN(mipi_dsi_dphy_bit_clk_hz, MIPI_DPHY_MAX_FREQ);

	mipi_dsi_esc_clk_hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Mipi_Esc);
	mipi_dsi_tx_esc_clk_hz = mipi_dsi_esc_clk_hz / 3;

	DSI_GetDphyDefaultConfig(&dphy_config, mipi_dsi_dphy_bit_clk_hz, mipi_dsi_tx_esc_clk_hz);

	mipi_dsi_dphy_bit_clk_hz = DSI_InitDphy((MIPI_DSI_Type *)&config->base,
						&dphy_config, mipi_dsi_dphy_ref_clk_hz);

	LOG_DBG("DPHY clock set to %u", mipi_dsi_dphy_bit_clk_hz);
	/*
	 * If nxp,lcdif node is present, then the MIPI DSI driver will
	 * accept input on the DPI port from the LCDIF, and convert the output
	 * to DSI data. This is useful for video mode, where the LCDIF can
	 * constantly refresh the MIPI panel.
	 */
	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		/* Init DPI interface. */
		DSI_SetDpiConfig((MIPI_DSI_Type *)&config->base,
				&config->dpi_config, mdev->data_lanes,
				mipi_dsi_dpi_clk_hz, mipi_dsi_dphy_bit_clk_hz);
	}

	imxrt_post_init_display_interface();

	return 0;
}

static ssize_t dsi_mcux_transfer(const struct device *dev, uint8_t channel,
				 struct mipi_dsi_msg *msg)
{
	const struct display_mcux_mipi_dsi_config *config = dev->config;
	dsi_transfer_t dsi_xfer = {0};
	status_t status;

	dsi_xfer.virtualChannel = channel;
	dsi_xfer.txDataSize = msg->tx_len;
	dsi_xfer.txData = msg->tx_buf;
	dsi_xfer.rxDataSize = msg->rx_len;
	dsi_xfer.rxData = msg->rx_buf;

	switch (msg->type) {

	case MIPI_DSI_DCS_READ:
		LOG_ERR("DCS Read not yet implemented or used");
		return -ENOTSUP;
	case MIPI_DSI_DCS_SHORT_WRITE:
		dsi_xfer.sendDscCmd = true;
		dsi_xfer.dscCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsShortWrNoParam;
		break;
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		dsi_xfer.sendDscCmd = true;
		dsi_xfer.dscCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsShortWrOneParam;
		break;
	case MIPI_DSI_DCS_LONG_WRITE:
		dsi_xfer.sendDscCmd = true;
		dsi_xfer.dscCmd = msg->cmd;
		dsi_xfer.flags = kDSI_TransferUseHighSpeed;
		dsi_xfer.txDataType = kDSI_TxDataDcsLongWr;
		/*
		 * Cap transfer size. Note that we subtract six bytes here,
		 * one for the DSC command and one to insure that
		 * transfers are still aligned on a pixel boundary
		 * (two or three byte pixel sizes are supported).
		 */
		dsi_xfer.txDataSize = MIN(dsi_xfer.txDataSize,
					(DSI_TX_MAX_PAYLOAD_BYTE - 6));
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		dsi_xfer.txDataType = kDSI_TxDataGenShortWrNoParam;
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		dsi_xfer.txDataType = kDSI_TxDataGenShortWrOneParam;
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		dsi_xfer.txDataType = kDSI_TxDataGenShortWrTwoParam;
		break;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		dsi_xfer.txDataType = kDSI_TxDataGenLongWr;
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		LOG_ERR("Generic Read not yet implemented or used");
		return -ENOTSUP;
	default:
		LOG_ERR("Unsupported message type (%d)", msg->type);
		return -ENOTSUP;
	}

	status = DSI_TransferBlocking(&config->base, &dsi_xfer);

	if (status != kStatus_Success) {
		LOG_ERR("Transmission failed");
		return -EIO;
	}

	if (msg->rx_len != 0) {
		/* Return rx_len on a read */
		return dsi_xfer.rxDataSize;
	}

	/* Return tx_len on a write */
	return dsi_xfer.txDataSize;

}

static struct mipi_dsi_driver_api dsi_mcux_api = {
	.attach = dsi_mcux_attach,
	.transfer = dsi_mcux_transfer,
};

static int display_mcux_mipi_dsi_init(const struct device *dev)
{
	imxrt_pre_init_display_interface();

	return 0;
}

#define MCUX_DSI_DPI_CONFIG(id)									\
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(id), nxp_lcdif),				\
	(.dpi_config = {									\
		.dpiColorCoding = DT_INST_ENUM_IDX(id, dpi_color_coding),			\
		.pixelPacket = DT_INST_ENUM_IDX(id, dpi_pixel_packet),				\
		.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),				\
		.bllpMode = DT_INST_ENUM_IDX(id, dpi_bllp_mode),				\
		.pixelPayloadSize = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, width),		\
		.panelHeight = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, height),			\
		.polarityFlags = (DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),		\
				display_timings), hsync_active) ?				\
				kDSI_DpiHsyncActiveHigh : kDSI_DpiHsyncActiveLow) |		\
				(DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),		\
				display_timings), vsync_active) ?				\
				kDSI_DpiVsyncActiveHigh : kDSI_DpiVsyncActiveLow),		\
		.hfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings), hfront_porch),			\
		.hbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
						display_timings), hback_porch),			\
		.hsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
						display_timings), hsync_len),			\
		.vfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings), vfront_porch),			\
		.vbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
						display_timings), vback_porch),			\
	},))

#define MCUX_MIPI_DSI_DEVICE(id)								\
	static const struct display_mcux_mipi_dsi_config display_mcux_mipi_dsi_config_##id = {	\
		.base = {									\
			.host = (DSI_HOST_Type *)DT_INST_REG_ADDR_BY_IDX(id, 0),		\
			.dpi = (DSI_HOST_DPI_INTFC_Type *)DT_INST_REG_ADDR_BY_IDX(id, 1),	\
			.apb = (DSI_HOST_APB_PKT_IF_Type *)DT_INST_REG_ADDR_BY_IDX(id, 2),	\
			.dphy = (DSI_HOST_NXP_FDSOI28_DPHY_INTFC_Type *)			\
				DT_INST_REG_ADDR_BY_IDX(id, 3),					\
		},										\
		MCUX_DSI_DPI_CONFIG(id)								\
		.auto_insert_eotp = DT_INST_PROP(id, autoinsert_eotp),				\
		.phy_clock = DT_INST_PROP(id, phy_clock),					\
	};											\
	static struct display_mcux_mipi_dsi_data display_mcux_mipi_dsi_data_##id;		\
	DEVICE_DT_INST_DEFINE(id,								\
			    &display_mcux_mipi_dsi_init,					\
			    NULL,								\
			    &display_mcux_mipi_dsi_data_##id,					\
			    &display_mcux_mipi_dsi_config_##id,					\
			    POST_KERNEL,							\
			    CONFIG_MIPI_DSI_INIT_PRIORITY,					\
			    &dsi_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_MIPI_DSI_DEVICE)
