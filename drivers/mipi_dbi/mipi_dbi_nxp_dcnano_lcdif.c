/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_dbi_dcnano_lcdif

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_lcdif.h>

LOG_MODULE_REGISTER(mipi_dbi_nxp_dcnano_lcdif, CONFIG_DISPLAY_LOG_LEVEL);

struct mcux_dcnano_lcdif_dbi_data {
	struct k_sem transfer_done;
	const struct mipi_dbi_config *active_cfg;
};

struct mcux_dcnano_lcdif_dbi_config {
	LCDIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	lcdif_dbi_config_t dbi_config;
	lcdif_panel_config_t panel_config;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec reset;
};

struct mcux_dcnano_lcdif_dbi_foramt_map_t {
	uint8_t bus_type;
	uint8_t color_coding;
	lcdif_dbi_out_format_t format;
};

const struct mcux_dcnano_lcdif_dbi_foramt_map_t format_map[] = {
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB332, kLCDIF_DbiOutD8RGB332},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB444, kLCDIF_DbiOutD8RGB444},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB565, kLCDIF_DbiOutD8RGB565},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB666_1, kLCDIF_DbiOutD8RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB888_1, kLCDIF_DbiOutD8RGB888},
	{MIPI_DBI_MODE_6800_BUS_9_BIT, MIPI_DBI_MODE_RGB666_1, kLCDIF_DbiOutD9RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB666_2, kLCDIF_DbiOutD8RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB888_2, kLCDIF_DbiOutD8RGB888},
	{MIPI_DBI_MODE_6800_BUS_9_BIT, MIPI_DBI_MODE_RGB666_2, kLCDIF_DbiOutD9RGB666},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB332, kLCDIF_DbiOutD16RGB332},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB444, kLCDIF_DbiOutD16RGB444},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB565, kLCDIF_DbiOutD16RGB565},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB666_1,
		kLCDIF_DbiOutD16RGB666Option1},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB666_2,
		kLCDIF_DbiOutD16RGB666Option2},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB888_1,
		kLCDIF_DbiOutD16RGB888Option1},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB888_2,
		kLCDIF_DbiOutD16RGB888Option2},
};

static int mcux_dcnano_lcdif_dbi_get_format(uint8_t bus_type, uint8_t color_coding,
	lcdif_dbi_out_format_t *format)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(format_map); i++) {
		if ((format_map[i].bus_type == (bus_type - MIPI_DBI_MODE_6800_BUS_16_BIT)) &&
			(format_map[i].color_coding == color_coding)) {
			*format = format_map[i].format;
			return 0;
		}
	}

	return -EINVAL;
}

static void mcux_dcnano_lcdif_dbi_isr(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;
	uint32_t status;

	status = LCDIF_GetAndClearInterruptPendingFlags(config->base);

	if (0 != (status & kLCDIF_Display0FrameDoneInterrupt)) {
		k_sem_give(&lcdif_data->transfer_done);
	}
}

static int mcux_dcnano_lcdif_dbi_configure(const struct device *dev,
					   const struct mipi_dbi_config *dbi_config)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;
	uint8_t bus_type = dbi_config->mode & 0xFU;
	uint8_t color_coding = dbi_config->mode & 0xF0U;
	lcdif_dbi_config_t lcdif_dbi_config = config->dbi_config;
	status_t status;

	/* No need to update if configuration is the same. */
	if (dbi_config == lcdif_data->active_cfg) {
		return 0;
	}

	/* SPI mode is not supported by the SDK LCDIF driver */
	if ((bus_type == MIPI_DBI_MODE_SPI_3WIRE) ||
		(bus_type == MIPI_DBI_MODE_SPI_4WIRE)) {
		LOG_ERR("Bus type not supported.");
		return -EINVAL;
	}

	/* 9-bit bus only has RGB666 color coding. */
	if (((bus_type == MIPI_DBI_MODE_6800_BUS_9_BIT) ||
		(bus_type == MIPI_DBI_MODE_8080_BUS_9_BIT)) &&
		((color_coding != MIPI_DBI_MODE_RGB666_1) &&
		(color_coding != MIPI_DBI_MODE_RGB666_2))) {
		return -EINVAL;
	}

	/* Get the bus type */
	switch (bus_type) {
	case MIPI_DBI_MODE_6800_BUS_16_BIT:
	case MIPI_DBI_MODE_6800_BUS_9_BIT:
	case MIPI_DBI_MODE_6800_BUS_8_BIT:
		lcdif_dbi_config.type = kLCDIF_DbiTypeA_FixedE;
		break;
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		lcdif_dbi_config.type = kLCDIF_DbiTypeB;
		break;
	default:
		return -EINVAL;
	}

	/* Get the color cosing */
	status = mcux_dcnano_lcdif_dbi_get_format(bus_type, color_coding,
		&lcdif_dbi_config.format);

	if (kStatus_Success != status) {
		return -EINVAL;
	}

	/* Update DBI configuration. */
	status = LCDIF_DbiModeSetConfig(config->base, 0, &lcdif_dbi_config);

	if (kStatus_Success != status) {
		return -EINVAL;
	}

	lcdif_data->active_cfg = dbi_config;

	return 0;
}

static int mcux_dcnano_lcdif_dbi_init(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;

#ifndef CONFIG_MIPI_DSI_MCUX_NXP_DCNANO_LCDIF
	int ret;

	/* Pin control is not applied when DCNano is used in MCUX DSI driver. */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif

	LCDIF_Init(config->base);

	LCDIF_DbiModeSetConfig(config->base, 0, &config->dbi_config);

	LCDIF_SetPanelConfig(config->base, 0, &config->panel_config);

	LCDIF_EnableInterrupts(config->base, kLCDIF_Display0FrameDoneInterrupt);

	config->irq_config_func(dev);

	k_sem_init(&lcdif_data->transfer_done, 0, 1);

	LOG_DBG("%s device init complete", dev->name);

	return 0;
}

static int mipi_dbi_dcnano_lcdif_write_display(const struct device *dev,
						   const struct mipi_dbi_config *dbi_config,
						   const uint8_t *framebuf,
						   struct display_buffer_descriptor *desc,
						   enum display_pixel_format pixfmt)
{
	const struct mcux_dcnano_lcdif_dbi_config *config = dev->config;
	struct mcux_dcnano_lcdif_dbi_data *lcdif_data = dev->data;
	int ret = 0;
	uint8_t bytes_per_pixel = 0U;

	/* The DBI bus type and color coding. */
	ret = mcux_dcnano_lcdif_dbi_configure(dev, dbi_config);

	if (ret) {
		return ret;
	}

	lcdif_fb_config_t fbConfig;

	LCDIF_FrameBufferGetDefaultConfig(&fbConfig);

	fbConfig.enable		= true;
	fbConfig.inOrder	= kLCDIF_PixelInputOrderARGB;
	fbConfig.rotateFlipMode	= kLCDIF_Rotate0;
	switch (pixfmt) {
	case PIXEL_FORMAT_RGB_888:
		fbConfig.format = kLCDIF_PixelFormatRGB888;
		bytes_per_pixel = 3U;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		fbConfig.format = kLCDIF_PixelFormatARGB8888;
		bytes_per_pixel = 4U;
		break;
	case PIXEL_FORMAT_BGR_565:
		fbConfig.inOrder = kLCDIF_PixelInputOrderABGR;
	case PIXEL_FORMAT_RGB_565:
		fbConfig.format = kLCDIF_PixelFormatRGB565;
		bytes_per_pixel = 2U;
		break;
	default:
		LOG_ERR("Bus tyoe not supported.");
		ret = -ENODEV;
		break;
	}

	if (ret) {
		return ret;
	}

	fbConfig.alpha.enable	 = false;
	fbConfig.colorkey.enable = false;
	fbConfig.topLeftX	 = 0U;
	fbConfig.topLeftY	 = 0U;
	fbConfig.width		 = desc->width;
	fbConfig.height = desc->height;

	LCDIF_SetFrameBufferConfig(config->base, 0, &fbConfig);

	if (bytes_per_pixel == 3U) {
		/* For RGB888 the stride shall be calculated as
		 * 4 bytes per pixel instead of 3.
		 */
		LCDIF_SetFrameBufferStride(config->base, 0, 4U * desc->pitch);
	} else {
		LCDIF_SetFrameBufferStride(config->base, 0, bytes_per_pixel * desc->pitch);
	}

	/* Set the updated area's size according to desc. */
	LCDIF_SetFrameBufferPosition(config->base, 0U, 0U, 0U, desc->width,
								desc->height);

	LCDIF_DbiSelectArea(config->base, 0, 0, 0, desc->width - 1U,
						desc->height - 1U, false);

	LCDIF_SetFrameBufferAddr(config->base, 0, (uint32_t)framebuf);

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
	int ret = 0;

	/* The DBI bus type and color coding. */
	ret = mcux_dcnano_lcdif_dbi_configure(dev, dbi_config);

	if (ret) {
		return ret;
	}

	LCDIF_DbiSendCommand(config->base, 0U, cmd);

	if (len != 0U) {
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

	/* Reset the LCD controller. */
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
	.write_display = mipi_dbi_dcnano_lcdif_write_display,
};

#define MCUX_DCNANO_LCDIF_DEVICE_INIT(n)						\
	static void mcux_dcnano_lcdif_dbi_config_func_##n(const struct device *dev)	\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
				DT_INST_IRQ(n, priority),				\
				mcux_dcnano_lcdif_dbi_isr,				\
				DEVICE_DT_INST_GET(n),					\
				0);							\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
	PINCTRL_DT_INST_DEFINE(n);							\
	struct mcux_dcnano_lcdif_dbi_data mcux_dcnano_lcdif_dbi_data_##n;		\
	struct mcux_dcnano_lcdif_dbi_config mcux_dcnano_lcdif_dbi_config_##n = {	\
		.base = (LCDIF_Type *) DT_INST_REG_ADDR(n),				\
		.irq_config_func = mcux_dcnano_lcdif_dbi_config_func_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.dbi_config = {								\
			.type = kLCDIF_DbiTypeA_FixedE,					\
			.swizzle = DT_INST_ENUM_IDX_OR(n, swizzle, 0),			\
			.format = kLCDIF_DbiOutD8RGB332,				\
			.acTimeUnit = DT_INST_PROP_OR(n, divider, 1) - 1,		\
			.writeWRPeriod = DT_INST_PROP(n, wr_period),			\
			.writeWRAssert = DT_INST_PROP(n, wr_assert),			\
			.writeWRDeassert = DT_INST_PROP(n, wr_deassert),		\
			.writeCSAssert = DT_INST_PROP(n, cs_assert),			\
			.writeCSDeassert = DT_INST_PROP(n, cs_deassert),		\
		},									\
		.panel_config = {							\
			.enable = true,							\
			.enableGamma = false,						\
			.order = kLCDIF_VideoOverlay0Overlay1,				\
			.endian = DT_INST_ENUM_IDX_OR(n, endian, 0),			\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
		&mcux_dcnano_lcdif_dbi_init,						\
		NULL,									\
		&mcux_dcnano_lcdif_dbi_data_##n,					\
		&mcux_dcnano_lcdif_dbi_config_##n,					\
		POST_KERNEL,								\
		CONFIG_MIPI_DBI_INIT_PRIORITY,						\
		&mcux_dcnano_lcdif_dbi_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DCNANO_LCDIF_DEVICE_INIT)
