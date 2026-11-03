/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raydium_rm67162

#include <zephyr/display/mipi_display.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dbi)
#include <zephyr/drivers/mipi_dbi.h>
#endif

LOG_MODULE_REGISTER(rm67162, CONFIG_DISPLAY_LOG_LEVEL);

/* Manufacturer commands */
#define RM67162_CMD_SET_DSPI_MODE 0xC4
#define RM67162_CMD_PAGE_SELECT   0xFE

/* SET_DSPI_MODE: allow frame memory writes over SPI */
#define RM67162_DSPI_MODE_SPI_WRAM BIT(7)
/* WRITE_CONTROL_DISPLAY: brightness control block on */
#define RM67162_WRCTRLD_BCTRL      BIT(5)

/* MIPI DBI reset pulse width, and time to wait after reset or sleep out */
#define RM67162_DBI_RESET_PULSE_MS 10
#define RM67162_DBI_WAKEUP_MS      120

struct rm67162_init_cmd {
	uint8_t cmd;
	uint8_t param;
};

struct rm67162_init_seq {
	const struct rm67162_init_cmd *cmds;
	size_t len;
};

/* Bus specific part of the driver */
struct rm67162_bus {
	int (*cmd_write)(const struct device *dev, uint8_t cmd, const uint8_t *tx_buf,
			 size_t tx_len);
	int (*write_memory)(const struct device *dev, const struct display_buffer_descriptor *desc,
			    const void *buf);
	int (*blanking_on)(const struct device *dev);
	int (*blanking_off)(const struct device *dev);
	uint32_t supported_pixel_formats;
	/* Address mode bits for the color order of PIXEL_FORMAT_RGB_888 */
	uint8_t rgb888_address_mode;
	/* Reported orientation with the address mode at its reset value */
	enum display_orientation native_orientation;
};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
struct rm67162_dsi_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
	const struct gpio_dt_spec te_gpio;
	uint16_t hsync;
	uint16_t hbp;
	uint16_t hfp;
	uint16_t vfp;
	uint16_t vbp;
	uint16_t vsync;
};

struct rm67162_dsi_data {
	struct gpio_callback te_gpio_cb;
	struct k_sem te_sem;
};
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dbi)
struct rm67162_dbi_config {
	const struct device *mipi_dbi;
	struct mipi_dbi_config dbi_config;
	uint8_t te_mode;
	uint32_t te_delay;
};
#endif

struct rm67162_config {
	const struct rm67162_bus *ops;
	const struct rm67162_init_seq *init_seq;
	uint16_t panel_width;
	uint16_t panel_height;
	enum display_pixel_format pixel_format;
	/* Index of the rotation property: 0, 90, 180 or 270 degrees */
	uint8_t rotation;
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
		struct rm67162_dsi_config dsi;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dbi)
		struct rm67162_dbi_config dbi;
#endif
	} bus;
};

struct rm67162_data {
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
	struct rm67162_dsi_data dsi;
#endif
};

/*
 * These commands are taken from NXP's MCUXpresso SDK.
 * Additional documentation is added where possible, but the
 * Manufacture command set pages are not described in the datasheet
 */
static const struct rm67162_init_cmd rm67162_init_g1120b0mipi[] = {
	/* CMD Mode switch, select manufacture command set page 0 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x01},
	{.cmd = 0x06, .param = 0x62},
	{.cmd = 0x0E, .param = 0x80},
	{.cmd = 0x0F, .param = 0x80},
	{.cmd = 0x10, .param = 0x71},
	{.cmd = 0x13, .param = 0x81},
	{.cmd = 0x14, .param = 0x81},
	{.cmd = 0x15, .param = 0x82},
	{.cmd = 0x16, .param = 0x82},
	{.cmd = 0x18, .param = 0x88},
	{.cmd = 0x19, .param = 0x55},
	{.cmd = 0x1A, .param = 0x10},
	{.cmd = 0x1C, .param = 0x99},
	{.cmd = 0x1D, .param = 0x03},
	{.cmd = 0x1E, .param = 0x03},
	{.cmd = 0x1F, .param = 0x03},
	{.cmd = 0x20, .param = 0x03},
	{.cmd = 0x25, .param = 0x03},
	{.cmd = 0x26, .param = 0x8D},
	{.cmd = 0x2A, .param = 0x03},
	{.cmd = 0x2B, .param = 0x8D},
	{.cmd = 0x36, .param = 0x00},
	{.cmd = 0x37, .param = 0x10},
	{.cmd = 0x3A, .param = 0x00},
	{.cmd = 0x3B, .param = 0x00},
	{.cmd = 0x3D, .param = 0x20},
	{.cmd = 0x3F, .param = 0x3A},
	{.cmd = 0x40, .param = 0x30},
	{.cmd = 0x41, .param = 0x30},
	{.cmd = 0x42, .param = 0x33},
	{.cmd = 0x43, .param = 0x22},
	{.cmd = 0x44, .param = 0x11},
	{.cmd = 0x45, .param = 0x66},
	{.cmd = 0x46, .param = 0x55},
	{.cmd = 0x47, .param = 0x44},
	{.cmd = 0x4C, .param = 0x33},
	{.cmd = 0x4D, .param = 0x22},
	{.cmd = 0x4E, .param = 0x11},
	{.cmd = 0x4F, .param = 0x66},
	{.cmd = 0x50, .param = 0x55},
	{.cmd = 0x51, .param = 0x44},
	{.cmd = 0x57, .param = 0xB3},
	{.cmd = 0x6B, .param = 0x19},
	{.cmd = 0x70, .param = 0x55},
	{.cmd = 0x74, .param = 0x0C},

	/* VGMP/VGSP Voltage Control (select manufacture command set page 1 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x02},
	{.cmd = 0x9B, .param = 0x40},
	{.cmd = 0x9C, .param = 0x67},
	{.cmd = 0x9D, .param = 0x20},

	/* VGMP/VGSP Voltage Control (select manufacture command set page 2 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x03},
	{.cmd = 0x9B, .param = 0x40},
	{.cmd = 0x9C, .param = 0x67},
	{.cmd = 0x9D, .param = 0x20},

	/* VSR Command (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x5D, .param = 0x10},

	/* VSR1 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x00, .param = 0x8D},
	{.cmd = 0x01, .param = 0x00},
	{.cmd = 0x02, .param = 0x01},
	{.cmd = 0x03, .param = 0x01},
	{.cmd = 0x04, .param = 0x10},
	{.cmd = 0x05, .param = 0x01},
	{.cmd = 0x06, .param = 0xA7},
	{.cmd = 0x07, .param = 0x20},
	{.cmd = 0x08, .param = 0x00},

	/* VSR2 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x09, .param = 0xC2},
	{.cmd = 0x0A, .param = 0x00},
	{.cmd = 0x0B, .param = 0x02},
	{.cmd = 0x0C, .param = 0x01},
	{.cmd = 0x0D, .param = 0x40},
	{.cmd = 0x0E, .param = 0x06},
	{.cmd = 0x0F, .param = 0x01},
	{.cmd = 0x10, .param = 0xA7},
	{.cmd = 0x11, .param = 0x00},

	/* VSR3 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x12, .param = 0xC2},
	{.cmd = 0x13, .param = 0x00},
	{.cmd = 0x14, .param = 0x02},
	{.cmd = 0x15, .param = 0x01},
	{.cmd = 0x16, .param = 0x40},
	{.cmd = 0x17, .param = 0x07},
	{.cmd = 0x18, .param = 0x01},
	{.cmd = 0x19, .param = 0xA7},
	{.cmd = 0x1A, .param = 0x00},

	/* VSR4 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x1B, .param = 0x82},
	{.cmd = 0x1C, .param = 0x00},
	{.cmd = 0x1D, .param = 0xFF},
	{.cmd = 0x1E, .param = 0x05},
	{.cmd = 0x1F, .param = 0x60},
	{.cmd = 0x20, .param = 0x02},
	{.cmd = 0x21, .param = 0x01},
	{.cmd = 0x22, .param = 0x7C},
	{.cmd = 0x23, .param = 0x00},

	/* VSR5 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x24, .param = 0xC2},
	{.cmd = 0x25, .param = 0x00},
	{.cmd = 0x26, .param = 0x04},
	{.cmd = 0x27, .param = 0x02},
	{.cmd = 0x28, .param = 0x70},
	{.cmd = 0x29, .param = 0x05},
	{.cmd = 0x2A, .param = 0x74},
	{.cmd = 0x2B, .param = 0x8D},
	{.cmd = 0x2D, .param = 0x00},

	/* VSR6 Timing Set (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x2F, .param = 0xC2},
	{.cmd = 0x30, .param = 0x00},
	{.cmd = 0x31, .param = 0x04},
	{.cmd = 0x32, .param = 0x02},
	{.cmd = 0x33, .param = 0x70},
	{.cmd = 0x34, .param = 0x07},
	{.cmd = 0x35, .param = 0x74},
	{.cmd = 0x36, .param = 0x8D},
	{.cmd = 0x37, .param = 0x00},

	/* VSR Marping command  (select manufacture command set page 3 ) */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x5E, .param = 0x20},
	{.cmd = 0x5F, .param = 0x31},
	{.cmd = 0x60, .param = 0x54},
	{.cmd = 0x61, .param = 0x76},
	{.cmd = 0x62, .param = 0x98},

	/* Select manufacture command set page 4 */
	/* ELVSS -2.4V(RT4723). 0x15: RT4723. 0x01: RT4723B. 0x17: STAM1332. */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x05},
	{.cmd = 0x05, .param = 0x15},
	{.cmd = 0x2A, .param = 0x04},
	{.cmd = 0x91, .param = 0x00},

	/* Select user command set */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x00},
	/* Set tearing effect signal to only output at V-blank*/
	{.cmd = 0x35, .param = 0x00},
};

/*
 * Manufacture command set values for the 1.91" 240x536 AMOLED panel used on
 * the LilyGO T-Display S3 AMOLED boards, taken from the vendor's reference code.
 */
static const struct rm67162_init_cmd rm67162_init_lilygo_t_display_s3_amoled[] = {
	/* Select manufacture command set page 3 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x04},
	{.cmd = 0x6A, .param = 0x00},
	/* Select manufacture command set page 4 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x05},
	/* Select manufacture command set page 6 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x07},
	{.cmd = 0x07, .param = 0x4F},
	/* Select manufacture command set page 0 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x01},
	{.cmd = 0x2A, .param = 0x02},
	{.cmd = 0x2B, .param = 0x73},
	/* Select manufacture command set page 9 */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x0A},
	{.cmd = 0x29, .param = 0x10},
	/* Select user command set */
	{.cmd = RM67162_CMD_PAGE_SELECT, .param = 0x00},
};

/* Indexed by the init-sequence devicetree property */
static const struct rm67162_init_seq rm67162_init_seqs[] = {
	{rm67162_init_g1120b0mipi, ARRAY_SIZE(rm67162_init_g1120b0mipi)},
	{rm67162_init_lilygo_t_display_s3_amoled,
	 ARRAY_SIZE(rm67162_init_lilygo_t_display_s3_amoled)},
};

static int rm67162_cmd_write(const struct device *dev, uint8_t cmd, const uint8_t *tx_buf,
			     size_t tx_len)
{
	const struct rm67162_config *config = dev->config;

	return config->ops->cmd_write(dev, cmd, tx_buf, tx_len);
}

static int rm67162_write_init_seq(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	int ret;

	for (size_t i = 0; i < config->init_seq->len; i++) {
		ret = rm67162_cmd_write(dev, config->init_seq->cmds[i].cmd,
					&config->init_seq->cmds[i].param, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* Rotation of an orientation relative to the address mode reset value, in quarter turns */
static uint8_t rm67162_quarter_turns(const struct device *dev, enum display_orientation orientation)
{
	const struct rm67162_config *config = dev->config;

	return ((uint8_t)orientation - (uint8_t)config->ops->native_orientation) & 0x3U;
}

static void rm67162_get_resolution(const struct device *dev, uint16_t *x_res, uint16_t *y_res)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;

	if ((rm67162_quarter_turns(dev, data->orientation) & 0x1U) != 0U) {
		*x_res = config->panel_height;
		*y_res = config->panel_width;
	} else {
		*x_res = config->panel_width;
		*y_res = config->panel_height;
	}
}

static int rm67162_set_window(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
			      uint16_t height)
{
	uint8_t param[4];
	int ret;

	sys_put_be16(x, &param[0]);
	sys_put_be16(x + width - 1U, &param[2]);
	ret = rm67162_cmd_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	sys_put_be16(y, &param[0]);
	sys_put_be16(y + height - 1U, &param[2]);
	return rm67162_cmd_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, param, sizeof(param));
}

static int rm67162_set_address_mode(const struct device *dev, enum display_orientation orientation,
				    enum display_pixel_format pixel_format)
{
	static const uint8_t rotations[] = {
		0U,
		MIPI_DCS_ADDRESS_MODE_SWAP_XY | MIPI_DCS_ADDRESS_MODE_MIRROR_X,
		MIPI_DCS_ADDRESS_MODE_MIRROR_X | MIPI_DCS_ADDRESS_MODE_MIRROR_Y,
		MIPI_DCS_ADDRESS_MODE_SWAP_XY | MIPI_DCS_ADDRESS_MODE_MIRROR_Y,
	};
	const struct rm67162_config *config = dev->config;
	uint8_t param;

	if (orientation > DISPLAY_ORIENTATION_ROTATED_270) {
		return -ENOTSUP;
	}

	param = rotations[rm67162_quarter_turns(dev, orientation)];
	if (pixel_format == PIXEL_FORMAT_RGB_888) {
		param |= config->ops->rgb888_address_mode;
	}

	return rm67162_cmd_write(dev, MIPI_DCS_SET_ADDRESS_MODE, &param, 1);
}

static int rm67162_blanking_on(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	return config->ops->blanking_on(dev);
}

static int rm67162_blanking_off(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	return config->ops->blanking_off(dev);
}

static int rm67162_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;
	const uint8_t bytes_per_pixel = DISPLAY_BITS_PER_PIXEL(data->pixel_format) / 8U;
	uint16_t x_res;
	uint16_t y_res;
	int ret;

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((desc->width == 0U) || (desc->height == 0U) || (desc->pitch < desc->width)) {
		LOG_ERR("Invalid buffer descriptor");
		return -EINVAL;
	}

	rm67162_get_resolution(dev, &x_res, &y_res);
	if (((uint32_t)x + desc->width > x_res) || ((uint32_t)y + desc->height > y_res)) {
		LOG_ERR("Write area outside of the display");
		return -EINVAL;
	}

	if (desc->buf_size <
	    ((size_t)(desc->height - 1U) * desc->pitch + desc->width) * bytes_per_pixel) {
		LOG_ERR("Buffer too small");
		return -EINVAL;
	}

	ret = rm67162_set_window(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	return config->ops->write_memory(dev, desc, buf);
}

static int rm67162_set_brightness(const struct device *dev, const uint8_t brightness)
{
	return rm67162_cmd_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 1);
}

static void rm67162_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;

	rm67162_get_resolution(dev, &capabilities->x_resolution, &capabilities->y_resolution);
	capabilities->supported_pixel_formats = config->ops->supported_pixel_formats;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = data->orientation;
}

static int rm67162_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	uint8_t param;
	int ret;

	if ((config->ops->supported_pixel_formats & pixel_format) == 0U) {
		return -ENOTSUP;
	}

	if (pixel_format == PIXEL_FORMAT_RGB_888) {
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
	} else {
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
	}

	ret = rm67162_cmd_write(dev, MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);
	if (ret < 0) {
		return ret;
	}

	ret = rm67162_set_address_mode(dev, data->orientation, pixel_format);
	if (ret < 0) {
		return ret;
	}

	data->pixel_format = pixel_format;

	return 0;
}

static int rm67162_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct rm67162_data *data = dev->data;
	int ret;

	ret = rm67162_set_address_mode(dev, orientation, data->pixel_format);
	if (ret < 0) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

static void rm67162_init_state(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;

	data->pixel_format = config->pixel_format;
	data->orientation = (enum display_orientation)(
		(config->ops->native_orientation + config->rotation) & 0x3U);
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)

static uint8_t rm67162_dsi_pixfmt(enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_RGB_888) {
		return MIPI_DSI_PIXFMT_RGB888;
	}

	return MIPI_DSI_PIXFMT_RGB565;
}

static int rm67162_dsi_cmd_write(const struct device *dev, uint8_t cmd, const uint8_t *tx_buf,
				 size_t tx_len)
{
	const struct rm67162_config *config = dev->config;
	ssize_t ret;

	ret = mipi_dsi_dcs_write(config->bus.dsi.mipi_dsi, config->bus.dsi.channel, cmd, tx_buf,
				 tx_len);

	return (ret < 0) ? (int)ret : 0;
}

static void rm67162_dsi_te_isr_handler(const struct device *gpio_dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct rm67162_data *data = CONTAINER_OF(cb, struct rm67162_data, dsi.te_gpio_cb);

	k_sem_give(&data->dsi.te_sem);
}

/* Helper to write framebuffer data to rm67162 via MIPI interface. */
static int rm67162_dsi_write_fb(const struct device *dev, const uint8_t *src,
				const struct display_buffer_descriptor *desc)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;
	const uint8_t bytes_per_pixel = DISPLAY_BITS_PER_PIXEL(data->pixel_format) / 8U;
	ssize_t wlen = 0;
	struct mipi_dsi_msg msg = {0};
	uint8_t *local_src = (uint8_t *)src;
	uint32_t len = desc->height * desc->width * bytes_per_pixel;
	uint32_t len_sent = 0U;

	/* Note- we need to set custom flags on the DCS message,
	 * so we bypass the mipi_dsi_dcs_write API
	 */
	msg.cmd = MIPI_DCS_WRITE_MEMORY_START;
	msg.type = MIPI_DSI_DCS_LONG_WRITE;
	msg.flags = MCUX_DSI_2L_FB_DATA;
	msg.user_data = (void *)desc;

	while (len > 0) {
		msg.tx_len = len;
		msg.tx_buf = local_src;
		wlen = mipi_dsi_transfer(config->bus.dsi.mipi_dsi, config->bus.dsi.channel, &msg);
		if (wlen < 0) {
			return (int)wlen;
		}
		/* Advance source pointer and decrement remaining */
		if (desc->pitch > desc->width) {
			len_sent += wlen;
			local_src += wlen + len_sent / (desc->width * bytes_per_pixel) *
						    ((desc->pitch - desc->width) * bytes_per_pixel);
		} else {
			local_src += wlen;
		}
		len -= wlen;
		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}

	return 0;
}

static int rm67162_dsi_write_memory(const struct device *dev,
				    const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;

	/*
	 * If the tearing effect GPIO is present, wait until the display
	 * controller issues an interrupt (which will give to the TE semaphore)
	 * before sending the frame
	 */
	if (config->bus.dsi.te_gpio.port != NULL) {
		/* Block sleep state until next TE interrupt so we can send
		 * frame during that interval
		 */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		k_sem_take(&data->dsi.te_sem, K_FOREVER);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}

	return rm67162_dsi_write_fb(dev, buf, desc);
}

static int rm67162_dsi_blanking_on(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	if (config->bus.dsi.bl_gpio.port == NULL) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&config->bus.dsi.bl_gpio, 0);
}

static int rm67162_dsi_blanking_off(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	if (config->bus.dsi.bl_gpio.port == NULL) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&config->bus.dsi.bl_gpio, 1);
}

static int rm67162_dsi_setup_te(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	const struct gpio_dt_spec *te_gpio = &config->bus.dsi.te_gpio;
	int ret;

	ret = gpio_pin_configure_dt(te_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure TE GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(te_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure TE interrupt (%d)", ret);
		return ret;
	}

	/* Init and install GPIO callback */
	gpio_init_callback(&data->dsi.te_gpio_cb, rm67162_dsi_te_isr_handler, BIT(te_gpio->pin));
	ret = gpio_add_callback(te_gpio->port, &data->dsi.te_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not add TE gpio callback");
		return ret;
	}

	/* Setup te pin semaphore */
	k_sem_init(&data->dsi.te_sem, 0, 1);

	return 0;
}

static int rm67162_dsi_init(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_dsi_config *dsi = &config->bus.dsi;
	struct rm67162_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};
	uint16_t x_res;
	uint16_t y_res;
	int ret;

	rm67162_init_state(dev);

	/* Attach to MIPI DSI host */
	mdev.data_lanes = dsi->num_of_lanes;
	mdev.pixfmt = rm67162_dsi_pixfmt(data->pixel_format);
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	mdev.timings.hactive = config->panel_width;
	mdev.timings.hbp = dsi->hbp;
	mdev.timings.hfp = dsi->hfp;
	mdev.timings.hsync = dsi->hsync;
	mdev.timings.vactive = config->panel_height;
	mdev.timings.vbp = dsi->vbp;
	mdev.timings.vfp = dsi->vfp;
	mdev.timings.vsync = dsi->vsync;

	ret = mipi_dsi_attach(dsi->mipi_dsi, dsi->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	if (dsi->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&dsi->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		/*
		 * Power to the display has been enabled via the regulator fixed api during
		 * regulator init. Per datasheet, we must wait at least 10ms before
		 * starting reset sequence after power on.
		 */
		k_sleep(K_MSEC(10));
		/* Start reset sequence */
		ret = gpio_pin_set_dt(&dsi->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}
		/* Per datasheet, reset low pulse width should be at least 10usec */
		k_sleep(K_USEC(30));
		ret = gpio_pin_set_dt(&dsi->reset_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high (%d)", ret);
			return ret;
		}
		/*
		 * It is necessary to wait at least 120msec after releasing reset,
		 * before sending additional commands. This delay can be 5msec
		 * if we are certain the display module is in SLEEP IN state,
		 * but this is not guaranteed (for example, with a warm reset)
		 */
		k_sleep(K_MSEC(150));
	}

	ret = rm67162_write_init_seq(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set pixel format and address mode */
	ret = rm67162_set_pixel_format(dev, data->pixel_format);
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	ret = rm67162_cmd_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	/*
	 * We must wait 5 ms after exiting sleep mode before sending additional
	 * commands. If we intend to enter sleep mode, we must delay
	 * 120 ms before sending that command. To be safe, delay 150ms
	 */
	k_sleep(K_MSEC(150));

	/* Setup backlight */
	if (dsi->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&dsi->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	if (dsi->te_gpio.port != NULL) {
		ret = rm67162_dsi_setup_te(dev);
		if (ret < 0) {
			return ret;
		}
	}

	rm67162_get_resolution(dev, &x_res, &y_res);
	ret = rm67162_set_window(dev, 0, 0, x_res, y_res);
	if (ret < 0) {
		return ret;
	}

	/* Now, enable display */
	return rm67162_cmd_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

#ifdef CONFIG_PM_DEVICE
static int rm67162_dsi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};

	mdev.data_lanes = config->bus.dsi.num_of_lanes;
	mdev.pixfmt = rm67162_dsi_pixfmt(data->pixel_format);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Detach from the MIPI DSI controller */
		return mipi_dsi_detach(config->bus.dsi.mipi_dsi, config->bus.dsi.channel, &mdev);
	case PM_DEVICE_ACTION_RESUME:
		return mipi_dsi_attach(config->bus.dsi.mipi_dsi, config->bus.dsi.channel, &mdev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static const struct rm67162_bus rm67162_dsi_bus = {
	.cmd_write = rm67162_dsi_cmd_write,
	.write_memory = rm67162_dsi_write_memory,
	.blanking_on = rm67162_dsi_blanking_on,
	.blanking_off = rm67162_dsi_blanking_off,
	.supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888,
	.rgb888_address_mode = 0U,
	/* Boards with MIPI DSI panels map their touch input to a panel rotated by 90 degrees */
	.native_orientation = DISPLAY_ORIENTATION_ROTATED_90,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dbi)

static int rm67162_dbi_cmd_write(const struct device *dev, uint8_t cmd, const uint8_t *tx_buf,
				 size_t tx_len)
{
	const struct rm67162_config *config = dev->config;

	return mipi_dbi_command_write(config->bus.dbi.mipi_dbi, &config->bus.dbi.dbi_config, cmd,
				      tx_buf, tx_len);
}

static int rm67162_dbi_write_memory(const struct device *dev,
				    const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;
	struct display_buffer_descriptor mipi_desc = *desc;
	const uint8_t bytes_per_pixel = DISPLAY_BITS_PER_PIXEL(data->pixel_format) / 8U;
	const uint8_t *src = buf;
	uint16_t nbr_of_writes;
	int ret;

	/* MIPI DBI API requires the pitch to match the width */
	if (desc->pitch > desc->width) {
		mipi_desc.height = 1U;
		nbr_of_writes = desc->height;
	} else {
		nbr_of_writes = 1U;
	}
	mipi_desc.pitch = desc->width;
	mipi_desc.buf_size = desc->width * mipi_desc.height * bytes_per_pixel;

	ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_WRITE_MEMORY_START, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	for (uint16_t i = 0U; i < nbr_of_writes; i++) {
		mipi_desc.frame_incomplete = (i < (nbr_of_writes - 1U)) || desc->frame_incomplete;
		ret = mipi_dbi_write_display(config->bus.dbi.mipi_dbi, &config->bus.dbi.dbi_config,
					     src, &mipi_desc, data->pixel_format);
		if (ret < 0) {
			return ret;
		}
		src += desc->pitch * bytes_per_pixel;
	}

	return 0;
}

/*
 * The controller only outputs the tearing effect signal while the display is
 * on, so the MIPI DBI controller must only wait for it while unblanked.
 */
static int rm67162_dbi_blanking_on(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	int ret;

	if (config->bus.dbi.te_mode != MIPI_DBI_TE_NO_EDGE) {
		ret = mipi_dbi_configure_te(config->bus.dbi.mipi_dbi, MIPI_DBI_TE_NO_EDGE, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return rm67162_dbi_cmd_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int rm67162_dbi_blanking_off(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	int ret;

	ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	if (config->bus.dbi.te_mode != MIPI_DBI_TE_NO_EDGE) {
		ret = mipi_dbi_configure_te(config->bus.dbi.mipi_dbi, config->bus.dbi.te_mode,
					    config->bus.dbi.te_delay);
		if (ret < 0) {
			LOG_ERR("Could not configure TE (%d)", ret);
			return ret;
		}
	}

	return 0;
}

static int rm67162_dbi_reset(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	int ret;

	ret = mipi_dbi_reset(config->bus.dbi.mipi_dbi, RM67162_DBI_RESET_PULSE_MS);
	if ((ret == -ENOTSUP) || (ret == -ENOSYS)) {
		/* No hardware reset, fall back to software reset */
		ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_SOFT_RESET, NULL, 0);
	}
	if (ret < 0) {
		return ret;
	}

	k_msleep(RM67162_DBI_WAKEUP_MS);

	return 0;
}

static int rm67162_dbi_init(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	uint8_t param;
	int ret;

	if (!device_is_ready(config->bus.dbi.mipi_dbi)) {
		LOG_ERR("MIPI DBI device not ready");
		return -ENODEV;
	}

	rm67162_init_state(dev);

	ret = rm67162_dbi_reset(dev);
	if (ret < 0) {
		LOG_ERR("Could not reset display (%d)", ret);
		return ret;
	}

	ret = rm67162_write_init_seq(dev);
	if (ret < 0) {
		return ret;
	}

	/* Frame memory can only be written over SPI with SPI_WRAM set */
	param = RM67162_DSPI_MODE_SPI_WRAM;
	ret = rm67162_dbi_cmd_write(dev, RM67162_CMD_SET_DSPI_MODE, &param, 1);
	if (ret < 0) {
		return ret;
	}

	/* Set pixel format and address mode */
	ret = rm67162_set_pixel_format(dev, config->pixel_format);
	if (ret < 0) {
		return ret;
	}

	if (config->bus.dbi.te_mode != MIPI_DBI_TE_NO_EDGE) {
		/* Output the tearing effect signal at V-blank only */
		param = 0U;
		ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_SET_TEAR_ON, &param, 1);
		if (ret < 0) {
			return ret;
		}
	}

	/* Enable brightness control and start at full brightness */
	param = RM67162_WRCTRLD_BCTRL;
	ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_WRITE_CONTROL_DISPLAY, &param, 1);
	if (ret < 0) {
		return ret;
	}

	ret = rm67162_set_brightness(dev, UINT8_MAX);
	if (ret < 0) {
		return ret;
	}

	ret = rm67162_dbi_cmd_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(RM67162_DBI_WAKEUP_MS);

	/* Display stays blanked until blanking_off is called */
	return 0;
}

static const struct rm67162_bus rm67162_dbi_bus = {
	.cmd_write = rm67162_dbi_cmd_write,
	.write_memory = rm67162_dbi_write_memory,
	.blanking_on = rm67162_dbi_blanking_on,
	.blanking_off = rm67162_dbi_blanking_off,
	/* The SPI interfaces receive 16-bit pixels most significant byte first */
	.supported_pixel_formats = PIXEL_FORMAT_RGB_565X | PIXEL_FORMAT_RGB_888,
	/*
	 * RGB_888 pixels are stored blue first and sent in memory order, while
	 * the controller expects red first.
	 */
	.rgb888_address_mode = MIPI_DCS_ADDRESS_MODE_BGR,
	.native_orientation = DISPLAY_ORIENTATION_NORMAL,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dbi) */

static DEVICE_API(display, rm67162_api) = {
	.blanking_on = rm67162_blanking_on,
	.blanking_off = rm67162_blanking_off,
	.write = rm67162_write,
	.set_brightness = rm67162_set_brightness,
	.get_capabilities = rm67162_get_capabilities,
	.set_pixel_format = rm67162_set_pixel_format,
	.set_orientation = rm67162_set_orientation,
};

#define RM67162_DSI_PIXEL_FORMAT(id)                                                               \
	((DT_INST_PROP(id, pixel_format) == MIPI_DSI_PIXFMT_RGB888) ? PIXEL_FORMAT_RGB_888         \
								    : PIXEL_FORMAT_RGB_565)

#define RM67162_DSI_TIMING(id, prop) DT_PROP_OR(DT_INST_CHILD(id, display_timings), prop, 1)

#define RM67162_DSI_PANEL(id)                                                                      \
	BUILD_ASSERT((DT_INST_PROP(id, pixel_format) == MIPI_DSI_PIXFMT_RGB888) ||                 \
			     (DT_INST_PROP(id, pixel_format) == MIPI_DSI_PIXFMT_RGB565),           \
		     "RM67162 driver only supports RGB888 and RGB565 on MIPI DSI");                \
	static const struct rm67162_config rm67162_config_##id = {                                 \
		.ops = &rm67162_dsi_bus,                                                           \
		.init_seq = &rm67162_init_seqs[DT_INST_ENUM_IDX(id, init_sequence)],               \
		.panel_width = DT_INST_PROP(id, width),                                            \
		.panel_height = DT_INST_PROP(id, height),                                          \
		.pixel_format = RM67162_DSI_PIXEL_FORMAT(id),                                      \
		.rotation = DT_INST_ENUM_IDX(id, rotation),                                        \
		.bus.dsi.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),                                \
		.bus.dsi.channel = DT_INST_REG_ADDR(id),                                           \
		.bus.dsi.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                    \
		.bus.dsi.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),              \
		.bus.dsi.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),                    \
		.bus.dsi.te_gpio = GPIO_DT_SPEC_INST_GET_OR(id, te_gpios, {0}),                    \
		.bus.dsi.hsync = RM67162_DSI_TIMING(id, hsync_len),                                \
		.bus.dsi.hbp = RM67162_DSI_TIMING(id, hback_porch),                                \
		.bus.dsi.hfp = RM67162_DSI_TIMING(id, hfront_porch),                               \
		.bus.dsi.vfp = RM67162_DSI_TIMING(id, vfront_porch),                               \
		.bus.dsi.vbp = RM67162_DSI_TIMING(id, vback_porch),                                \
		.bus.dsi.vsync = RM67162_DSI_TIMING(id, vsync_len),                                \
	};                                                                                         \
	static struct rm67162_data rm67162_data_##id;                                              \
	PM_DEVICE_DT_INST_DEFINE(id, rm67162_dsi_pm_action);                                       \
	DEVICE_DT_INST_DEFINE(id, &rm67162_dsi_init, PM_DEVICE_DT_INST_GET(id),                    \
			      &rm67162_data_##id, &rm67162_config_##id, POST_KERNEL,               \
			      CONFIG_APPLICATION_INIT_PRIORITY, &rm67162_api);

/* 3-wire SPI sends the D/C bit with every byte as a 9-bit word */
#define RM67162_DBI_WORD_SIZE(id)                                                                  \
	((DT_INST_STRING_UPPER_TOKEN(id, mipi_mode) == MIPI_DBI_MODE_SPI_3WIRE) ? SPI_WORD_SET(9)  \
										: SPI_WORD_SET(8))

#define RM67162_DBI_PANEL(id)                                                                      \
	BUILD_ASSERT(                                                                              \
		(DT_INST_STRING_UPPER_TOKEN(id, mipi_mode) == MIPI_DBI_MODE_SPI_3WIRE) ||          \
			(DT_INST_STRING_UPPER_TOKEN(id, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE),    \
		"RM67162 driver only supports 3-wire and 4-wire SPI on MIPI DBI");                 \
	static const struct rm67162_config rm67162_config_##id = {                                 \
		.ops = &rm67162_dbi_bus,                                                           \
		.init_seq = &rm67162_init_seqs[DT_INST_ENUM_IDX(id, init_sequence)],               \
		.panel_width = DT_INST_PROP(id, width),                                            \
		.panel_height = DT_INST_PROP(id, height),                                          \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
		.rotation = DT_INST_ENUM_IDX(id, rotation),                                        \
		.bus.dbi.mipi_dbi = DEVICE_DT_GET(DT_INST_PARENT(id)),                             \
		.bus.dbi.dbi_config = MIPI_DBI_CONFIG_DT_INST(                                     \
			id, SPI_OP_MODE_CONTROLLER | RM67162_DBI_WORD_SIZE(id), 0),                \
		.bus.dbi.te_mode = MIPI_DBI_TE_MODE_DT_INST(id, te_mode),                          \
		.bus.dbi.te_delay = DT_INST_PROP(id, te_delay),                                    \
	};                                                                                         \
	static struct rm67162_data rm67162_data_##id;                                              \
	DEVICE_DT_INST_DEFINE(id, &rm67162_dbi_init, NULL, &rm67162_data_##id,                     \
			      &rm67162_config_##id, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
			      &rm67162_api);

#define RM67162_DEVICE_INIT(id)                                                                    \
	COND_CODE_1(DT_INST_ON_BUS(id, mipi_dsi), (RM67162_DSI_PANEL(id)), (RM67162_DBI_PANEL(id)))

DT_INST_FOREACH_STATUS_OKAY(RM67162_DEVICE_INIT)
