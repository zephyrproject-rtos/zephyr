/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//@IMPORTANT @INCOMPLETE(Emilio): None of the function are checking for errors at the moment!

#define DT_DRV_COMPAT chipone_co5300

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(co5300, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

/******TEMP CODE********/
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int32_t b32;
/******END OF TEMP CODE********/

//#include "co5300_regs.h"

/******OLD CODE********/
static struct display_cmds {
	uint8_t *cmd_code;
	uint8_t size;
};

#if 0
struct co5300_config {
	LCDIF_Type *lcdif_base;
	MIPI_DSI_HOST_Type *mipi_base;
	enum display_pixel_format initial_pixel_format;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec power_gpio;
	const struct gpio_dt_spec mipi_te_gpio;
};

struct co5300_data {
	int Placeholder;
};
#endif

/******END OF OLD CODE********/


static struct co5300_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec backlight_gpio;
	const struct gpio_dt_spec tear_effect_gpio;
	const struct gpio_dt_spec power_gpio;
	uint16_t panel_width;
	uint16_t panel_height;
	uint16_t channel;
	uint16_t num_of_lanes;
};

static struct co5300_data {
	enum display_pixel_format pixel_format;
	uint8_t bytes_per_pixel;
	struct gpio_callback te_gpio_cb;
	struct k_sem te_sem;
};

const struct display_cmds lcm_init_settings[] = {
	{{0xFE, 0x20}, 2}, {{0xF4, 0x5A}, 2},
	{{0xF5, 0x59}, 2}, {{0xFE, 0x40}, 2},
	{{0x96, 0x00}, 2}, {{0xC9, 0x00}, 2},
	{{0xFE, 0x00}, 2}, {{0x35, 0x00}, 2},
	{{0x53, 0x20}, 2}, {{0x51, 0xFF}, 2},
	{{0x63, 0xFF}, 2}, {{0x2A, 0x00, 0x06, 0x01, 0xD7}, 5},
	{{0x2B, 0x00, 0x00, 0x01, 0xD1}, 5}
};

//@IMPORTANT(Emilio): We may need to be more particular on how to choose the txDataType, there seems
//			to be many definitions.
static int co5300_write_cmd_codes(const struct device *dev,
					const struct display_cmds *cmds)
{
#if 0
	const struct co5300_config *config = dev->config;
	MIPI_DSI_HOST_Type *mipi_base = config->mipi_dsi;
	dsi_transfer_t transfer_config = {};

	for (int iter = 0; iter < ARRAY_SIZE(cmds); iter++)
	{
		switch(cmds[iter]->size)
		{
			case 0:
			{
				//@TODO(Emilio): Error case.

			} break;

			case 1:
			{
				transfer_config.txDataType = kDSI_TxDataDcsShortWrNoParam;
			} break;

			case 2:
			{
				transfer_config.txDataType = kDSI_TxDataDcsShortWrOneParam;

			} break;

			default
			{
				transfer_config.txDataType = kDSI_TxDataDcsLongWr;
			}
		}

		transfer_config.txData = cmds[iter]->cmd_code;
		transfer_config.txSize = cmds[iter]->size;
		DSI_TransferBlocking(mipi_base, &transfer_config);
	}
#endif
}

static int co5300_blanking_on(const struct device *dev)
{

}

static int co5300_blanking_off(const struct device *dev)
{

}

static int co5300_write(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{

	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	struct device *mipi_base = config->mipi_dsi;
	enum display_pixel_format pixel_format = data->pixel_format;

	struct mipi_dsi_device mdev = {0};
	int ret;
	uint32_t i;
	uint8_t cmd, param;

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		//LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}


#if 0
	uint8_t cmd                   = (uint8_t)command;
	dbi_lcdif_prv_data_t *prvData = (dbi_lcdif_prv_data_t *)dbiIface->prvData;
	uint16_t height               = prvData->height;
	uint32_t stride;

	if (isInterleave)
	{
	    stride = stride_byte;
	}
	else
	{
	    stride = (uint32_t)prvData->width * (uint32_t)prvData->bytePerPixel;
	}

	/* For RGB888 the stride shall be calculated as 4 bytes per pixel. */
	if (prvData->bytePerPixel == 3U)
	{
	    LCDIF_SetFrameBufferStride(lcdif, 0, stride / 3U * 4U);
	}
	else
	{
	    LCDIF_SetFrameBufferStride(lcdif, 0, stride);
	}
#if DBI_USE_MIPI_PANEL
	MIPI_DSI_HOST_Type *dsi = prvData->dsi;
	uint8_t payloadBytePerPixel;

	if (dsi != NULL)
	{
	    prvData->stride = (uint16_t)stride;

	    /* 1 pixel sent in 1 cycle. */
	    if (prvData->dsiFormat == kDSI_DbiRGB565)
	    {
	        payloadBytePerPixel = 2U;
	    }
	    /* 2 pixel sent in 1 cycle. */
	    else if (prvData->dsiFormat == kDSI_DbiRGB332)
	    {
	        payloadBytePerPixel = 1U;
	    }
	    /* 2 pixels sent in 3 cycles. kDSI_DbiRGB888, kDSI_DbiRGB444 and kDSI_DbiRGB666 */
	    else
	    {
	        payloadBytePerPixel = 3U;
	    }

	    /* If the whole update payload exceeds the DSI max payload size, send the payload in multiple times. */
	    if ((prvData->width * prvData->height * payloadBytePerPixel) > MIPI_DSI_MAX_PAYLOAD_SIZE)
	    {
	        /* Calculate how may lines to send each time. Make sure each time the buffer address meets the align
	         * requirement. */
	        height = MIPI_DSI_MAX_PAYLOAD_SIZE / prvData->width / (uint16_t)payloadBytePerPixel;
	        while (((height * prvData->stride) & (LCDIF_FB_ALIGN - 1U)) != 0U)
	        {
	            height--;
	        }
	        prvData->heightEachWrite = height;

	        /* Point the data to the next piece. */
	        prvData->data = (uint8_t *)((uint32_t)data + (height * prvData->stride));
	    }

	    /* Set payload size. */
	    DSI_SetDbiPixelPayloadSize(dsi, (height * prvData->width * (uint16_t)payloadBytePerPixel) >> 1U);
	}

	/* Update height info. */
	prvData->heightSent = height;
#endif
	prvData->height -= height;

	/* Configure buffer position, address and area. */
	LCDIF_SetFrameBufferPosition(lcdif, 0U, 0U, 0U, prvData->width, height);
	LCDIF_DbiSelectArea(lcdif, 0, 0, 0, prvData->width - 1U, height - 1U, false);
	LCDIF_SetFrameBufferAddr(lcdif, 0, (uint32_t)data);

	/* Send command. */
	LCDIF_DbiSendCommand(lcdif, 0, cmd);

	/* Start memory transfer. */
	LCDIF_DbiWriteMem(lcdif, 0);
#endif

}

static int co5300_read(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc, void *buf)
{

}

static int co5300_clear(const struct device *dev)
{

}

static void *co5300_get_framebuffer(const struct device *dev)
{

}

static int co5300_set_brightness(const struct device *dev, uint8_t)
{

}

static int co5300_set_contrast(const struct device *dev, uint8_t contrast)
{

}

static void co5300_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	capabilities->x_resolution = 256;
	capabilities->y_resolution = 466;
	//@TODO(Emilio): capabilities->pixel_formats = PIXEL_FORMAT_*** | *** | ***;
//	capabilities->pixel_formats = PIXEL_FORMAT_RGB_565;
	//@TODO(Emilio): capabilities->screen_info = SCREEN_INFO_MONO*** | *** | ***;
	//@TODO(Emilio): Find the screen info here.
//	capabilities->current_pixel_format = PXIEL_FORMAT_RGB_565;
//	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int co5300_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	//@NOTE(Emilio): The following is how you set the pixel format for the MIPI_DSI
	//	base->CFG_DBI_PIXEL_FIFO_SEND_LEVEL = (u32)sendLevel; //@NOTE(Emilio): Found in
	//	drivers/mipi_dsi/fsl_mipi_dsi.h
	//	also
	//	base->CFG_DBI_PIXEL_FORMAT
	//	LCDIF_SetFrameBufferConfig(***);

}

static int co5300_set_orientation(const struct device *dev,
		const enum display_orientation orientation)
{

}

static int co5300_init(const struct device *dev)
{
/******OLD CODE********/
#if 0
	const struct co5300_config *config = dev->config;
	LCDIF_Type *lcdif_base = config->lcdif_base;
	MIPI_DSI_HOST_Type *mipi_base = config->mipi_base;
	enum display_pixel_format pixel_format = config->initial_pixel_format;

	//@HARDCODE @INCOMPLETE(Emilio): All the values here are hardcoded from display_support.c
	//@NOTE(Emilio): Enable MIPI
	dsi_config_t dsiConfig;
	DSI_GetDefaultConfig(&dsiConfig);
	dsiConfig.numLanes       = 1;
	dsiConfig.autoInsertEoTp = true;
	DSI_Init(mipi_base, &dsiConfig);

	uint32_t mipiDsiDphyBitClkFreq_Hz = CLOCK_GetMipiDphyClkFreq();
	uint32_t mipiDsiTxEscClkFreq_Hz = CLOCK_GetMipiDphyEscTxClkFreq();

	dsi_dphy_config_t dphyConfig;
	DSI_GetDphyDefaultConfig(&dphyConfig, mipiDsiDphyBitClkFreq_Hz, mipiDsiTxEscClkFreq_Hz);
	DSI_InitDphy(mipi_base, &dphyConfig, 0);

	//@HARDCODE(Emilio): Not sure what 64 does here.
	DSI_SetDbiPixelFifoSendLevel(mipi_base, 64U);
	DSI_SetDbiPixelFormat(mipi_base, kDSI_DbiRGB565);



	//@NOTE(Emilio): Enable LCDIF

	/* DBI configurations. */
	lcdif_dbi_config_t dbi_config;
	LCDIF_DbiModeGetDefaultConfig(&dbi_config);
	dbi_config.acTimeUnit      = 0;
	dbi_config.writeWRPeriod   = 14U;

	/* With 279.53MHz source and 16bpp format, a 14 cycle period requires a 279.53MHz / 14 * 16 = 319.46Mhz DPHY clk
	 * source. */
	if (pixel_format == PIXEL_FORMAT_RGB_565)
	{
	    dbi_config.format = kLCDIF_DbiOutD16RGB565;
	}
	else
	{
	    dbi_config.format = kLCDIF_DbiOutD16RGB888Option1;
	}
	dbi_config.type            = kLCDIF_DbiTypeB;
	dbi_config.writeCSAssert   = 1;
	dbi_config.writeCSDeassert = 4;
	dbi_config.writeWRAssert   = (dbi_config.writeWRPeriod - 1U) / 2U; /* Asset at the middle. */
	dbi_config.writeWRDeassert = (dbi_config.writeWRPeriod - 1U);      /* Deassert at the end */

	LCDIF_Init(lcdif_base);
	LCDIF_DbiModeSetConfig(lcdif_base, 0, &dbi_config);

	lcdif_panel_config_t lcdif_config;
	LCDIF_PanelGetDefaultConfig(&lcdif_config);
	if (pixel_format == PIXEL_FORMAT_RGB_565)
	{
	    lcdif_config.endian = kLCDIF_HalfWordSwap;
	}

	LCDIF_SetPanelConfig(lcdif_base, 0, &lcdif_config);

	//@TODO(Emilio): Add irq_config func here.


	//@NOTE(Emilio): Init LCDIF Controller
	LCDIF_EnableInterrupts(lcdif_base, kLCDIF_Display0FrameDoneInterrupt | kLCDIF_PanelUnderflowInterrupt);


	//@NOTE(Emilio): Init LCDIF Panel

	//@IMPORTANT @INCOMPLETE(Emilio): Check that we configure these gpio correctly, polarity
	gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_LOW);
	//@SPEED @HARDCODE(Emilio): Hardcode GPIO to HIGH to bypass sdk steps.
	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_HIGH);
	gpio_pin_configure_dt(&config->mipi_te_gpio, GPIO_OUTPUT_LOW);

#if 0
	//@IMPORTANT(Emilio): Check to see if these interrupts get set.
	GPIO_SetPinInterruptConfig(BOARD_MIPI_TE_GPIO, BOARD_MIPI_TE_PIN, kGPIO_InterruptRisingEdge);
	GPIO_SetPinInterruptChannel(BOARD_MIPI_TE_GPIO, BOARD_MIPI_TE_PIN, kGPIO_InterruptOutput0);
#endif

	//@IMPORTANT @INCOMPLETE(Emilio): Ensure These GPIOs NVICs are enabled correctly.



	//@NOTE(Emilio): Init LCDIF Panel
#if 0
	//@INCOMPLETE @NOTE(Emilio): This Should be done at compile time in DEVICE_INIT
	handle->height = FSL_VIDEO_EXTRACT_HEIGHT(config->resolution);
	handle->width  = FSL_VIDEO_EXTRACT_WIDTH(config->resolution);
	handle->pixelFormat = config->pixelFormat;

	@TODO(Emilio): Why does SDK have this sequence of gpio toggles??
	@NOTE(Emilio): BTW -> gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_HIGH);
	/* Power on. */
	resource->pullPowerPin(true);
	co5300_DelayMs(100);

	/* Perform reset. */
	resource->pullResetPin(false);
	co5300_DelayMs(1);
	resource->pullResetPin(true);
	co5300_DelayMs(150);
#endif

	//@NOTE(Emilio): Set LCM Init Settings

	//@HARDCODE(Emilio): Comes straight from components/video/display/c05300/fsl_co5300.c
	co5300_write_cmd_codes(dev, lcm_init_settings);


	display_cmds pixel_setting_cmd = {{0x36, 0x0}, 2};
	if (config->pixelFormat != kVIDEO_PixelFormatRGB565)
	{
		pixel_setting_cmd.cmd_code[1] = 0x8;
	}
	co5300_write_cmd_codes(dev, pixel_setting_cmd);



	/* Set pixel format. */
	//@HARDCODE @NOTE(Emilio): Eventually we will make defines in a header file
	//	the Most sig bits are for the dpi pixel_format the Least sig bits
	//	are for the dbi pixel format, just the abi for the command code.
	//	The two valid pixel formats are 0x77 or 0x75 hints the |= 2.
	char pixel_format_cmd = (7 << 4) | 5;
	if ((kVIDEO_PixelFormatXRGB8888 == config->pixelFormat) || (kVIDEO_PixelFormatRGB888 == config->pixelFormat))
	{
		dbi_pixel_format |= 2;
	}
	display_cmds pixel_foramt_cmd = {{0x3A, pixel_format_cmd}, 2};
	co5300_write_cmd_codes(dev, pixel_setting_cmd);


	//@TODO(Emilio): Again, unsure why we attempt a sleep in SDK.
//	co5300_DelayMs(50);


	//@TODO(Emilio): Part of the definitions.
	// Entering sleep CMD is 0x10
	// Exiting  sleep CMD is 0x11
	display_cmds exit_sleep_cmd = {{0x11}, 1};
	co5300_write_cmd_codes(dev, exit_sleep_cmd);

	//@TODO(Emilio): Again, unsure why we attempt a sleep in SDK.
//	co5300_DelayMs(150);


	//@NOTE(Emilio): Turn on Display.
	display_cmds exit_sleep_cmd = {{0x29}, 1};
	co5300_write_cmd_codes(dev, exit_sleep_cmd);


	//@NOTE(Emilio): SDK Setups a callback at this point for FrameDoneCallback
	//@NOTE(Emilio): The SDK also has a helper function to get size in bits based on
	//		Pixel Format, might be a good idea.


	//@NOTE(Emilio): More set Pixel Format but for LCDIF
#if 0
	lcdif_fb_config_t fbConfig;
	dbi_lcdif_prv_data_t *prvData = (dbi_lcdif_prv_data_t *)dbiIface->prvData;
	lcdif_layer_input_order_t componentOrder;
	lcdif_fb_format_t pixelFormat;

	status = DBI_LCDIF_GetPixelFormat(format, &pixelFormat, &componentOrder);
	//@NOTE(Emilio): The nested for loop is GetPixelFormat
	for (int i = 0; i < ARRAY_SIZE(s_lcdifPixelFormatMap); i++)
	{
	    if (s_lcdifPixelFormatMap[i].videoFormat == input)
	    {
	        *output = s_lcdifPixelFormatMap[i].lcdifFormat;
	        *order  = s_lcdifPixelFormatMap[i].componentOrder;
	        return kStatus_Success;
	    }
	}


	LCDIF_FrameBufferGetDefaultConfig(&fbConfig);
	fbConfig.format  = pixelFormat;
	fbConfig.inOrder = componentOrder;
	LCDIF_SetFrameBufferConfig(lcdif, 0, &fbConfig);

	/* Set byte per pixel for stride calculation later. */
	//@NOTE(Emilio): VIDEO_** is the helper function mentioned above.
	prvData->bytePerPixel = VIDEO_GetPixelSizeBits(format) / 8U;
#endif


	//@NOTE(Emilio): SDK Then selects Area?
#if 0
	uint8_t data[4];
	status_t status;

	/*Column addresses*/
	data[0] = (startX >> 8) & 0xFF;
	data[1] = startX & 0xFF;
	data[2] = (endX >> 8) & 0xFF;
	data[3] = endX & 0xFF;

	status = DBI_IFACE_WriteCmdData(dbiIface, kMIPI_DBI_SetColumnAddress, data, 4);

	if (status != kStatus_Success)
	{
	    return status;
	}

	/*Page addresses*/
	data[0] = (startY >> 8) & 0xFF;
	data[1] = startY & 0xFF;
	data[2] = (endY >> 8) & 0xFF;
	data[3] = endY & 0xFF;

	status = DBI_IFACE_WriteCmdData(dbiIface, kMIPI_DBI_SetPageAddress, data, 4);

	//@NOTE(Emilio): DBI_IFACE_WriteCmdData
	if ((len_byte != 0U) && (data == NULL))
	{
	    return kStatus_Fail;
	}

	uint8_t cmd                   = (uint8_t)command;
	uint8_t *pData                = (uint8_t *)data;
	dbi_lcdif_prv_data_t *prvData = (dbi_lcdif_prv_data_t *)dbiIface->prvData;
	LCDIF_Type *lcdif             = prvData->lcdif;

	/* If the command is set address, calculate the selected area size, since LCDIF needs these info for configuration.
	 */
	if ((cmd == (uint8_t)kMIPI_DBI_SetColumnAddress) || (cmd == (uint8_t)kMIPI_DBI_SetPageAddress))
	{
	    uint16_t lrValue = ((uint16_t)pData[2] << 8U) | (uint16_t)pData[3];
	    uint16_t tpValue = ((uint16_t)pData[0] << 8U) | (uint16_t)pData[1];

	    if (lrValue < tpValue)
	    {
	        return kStatus_Fail;
	    }

	    if (cmd == kMIPI_DBI_SetColumnAddress)
	    {
	        prvData->width = lrValue - tpValue + 1U;
	    }
	    else
	    {
	        prvData->height = lrValue - tpValue + 1U;
	    }
	}

	LCDIF_DbiSendCommand(lcdif, 0U, cmd);
	if (len_byte != 0U)
	{
	    LCDIF_DbiSendData(lcdif, 0U, pData, len_byte);
	}

#endif

	//@NOTE(Emilio): At this point the SDK sets up a callback for BufferSwitchOff
	//		Creates a semaphore
	//		and clears an area of memory to use as the framebuffer.



	//@NOTE(Emilio): Turning on the LCDIF
#if 0
	uint8_t cmd = (on ? kMIPI_DBI_SetDisplayOn : kMIPI_DBI_SetDisplayOff);
	dbiIface->xferOps->writeCommandData(dbiIface, cmd, NULL, 0);

	//@NOTE(Emilio): The following code is writeCommmandData
	if ((len_byte != 0U) && (data == NULL))
	{
	    return kStatus_Fail;
	}

	uint8_t cmd                   = (uint8_t)command;
	uint8_t *pData                = (uint8_t *)data;
	dbi_lcdif_prv_data_t *prvData = (dbi_lcdif_prv_data_t *)dbiIface->prvData;
	LCDIF_Type *lcdif             = prvData->lcdif;

	/* If the command is set address, calculate the selected area size, since LCDIF needs these info for configuration.
	 */
	if ((cmd == (uint8_t)kMIPI_DBI_SetColumnAddress) || (cmd == (uint8_t)kMIPI_DBI_SetPageAddress))
	{
	    uint16_t lrValue = ((uint16_t)pData[2] << 8U) | (uint16_t)pData[3];
	    uint16_t tpValue = ((uint16_t)pData[0] << 8U) | (uint16_t)pData[1];

	    if (lrValue < tpValue)
	    {
	        return kStatus_Fail;
	    }

	    if (cmd == kMIPI_DBI_SetColumnAddress)
	    {
	        prvData->width = lrValue - tpValue + 1U;
	    }
	    else
	    {
	        prvData->height = lrValue - tpValue + 1U;
	    }
	}

	LCDIF_DbiSendCommand(lcdif, 0U, cmd);

	if (len_byte != 0U)
	{
	    LCDIF_DbiSendData(lcdif, 0U, pData, len_byte);
	}
#endif




	//@NOTE(Emilio): The SDK Now starts to init VGLite, probably not needed on our side.
	//@NOTE(Emilio): The SDK Then enables the touchscreen via I2C.

	//@NOTE(Emilio): Finally done!
	return 0;
#endif
/******END OF OLD CODE********/
	struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	u32 iter;
	u32 Result = 1;
	enum display_pixel_format zephyr_pixel_format;
	const struct mipi_device *mipi_device = config->mipi_dsi;

#if 0
	//@TODO(Emilio): Coming from display_rm67162 unsure if
	//	api requires this dual attachment paradigm.
	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}
#endif
	if (config->power_gpio.port != NULL) {
		int ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
		//	LOG_ERR("Could not configure power GPIO (%d)", ret);
			return ret;
		}

		//@NOTE(Emilio): Power On Sequence
		ret = gpio_pin_set_dt(&config->power_gpio, 1);
		if (ret < 0) {
		//	LOG_ERR("Could not pull power high (%d)", ret);
			return ret;
		}

		k_sleep(K_MSEC(100));

		if (config->reset_gpio.port != NULL) {
			ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
			if (ret < 0) {
		//		LOG_ERR("Could not configure reset GPIO (%d)", ret);
				return ret;
			}

			ret = gpio_pin_set_dt(&config->reset_gpio, 0);
			if (ret < 0) {
		//		LOG_ERR("Could not pull reset low (%d)", ret);
				return ret;
			}

			k_sleep(K_MSEC(1));
			gpio_pin_set_dt(&config->reset_gpio, 1);
			if (ret < 0) {
		//		LOG_ERR("Could not pull reset high (%d)", ret);
				return ret;
			}
			k_sleep(K_MSEC(150));
		}
	}

	int cmd = 0;
	int param = 0;
	/* Set the LCM init settings. */
	for (int i = 0; i < ARRAY_SIZE(lcm_init_settings); i++) {
		//@TODO(Emilio): rename and make correct cmds
		cmd = lcm_init_settings[i].cmd_code;
		param = lcm_init_settings[i].size;
		int ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
					cmd, &param, 1);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set pixel format */
	if (data->pixel_format == MIPI_DSI_PIXFMT_RGB888) {
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
	} else if (data->pixel_format == MIPI_DSI_PIXFMT_RGB565) {
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
	} else {
		/* Unsupported pixel format */
//		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}
	int ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	/*
	 * We must wait 5 ms after exiting sleep mode before sending additional
	 * commands. If we intend to enter sleep mode, we must delay
	 * 120 ms before sending that command. To be safe, delay 150ms
	 */
	k_sleep(K_MSEC(150));
}

static DEVICE_API(display, co5300_api) = {
	.blanking_on = co5300_blanking_on,
	.blanking_off = co5300_blanking_off,
	.write = co5300_write,
	.read = co5300_read,
	.clear = co5300_clear,
	.get_framebuffer = co5300_get_framebuffer,
	.set_brightness = co5300_set_brightness,
	.set_contrast = co5300_set_contrast,
	.get_capabilities = co5300_get_capabilities,
	.set_pixel_format = co5300_set_pixel_format,
	.set_orientation = co5300_set_orientation,
};

/******OLD CODE********/
#if 0
#define CO5300_DEVICE_INIT(node_id)									\
	static struct co5300_data co5300_data##node_id;							\
	static const struct co5300_config co5300_config##node_id = {					\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, bl_gpios, {0}),				\
		.te_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, te_gpios, {0}),				\
	};												\
													\
	DEVICE_DT_INST_DEFINE(node_id, co5300_init, NULL,						\
			&co5300_data##node_id, &co5300_config##node_id,					\
			POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &co5300_driver_api);
#endif
/******END OF OLD CODE********/


#define CO5300_DEVICE_INIT(node_id)								\
	static const struct co5300_config co5300_config_##node_id = {				\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),					\
		.num_of_lanes = DT_INST_PROP_BY_IDX(node_id, data_lanes, 0),				\
		.channel = DT_INST_REG_ADDR(node_id),						\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, backlight_gpios, {0}),				\
		.tear_effect_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, tear_effect_gpios, {0}),				\
		.panel_width = DT_INST_PROP(node_id, width),						\
		.panel_height = DT_INST_PROP(node_id, height),					\
	};											\
	static struct co5300_data co5300_data_##node_id = {						\
	};											\
	DEVICE_DT_INST_DEFINE(node_id,								\
			    &co5300_init,							\
			    0,								\
			    &co5300_data_##node_id,							\
			    &co5300_config_##node_id,						\
			    POST_KERNEL,							\
			    CONFIG_APPLICATION_INIT_PRIORITY,					\
			    &co5300_api);


DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE_INIT)
