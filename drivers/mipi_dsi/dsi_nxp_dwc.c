/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_dsi_dwc

#include <fsl_clock.h>
#include <fsl_mipi_dsi.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dsi_dwc, CONFIG_MIPI_DSI_LOG_LEVEL);

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct dwc_mipi_dsi_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct dwc_mipi_dsi_data *)(_dev)->data)

struct dwc_mipi_dsi_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);

	const struct device *phy_cfg_clk_dev;
	clock_control_subsys_t phy_cfg_clk_subsys;
	struct _clock_root_config_t phy_cfg_clk_config;

	dsi_dpi_config_t dpi_config;
	bool noncontinuous_hs_clk;
	dsi_config_t dsi_config;
	dsi_command_config_t command_config;
	uint32_t dphy_ref_frequency;
	uint32_t data_rate_clock;
};

struct dwc_mipi_dsi_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	const struct device *dev;
};

static int dsi_dwc_attach(const struct device *dev, uint8_t channel,
			  const struct mipi_dsi_device *mdev)
{
	MIPI_DSI_Type *base = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct dwc_mipi_dsi_config *config = dev->config;
	dsi_dphy_config_t dphy_config;
	dsi_config_t dsi_config;
	dsi_dpi_config_t dpi_config = config->dpi_config;
	dsi_command_config_t command_config = config->command_config;
	uint32_t phy_hsfreqrange;
	uint32_t m;
	uint32_t n;
	uint32_t vco_freq;

	DSI_GetDefaultConfig(&dsi_config);
	dphy_config.numLanes = mdev->data_lanes;
	dsi_config.enableNoncontinuousClk = config->noncontinuous_hs_clk;

	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		dsi_config.mode = kDSI_VideoMode;
	} else {
		dsi_config.mode = kDSI_CommandMode;
	}

	/* Init the DSI module. */
	DSI_Init(base, &dsi_config);

	DSI_SetDpiConfig(base, &dpi_config, mdev->data_lanes);

#if CONFIG_SOC_MIMX9352_A55
	uint32_t phyByteClkFreq_Hz = config->data_rate_clock * mdev->data_lanes / 8;

	DSI_SetCommandModeConfig(base, &command_config, phyByteClkFreq_Hz);

	vco_freq = Pll_Set_Pll_Vco_Freq(config->data_rate_clock / 2);

	/* Get the divider value to set to the mediamix block. */
	DSI_DphyGetPllDivider(&m, &n, config->dphy_ref_frequency, vco_freq);

	/* MEDIAMIX */
	/* Clear the bit to reset the clock logic */
	BLK_CTRL_MEDIAMIX->CLK_RESETN.RESET &= ~(MEDIAMIX_BLK_CTRL_RESET_dsi_apb_en_MASK |
						 MEDIAMIX_BLK_CTRL_RESET_ref_clk_en_MASK);

	BLK_CTRL_MEDIAMIX->CLK_RESETN.RESET |=
		(MEDIAMIX_BLK_CTRL_RESET_dsi_apb_en_MASK | MEDIAMIX_BLK_CTRL_RESET_ref_clk_en_MASK);

	BLK_CTRL_MEDIAMIX->MIPI.DSI_W0 =
		MEDIAMIX_BLK_CTRL_DSI_W0_PROP_CNTRL(
			Pll_Set_Pll_Prop_Param(config->data_rate_clock / MHZ(2))) |
		MEDIAMIX_BLK_CTRL_DSI_W0_VCO_CNTRL(
			Pll_Set_Pll_Vco_Param(config->data_rate_clock / MHZ(2))) |
		MEDIAMIX_BLK_CTRL_DSI_W0_N(n) | MEDIAMIX_BLK_CTRL_DSI_W0_M(m);

	BLK_CTRL_MEDIAMIX->MIPI.DSI_W1 =
		MEDIAMIX_BLK_CTRL_DSI_W1_CPBIAS_CNTRL(0x10) | MEDIAMIX_BLK_CTRL_DSI_W1_GMP_CNTRL(1);

#endif

	/* Calculate data rate per line */
	DSI_GetDefaultDphyConfig(&dphy_config, config->data_rate_clock * mdev->data_lanes / 8,
				 mdev->data_lanes);
	DSI_InitDphy(base, &dphy_config);
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

	DSI_ConfigDphy(base, config->dphy_ref_frequency, config->data_rate_clock);
#endif

#if CONFIG_SOC_MIMX9352_A55
	BLK_CTRL_MEDIAMIX->MIPI.DSI = MEDIAMIX_BLK_CTRL_DSI_updatepll(1) |
				      MEDIAMIX_BLK_CTRL_DSI_HSFREQRANGE(phy_hsfreqrange) |
				      MEDIAMIX_BLK_CTRL_DSI_CLKSEL(1) |
				      MEDIAMIX_BLK_CTRL_DSI_CFGCLKFREQRANGE(0x1c);

#endif
	status_t result = DSI_PowerUp(base);

	if (result != 0U) {
		LOG_ERR("DSI PHY init failed.\r\n");
	}

	return result;
}

static ssize_t dsi_dwc_transfer(const struct device *dev, uint8_t channel, struct mipi_dsi_msg *msg)
{
	MIPI_DSI_Type *base = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
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

	status = DSI_TransferBlocking(base, &dsi_xfer);

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
	MIPI_DSI_Type *base = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	DSI_EnableCommandMode(base, false);

	return 0;
}

static DEVICE_API(mipi_dsi, dsi_dwc_api) = {
	.attach = dsi_dwc_attach,
	.transfer = dsi_dwc_transfer,
	.detach = dsi_dwc_detach,
};

static int dwc_mipi_dsi_init(const struct device *dev)
{
	const struct dwc_mipi_dsi_config *config = dev->config;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	clock_control_set_rate(config->phy_cfg_clk_dev, config->phy_cfg_clk_subsys,
			       (clock_control_subsys_rate_t)&config->dphy_ref_frequency);
	return 0;
}

#define DWC_DSI_DPI_CONFIG(id)                                                                     \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(id), nxp_dc),					\
	(.dpi_config = {									\
		.virtualChannel   = 0U,								\
		.colorCoding = DT_INST_ENUM_IDX(id, dpi_color_coding),				\
		.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),				\
		.pixelPayloadSize = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, width),			\
		.panelHeight = DT_INST_PROP_BY_PHANDLE(id, nxp_dc, height),			\
		.enableAck        = false,							\
		.enablelpSwitch   = true,							\
		.pattern          = kDSI_PatternDisable,					\
		.polarityFlags = (kDSI_DpiVsyncActiveLow | kDSI_DpiHsyncActiveLow),		\
		.hfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), hfront_porch),                        \
		.hbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), hback_porch),                         \
		.hsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), hsync_len),                           \
		.vfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), vfront_porch),			\
		.vbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), vback_porch),                         \
		.vsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_dc),				\
					display_timings), vsync_len),                           \
	},))

#define DWC_MIPI_DSI_DEVICE(id)                                                                    \
	static const struct dwc_mipi_dsi_config mipi_dsi_config_##id = {                           \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(id)),                             \
		.phy_cfg_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(id, 0)),               \
		.phy_cfg_clk_subsys =                                                              \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 0, name),           \
		.phy_cfg_clk_config =                                                              \
			{                                                                          \
				.clockOff = false,                                                 \
			},                                                                         \
		DWC_DSI_DPI_CONFIG(id).dsi_config =                                                \
			{                                                                          \
				.mode = kDSI_VideoMode,                                            \
				.packageFlags =                                                    \
					kDSI_DpiEnableBta | kDSI_DpiEnableEcc | kDSI_DpiEnableCrc, \
				.enableNoncontinuousClk = false,                                   \
				.HsRxDeviceReady_ByteClk = 0U,                                     \
				.lpRxDeviceReady_ByteClk = 0U,                                     \
				.HsTxDeviceReady_ByteClk = 0U,                                     \
				.lpTxDeviceReady_ByteClk = 0U,                                     \
			},                                                                         \
		.command_config =                                                                  \
			{                                                                          \
				.escClkFreq_Hz = 20000000,                                         \
				.btaTo_Ns = 10000,                                                 \
				.hsTxTo_Ns = 60000,                                                \
				.lpRxTo_Ns = 60000,                                                \
			},                                                                         \
		.data_rate_clock = DT_INST_PROP(id, data_rate_clock),                              \
		.dphy_ref_frequency = DT_INST_PROP(id, dphy_ref_frequency),                        \
	};                                                                                         \
                                                                                                   \
	static struct dwc_mipi_dsi_data mipi_dsi_data_##id;                                        \
	DEVICE_DT_INST_DEFINE(id, &dwc_mipi_dsi_init, NULL, &mipi_dsi_data_##id,                   \
			      &mipi_dsi_config_##id, POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY,   \
			      &dsi_dwc_api);

DT_INST_FOREACH_STATUS_OKAY(DWC_MIPI_DSI_DEVICE)
