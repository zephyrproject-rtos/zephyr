/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /* Based on dsi_mcux.c, which is (c) 2022 NXP */

#define DT_DRV_COMPAT nxp_mipi_dsi_2l

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <fsl_mipi_dsi.h>
#include <fsl_clock.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(dsi_mcux_host, CONFIG_MIPI_DSI_LOG_LEVEL);

struct mcux_mipi_dsi_config {
	MIPI_DSI_HOST_Type *base;
	dsi_dpi_config_t dpi_config;
	bool auto_insert_eotp;
	const struct device *bit_clk_dev;
	clock_control_subsys_t bit_clk_subsys;
	const struct device *esc_clk_dev;
	clock_control_subsys_t esc_clk_subsys;
	const struct device *pixel_clk_dev;
	clock_control_subsys_t pixel_clk_subsys;
	uint32_t dphy_ref_freq;
};

static int dsi_mcux_attach(const struct device *dev,
			   uint8_t channel,
			   const struct mipi_dsi_device *mdev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	dsi_dphy_config_t dphy_config;
	dsi_config_t dsi_config;
	uint32_t dphy_bit_clk_freq;
	uint32_t dphy_esc_clk_freq;
	uint32_t dsi_pixel_clk_freq;
	uint32_t bit_width;

	DSI_GetDefaultConfig(&dsi_config);
	dsi_config.numLanes = mdev->data_lanes;
	dsi_config.autoInsertEoTp = config->auto_insert_eotp;

	/* Init the DSI module. */
	DSI_Init(config->base, &dsi_config);

	/* Get the DPHY bit clock frequency */
	if (clock_control_get_rate(config->bit_clk_dev,
				config->bit_clk_subsys,
				&dphy_bit_clk_freq)) {
		return -EINVAL;
	};
	/* Get the DPHY ESC clock frequency */
	if (clock_control_get_rate(config->esc_clk_dev,
				config->esc_clk_subsys,
				&dphy_esc_clk_freq)) {
		return -EINVAL;
	}
	/* Get the Pixel clock frequency */
	if (clock_control_get_rate(config->pixel_clk_dev,
				config->pixel_clk_subsys,
				&dsi_pixel_clk_freq)) {
		return -EINVAL;
	}

	switch (config->dpi_config.pixelPacket) {
	case kDSI_PixelPacket16Bit:
		bit_width = 16;
		break;
	case kDSI_PixelPacket18Bit:
		__fallthrough;
	case kDSI_PixelPacket18BitLoosely:
		bit_width = 18;
		break;
	case kDSI_PixelPacket24Bit:
		bit_width = 24;
		break;
	default:
		return -EINVAL; /* Invalid bit width enum value? */
	}
	/* Init DPHY.
	 *
	 * The DPHY bit clock must be fast enough to send out the pixels, it should be
	 * larger than:
	 *
	 *         (Pixel clock * bit per output pixel) / number of MIPI data lane
	 */
	if (((dsi_pixel_clk_freq * bit_width) / mdev->data_lanes) > dphy_bit_clk_freq) {
		return -EINVAL;
	}

	DSI_GetDphyDefaultConfig(&dphy_config, dphy_bit_clk_freq, dphy_esc_clk_freq);

	if (config->dphy_ref_freq != 0) {
		dphy_bit_clk_freq = DSI_InitDphy(config->base,
					&dphy_config, config->dphy_ref_freq);
	} else {
		/* DPHY PLL is not present, ref clock is unused */
		DSI_InitDphy(config->base, &dphy_config, 0);
	}

	/* Init DPI interface. */
	DSI_SetDpiConfig(config->base, &config->dpi_config, mdev->data_lanes,
					dsi_pixel_clk_freq, dphy_bit_clk_freq);

	imxrt_post_init_display_interface();

	return 0;
}

static ssize_t dsi_mcux_transfer(const struct device *dev, uint8_t channel,
				 struct mipi_dsi_msg *msg)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
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

	status = DSI_TransferBlocking(config->base, &dsi_xfer);

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

static int mcux_mipi_dsi_init(const struct device *dev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;

	imxrt_pre_init_display_interface();

	if (!device_is_ready(config->bit_clk_dev) ||
			!device_is_ready(config->esc_clk_dev) ||
			!device_is_ready(config->pixel_clk_dev)) {
		return -ENODEV;
	}
	return 0;
}

#define MCUX_MIPI_DSI_DEVICE(id)								\
	static const struct mcux_mipi_dsi_config mipi_dsi_config_##id = {	\
		.base = (MIPI_DSI_HOST_Type *)DT_INST_REG_ADDR(id),				\
		.dpi_config = {									\
			.dpiColorCoding = DT_INST_ENUM_IDX(id, dpi_color_coding),		\
			.pixelPacket = DT_INST_ENUM_IDX(id, dpi_pixel_packet),			\
			.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),			\
			.bllpMode = DT_INST_ENUM_IDX(id, dpi_bllp_mode),			\
			.pixelPayloadSize = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, width),	\
			.panelHeight = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, height),		\
			.polarityFlags = (DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),	\
						display_timings),  vsync_active) ?		\
						kDSI_DpiVsyncActiveHigh :			\
						kDSI_DpiVsyncActiveLow) |			\
					(DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),	\
						display_timings),  hsync_active) ?		\
						kDSI_DpiHsyncActiveHigh :			\
						kDSI_DpiHsyncActiveLow),			\
			.hfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings),  hfront_porch),		\
			.hbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings),  hback_porch),		\
			.hsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings),  hsync_len),			\
			.vfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings),  vfront_porch),		\
			.vbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),			\
						display_timings),  vback_porch),		\
		},										\
		.auto_insert_eotp = DT_INST_PROP(id, autoinsert_eotp),				\
		.dphy_ref_freq = DT_INST_PROP_OR(id, dphy_ref_frequency, 0),			\
		.bit_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, dphy)),		\
		.bit_clk_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(id, dphy, name),	\
		.esc_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, esc)),		\
		.esc_clk_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(id, esc, name),	\
		.pixel_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, pixel)),		\
		.pixel_clk_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(id, pixel, name),	\
	};											\
	DEVICE_DT_INST_DEFINE(id,								\
			    &mcux_mipi_dsi_init,						\
			    NULL,								\
			    NULL,								\
			    &mipi_dsi_config_##id,						\
			    POST_KERNEL,							\
			    CONFIG_MIPI_DSI_INIT_PRIORITY,					\
			    &dsi_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_MIPI_DSI_DEVICE)
