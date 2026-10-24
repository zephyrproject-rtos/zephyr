/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_dbi_dcif

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_dcif.h>

LOG_MODULE_REGISTER(mipi_dbi_nxp_dcif, CONFIG_DISPLAY_LOG_LEVEL);

/* This driver drives DCIF layer 0 as the DBI transfer source. */
#define NXP_DCIF_DBI_LAYER 0U
/* Interrupts are routed to CPU domain 0 in a single-domain Zephyr system. */
#define NXP_DCIF_DBI_DOMAIN 0U

struct mcux_dcif_dbi_data {
	struct k_sem transfer_done;
	const struct mipi_dbi_config *active_cfg;
};

struct mcux_dcif_dbi_config {
	DCIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	dcif_dbi_config_t dbi_config;
	uint16_t width;
	uint16_t height;
	const struct pinctrl_dev_config *pincfg;
	struct reset_dt_spec reset_ctl;
	const struct gpio_dt_spec reset;
};

struct mcux_dcif_dbi_format_map_t {
	uint8_t bus_type;
	uint8_t color_coding;
	dcif_dbi_out_format_t format;
};

static const struct mcux_dcif_dbi_format_map_t format_map[] = {
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB332, kDCIF_DbiOutD8RGB332},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB444, kDCIF_DbiOutD8RGB444},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB565, kDCIF_DbiOutD8RGB565},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB666_1, kDCIF_DbiOutD8RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB888_1, kDCIF_DbiOutD8RGB888},
	{MIPI_DBI_MODE_6800_BUS_9_BIT, MIPI_DBI_MODE_RGB666_1, kDCIF_DbiOutD9RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB666_2, kDCIF_DbiOutD8RGB666},
	{MIPI_DBI_MODE_6800_BUS_8_BIT, MIPI_DBI_MODE_RGB888_2, kDCIF_DbiOutD8RGB888},
	{MIPI_DBI_MODE_6800_BUS_9_BIT, MIPI_DBI_MODE_RGB666_2, kDCIF_DbiOutD9RGB666},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB332, kDCIF_DbiOutD16RGB332},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB444, kDCIF_DbiOutD16RGB444},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB565, kDCIF_DbiOutD16RGB565},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB666_1,
		kDCIF_DbiOutD16RGB666Option1},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB666_2,
		kDCIF_DbiOutD16RGB666Option2},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB888_1,
		kDCIF_DbiOutD16RGB888Option1},
	{MIPI_DBI_MODE_6800_BUS_16_BIT, MIPI_DBI_MODE_RGB888_2,
		kDCIF_DbiOutD16RGB888Option2},
};

/*
 * True when bus_type selects an 8-bit-wide DBI bus. DCIF_DbiWriteParam() packs two
 * source bytes into one bus word on anything wider than 8 bits (correct for pixel
 * data, but not for independent command-parameter bytes), so callers that write
 * discrete parameter bytes need to know this to avoid mis-packing them.
 */
static bool mcux_dcif_dbi_is_8bit_bus(uint8_t bus_type)
{
	return (bus_type == MIPI_DBI_MODE_6800_BUS_8_BIT) ||
	       (bus_type == MIPI_DBI_MODE_8080_BUS_8_BIT);
}

static int mcux_dcif_dbi_get_format(uint8_t bus_type, uint8_t color_coding,
	dcif_dbi_out_format_t *format)
{
	uint8_t normalized_bus_type = bus_type;

	if (bus_type >= MIPI_DBI_MODE_8080_BUS_16_BIT) {
		normalized_bus_type -= MIPI_DBI_MODE_6800_BUS_16_BIT;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(format_map); i++) {
		if ((format_map[i].bus_type == normalized_bus_type) &&
			(format_map[i].color_coding == color_coding)) {
			*format = format_map[i].format;
			return 0;
		}
	}

	return -EINVAL;
}

static void mcux_dcif_dbi_isr(const struct device *dev)
{
	const struct mcux_dcif_dbi_config *config = dev->config;
	struct mcux_dcif_dbi_data *data = dev->data;
	uint32_t status;

	status = DCIF_GetInterruptStatus(config->base, NXP_DCIF_DBI_DOMAIN);
	DCIF_ClearInterruptStatus(config->base, NXP_DCIF_DBI_DOMAIN, status);

	if (0 != (status & kDCIF_InterruptDbiCommandDone)) {
		k_sem_give(&data->transfer_done);
	}
}

static int mcux_dcif_dbi_configure(const struct device *dev,
				    const struct mipi_dbi_config *dbi_config)
{
	const struct mcux_dcif_dbi_config *config = dev->config;
	struct mcux_dcif_dbi_data *data = dev->data;
	uint8_t bus_type = dbi_config->mode & 0xFU;
	uint8_t color_coding = dbi_config->color_coding & 0xF0U;
	dcif_dbi_config_t dcif_dbi_config = config->dbi_config;
	status_t status;

	/* No need to update if configuration is the same. */
	if (dbi_config == data->active_cfg) {
		return 0;
	}

	/* SPI mode is not supported by the SDK DCIF driver */
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
		dcif_dbi_config.type = kDCIF_DbiTypeA_FixedE;
		break;
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		dcif_dbi_config.type = kDCIF_DbiTypeB;
		break;
	default:
		return -EINVAL;
	}

	/* Get the output format. */
	status = mcux_dcif_dbi_get_format(bus_type, color_coding, &dcif_dbi_config.format);

	if (kStatus_Success != status) {
		return -EINVAL;
	}

	/* Update DBI configuration. */
	DCIF_DbiSetConfig(config->base, &dcif_dbi_config);

	data->active_cfg = dbi_config;

	return 0;
}

static int mcux_dcif_dbi_init(const struct device *dev)
{
	const struct mcux_dcif_dbi_config *config = dev->config;
	struct mcux_dcif_dbi_data *data = dev->data;
	dcif_output_config_t output_config = {
		.interface = kDCIF_OutputDbi,
		.width = config->width,
		.height = config->height,
	};
	int ret;

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret != 0) {
			return ret;
		}
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (config->reset_ctl.dev != NULL) {
		if (!device_is_ready(config->reset_ctl.dev)) {
			return -ENODEV;
		}

		ret = reset_line_deassert_dt(&config->reset_ctl);
		if (ret != 0) {
			return ret;
		}
	}

	DCIF_Init(config->base);

	/* Placeholder size, satisfies DCIF_SetOutputConfig()'s nonzero assert until the
	 * first write_display() reprograms it to the real update area.
	 */
	DCIF_SetOutputConfig(config->base, &output_config);

	DCIF_SetBackGroundLayerColor(config->base, 0x0U);

	/* Real CSC mode is set per-write in write_display(), based on that call's
	 * pixel format; this is just the init-time default.
	 */
	DCIF_SetCscMode(config->base, NXP_DCIF_DBI_LAYER, kDCIF_CscDisable);

	DCIF_DbiSetConfig(config->base, &config->dbi_config);

	DCIF_EnableOutput(config->base, true);

	/*
	 * The DBI-done interrupt is left disabled here and only enabled for the
	 * duration of write_display()'s pixel transfer -- see write_display() for why.
	 */
	config->irq_config_func(dev);

	k_sem_init(&data->transfer_done, 0, 1);

	LOG_DBG("%s device init complete", dev->name);

	return 0;
}

static int mipi_dbi_dcif_write_display(const struct device *dev,
					const struct mipi_dbi_config *dbi_config,
					const uint8_t *framebuf,
					struct display_buffer_descriptor *desc,
					enum display_pixel_format pixfmt)
{
	const struct mcux_dcif_dbi_config *config = dev->config;
	struct mcux_dcif_dbi_data *data = dev->data;
	int ret;
	uint8_t bytes_per_pixel;
	dcif_layer_format_t format;
	dcif_output_config_t output_config = {
		.interface = kDCIF_OutputDbi,
		.width = desc->width,
		.height = desc->height,
	};

	/* The DBI bus type and color coding. */
	ret = mcux_dcif_dbi_configure(dev, dbi_config);
	if (ret) {
		return ret;
	}

	switch (pixfmt) {
	case PIXEL_FORMAT_RGB_565:
		format = kDCIF_LayerPixelFormatRGB565;
		bytes_per_pixel = 2U;
		break;
	case PIXEL_FORMAT_RGB_888:
		format = kDCIF_LayerPixelFormatRGB888;
		bytes_per_pixel = 3U;
		break;
	case PIXEL_FORMAT_BGR_888:
		format = kDCIF_LayerPixelFormatBGR888;
		bytes_per_pixel = 3U;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		format = kDCIF_LayerPixelFormatARGB8888;
		bytes_per_pixel = 4U;
		break;
	case PIXEL_FORMAT_ABGR_8888:
		format = kDCIF_LayerPixelFormatABGR8888;
		bytes_per_pixel = 4U;
		break;
	case PIXEL_FORMAT_NV12:
		format = kDCIF_LayerPixelFormatNV21;
		bytes_per_pixel = 1U;
		break;
	default:
		LOG_ERR("Pixel format not supported.");
		return -ENOTSUP;
	}

	DCIF_SetCscMode(config->base, NXP_DCIF_DBI_LAYER,
			pixfmt == PIXEL_FORMAT_NV12 ? kDCIF_CscYCbCr2RGB : kDCIF_CscDisable);

	/* Resize the output to the current update area -- DCIF requires DISP_SIZE to
	 * be reprogrammed for every transfer, not just once at init.
	 */
	DCIF_SetOutputConfig(config->base, &output_config);

	dcif_layer_config_t layer_config = {
		.enable = true,
		.format = format,
		.topLeftX = 0,
		.topLeftY = 0,
		.width = desc->width,
		.height = desc->height,
		.background = 0U,
		.globalAlpha = 0xFFU,
		.alphaBlendMode = kDCIF_AlphaBlendOverride,
	};

	/* Also programs the layer position -- no separate DCIF_SetLayerPosition() call
	 * needed.
	 */
	if (kStatus_Success != DCIF_SetLayerConfig(config->base, NXP_DCIF_DBI_LAYER,
						    &layer_config)) {
		return -EINVAL;
	}

	DCIF_SetLayerStride(config->base, NXP_DCIF_DBI_LAYER, bytes_per_pixel * desc->pitch);
	DCIF_SetLayerAddr(config->base, NXP_DCIF_DBI_LAYER, (uint32_t)framebuf);

	if (pixfmt == PIXEL_FORMAT_NV12) {
		DCIF_SetLayerUVAddr(config->base, NXP_DCIF_DBI_LAYER,
				(uint32_t)framebuf + bytes_per_pixel * desc->pitch * desc->height);
	}

	if (IS_ENABLED(CONFIG_MIPI_DBI_NXP_DCIF_MAINTAIN_CACHE)) {
		sys_cache_data_flush_range((void *)framebuf, desc->buf_size);
	}

	DCIF_TriggerLayerShadowLoad(config->base, NXP_DCIF_DBI_LAYER);

	DCIF_DisableInterrupts(config->base, NXP_DCIF_DBI_DOMAIN, kDCIF_InterruptDbiCommandDone);

	DCIF_EnableOutput(config->base, true);
	DCIF_DbiWriteCommand(config->base, 0x2CU);
	DCIF_DbiWritePixel(config->base);

	DCIF_EnableInterrupts(config->base, NXP_DCIF_DBI_DOMAIN, kDCIF_InterruptDbiCommandDone);

	/* Wait for transfer done. */
	k_sem_take(&data->transfer_done, K_FOREVER);

	return 0;
}

static int mipi_dbi_dcif_command_write(const struct device *dev,
					const struct mipi_dbi_config *dbi_config,
					uint8_t cmd, const uint8_t *data_buf,
					size_t len)
{
	const struct mcux_dcif_dbi_config *config = dev->config;
	int ret;

	/* The DBI bus type and color coding. */
	ret = mcux_dcif_dbi_configure(dev, dbi_config);
	if (ret) {
		return ret;
	}

	DCIF_DbiWriteCommand(config->base, cmd);

	if (len != 0U) {
		if (mcux_dcif_dbi_is_8bit_bus(dbi_config->mode & 0xFU)) {
			DCIF_DbiWriteParam(config->base, data_buf, len);
		} else {
			/*
			 * DCIF_DbiWriteParam() packs two consecutive bytes of
			 * whatever buffer it is given into one bus word on a
			 * bus wider than 8 bits. That is correct for pixel data,
			 * but each of these parameter bytes is an independent
			 * value, so each one must go out as its own bus word.
			 */
			for (size_t i = 0; i < len; i++) {
				uint16_t word = (uint16_t)data_buf[i];

				DCIF_DbiWriteParam(config->base, (const uint8_t *)&word, 2U);
			}
		}
	}

	return 0;
}

static int mipi_dbi_dcif_reset(const struct device *dev, k_timeout_t delay)
{
	int ret;

	const struct mcux_dcif_dbi_config *config = dev->config;

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

static DEVICE_API(mipi_dbi, mcux_dcif_dbi_api) = {
	.reset = mipi_dbi_dcif_reset,
	.command_write = mipi_dbi_dcif_command_write,
	.write_display = mipi_dbi_dcif_write_display,
};

#define MCUX_DCIF_DBI_DEVICE_INIT(n)							\
	static void mcux_dcif_dbi_config_func_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
				DT_INST_IRQ(n, priority),				\
				mcux_dcif_dbi_isr,					\
				DEVICE_DT_INST_GET(n),					\
				0);							\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
	PINCTRL_DT_INST_DEFINE(n);							\
	static struct mcux_dcif_dbi_data mcux_dcif_dbi_data_##n;			\
	static const struct mcux_dcif_dbi_config mcux_dcif_dbi_config_##n = {		\
		.base = (DCIF_Type *) DT_INST_REG_ADDR(n),				\
		.irq_config_func = mcux_dcif_dbi_config_func_##n,			\
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL)),		\
		.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)),		\
			((clock_control_subsys_t)0U)),					\
		.width = DT_INST_PROP(n, width),					\
		.height = DT_INST_PROP(n, height),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.reset_ctl = RESET_DT_SPEC_INST_GET_OR(n, {0}),				\
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.dbi_config = {								\
			.type = kDCIF_DbiTypeA_FixedE,					\
			.format = kDCIF_DbiOutD8RGB332,					\
			.signalFlags = 0U,						\
			.rdHigh = 0U,							\
			.rdLow = 0U,							\
			.wrHigh = DT_INST_PROP(n, wr_high),				\
			.wrLow = DT_INST_PROP(n, wr_low),				\
			.csSetup = DT_INST_PROP(n, cs_setup),				\
			.csHold = DT_INST_PROP(n, cs_hold),				\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
		&mcux_dcif_dbi_init,							\
		NULL,									\
		&mcux_dcif_dbi_data_##n,						\
		&mcux_dcif_dbi_config_##n,						\
		POST_KERNEL,								\
		CONFIG_MIPI_DBI_INIT_PRIORITY,						\
		&mcux_dcif_dbi_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DCIF_DBI_DEVICE_INIT)
