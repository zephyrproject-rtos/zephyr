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

/*
 * The DPHY bit clock must be fast enough to send out the pixels, it should be
 * larger than:
 *
 *         (Pixel clock * bit per output pixel) / number of MIPI data lane
 *
 * Here the desired DPHY bit clock multiplied by ( 9 / 8 = 1.125) to ensure
 * it is fast enough.
 */
#define MIPI_DPHY_BIT_CLK_ENLARGE(origin) (((origin) / 8) * 9)

struct display_mcux_mipi_dsi_config {
	MIPI_DSI_Type base;
	dsi_dpi_config_t dpi_config;
	bool auto_insert_eotp;
	uint32_t phy_clock;
};

struct display_mcux_mipi_dsi_data {
	const struct device *dev;
};

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
	 *
	 * Here the desired DPHY bit clock multiplied by ( 9 / 8 = 1.125) to ensure
	 * it is fast enough.
	 *
	 * Note that the DSI output pixel is 24bit per pixel.
	 */
	uint32_t mipi_dsi_dpi_clk_hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Lcdif);
	uint32_t mipi_dsi_dphy_bit_clk_hz = config->phy_clock;

	mipi_dsi_dphy_bit_clk_hz = MIPI_DPHY_BIT_CLK_ENLARGE(mipi_dsi_dphy_bit_clk_hz);

	mipi_dsi_esc_clk_hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Mipi_Esc);
	mipi_dsi_tx_esc_clk_hz = mipi_dsi_esc_clk_hz / 3;

	DSI_GetDphyDefaultConfig(&dphy_config, mipi_dsi_dphy_bit_clk_hz, mipi_dsi_tx_esc_clk_hz);

	mipi_dsi_dphy_bit_clk_hz = DSI_InitDphy((MIPI_DSI_Type *)&config->base,
						&dphy_config, mipi_dsi_dphy_ref_clk_hz);
	/* Init DPI interface. */
	DSI_SetDpiConfig((MIPI_DSI_Type *)&config->base, &config->dpi_config, mdev->data_lanes,
					mipi_dsi_dpi_clk_hz, mipi_dsi_dphy_bit_clk_hz);

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
		__fallthrough;
	case MIPI_DSI_DCS_LONG_WRITE:
		dsi_xfer.sendDscCmd = true;
		dsi_xfer.dscCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsShortWrOneParam;
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
		return msg->rx_len;
	}

	/* Return tx_len on a write */
	return msg->tx_len;

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

#define MCUX_MIPI_DSI_DEVICE(id)								\
	static const struct display_mcux_mipi_dsi_config display_mcux_mipi_dsi_config_##id = {	\
		.base = {									\
			.host = (DSI_HOST_Type *)DT_INST_REG_ADDR_BY_IDX(id, 0),		\
			.dpi = (DSI_HOST_DPI_INTFC_Type *)DT_INST_REG_ADDR_BY_IDX(id, 1),	\
			.apb = (DSI_HOST_APB_PKT_IF_Type *)DT_INST_REG_ADDR_BY_IDX(id, 2),	\
			.dphy = (DSI_HOST_NXP_FDSOI28_DPHY_INTFC_Type *)			\
				DT_INST_REG_ADDR_BY_IDX(id, 3),					\
		},										\
		.dpi_config = {									\
			.dpiColorCoding = DT_INST_ENUM_IDX(id, dpi_color_coding),		\
			.pixelPacket = DT_INST_ENUM_IDX(id, dpi_pixel_packet),			\
			.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),			\
			.bllpMode = DT_INST_ENUM_IDX(id, dpi_bllp_mode),			\
			.pixelPayloadSize = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, width),	\
			.panelHeight = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, height),		\
			.polarityFlags = (DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),	\
					display_timings), hsync_active) ?			\
					kDSI_DpiHsyncActiveHigh : kDSI_DpiHsyncActiveLow) |	\
					(DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),	\
					display_timings), vsync_active) ?			\
					kDSI_DpiVsyncActiveHigh : kDSI_DpiVsyncActiveLow),	\
			.hfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings), hfront_porch),		\
			.hbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
							display_timings), hback_porch),		\
			.hsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
							display_timings), hsync_len),		\
			.vfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings), vfront_porch),		\
			.vbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
							display_timings), vback_porch),		\
		},										\
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
