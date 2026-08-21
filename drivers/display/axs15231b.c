/*
 * Copyright 2026, NXP
 *
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 */

#define DT_DRV_COMPAT espressif_axs15231b

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_axs15231b);
#define AXS15231B_MADCTL_BGR BIT(3) /* Sets BGR color mode */
#define AXS15231B_BLANKING_ON 0x28
#define AXS15231B_BLANKNIG_OFF 0x29
#define AXS15231B_CONTROL_16BIT 0x55
#define AXS15231B_CMD_COLMOD 0x3A
#define AXS15231B_CMD_DISPON 0x29
#define AXS15231B_CMD_MADCTL 0x36
#define AXS15231B_NORMAL_MODE 0x13
#define AXS15231B_CMD_INVOFF 0x20
#define AXS15231B_CMD_INVON 0x21
#define AXS15231B_CMD_SLPOUT 0x11
#define AXS15231B_SRESET 0x01
struct axs15231b_config {
	const struct device *mipi_dbi;
	const struct mipi_dbi_config dbi_config;
	uint16_t width;
	uint16_t height;
	bool inverted; /* Display color inversion */
	/* Display configuration parameters */
	uint8_t dic; /* Display inversion control */
	uint8_t frmctl1[2]; /* Frame rate control, normal mode */
	uint8_t frmctl2[2]; /* Frame rate control, idle mode */
	uint8_t frmctl3[2]; /* Frame rate control, partial mode */
	uint8_t bpc[4]; /* Blanking porch control */
	uint8_t dfc[3]; /* Display function control */
	uint8_t pwr1[2]; /* Power control 1 */
	uint8_t pwr2; /* Power control 2 */
	uint8_t pwr3; /* Power control 3 */
	uint8_t vcmpctl; /* VCOM control */
	uint8_t doca[8]; /* Display output ctrl */
	uint8_t madctl; /* Memory data access control */
	uint8_t te_mode; /* Tearing enable mode */
	uint32_t te_delay; /* Tearing enable delay */
	bool rgb_is_inverted;
};

/* Helper: send a command byte with up to 4 parameter bytes. */
static int lcd_cmd(const struct device *dev, uint8_t cmd,
		   const uint8_t *params, size_t nparams)
{
	const struct axs15231b_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dbi, &config->dbi_config,
				      cmd, params, nparams);
}

static int axs15231b_blanking_on(const struct device *dev)
{
	return lcd_cmd(dev, AXS15231B_BLANKING_ON, NULL, 0);
}

static int axs15231b_blanking_off(const struct device *dev)
{
	return lcd_cmd(dev, AXS15231B_BLANKNIG_OFF, NULL, 0);
}
static int axs15231b_get_pixelfmt(const struct device *dev)
{
	const struct axs15231b_config *config = dev->config;

	/*
	 * Invert the pixel format for 8-bit 8080 Parallel Interface.
	 *
	 * Zephyr uses little endian byte order when the pixel format has
	 * multiple bytes.
	 *
	 * For RGB565, Red is placed in byte 1 and Blue in byte 0.
	 * For RGB565X, Red is placed in byte 0 and Blue in byte 1.
	 *
	 * This is not an issue when using a 16-bit interface.
	 * For RGB565, this would map to Red being in D[11:15] and
	 * Blue in D[0:4] and vice versa for RGB565X.
	 *
	 * However this is an issue when using a 8-bit interface.
	 * For RGB565, Blue is placed in byte 0 as mentioned earlier.
	 * However the controller expects Red to be in D[3:7] of byte 0.
	 *
	 * Hence we report pixel format as RGB565 when MADCTL setting is
	 * RGB565X and vice versa.
	 */
	if (config->dbi_config.mode == MIPI_DBI_MODE_8080_BUS_8_BIT) {
		/*
		 * Similar to the handling for other interface modes,
		 * invert the reported pixel format if "rgb_is_inverted"
		 * is enabled
		 */
		if (((bool)(config->madctl & AXS15231B_MADCTL_BGR)) !=
		    config->rgb_is_inverted) {
			return PIXEL_FORMAT_RGB_565;
		} else {
			return PIXEL_FORMAT_RGB_565X;
		}
	}

	/*
	 * Invert the pixel format if rgb_is_inverted is enabled.
	 * Report pixel format as the same format set in the MADCTL
	 * if rgb_is_inverted is disabled.
	 * Report pixel format as RGB if MADCTL setting is BGR and vice versa
	 * if rgb_is_inverted is enabled.
	 * It is a workaround for supporting buggy modules that display RGB as BGR.
	 */
	if (((bool)(config->madctl & AXS15231B_MADCTL_BGR)) !=
	    config->rgb_is_inverted) {
		return PIXEL_FORMAT_RGB_565;
	} else {
		return PIXEL_FORMAT_RGB_565X;
	}
}
static int axs15231b_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	/* Just check again the current pixel format as changing format at
	 * runtime is not supported
	 */
	if (pixel_format == axs15231b_get_pixelfmt(dev)) {
		return 0;
	}

	return -ENOTSUP;
}
static void axs15231b_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct axs15231b_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->current_pixel_format = axs15231b_get_pixelfmt(dev);
	capabilities->supported_pixel_formats = capabilities->current_pixel_format;
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}
/* Minimal AXS15231B panel bring-up sequence. */
static int axs15231b_init(const struct device *dev)
{
	const struct axs15231b_config *config = dev->config;
	int ret;
	uint8_t param;
	/* Software reset. */
	ret = lcd_cmd(dev, AXS15231B_SRESET, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_sleep(K_MSEC(120));

	/* Sleep out. */
	ret = lcd_cmd(dev, AXS15231B_CMD_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_sleep(K_MSEC(120));

	if (config->inverted) {
		ret = lcd_cmd(dev, AXS15231B_CMD_INVON, NULL, 0);
	} else {
		/* Display inversion off - colors are correct without inversion. */
		ret = lcd_cmd(dev, AXS15231B_CMD_INVOFF, NULL, 0);
	}
	if (ret < 0) {
		return ret;
	}

	param = AXS15231B_CONTROL_16BIT;
	/* Pixel format: 16-bit RGB565 (0x55). */
	ret = lcd_cmd(dev, AXS15231B_CMD_COLMOD, &param, 1);
	if (ret < 0) {
		return ret;
	}

	/* Normal display mode on. */
	ret = lcd_cmd(dev, AXS15231B_NORMAL_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	param = config->madctl;
	ret = lcd_cmd(dev, AXS15231B_CMD_MADCTL, &param, 1u);
	if (ret < 0) {
		return ret;
	}
	/* Display on. */
	ret = lcd_cmd(dev, AXS15231B_CMD_DISPON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(20));


	return 0;
}
static int axs15231b_set_cursor(const struct device *dev,
			      const uint16_t x, const uint16_t y,
			      const uint16_t width, const uint16_t height)
{
	int ret = 0;
	uint16_t addr_data[2];
	/* Column address */
	addr_data[0] = sys_cpu_to_be16(x);
	addr_data[1] = sys_cpu_to_be16(x + width - 1);

	ret = lcd_cmd(dev, 0x2A, (uint8_t *)addr_data, sizeof(addr_data));
	if (ret < 0) {
		return ret;
	}

	/* Row address */
	addr_data[0] = sys_cpu_to_be16(y);
	addr_data[1] = sys_cpu_to_be16(y + height - 1);
	ret = lcd_cmd(dev, 0x2B, (uint8_t *)addr_data, sizeof(addr_data));

	return ret;
}
static int draw_axs15231b_display(const struct device *dev,
				  const uint16_t x,
				  const uint16_t y,
				  const struct display_buffer_descriptor *desc,
				  const void *buf)
{
	const struct axs15231b_config *config = dev->config;
	int ret;
	struct display_buffer_descriptor mipi_desc = {0};
	enum display_pixel_format pixfmt;

	ret = axs15231b_set_cursor(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}


	mipi_desc.buf_size = desc->width * desc->height * 2;
	mipi_desc.frame_incomplete = desc->frame_incomplete;
	mipi_desc.pitch = desc->pitch;
	mipi_desc.width = desc->width;
	mipi_desc.height = desc->height;

	 mipi_dbi_command_write(config->mipi_dbi,
				      &config->dbi_config, 0x2C,
				      NULL, 0);
	pixfmt = axs15231b_get_pixelfmt(dev);

	return mipi_dbi_write_display(config->mipi_dbi,
				      &config->dbi_config, buf,
				      &mipi_desc, pixfmt);

}

static DEVICE_API(display, axs15231b_api) = {
	.blanking_on = axs15231b_blanking_on,
	.blanking_off = axs15231b_blanking_off,
	.write = draw_axs15231b_display,
	.get_capabilities = axs15231b_get_capabilities,
	.set_pixel_format = axs15231b_set_pixel_format,
};


#define AXS15231B_INIT(n)							\
	static const struct axs15231b_config axs15231b_config_##n = {		\
		.mipi_dbi = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		.dbi_config = {							\
			.config = MIPI_DBI_SPI_CONFIG_DT(			\
						DT_DRV_INST(n),			\
						SPI_OP_MODE_MASTER |		\
						SPI_WORD_SET(8),		\
						0),				\
			.mode = DT_INST_STRING_UPPER_TOKEN_OR(n, mipi_mode,	\
						MIPI_DBI_MODE_SPI_4WIRE),	\
			.color_coding = DT_INST_STRING_UPPER_TOKEN_OR(n,	\
						color_coding,			\
						MIPI_DBI_MODE_RGB565),		\
		},								\
		.width = DT_INST_PROP(n, width),				\
		.height = DT_INST_PROP(n, height),				\
		.inverted = DT_INST_PROP(n, color_invert),			\
		.dic = DT_INST_ENUM_IDX(n, invert_mode),			\
		.frmctl1 = DT_INST_PROP(n, frmctl1),				\
		.frmctl2 = DT_INST_PROP(n, frmctl2),				\
		.frmctl3 = DT_INST_PROP(n, frmctl3),				\
		.bpc = DT_INST_PROP(n, bpc),					\
		.dfc = DT_INST_PROP(n, dfc),					\
		.pwr1 = DT_INST_PROP(n, pwr1),					\
		.pwr2 = DT_INST_PROP(n, pwr2),					\
		.pwr3 = DT_INST_PROP(n, pwr3),					\
		.vcmpctl = DT_INST_PROP(n, vcmpctl),				\
		.doca = DT_INST_PROP(n, doca),					\
		.madctl = DT_INST_PROP(n, madctl),				\
		.rgb_is_inverted = DT_INST_PROP(n, rgb_is_inverted),		\
		.te_mode = MIPI_DBI_TE_MODE_DT_INST(n, te_mode),                \
		.te_delay = DT_INST_PROP(n, te_delay),                          \
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &axs15231b_init,				\
			NULL,							\
			NULL,							\
			&axs15231b_config_##n,					\
			POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,		\
			&axs15231b_api);

DT_INST_FOREACH_STATUS_OKAY(AXS15231B_INIT)
