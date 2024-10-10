/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /* Based on dsi_mcux.c, which is (c) 2022 NXP */

#define DT_DRV_COMPAT nxp_mipi_dsi_2l

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_mcux_smartdma.h>
#include <zephyr/logging/log.h>

#include <fsl_inputmux.h>
#include <fsl_mipi_dsi.h>
#include <fsl_clock.h>
#ifdef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA
#include <fsl_smartdma.h>
#endif

#include <soc.h>

LOG_MODULE_REGISTER(dsi_mcux_host, CONFIG_MIPI_DSI_LOG_LEVEL);

struct mcux_mipi_dsi_config {
	MIPI_DSI_HOST_Type *base;
	dsi_dpi_config_t dpi_config;
	bool auto_insert_eotp;
	bool noncontinuous_hs_clk;
	const struct device *bit_clk_dev;
	clock_control_subsys_t bit_clk_subsys;
	const struct device *esc_clk_dev;
	clock_control_subsys_t esc_clk_subsys;
	const struct device *pixel_clk_dev;
	clock_control_subsys_t pixel_clk_subsys;
	uint32_t dphy_ref_freq;
#ifdef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA
	const struct device *smart_dma;
#else
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct mcux_mipi_dsi_data {
	dsi_handle_t mipi_handle;
	struct k_sem transfer_sem;
#ifdef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA
	smartdma_dsi_param_t smartdma_params __aligned(4);
	uint32_t smartdma_stack[32];
	uint8_t dma_slot;
#endif
};


/* MAX DSI TX payload */
#define DSI_TX_MAX_PAYLOAD_BYTE (64U * 4U)


#ifdef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA

/* Callback for DSI DMA transfer completion, called in ISR context */
static void dsi_mcux_dma_cb(const struct device *dma_dev,
				void *user_data, uint32_t channel, int status)
{
	const struct device *dev = user_data;
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	uint32_t int_flags1, int_flags2;

	if (status != 0) {
		LOG_ERR("SMARTDMA transfer failed");
	} else {
		/* Disable DSI interrupts at transfer completion */
		DSI_DisableInterrupts(config->base, kDSI_InterruptGroup1ApbTxDone |
						kDSI_InterruptGroup1HtxTo, 0U);
		DSI_GetAndClearInterruptStatus(config->base, &int_flags1, &int_flags2);
		k_sem_give(&data->transfer_sem);
	}
}

/* Helper function to transfer DSI color (DMA based implementation) */
static int dsi_mcux_tx_color(const struct device *dev, uint8_t channel,
			     struct mipi_dsi_msg *msg)
{
	/*
	 * Color streams are a special case for this DSI peripheral, because
	 * the SMARTDMA peripheral (if enabled) can be used to accelerate
	 * the transfer of data to the DSI. The SMARTDMA has the additional
	 * advantage over traditional DMA of being able to automatically
	 * byte swap color data. This is advantageous, as most graphical
	 * frameworks store RGB data in little endian format, but many
	 * MIPI displays expect color data in big endian format.
	 */
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	int ret;

	if (channel != 0) {
		return -ENOTSUP; /* DMA can only transfer on virtual channel 0 */
	}

	/* Configure smartDMA device, and run transfer */
	data->smartdma_params.p_buffer = msg->tx_buf;
	data->smartdma_params.buffersize = msg->tx_len;

	dma_cfg.dma_callback = dsi_mcux_dma_cb;
	dma_cfg.user_data = (struct device *)dev;
	dma_cfg.head_block = (struct dma_block_config *)&data->smartdma_params;
	dma_cfg.block_count = 1;
	dma_cfg.dma_slot = data->dma_slot;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	ret = dma_config(config->smart_dma, 0, &dma_cfg);
	if (ret < 0) {
		LOG_ERR("Could not configure SMARTDMA");
		return ret;
	}
	/*
	 * SMARTDMA uses DSI interrupt line as input for the DMA
	 * transfer trigger. Therefore, we need to enable DSI TX
	 * interrupts in order to trigger the DMA engine.
	 * Note that if the MIPI IRQ is enabled in
	 * the NVIC, it will fire on every SMARTDMA transfer
	 */
	DSI_EnableInterrupts(config->base, kDSI_InterruptGroup1ApbTxDone |
					kDSI_InterruptGroup1HtxTo, 0U);
	/* Trigger DMA engine */
	ret = dma_start(config->smart_dma, 0);
	if (ret < 0) {
		LOG_ERR("Could not start SMARTDMA");
		return ret;
	}
	/* Wait for TX completion */
	k_sem_take(&data->transfer_sem, K_FOREVER);
	return msg->tx_len;
}

#else /* CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA is not set */

/* Callback for DSI transfer completion, called in ISR context */
static void dsi_transfer_complete(MIPI_DSI_HOST_Type *base,
	dsi_handle_t *handle, status_t status, void *userData)
{
	struct mcux_mipi_dsi_data *data = userData;

	k_sem_give(&data->transfer_sem);
}


/* Helper function to transfer DSI color (Interrupt based implementation) */
static int dsi_mcux_tx_color(const struct device *dev, uint8_t channel,
			     struct mipi_dsi_msg *msg)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;
	status_t status;
	dsi_transfer_t xfer = {
		.virtualChannel = channel,
		.txData = msg->tx_buf,
		.rxDataSize = (uint16_t)msg->rx_len,
		.rxData = msg->rx_buf,
		.sendDscCmd = true,
		.dscCmd = msg->cmd,
		.txDataType = kDSI_TxDataDcsLongWr,
		/* default to high speed unless told to use low power */
		.flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) ? 0 : kDSI_TransferUseHighSpeed,
	};

	/*
	 * Cap transfer size. Note that we subtract six bytes here,
	 * one for the DSC command and five to insure that
	 * transfers are still aligned on a pixel boundary
	 * (two or three byte pixel sizes are supported).
	 */
	xfer.txDataSize = MIN(msg->tx_len, (DSI_TX_MAX_PAYLOAD_BYTE - 6));

	if (IS_ENABLED(CONFIG_MIPI_DSI_MCUX_2L_SWAP16)) {
		/* Manually swap the 16 byte color data in software */
		uint8_t *src = (uint8_t *)xfer.txData;
		uint8_t tmp;

		for (uint32_t i = 0; i < xfer.txDataSize; i += 2) {
			tmp = src[i];
			src[i] = src[i + 1];
			src[i + 1] = tmp;
		}
	}
	/* Send TX data using non-blocking DSI API */
	status = DSI_TransferNonBlocking(config->base,
					&data->mipi_handle, &xfer);
	/* Wait for transfer completion */
	k_sem_take(&data->transfer_sem, K_FOREVER);
	if (status != kStatus_Success) {
		LOG_ERR("Transmission failed");
		return -EIO;
	}
	return xfer.txDataSize;
}

/* ISR is used for DSI interrupt based implementation, unnecessary if DMA is used */
static int mipi_dsi_isr(const struct device *dev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;

	DSI_TransferHandleIRQ(config->base, &data->mipi_handle);
	return 0;
}

#endif

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
	dsi_config.enableNonContinuousHsClk = config->noncontinuous_hs_clk;

	imxrt_pre_init_display_interface();

	/* Init the DSI module. */
	DSI_Init(config->base, &dsi_config);

#ifdef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA
	/* Connect DSI IRQ line to SMARTDMA trigger via
	 * INPUTMUX.
	 */
	/* Attach INPUTMUX from MIPI to SMARTDMA */
	INPUTMUX_Init(INPUTMUX);
	INPUTMUX_AttachSignal(INPUTMUX, 0, kINPUTMUX_MipiIrqToSmartDmaInput);
	/* Gate inputmux clock to save power */
	INPUTMUX_Deinit(INPUTMUX);

	if (!device_is_ready(config->smart_dma)) {
		return -ENODEV;
	}

	struct mcux_mipi_dsi_data *data = dev->data;

	switch (mdev->pixfmt) {
	case MIPI_DSI_PIXFMT_RGB888:
		data->dma_slot = kSMARTDMA_MIPI_RGB888_DMA;
		data->smartdma_params.disablePixelByteSwap = true;
		break;
	case MIPI_DSI_PIXFMT_RGB565:
		data->dma_slot = kSMARTDMA_MIPI_RGB565_DMA;
		if (IS_ENABLED(CONFIG_MIPI_DSI_MCUX_2L_SWAP16)) {
			data->smartdma_params.disablePixelByteSwap = false;
		} else {
			data->smartdma_params.disablePixelByteSwap = true;
		}
		break;
	default:
		LOG_ERR("SMARTDMA does not support pixel_format %u",
			mdev->pixfmt);
		return -ENODEV;
	}

	data->smartdma_params.smartdma_stack = data->smartdma_stack;

	dma_smartdma_install_fw(config->smart_dma,
				(uint8_t *)s_smartdmaDisplayFirmware,
				s_smartdmaDisplayFirmwareSize);
#else
	struct mcux_mipi_dsi_data *data = dev->data;

	/* Create transfer handle */
	if (DSI_TransferCreateHandle(config->base, &data->mipi_handle,
				dsi_transfer_complete, data) != kStatus_Success) {
		return -ENODEV;
	}
#endif

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

	/*
	 * If nxp,lcdif node is present, then the MIPI DSI driver will
	 * accept input on the DPI port from the LCDIF, and convert the output
	 * to DSI data. This is useful for video mode, where the LCDIF can
	 * constantly refresh the MIPI panel.
	 */
	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		/* Init DPI interface. */
		DSI_SetDpiConfig(config->base, &config->dpi_config, mdev->data_lanes,
						dsi_pixel_clk_freq, dphy_bit_clk_freq);
	}

	imxrt_post_init_display_interface();

	return 0;
}

static int dsi_mcux_detach(const struct device *dev, uint8_t channel,
			   const struct mipi_dsi_device *mdev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;

	/* Enable DPHY auto power down */
	DSI_DeinitDphy(config->base);
	/* Fully power off DPHY */
	config->base->PD_DPHY = 0x1;
	/* Deinit MIPI */
	DSI_Deinit(config->base);
	/* Call IMX RT clock function to gate clocks and power at SOC level */
	imxrt_deinit_display_interface();
	return 0;
}



static ssize_t dsi_mcux_transfer(const struct device *dev, uint8_t channel,
				 struct mipi_dsi_msg *msg)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	dsi_transfer_t dsi_xfer = {0};
	status_t status;
	int ret;

	dsi_xfer.virtualChannel = channel;
	dsi_xfer.txDataSize = msg->tx_len;
	dsi_xfer.txData = msg->tx_buf;
	dsi_xfer.rxDataSize = msg->rx_len;
	dsi_xfer.rxData = msg->rx_buf;
	/* default to high speed unless told to use low power */
	dsi_xfer.flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) ? 0 : kDSI_TransferUseHighSpeed;

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
		dsi_xfer.txDataType = kDSI_TxDataDcsLongWr;
		if (msg->flags & MCUX_DSI_2L_FB_DATA) {
			/*
			 * Special case- transfer framebuffer data using
			 * SMARTDMA or non blocking DSI API. The framebuffer
			 * will also be color swapped, if enabled.
			 */
			ret = dsi_mcux_tx_color(dev, channel, msg);
			if (ret < 0) {
				LOG_ERR("Transmission failed");
				return -EIO;
			}
			return ret;
		}
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
		return msg->rx_len;
	}

	/* Return tx_len on a write */
	return msg->tx_len;

}

static struct mipi_dsi_driver_api dsi_mcux_api = {
	.attach = dsi_mcux_attach,
	.detach = dsi_mcux_detach,
	.transfer = dsi_mcux_transfer,
};

static int mcux_mipi_dsi_init(const struct device *dev)
{
	const struct mcux_mipi_dsi_config *config = dev->config;
	struct mcux_mipi_dsi_data *data = dev->data;

#ifndef CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA
	/* Enable IRQ */
	config->irq_config_func(dev);
#endif

	k_sem_init(&data->transfer_sem, 0, 1);

	if (!device_is_ready(config->bit_clk_dev) ||
			!device_is_ready(config->esc_clk_dev) ||
			!device_is_ready(config->pixel_clk_dev)) {
		return -ENODEV;
	}
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
					display_timings),  vsync_active) ?			\
					kDSI_DpiVsyncActiveHigh :				\
					kDSI_DpiVsyncActiveLow) |				\
				(DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),		\
					display_timings),  hsync_active) ?			\
					kDSI_DpiHsyncActiveHigh :				\
					kDSI_DpiHsyncActiveLow),				\
		.hfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings),  hfront_porch),			\
		.hbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings),  hback_porch),			\
		.hsw = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings),  hsync_len),				\
		.vfp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings),  vfront_porch),			\
		.vbp = DT_PROP(DT_CHILD(DT_INST_PHANDLE(id, nxp_lcdif),				\
					display_timings),  vback_porch),			\
	},))

#define MCUX_MIPI_DSI_DEVICE(id)								\
	COND_CODE_1(CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA,						\
	(), (static void mipi_dsi_##n##_irq_config_func(const struct device *dev)		\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority),			\
			mipi_dsi_isr, DEVICE_DT_INST_GET(id), 0);				\
			irq_enable(DT_INST_IRQN(id));						\
	}))											\
												\
	static const struct mcux_mipi_dsi_config mipi_dsi_config_##id = {			\
		MCUX_DSI_DPI_CONFIG(id)								\
		COND_CODE_1(CONFIG_MIPI_DSI_MCUX_2L_SMARTDMA,					\
		(.smart_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, smartdma)),),		\
		(.irq_config_func = mipi_dsi_##n##_irq_config_func,))				\
		.base = (MIPI_DSI_HOST_Type *)DT_INST_REG_ADDR(id),				\
		.auto_insert_eotp = DT_INST_PROP(id, autoinsert_eotp),				\
		.noncontinuous_hs_clk = DT_INST_PROP(id, noncontinuous_hs_clk),			\
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
												\
	static struct mcux_mipi_dsi_data mipi_dsi_data_##id;					\
	DEVICE_DT_INST_DEFINE(id,								\
			    &mcux_mipi_dsi_init,						\
			    NULL,								\
			    &mipi_dsi_data_##id,						\
			    &mipi_dsi_config_##id,						\
			    POST_KERNEL,							\
			    CONFIG_MIPI_DSI_INIT_PRIORITY,					\
			    &dsi_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_MIPI_DSI_DEVICE)
