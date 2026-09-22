/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_dsi_split

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_mipi_dsi.h>

#include <soc.h>

LOG_MODULE_REGISTER(dsi_mcux_split, CONFIG_MIPI_DSI_LOG_LEVEL);

/* MAX DSI TX payload */
#define DSI_TX_MAX_PAYLOAD_BYTE (64U * 4U)

struct mcux_mipi_dsi_config {
	MIPI_DSI_Type base;
	dsi_dpi_config_t dpi_config;
	bool auto_insert_eotp;
	bool continuous_hs_clk;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t phy_clk_freq;
	const struct device *ref_clock_dev;
	clock_control_subsys_t ref_clock_subsys;
	uint32_t pixel_clk_freq;
	const struct device *pixel_clock_dev;
	clock_control_subsys_t pixel_clock_subsys;
	uint32_t esc_rx_clk_frq;
	uint32_t esc_tx_clk_frq;
	clock_control_subsys_t esc_target_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_mipi_dsi_data {
	bool attached;
	bool video_mode_requested;
	bool update_tx_len;
	uint8_t data_lanes;
	uint8_t src_bytes_per_pixel;
	uint32_t data_left_each_line;
	dsi_handle_t mipi_handle;
	/* D-PHY PLL reference rate, read once at init from "ref-source". */
	uint32_t dphy_ref_clk_freq;
	struct k_sem transfer_sem;
};

/* Callback for DSI transfer completion, called in ISR context */
static void dsi_transfer_complete(const MIPI_DSI_Type *base, dsi_handle_t *handle,
				  status_t status, void *userData)
{
	struct device *dev = userData;
	struct mcux_mipi_dsi_data *data = dev->data;

	k_sem_give(&data->transfer_sem);
}

/* ISR is used for DSI interrupt based implementation */
static int mipi_dsi_isr(const struct device *dev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;

	DSI_TransferHandleIRQ(&config->base, &data->mipi_handle);
	return 0;
}

static int dsi_mcux_attach(const struct device *dev, uint8_t channel,
			   const struct mipi_dsi_device *mdev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	dsi_dphy_config_t dphy_config;
	dsi_config_t dsi_config;
	uint32_t dphy_bit_clk_required;
	uint32_t dphy_bit_clk_target;
	uint32_t bit_width;
	uint32_t pixel_clk_freq;
	int ret;

	if (!data->attached) {
		data->video_mode_requested = (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) != 0;

		DSI_GetDefaultConfig(&dsi_config);
		dsi_config.numLanes = mdev->data_lanes;
		dsi_config.autoInsertEoTp = config->auto_insert_eotp;
		dsi_config.enableContinuousHsClk = config->continuous_hs_clk;

		/* Init the DSI module. */
		DSI_Init(&config->base, &dsi_config);

		/* Create transfer handle */
		if (DSI_TransferCreateHandle(&config->base, &data->mipi_handle,
					      dsi_transfer_complete, (void *)dev) !=
		    kStatus_Success) {
			return -ENODEV;
		}

		/*
		 * config->pixel_clk_freq (and dpi_config) is only populated when
		 * in video mode, a command-mode-only panel leaves it zeroed, so
		 * skip this DPI-specific bandwidth check entirely.
		 */
		dphy_bit_clk_target = config->phy_clk_freq;
		if (data->video_mode_requested) {
			switch (config->dpi_config.pixelPacketFormat) {
			case kDSI_DpiPixelPacket16Bit:
				bit_width = 16;
				break;
			case kDSI_DpiPixelPacket18Bit:
				__fallthrough;
			case kDSI_DpiPixelPacket18BitLoosely:
				bit_width = 18;
				break;
			case kDSI_DpiPixelPacket24Bit:
				bit_width = 24;
				break;
			default:
				return -EINVAL; /* Invalid bit width enum value */
			}

			ret = clock_control_get_rate(config->pixel_clock_dev,
						      config->pixel_clock_subsys, &pixel_clk_freq);
			if (ret != 0) {
				return ret;
			}

			/*
			 * The DPHY bit clock must be fast enough to send out the
			 * pixels, it should be larger than:
			 *
			 *   (Pixel clock * bit per output pixel) / number of MIPI data lane
			 */
			dphy_bit_clk_required =
				(pixel_clk_freq * bit_width) / mdev->data_lanes / 96U * 100U;

			dphy_bit_clk_target = dphy_bit_clk_required;
		}

		DSI_GetDphyDefaultConfig(&dphy_config, dphy_bit_clk_target,
			config->esc_tx_clk_frq, config->esc_rx_clk_frq);

		ret = DSI_InitDphy(&config->base, &dphy_config, data->dphy_ref_clk_freq);
		if (ret == 0) {
			/* Cannot generate desired DPHY clock. */
			return -EINVAL;
		}

		data->data_lanes = mdev->data_lanes;
		data->attached = true;

		if (data->video_mode_requested) {
			DSI_SetDpiConfig(&config->base, &config->dpi_config);
		}
	}

	return 0;
}

static int dsi_mcux_detach(const struct device *dev, uint8_t channel,
			   const struct mipi_dsi_device *mdev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;

	/* Power down the DPHY. */
	DSI_DeinitDphy(&config->base);

	/* Deinit MIPI */
	DSI_Deinit(&config->base);

	data->attached = false;
	data->video_mode_requested = false;

	return 0;
}

static ssize_t dsi_mcux_transfer(const struct device *dev, uint8_t channel,
				 struct mipi_dsi_msg *msg)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	dsi_transfer_t dsi_xfer = {0};
	status_t status;
	struct display_buffer_descriptor *desc =
		(struct display_buffer_descriptor *)(msg->user_data);

	/* Get and store the bytes-per-pixel for later usage. */
	if (msg->cmd == MIPI_DCS_SET_PIXEL_FORMAT) {
		if (((uint8_t *)msg->tx_buf)[0] == MIPI_DCS_PIXEL_FORMAT_16BIT) {
			data->src_bytes_per_pixel = 2U;
		} else {
			data->src_bytes_per_pixel = 3U;
		}
	}

	dsi_xfer.virtualChannel = channel;
	dsi_xfer.txDataSize = msg->tx_len;
	dsi_xfer.txData = msg->tx_buf;
	dsi_xfer.rxDataSize = msg->rx_len;
	dsi_xfer.rxData = msg->rx_buf;
	/* default to high speed unless told to use low power */
	dsi_xfer.flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) ? 0 : kDSI_TransferUseHighSpeed;

	/* When the message command is MIPI_DCS_WRITE_MEMORY_START, update tx_len
	 * value if needed.
	 */
	if (msg->cmd == MIPI_DCS_WRITE_MEMORY_START) {
		/* If the higher level passes the display descriptor as user data, it implies
		 * the pitch may be larger than the image width, then update tx_len to the
		 * length of each line.
		 */
		if (desc != NULL) {
			if ((desc->pitch * data->src_bytes_per_pixel) >
			    (msg->tx_len / desc->height)) {
				data->data_left_each_line = desc->width * data->src_bytes_per_pixel;
				msg->tx_len = data->data_left_each_line;
				data->update_tx_len = true;
			} else {
				data->update_tx_len = false;
			}
		}
	/* The descriptor for each memory write is unchanged, so the update_tx_len flag
	 * only need to set once. Then when command is MIPI_DCS_WRITE_MEMORY_CONTINUE,
	 * adjust the tx_len according to the flag.
	 */
	} else if (msg->cmd == MIPI_DCS_WRITE_MEMORY_CONTINUE) {
		if (data->update_tx_len) {
			msg->tx_len = data->data_left_each_line;
		}
	}

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

static DEVICE_API(mipi_dsi, dsi_mcux_api) = {
	.attach = dsi_mcux_attach,
	.detach = dsi_mcux_detach,
	.transfer = dsi_mcux_transfer,
};

static int mcux_mipi_dsi_init(const struct device *dev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	int ret;

	/* Enable IRQ */
	config->irq_config_func(dev);

	k_sem_init(&data->transfer_sem, 0, 1);

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	/* Configure escape clock frequency. */
	ret = clock_control_configure(config->clock_dev, config->esc_target_cfg, NULL);
	if (ret != 0) {
		return ret;
	}

	/* Get the reference clock frequency for DPHY generation in attach */
	ret = clock_control_get_rate(config->ref_clock_dev, config->ref_clock_subsys,
				      &data->dphy_ref_clk_freq);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#define MCUX_MIPI_DSI_BASE(id)								\
	{										\
		.host = DSI2_HOST,							\
		.dpi = DSI2_HOST_VID_IF,						\
		.dbi = DSI2_HOST_DBI_IF,						\
		.apb = DSI2_HOST_APB_PKT_IF,						\
		.dphy = DSI2_TX_PHY,							\
	}

/*
 * The dpi-pixel-packet enum indices (0..3) do not equal the HAL
 * dsi_dpi_pixel_packet_t codes, so map them explicitly. dpi-video-mode does
 * match the HAL enum order (sync-pulse/sync-event/burst = 0/1/2) and is used
 * directly. dpi-bllp-mode maps blank/null/vertical-lp/horizontal-lp to the
 * OR-able _dsi_dpi_bllp_mode flags.
 */
#define MCUX_DSI_PIXEL_PACKET(idx)							\
	((idx) == 0 ? kDSI_DpiPixelPacket16Bit :					\
	 (idx) == 1 ? kDSI_DpiPixelPacket18Bit :					\
	 (idx) == 2 ? kDSI_DpiPixelPacket18BitLoosely :					\
		      kDSI_DpiPixelPacket24Bit)

#define MCUX_DSI_BLLP_MODE(idx)								\
	((idx) == 0 ? kDSI_DpiBllpBlanking :						\
	 (idx) == 1 ? kDSI_DpiBllpNull :						\
	 (idx) == 2 ? kDSI_DpiBllpVerticalLowPower :					\
	 (idx) == 3 ? kDSI_DpiBllpHorizontalLowPower :					\
		      (kDSI_DpiBllpVerticalLowPower | kDSI_DpiBllpHorizontalLowPower))

#define MCUX_DSI_DPI_CONFIG(id)								\
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(id), nxp_lcdif),				\
	(.dpi_config = {									\
		.pixelPacketFormat = MCUX_DSI_PIXEL_PACKET(DT_INST_ENUM_IDX(id, dpi_pixel_packet)),\
		.videoMode = DT_INST_ENUM_IDX(id, dpi_video_mode),			\
		.bllpMode = MCUX_DSI_BLLP_MODE(DT_INST_ENUM_IDX(id, dpi_bllp_mode)),	\
		.enable = 1U,								\
		.pixelPerPacket = DT_INST_PROP_BY_PHANDLE(id, nxp_lcdif, width),		\
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
	},))

/* Only present in video mode. */
#define MCUX_DSI_PIXEL_CLK_FREQ(id)							\
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(id), nxp_lcdif),			\
	(.pixel_clk_freq = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),		\
				display_timings), clock_frequency),))

#define MCUX_MIPI_DSI_DEVICE(id)							\
	static void mipi_dsi_##id##_irq_config_func(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority),		\
			mipi_dsi_isr, DEVICE_DT_INST_GET(id), 0);			\
		irq_enable(DT_INST_IRQN(id));						\
	}										\
											\
	static const struct mcux_mipi_dsi_config mipi_dsi_config_##id = {		\
		MCUX_DSI_DPI_CONFIG(id)							\
		MCUX_DSI_PIXEL_CLK_FREQ(id)						\
		.irq_config_func = mipi_dsi_##id##_irq_config_func,			\
		.base = MCUX_MIPI_DSI_BASE(id),						\
		.auto_insert_eotp = DT_INST_PROP(id, autoinsert_eotp),			\
		.continuous_hs_clk = DT_INST_PROP(id, continuous_hs_clk),		\
		.phy_clk_freq = DT_INST_PROP(id, phy_clock),				\
		.esc_rx_clk_frq = DT_INST_PROP(id, esc_rx_clock),			\
		.esc_tx_clk_frq = DT_INST_PROP(id, esc_tx_clock),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, gate)),	\
		.clock_subsys =								\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(id, gate, name),\
		.esc_target_cfg =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(id, esc_target, name),\
		.ref_clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, ref_source)),	\
		.ref_clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(	\
			id, ref_source, name),						\
		.pixel_clock_dev =							\
			DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, pixel_source)),	\
		.pixel_clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(	\
			id, pixel_source, name),					\
	};										\
											\
	static struct mcux_mipi_dsi_data mipi_dsi_data_##id;				\
	DEVICE_DT_INST_DEFINE(id,							\
			    &mcux_mipi_dsi_init,					\
			    NULL,							\
			    &mipi_dsi_data_##id,					\
			    &mipi_dsi_config_##id,					\
			    POST_KERNEL,						\
			    CONFIG_MIPI_DSI_INIT_PRIORITY,				\
			    &dsi_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_MIPI_DSI_DEVICE)
