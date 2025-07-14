/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_dsi_dwc

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/logging/log.h>
#include <fsl_mipi_dsi.h>
#include <fsl_clock.h>
#include <soc.h>

LOG_MODULE_REGISTER(dsi_dwc, CONFIG_MIPI_DSI_LOG_LEVEL);

struct dwc_mipi_dsi_config {
	MIPI_DSI_Type *base;
	dsi_dpi_config_t dpi_config;
	bool noncontinuous_hs_clk;
	dsi_config_t dsi_config;
	uint32_t dphy_ref_frequency;
	uint32_t data_rate_clock;
	const struct device *dev;
};

struct dwc_mipi_dsi_data {
	uint16_t flags;
	uint8_t lane_mask;
};

static int dsi_dwc_attach(const struct device *dev, uint8_t channel,
			  const struct mipi_dsi_device *mdev)
{
	const struct dwc_mipi_dsi_config *config = dev->config;
	dsi_dphy_config_t dphy_config;
	dsi_config_t dsi_config;
	uint32_t phy_hsfreqrange;

	DSI_GetDefaultConfig(&dsi_config);
	dphy_config.numLanes = mdev->data_lanes;
	dsi_config.enableNoncontinuousClk = config->noncontinuous_hs_clk;

	/* Init the DSI module. */
	DSI_Init(config->base, &dsi_config);

	DSI_SetDpiConfig(config->base, &config->dpi_config, mdev->data_lanes);

	/* Calculate data rate per line */
	DSI_GetDefaultDphyConfig(&dphy_config, config->data_rate_clock * mdev->data_lanes / 8,
				 mdev->data_lanes);
	DSI_InitDphy(config->base, &dphy_config);
	phy_hsfreqrange = Pll_Set_Hs_Freqrange(config->data_rate_clock);
#if CONFIG_SOC_MIMX9596_M7
	CAMERA__DSI_OR_CSI_PHY_CSR->COMBO_PHY_FREQ_CONTROL =
		CAMERA_DSI_OR_CSI_PHY_CSR_COMBO_PHY_FREQ_CONTROL_Phy_hsfreqrange(phy_hsfreqrange) |
		CAMERA_DSI_OR_CSI_PHY_CSR_COMBO_PHY_FREQ_CONTROL_Phy_cfgclkfreqrange(0x1CU);
	CAMERA__DSI_MASTER_CSR->DSI_PIXEL_LINK_CONTROL =
		CAMERA_DSI_MASTER_CSR_DSI_PIXEL_LINK_CONTROL_Pixel_link_sel(0x0);
	DISPLAY__BLK_CTRL_DISPLAYMIX->PIXEL_LINK_CTRL =
		(DISPLAY_BLK_CTRL_DISPLAYMIX_PIXEL_LINK_CTRL_PL0_enable(0x1) |
		 DISPLAY_BLK_CTRL_DISPLAYMIX_PIXEL_LINK_CTRL_PL0_valid(0x1));
	CAMERA__DSI_OR_CSI_PHY_CSR->COMBO_PHY_MODE_CONTROL = 0x3U;
#endif
	DSI_ConfigDphy(config->base, config->dphy_ref_frequency, config->data_rate_clock);
	status_t result = DSI_PowerUp(config->base);

	if (result != 0U) {
		LOG_ERR("DSI PHY init failed.\r\n");
	}

	return 0;
}

static ssize_t dsi_dwc_transfer(const struct device *dev, uint8_t channel, struct mipi_dsi_msg *msg)
{
	const struct dwc_mipi_dsi_config *config = dev->config;
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
		dsi_xfer.sendDcsCmd = true;
		dsi_xfer.dcsCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsShortWrNoParam;
		break;
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		dsi_xfer.sendDcsCmd = true;
		dsi_xfer.dcsCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsShortWrOneParam;
		break;

	case MIPI_DSI_DCS_LONG_WRITE:
		dsi_xfer.sendDcsCmd = true;
		dsi_xfer.dcsCmd = msg->cmd;
		dsi_xfer.txDataType = kDSI_TxDataDcsLongWr;
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

	status = DSI_TransferBlocking(config->base, &dsi_xfer);

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

static int dsi_dwc_detach(const struct device *dev, uint8_t channel,
			  const struct mipi_dsi_device *mdev)
{
	const struct dwc_mipi_dsi_config *config = dev->config;

	DSI_EnableCommandMode(config->base, false);

	return 0;
}

static DEVICE_API(mipi_dsi, dsi_dwc_api) = {
	.attach = dsi_dwc_attach,
	.transfer = dsi_dwc_transfer,
	.detach = dsi_dwc_detach,
};

static int dwc_mipi_dsi_init(const struct device *dev)
{
	return 0;
}

#define DWC_DSI_DPI_CONFIG(id)                                                                     \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(id), nxp_dc),					\
		(.dpi_config = {								\
		.virtualChannel   = 0U,								\
		.colorCoding = DT_INST_ENUM_IDX(id, dpi_color_coding),				\
		.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),				\
		.pixelPayloadSize = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, width),			\
		.panelHeight = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, height),			\
		.enableAck        = false,							\
		.enablelpSwitch   = true,							\
		.pattern          = kDSI_PatternDisable,					\
		.polarityFlags = (kDSI_DpiVsyncActiveLow | kDSI_DpiHsyncActiveLow),		\
		.hfp = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, hfp),				\
		.hbp = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, hbp),				\
		.hsw = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, hsw),				\
		.vfp = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, vfp),				\
		.vbp = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, vbp),				\
		.vsw = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, vsw),				\
	}))

#define DWC_MIPI_DSI_DEVICE(id)                                                                    \
	static const struct dwc_mipi_dsi_config mipi_dsi_config_##id = {                           \
		.base = (MIPI_DSI_Type *)DT_INST_REG_ADDR(id),                                     \
		.data_rate_clock = DT_INST_PROP(id, data_rate_clock),                              \
		.dphy_ref_frequency = DT_INST_PROP(id, dphy_ref_frequency),                        \
		DWC_DSI_DPI_CONFIG(id),                                                            \
	};                                                                                         \
                                                                                                   \
	static struct dwc_mipi_dsi_data mipi_dsi_data_##id;                                        \
	DEVICE_DT_INST_DEFINE(id, &dwc_mipi_dsi_init, NULL, &mipi_dsi_data_##id,                   \
			      &mipi_dsi_config_##id, POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY,   \
			      &dsi_dwc_api);

DT_INST_FOREACH_STATUS_OKAY(DWC_MIPI_DSI_DEVICE)
