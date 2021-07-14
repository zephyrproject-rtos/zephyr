/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_mipi_dsi

#include <zephyr.h>

#include <fsl_mipi_dsi.h>
#include <fsl_clock.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <drivers/video.h>

/*
 * The DPHY bit clock must be fast enough to send out the pixels, it should be
 * larger than:
 *
 *         (Pixel clock * bit per output pixel) / number of MIPI data lane
 *
 * Here the desired DPHY bit clock multiplied by ( 9 / 8 = 1.125) to ensure
 * it is fast enough.
 */
#define DEMO_MIPI_DPHY_BIT_CLK_ENLARGE(origin) (((origin) / 8) * 9)

struct display_mcux_mipi_dsi_config {
	MIPI_DSI_Type base;
};

struct display_mcux_mipi_dsi_data {
	const struct device *dev;
	VIDEO_MUX_Type *video_mux_base;
	PGMC_BPC_Type *pgmc_bpc_base;
	IOMUXC_GPR_Type *iomuxc_base;
	uint8_t virtualChannel;
};

int mipi_dsi_dcs_write(struct device *dev, uint8_t *txData, uint8_t txDataSize)
{
	struct display_mcux_mipi_dsi_data *data = dev->data;
	const struct display_mcux_mipi_dsi_config *config = dev->config;

	dsi_transfer_t dsiXfer = {0};

	dsiXfer.virtualChannel = data->virtualChannel;
	dsiXfer.txDataSize = txDataSize;
	dsiXfer.txData = txData;

	if (txDataSize == 0) {
		dsiXfer.txDataType = kDSI_TxDataGenShortWrNoParam;
	} else if (txDataSize == 1) {
		dsiXfer.txDataType = kDSI_TxDataGenShortWrOneParam;
	} else if (txDataSize == 2) {
		dsiXfer.txDataType = kDSI_TxDataGenShortWrTwoParam;
	} else {
		dsiXfer.txDataType = kDSI_TxDataGenLongWr;
	}

	return DSI_TransferBlocking((MIPI_DSI_Type *)&config->base, &dsiXfer);
}

static int display_mcux_mipi_dsi_init(const struct device *dev)
{
	struct display_mcux_mipi_dsi_data *data = dev->data;
	const struct display_mcux_mipi_dsi_config *config = dev->config;
	uint32_t mipiDsiEscClkFreq_Hz;
	uint32_t mipiDsiTxEscClkFreq_Hz;
	uint32_t mipiDsiDphyRefClkFreq_Hz = 24000000;
	dsi_config_t dsiConfig;
	dsi_dphy_config_t dphyConfig;

    /* elcdif output to MIPI DSI */
	CLOCK_EnableClock(kCLOCK_Video_Mux);
	data->video_mux_base->VID_MUX_CTRL.CLR = 0x04;

	/* power on and isolation off */
	data->pgmc_bpc_base->BPC_POWER_CTRL |=
		(PGMC_BPC_BPC_POWER_CTRL_PSW_ON_SOFT_MASK |
		PGMC_BPC_BPC_POWER_CTRL_ISO_OFF_SOFT_MASK);

	/* assert mipi reset */
	data->iomuxc_base->GPR62 &=
		~(IOMUXC_GPR_GPR62_MIPI_DSI_PCLK_SOFT_RESET_N_MASK |
		IOMUXC_GPR_GPR62_MIPI_DSI_ESC_SOFT_RESET_N_MASK |
		IOMUXC_GPR_GPR62_MIPI_DSI_BYTE_SOFT_RESET_N_MASK |
		IOMUXC_GPR_GPR62_MIPI_DSI_DPI_SOFT_RESET_N_MASK);

	/* setup clock */
	const clock_root_config_t mipiEscClockConfig = {
		.clockOff = false,
		.mux      = 4,
		.div      = 11,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Mipi_Esc, &mipiEscClockConfig);
	mipiDsiEscClkFreq_Hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Mipi_Esc);

	const clock_group_config_t mipiEscClockGroupConfig = {
		.clockOff = false,
		.resetDiv = 2,
		.div0     = 2,  /* TX esc clock */
	};

	CLOCK_SetGroupConfig(kCLOCK_Group_MipiDsi, &mipiEscClockGroupConfig);

	mipiDsiTxEscClkFreq_Hz = mipiDsiEscClkFreq_Hz / 3;

	const clock_root_config_t mipiDphyRefClockConfig = {
		.clockOff = false,
		.mux      = 1,
		.div      = 1,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Mipi_Ref, &mipiDphyRefClockConfig);

	data->iomuxc_base->GPR62 |=
		(IOMUXC_GPR_GPR62_MIPI_DSI_PCLK_SOFT_RESET_N_MASK |
		IOMUXC_GPR_GPR62_MIPI_DSI_ESC_SOFT_RESET_N_MASK);

	/* configure peripheral */
	const dsi_dpi_config_t dpiConfig = {
						.pixelPayloadSize = 720,
						.dpiColorCoding = kDSI_Dpi24Bit,
						.pixelPacket    = kDSI_PixelPacket24Bit,
						.videoMode      = kDSI_DpiBurst,
						.bllpMode       = kDSI_DpiBllpLowPower,
						.polarityFlags  =
						kDSI_DpiVsyncActiveLow | kDSI_DpiHsyncActiveLow,
						.hfp            = 32,
						.hbp            = 32,
						.hsw            = 8,
						.vfp            = 16,
						.vbp            = 14,
						.panelHeight    = 1280,
						.virtualChannel = data->virtualChannel};

	/*
	 * dsiConfig.numLanes = 4;
	 * dsiConfig.enableNonContinuousHsClk = false;
	 * dsiConfig.autoInsertEoTp = true;
	 * dsiConfig.numExtraEoTp = 0;
	 * dsiConfig.htxTo_ByteClk = 0;
	 * dsiConfig.lrxHostTo_ByteClk = 0;
	 * dsiConfig.btaTo_ByteClk = 0;
	 */
	DSI_GetDefaultConfig(&dsiConfig);
	dsiConfig.numLanes       = 2;
	dsiConfig.autoInsertEoTp = true;

	/* Init the DSI module. */
	DSI_Init((MIPI_DSI_Type *)&config->base, &dsiConfig);

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
	uint32_t mipiDsiDpiClkFreq_Hz = CLOCK_GetRootClockFreq(kCLOCK_Root_Lcdif);
	uint32_t mipiDsiDphyBitClkFreq_Hz = mipiDsiDpiClkFreq_Hz * (24 / 2);

	mipiDsiDphyBitClkFreq_Hz = DEMO_MIPI_DPHY_BIT_CLK_ENLARGE(mipiDsiDphyBitClkFreq_Hz);

	DSI_GetDphyDefaultConfig(&dphyConfig, mipiDsiDphyBitClkFreq_Hz, mipiDsiTxEscClkFreq_Hz);


	mipiDsiDphyBitClkFreq_Hz = DSI_InitDphy((MIPI_DSI_Type *)&config->base,
						&dphyConfig, mipiDsiDphyRefClkFreq_Hz);

	/* Init DPI interface. */
	DSI_SetDpiConfig((MIPI_DSI_Type *)&config->base, &dpiConfig, 2,
					mipiDsiDpiClkFreq_Hz, mipiDsiDphyBitClkFreq_Hz);

	/* deassert BYTE and DBI reset */
	data->iomuxc_base->GPR62 |=
		(IOMUXC_GPR_GPR62_MIPI_DSI_BYTE_SOFT_RESET_N_MASK |
		IOMUXC_GPR_GPR62_MIPI_DSI_DPI_SOFT_RESET_N_MASK);

	/* configure the panel */

	return 0;
}

/* Unique Instance */
static const struct display_mcux_mipi_dsi_config display_mcux_mipi_dsi_config_0 = {
	.base = {
				.host = (DSI_HOST_Type *)DT_INST_REG_ADDR_BY_IDX(0, 0),
				.dpi = (DSI_HOST_DPI_INTFC_Type *)DT_INST_REG_ADDR_BY_IDX(0, 1),
				.apb = (DSI_HOST_APB_PKT_IF_Type *)DT_INST_REG_ADDR_BY_IDX(0, 2),
				.dphy = (DSI_HOST_NXP_FDSOI28_DPHY_INTFC_Type *)
						DT_INST_REG_ADDR_BY_IDX(0, 3),
			},
};

static struct display_mcux_mipi_dsi_data display_mcux_mipi_dsi_data_0;

static int display_mcux_mipi_dsi_init_0(const struct device *dev)
{
	struct display_mcux_mipi_dsi_data *data = dev->data;

	data->dev = dev;
	data->video_mux_base = (VIDEO_MUX_Type *)DT_REG_ADDR(DT_NODELABEL(video_mux));
	data->pgmc_bpc_base = (PGMC_BPC_Type *)DT_REG_ADDR(DT_NODELABEL(pgmc_bpc4));
	data->iomuxc_base = (IOMUXC_GPR_Type *)DT_REG_ADDR(DT_NODELABEL(iomuxc_gpr));
	data->virtualChannel = DT_INST_PROP(0, virtualchannel);
	return display_mcux_mipi_dsi_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &display_mcux_mipi_dsi_init_0,
		    device_pm_control_nop, &display_mcux_mipi_dsi_data_0,
		    &display_mcux_mipi_dsi_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    NULL);
