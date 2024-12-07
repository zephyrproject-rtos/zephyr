/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dcnano_lcdif_dbi

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

LOG_MODULE_REGISTER(mipi_dbi_nxp_dcnano_lcdif, CONFIG_DISPLAY_LOG_LEVEL);

#if CONFIG_MIPI_DSI
#define MIPI_DSI_MAX_PAYLOAD_SIZE 0xFFFF
#endif

struct mcux_dcnano_lcdif_dbi_data {
	struct k_sem transfer_done;
	uint8_t *data;
	uint8_t dsiFormat;
	uint16_t pitch;
	uint16_t heightEachWrite;
	uint16_t heightSent;
	uint16_t height;
	uint16_t width;
	uint16_t stride;
};

struct mcux_dcnano_lcdif_dbi_config {
	LCDIF_Type *base;
	MIPI_DSI_HOST_Type *dsi_base;
	void (*irq_config_func)(const struct device *dev);
	lcdif_dbi_config_t dbi_config;
	lcdif_panel_config_t panel_config;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec reset;
};

static void mcux_dcnano_lcdif_dbi_isr(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;
	uint32_t status;

	status = LCDIF_GetAndClearInterruptPendingFlags(config->base);

	if (0 != (status & kLCDIF_Display0FrameDoneInterrupt))
	{
		/* Handle multi chunk with mipi. */
#if CONFIG_MIPI_DSI
		MIPI_DSI_HOST_Type *mipi_base = config->dsi_base;
		uint16_t height			   = lcdif_data->heightEachWrite;

		/* There is left data to send */
		if (lcdif_data->height > 0)
		{
			if (lcdif_data->heightEachWrite > lcdif_data->height)
			{
				/* Change height, payload size and buffer for the last piece. */
				height = lcdif_data->height;

				/* 1 pixel sent in 1 cycle. */
				if (lcdif_data->dsiFormat == kDSI_DbiRGB565)
				{
					DSI_SetDbiPixelPayloadSize(mipi_base, height * lcdif_data->width);
				}
				/* 2 pixel sent in 1 cycle. */
				else if (lcdif_data->dsiFormat == kDSI_DbiRGB332)
				{
					DSI_SetDbiPixelPayloadSize(mipi_base, (height * lcdif_data->width) >> 1U);
				}
				/* 2 pixels sent in 3 cycles. kDSI_DbiRGB888, kDSI_DbiRGB444 and kDSI_DbiRGB666 */
				else
				{
					DSI_SetDbiPixelPayloadSize(mipi_base, (height * lcdif_data->width * 3U) >> 1);
				}
				LCDIF_SetFrameBufferPosition(config->base, 0U, 0U, 0U, lcdif_data->width, height);
			}

			LCDIF_DbiSelectArea(config->base, 0, 0, lcdif_data->heightSent, lcdif_data->width - 1U,
								lcdif_data->heightSent + height - 1U, false);
			LCDIF_SetFrameBufferAddr(config->base, 0, (uint32_t)lcdif_data->data);

			/* Update data and height for next piece. */
			lcdif_data->data += (height * lcdif_data->stride);
			lcdif_data->heightSent += height;
			lcdif_data->height -= height;

			/* Write memory continue. */
			LCDIF_DbiSendCommand(config->base, 0, 0x3CU);

			/* Start memory transfer. */
			LCDIF_DbiWriteMem(config->base, 0);
		}
		else
		{
#endif
			k_sem_give(&lcdif_data->transfer_done);
#if CONFIG_MIPI_DSI
		}
#endif
	}
}

static int mcux_dcnano_lcdif_dbi_init(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;

#if !CONFIG_MIPI_DSI
	int ret;
	/* No DBI pin control if using mipi panel */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif

	LCDIF_Init(config->base);

	if (config->dbi_config.type == 0xFF)
	{
		LOG_ERR("Bus type not supported.");
		return -ENODEV;
	}

	LCDIF_DbiModeSetConfig(config->base, 0, &config->dbi_config);

	LCDIF_SetPanelConfig(config->base, 0, &config->panel_config);

	LCDIF_EnableInterrupts(config->base, kLCDIF_Display0FrameDoneInterrupt);

	config->irq_config_func(dev);

	k_sem_init(&lcdif_data->transfer_done, 0, 1);

	LOG_DBG("%s device init complete", dev->name);

	return 0;
}

static int mipi_dbi_dcnano_ldcif_write_display(const struct device *dev,
						   const struct mipi_dbi_config *dbi_config,
						   const uint8_t *framebuf,
						   struct display_buffer_descriptor *desc,
						   enum display_pixel_format pixfmt)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;
	int ret = 0;
	uint8_t bypes_per_pixel = 0U;

	/* The bus of dbi_config shall be set in init. */
	ARG_UNUSED(dbi_config);

#if CONFIG_MIPI_DSI
	MIPI_DSI_HOST_Type *mipi_base = config->dsi_base;
	/* Every time buffer 64 pixels first then begin the send. */
	DSI_SetDbiPixelFifoSendLevel(mipi_base, 64U);

	/* Get the LCDIF DBI output format to set to the MIPI DSI. */
	uint8_t format = config->dbi_config.format;
	switch(format)
	{
		case (uint8_t)kLCDIF_DbiOutD8RGB444:
			lcdif_data->dsiFormat = (uint8_t)kDSI_DbiRGB444;
			break;
		case (uint8_t)kLCDIF_DbiOutD16RGB332:
			lcdif_data->dsiFormat = (uint8_t)kDSI_DbiRGB332;
			break;
		case (uint8_t)kLCDIF_DbiOutD16RGB565:
			lcdif_data->dsiFormat = (uint8_t)kDSI_DbiRGB565;
			break;
		case (uint8_t)kLCDIF_DbiOutD16RGB666Option1:
			lcdif_data->dsiFormat = (uint8_t)kDSI_DbiRGB666;
			break;
		case (uint8_t)kLCDIF_DbiOutD16RGB888Option1:
			lcdif_data->dsiFormat = (uint8_t)kDSI_DbiRGB888;
			break;
		default:
			/* MIPI-DSI donot support other format of DBI pixel. */
			assert(false);
			break;
	}
	DSI_SetDbiPixelFormat(mipi_base, lcdif_data->dsiFormat);
#endif

	lcdif_fb_config_t fbConfig;

	LCDIF_FrameBufferGetDefaultConfig(&fbConfig);

	fbConfig.enable		  = true;
	fbConfig.inOrder		 = kLCDIF_PixelInputOrderARGB;
	fbConfig.rotateFlipMode  = kLCDIF_Rotate0;
	switch(pixfmt)
	{
		case PIXEL_FORMAT_RGB_888:
			fbConfig.format = kLCDIF_PixelFormatRGB888;
			bypes_per_pixel = 3U;
			break;
		case PIXEL_FORMAT_ARGB_8888:
			fbConfig.format = kLCDIF_PixelFormatARGB8888;
			bypes_per_pixel = 4U;
			break;
		case PIXEL_FORMAT_BGR_565:
			fbConfig.inOrder = kLCDIF_PixelInputOrderABGR;
		case PIXEL_FORMAT_RGB_565:
			break;
			fbConfig.format = kLCDIF_PixelFormatRGB565;
			bypes_per_pixel = 2U;
			break;
		default:
			LOG_ERR("Bus tyoe not supported.");
			ret = -ENODEV;
			break;
	}

	if (ret) {
		return ret;
	}

	fbConfig.alpha.enable	= false;
	fbConfig.colorkey.enable = false;
	fbConfig.topLeftX		= 0U;
	fbConfig.topLeftY		= 0U;
	fbConfig.width		   = desc->width;
	/* Only one layer is used, so for each memory write the select area's size shall be the same as the buffer. */
	fbConfig.height = desc->height;

	LCDIF_SetFrameBufferConfig(config->base, 0, &fbConfig);

	if (bypes_per_pixel == 3U)
	{
		/* For RGB888 the stride shall be calculated as 4 bytes per pixel instead of 3. */
		LCDIF_SetFrameBufferStride(config->base, 0, 4U * desc->pitch);
	}
	else
	{
		LCDIF_SetFrameBufferStride(config->base, 0, bypes_per_pixel * desc->pitch);
	}

	lcdif_data->heightEachWrite = desc->height;
#if CONFIG_MIPI_DSI
	uint8_t payloadBytePerPixel;

	/* Save stride in the lcdif_data if the payload has to be sent in multiple times. */
	lcdif_data->stride = bypes_per_pixel * desc->pitch;

	/* 1 pixel sent in 1 cycle. */
	if (lcdif_data->dsiFormat == kDSI_DbiRGB565)
	{
		payloadBytePerPixel = 2U;
	}
	/* 2 pixel sent in 1 cycle. */
	else if (lcdif_data->dsiFormat == kDSI_DbiRGB332)
	{
		payloadBytePerPixel = 1U;
	}
	/* 2 pixels sent in 3 cycles. kDSI_DbiRGB888, kDSI_DbiRGB444 and kDSI_DbiRGB666 */
	else
	{
		payloadBytePerPixel = 3U;
	}

	/* If the whole update payload exceeds the DSI max payload size, send the payload in multiple times. */
	if ((desc->width * desc->height * payloadBytePerPixel) > MIPI_DSI_MAX_PAYLOAD_SIZE)
	{
		/* Calculate how may lines to send each time. Make sure each time the buffer address meets the align
		 * requirement. */
		lcdif_data->heightEachWrite = MIPI_DSI_MAX_PAYLOAD_SIZE / desc->width / payloadBytePerPixel;
		while (((lcdif_data->heightEachWrite * bypes_per_pixel * desc->pitch) & (LCDIF_FB_ALIGN - 1U)) != 0U)
		{
			lcdif_data->heightEachWrite--;
		}

		/* Point the data to the next piece. */
		lcdif_data->data = (uint8_t *)((uint32_t)framebuf + (lcdif_data->heightEachWrite * bypes_per_pixel * desc->pitch));
	}

	/* Set payload size. */
	DSI_SetDbiPixelPayloadSize(mipi_base, (lcdif_data->heightEachWrite * desc->width * payloadBytePerPixel) >> 1U);

	/* Update height info. */
	lcdif_data->heightSent = lcdif_data->heightEachWrite;
	lcdif_data->width = desc->width;
	lcdif_data->height = desc->height - lcdif_data->heightEachWrite;
#endif

	LCDIF_SetFrameBufferPosition(config->base, 0U, 0U, 0U, desc->width, lcdif_data->heightEachWrite);

	LCDIF_DbiSelectArea(config->base, 0, 0, 0, desc->width - 1U, lcdif_data->heightEachWrite - 1U, false);

	LCDIF_SetFrameBufferAddr(config->base, 0, (uint32_t)framebuf);

	/* Send command: Memory write start. */
	LCDIF_DbiSendCommand(config->base, 0, 0x2CU);

	/* Enable DMA and send out data. */
	LCDIF_DbiWriteMem(config->base, 0);

	/* Wait for transfer done. */
	k_sem_take(&lcdif_data->transfer_done, K_FOREVER);

	return 0;
}

static int mipi_dbi_dcnano_lcdif_command_write(const struct device *dev,
						   const struct mipi_dbi_config *dbi_config,
						   uint8_t cmd, const uint8_t *data_buf,
						   size_t len)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;

	/* The bus of dbi_config shall be set in init. */
	ARG_UNUSED(dbi_config);

	LCDIF_DbiSendCommand(config->base, 0U, cmd);

	if (len != 0U)
	{
		LCDIF_DbiSendData(config->base, 0U, data_buf, len);
	}

	return kStatus_Success;
}

static int mipi_dbi_dcnano_lcdif_reset(const struct device *dev, k_timeout_t delay)
{
	int ret;
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;

	/* Check if a reset port is provided to reset the LCD controller */
	if (config->reset.port == NULL) {
		return 0;
	}

	/* Reset the LCD controller. */ //RT700 2-15
	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_HIGH);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(delay);

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%s device reset complete", dev->name);

	return 0;
}

static struct mipi_dbi_driver_api mcux_dcnano_lcdif_dbi_api = {
	.reset = mipi_dbi_dcnano_lcdif_reset,
	.command_write = mipi_dbi_dcnano_lcdif_command_write,
	.write_display = mipi_dbi_dcnano_ldcif_write_display,
};

#define MCUX_DCNANO_LCDIF_DEVICE_INIT(n)										\
	static void mcux_dcnano_lcdif_dbi_config_func_##n(const struct device *dev)	\
	{																			\
		IRQ_CONNECT(DT_INST_IRQN(n),											\
			   DT_INST_IRQ(n, priority),										\
			   mcux_dcnano_lcdif_dbi_isr,										\
			   DEVICE_DT_INST_GET(n),											\
			   0);																\
		irq_enable(DT_INST_IRQN(n));											\
	}																			\
	PINCTRL_DT_INST_DEFINE(n);													\
	struct mcux_dcnano_lcdif_dbi_data mcux_dcnano_lcdif_dbi_data_##n = {		\
		.data = NULL,															\
		.dsiFormat = kDSI_DbiRGB565,											\
		.pitch = 0U,															\
		.heightEachWrite = 0U,													\
		.heightSent = 0U,														\
		.height = 0U,															\
		.width = 0U,															\
		.stride = 0U,															\
	};																			\
	struct mcux_dcnano_lcdif_dbi_config mcux_dcnano_lcdif_dbi_config_##n = {	\
		.base = (LCDIF_Type *) DT_INST_REG_ADDR(n),								\
		.dsi_base = (MIPI_DSI_HOST_Type *)DT_INST_PROP_OR(n, mipi_dsi, 0),		\
		.irq_config_func = mcux_dcnano_lcdif_dbi_config_func_##n,				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),							\
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),					\
		.dbi_config = {															\
			.type =																\
			((DT_INST_PROP(n, mipi_mode) >= MIPI_DBI_MODE_8080_BUS_16_BIT) ? kLCDIF_DbiTypeB :					\
			((DT_INST_PROP(n, mipi_mode) >= MIPI_DBI_MODE_6800_BUS_16_BIT) ? kLCDIF_DbiTypeA_FixedE : 0xFF)),	\
			.swizzle = DT_INST_ENUM_IDX_OR(n, swizzle, 0),									\
			.format = LCDIF_DBICONFIG0_DBI_DATA_FORMAT(DT_INST_ENUM_IDX(n, color_coding)),	\
			.acTimeUnit = DT_INST_PROP_OR(n, divider, 1) - 1,		\
			.writeWRPeriod = DT_INST_PROP(n, wr_period),			\
			.writeWRAssert = DT_INST_PROP(n, wr_assert),			\
			.writeWRDeassert = DT_INST_PROP(n, wr_deassert),		\
			.writeCSAssert = DT_INST_PROP(n, cs_assert),			\
			.writeCSDeassert = DT_INST_PROP(n, cs_deassert),		\
		},															\
		.panel_config = {											\
			.enable = true,											\
			.enableGamma = false,									\
			.order = kLCDIF_VideoOverlay0Overlay1,					\
			.endian = DT_INST_ENUM_IDX_OR(n, endian, 0),			\
		},															\
	};																\
	DEVICE_DT_INST_DEFINE(n,				\
		&mcux_dcnano_lcdif_dbi_init,		\
		NULL,								\
		&mcux_dcnano_lcdif_dbi_data_##n,	\
		&mcux_dcnano_lcdif_dbi_config_##n,	\
		POST_KERNEL,						\
		CONFIG_MIPI_DBI_INIT_PRIORITY,		\
		&mcux_dcnano_lcdif_dbi_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DCNANO_LCDIF_DEVICE_INIT)
