/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 *
 * Copyright (c) 2024 VCI Development
 * SPDX-License-Identifier: Apache-2.0
 *
 * LCD Controller (LCDC) driver for NXP LPC54S018
 * Supports parallel RGB interfaces with ST7701S controller
 */

#define DT_DRV_COMPAT nxp_lpc_lcdc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <fsl_clock.h>
#include <fsl_iocon.h>

LOG_MODULE_REGISTER(display_lpc_lcdc, CONFIG_DISPLAY_LOG_LEVEL);

/* LCDC Register offsets */
#define LCDC_TIMH              0x00  /* Horizontal timing register */
#define LCDC_TIMV              0x04  /* Vertical timing register */
#define LCDC_POL               0x08  /* Clock and signal polarity register */
#define LCDC_LE                0x0C  /* Line end control register */
#define LCDC_UPBASE            0x10  /* Upper panel base address register */
#define LCDC_LPBASE            0x14  /* Lower panel base address register */
#define LCDC_CTRL              0x18  /* Control register */
#define LCDC_INTMSK            0x1C  /* Interrupt mask register */
#define LCDC_INTRAW            0x20  /* Raw interrupt status register */
#define LCDC_INTSTAT           0x24  /* Masked interrupt status register */
#define LCDC_INTCLR            0x28  /* Interrupt clear register */
#define LCDC_UPCURR            0x2C  /* Upper panel current address register */
#define LCDC_LPCURR            0x30  /* Lower panel current address register */
#define LCDC_PAL               0x200 /* Color palette registers */

/* LCDC_CTRL bits */
#define LCDC_CTRL_LCDEN        BIT(0)   /* LCD enable */
#define LCDC_CTRL_LCDBPP(x)    ((x) << 1) /* Bits per pixel */
#define LCDC_CTRL_LCDBW        BIT(4)   /* Black and white */
#define LCDC_CTRL_LCDTFT       BIT(5)   /* TFT panel */
#define LCDC_CTRL_LCDMONO8     BIT(6)   /* Monochrome 8-bit */
#define LCDC_CTRL_LCDDUAL      BIT(7)   /* Dual panel */
#define LCDC_CTRL_BGR          BIT(8)   /* Blue/red swap */
#define LCDC_CTRL_BEBO         BIT(9)   /* Big-endian byte order */
#define LCDC_CTRL_BEPO         BIT(10)  /* Big-endian pixel order */
#define LCDC_CTRL_LCDPWR       BIT(11)  /* LCD power enable */
#define LCDC_CTRL_LCDVCOMP(x)  ((x) << 12) /* Vertical compare */

/* LCDC_POL bits */
#define LCDC_POL_CLKSEL        BIT(5)   /* Clock select */
#define LCDC_POL_ACB(x)        ((x) << 6)  /* AC bias frequency */
#define LCDC_POL_IVS           BIT(11)  /* Invert vertical sync */
#define LCDC_POL_IHS           BIT(12)  /* Invert horizontal sync */
#define LCDC_POL_IPC           BIT(13)  /* Invert pixel clock */
#define LCDC_POL_IOE           BIT(14)  /* Invert output enable */
#define LCDC_POL_CPL(x)        ((x) << 16) /* Clocks per line */
#define LCDC_POL_BCD           BIT(26)  /* Bypass clock divider */
#define LCDC_POL_CPL_MASK      (0x3FF << 16)

/* Bits per pixel values */
#define LCDC_BPP_1             0
#define LCDC_BPP_2             1
#define LCDC_BPP_4             2
#define LCDC_BPP_8             3
#define LCDC_BPP_16            4
#define LCDC_BPP_24            5
#define LCDC_BPP_16_565        6
#define LCDC_BPP_12_444        7

/* Interrupt bits */
#define LCDC_INT_FUF           BIT(1)   /* FIFO underflow */
#define LCDC_INT_LNBU          BIT(2)   /* LCD next base update */
#define LCDC_INT_VCOMP         BIT(3)   /* Vertical compare */
#define LCDC_INT_BER           BIT(4)   /* AHB master error */

struct display_lpc_lcdc_config {
	uint32_t base;
	uint32_t clock_dev;
	uint32_t clock_name;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
	const struct gpio_dt_spec power_gpio;
	const struct gpio_dt_spec reset_gpio;
	/* ST7701S bit-bang SPI pins */
	bool st7701s_spi_init;
	const struct gpio_dt_spec cs_gpio;
	const struct gpio_dt_spec sck_gpio;
	const struct gpio_dt_spec mosi_gpio;
	/* Display parameters */
	uint16_t width;
	uint16_t height;
	uint8_t bits_per_pixel;
	bool is_tft;
	bool swap_red_blue;
	/* Panel timing from device tree */
	uint32_t clock_frequency;
	uint16_t hsync_len;
	uint16_t hfront_porch;
	uint16_t hback_porch;
	uint16_t vsync_len;
	uint16_t vfront_porch;
	uint16_t vback_porch;
	uint8_t hsync_active;
	uint8_t vsync_active;
	uint8_t de_active;
	uint8_t pixelclk_active;
};

struct display_lpc_lcdc_data {
	enum display_pixel_format pixel_format;
	uint8_t *framebuffer;
	size_t fb_size;
	bool display_on;
};

/* ST7701S initialization commands - based on baremetal implementation */
static const uint8_t st7701s_init_sequence[][18] = {
	/* Command2 BK3 */
	{0xFF, 0x77, 0x01, 0x00, 0x00, 0x13},
	{0xEF, 0x08},
	
	/* Command2 BK0 */
	{0xFF, 0x77, 0x01, 0x00, 0x00, 0x10},
	{0xC0, 0x3B, 0x00},  /* Display line setting */
	{0xC1, 0x0D, 0x02},  /* Porch control */
	{0xC2, 0x21, 0x08},  /* Inversion selection */
	{0xCD, 0x08},
	
	/* Positive gamma control */
	{0xB0, 0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08,
	 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18},
	
	/* Negative gamma control */
	{0xB1, 0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08,
	 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18},
	
	/* Command2 BK1 */
	{0xFF, 0x77, 0x01, 0x00, 0x00, 0x11},
	{0xB0, 0x60},  /* Vop amplitude */
	{0xB1, 0x32},  /* VCOM amplitude */
	{0xB2, 0x07},  /* VGH voltage */
	{0xB3, 0x80},
	{0xB5, 0x49},  /* VGL voltage */
	{0xB7, 0x85},  /* Power control 1 */
	{0xB8, 0x21},  /* Power control 2 */
	{0xC1, 0x78},
	{0xC2, 0x78},
	{0xD0, 0x88},
	
	/* GIP sequence */
	{0xE0, 0x00, 0x00, 0x02},
	{0xE1, 0x01, 0xA0, 0x03, 0xA0, 0x02, 0xA0, 0x04, 0xA0,
	 0x00, 0x44, 0x44},
	{0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00},
	{0xE3, 0x00, 0x00, 0x33, 0x33},
	{0xE4, 0x44, 0x44},
	{0xE5, 0x01, 0x26, 0xA0, 0xA0, 0x03, 0x28, 0xA0, 0xA0,
	 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0},
	{0xE6, 0x00, 0x00, 0x33, 0x33},
	{0xE7, 0x44, 0x44},
	{0xE8, 0x02, 0x26, 0xA0, 0xA0, 0x04, 0x28, 0xA0, 0xA0,
	 0x06, 0x2A, 0xA0, 0xA0, 0x08, 0x2C, 0xA0, 0xA0},
	{0xEB, 0x00, 0x00, 0xE4, 0xE4, 0x44, 0x00},
	{0xEC, 0x00, 0x00},
	{0xED, 0xF3, 0xB1, 0x7F, 0x0F, 0xCF, 0x9F, 0xF7, 0xF2,
	 0x2F, 0xF7, 0xF9, 0xFC, 0xF0, 0xF7, 0x1B, 0x3F},
	
	/* Bank selection disable */
	{0xFF, 0x77, 0x01, 0x00, 0x00, 0x00},
	
	/* End marker */
	{0x00}
};

static void st7701s_write_byte(const struct display_lpc_lcdc_config *config, uint8_t data)
{
	int i;
	
	for (i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&config->sck_gpio, 0);
		gpio_pin_set_dt(&config->mosi_gpio, (data >> i) & 1);
		k_busy_wait(1);
		gpio_pin_set_dt(&config->sck_gpio, 1);
		k_busy_wait(1);
	}
}

static void st7701s_write_cmd(const struct display_lpc_lcdc_config *config, 
			      const uint8_t *cmd, size_t len)
{
	size_t i;
	
	if (len == 0) {
		return;
	}
	
	gpio_pin_set_dt(&config->cs_gpio, 0);
	
	/* Send command byte (D/C = 0) */
	st7701s_write_byte(config, 0x00);  /* Command mode */
	st7701s_write_byte(config, cmd[0]);
	
	/* Send parameters (D/C = 1) */
	for (i = 1; i < len; i++) {
		st7701s_write_byte(config, 0x01);  /* Data mode */
		st7701s_write_byte(config, cmd[i]);
	}
	
	gpio_pin_set_dt(&config->cs_gpio, 1);
	k_busy_wait(10);
}

static int st7701s_init(const struct display_lpc_lcdc_config *config)
{
	int i;
	
	LOG_DBG("Initializing ST7701S controller");
	
	/* Initialize SPI GPIO pins */
	gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&config->sck_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&config->mosi_gpio, GPIO_OUTPUT_INACTIVE);
	
	/* Reset sequence */
	if (config->reset_gpio.port) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 0);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_msleep(120);
	}
	
	/* Send initialization sequence */
	for (i = 0; st7701s_init_sequence[i][0] != 0x00; i++) {
		const uint8_t *cmd = st7701s_init_sequence[i];
		size_t len = 2;  /* Default for most commands */
		
		/* Determine command length */
		if (cmd[0] == 0xFF) {
			len = 6;  /* Bank selection */
		} else if (cmd[0] == 0xB0 || cmd[0] == 0xB1) {
			len = 17;  /* Gamma control */
		} else if (cmd[0] == 0xE1 || cmd[0] == 0xE5 || cmd[0] == 0xE8) {
			len = 17;  /* GIP sequences */
		} else if (cmd[0] == 0xED) {
			len = 17;  /* GIP ED */
		} else if (cmd[0] == 0xE2) {
			len = 13;  /* GIP E2 */
		}
		
		st7701s_write_cmd(config, cmd, len);
	}
	
	/* Exit sleep mode */
	uint8_t exit_sleep[] = {0x11};
	st7701s_write_cmd(config, exit_sleep, 1);
	k_msleep(120);
	
	/* Display on */
	uint8_t display_on[] = {0x29};
	st7701s_write_cmd(config, display_on, 1);
	k_msleep(20);
	
	LOG_DBG("ST7701S initialization complete");
	
	return 0;
}

static inline void lcdc_write(uint32_t base, uint32_t reg, uint32_t val)
{
	sys_write32(val, base + reg);
}

static inline uint32_t lcdc_read(uint32_t base, uint32_t reg)
{
	return sys_read32(base + reg);
}

static int display_lpc_lcdc_init(const struct device *dev)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	uint32_t reg_val;
	int ret;
	
	LOG_DBG("Initializing LPC LCDC");
	
	/* Configure pins */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}
	
	/* Enable LCD clock */
	CLOCK_EnableClock(kCLOCK_Lcd);
	
	/* Configure backlight GPIO if available */
	if (config->backlight_gpio.port) {
		ret = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure backlight GPIO");
			return ret;
		}
	}
	
	/* Configure power GPIO if available */
	if (config->power_gpio.port) {
		ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure power GPIO");
			return ret;
		}
	}
	
	/* Configure reset GPIO if available */
	if (config->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO");
			return ret;
		}
	}
	
	/* Initialize ST7701S if enabled */
	if (config->st7701s_spi_init) {
		ret = st7701s_init(config);
		if (ret < 0) {
			LOG_ERR("Failed to initialize ST7701S");
			return ret;
		}
	}
	
	/* Set pixel format based on bits per pixel */
	switch (config->bits_per_pixel) {
	case 16:
		data->pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	case 24:
		data->pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	default:
		LOG_ERR("Unsupported bits per pixel: %d", config->bits_per_pixel);
		return -EINVAL;
	}
	
	/* Allocate framebuffer */
	data->fb_size = config->width * config->height * (config->bits_per_pixel / 8);
	data->framebuffer = k_malloc(data->fb_size);
	if (!data->framebuffer) {
		LOG_ERR("Failed to allocate framebuffer");
		return -ENOMEM;
	}
	
	/* Clear framebuffer */
	memset(data->framebuffer, 0, data->fb_size);
	
	/* Disable LCD controller during configuration */
	lcdc_write(config->base, LCDC_CTRL, 0);
	
	/* Configure horizontal timing */
	reg_val = ((config->hback_porch - 1) << 24) |
		  ((config->hfront_porch - 1) << 16) |
		  ((config->hsync_len - 1) << 8) |
		  ((config->width / 16 - 1) << 2);
	lcdc_write(config->base, LCDC_TIMH, reg_val);
	
	/* Configure vertical timing */
	reg_val = (config->vback_porch << 24) |
		  (config->vfront_porch << 16) |
		  ((config->vsync_len - 1) << 10) |
		  (config->height - 1);
	lcdc_write(config->base, LCDC_TIMV, reg_val);
	
	/* Configure signal polarities and pixel clock */
	reg_val = LCDC_POL_BCD |  /* Bypass clock divider */
		  LCDC_POL_CPL(config->width - 1);  /* Clocks per line */
	
	if (!config->hsync_active) {
		reg_val |= LCDC_POL_IHS;  /* Invert if active low */
	}
	if (!config->vsync_active) {
		reg_val |= LCDC_POL_IVS;  /* Invert if active low */
	}
	if (config->pixelclk_active) {
		reg_val |= LCDC_POL_IPC;  /* Invert if data on rising edge */
	}
	if (!config->de_active) {
		reg_val |= LCDC_POL_IOE;  /* Invert if active low */
	}
	
	lcdc_write(config->base, LCDC_POL, reg_val);
	
	/* Set framebuffer address */
	lcdc_write(config->base, LCDC_UPBASE, (uint32_t)data->framebuffer);
	
	/* Configure control register */
	reg_val = LCDC_CTRL_LCDPWR;  /* LCD power enable */
	
	if (config->is_tft) {
		reg_val |= LCDC_CTRL_LCDTFT;
	}
	
	if (config->swap_red_blue) {
		reg_val |= LCDC_CTRL_BGR;
	}
	
	/* Set bits per pixel */
	switch (config->bits_per_pixel) {
	case 16:
		reg_val |= LCDC_CTRL_LCDBPP(LCDC_BPP_16_565);
		break;
	case 24:
		reg_val |= LCDC_CTRL_LCDBPP(LCDC_BPP_24);
		break;
	}
	
	/* Enable LCD controller */
	reg_val |= LCDC_CTRL_LCDEN;
	lcdc_write(config->base, LCDC_CTRL, reg_val);
	
	/* Clear any pending interrupts */
	lcdc_write(config->base, LCDC_INTCLR, 0x0F);
	
	/* Enable backlight */
	if (config->backlight_gpio.port) {
		gpio_pin_set_dt(&config->backlight_gpio, 1);
	}
	
	data->display_on = true;
	
	LOG_INF("LPC LCDC initialized: %dx%d @ %d bpp", 
		config->width, config->height, config->bits_per_pixel);
	
	return 0;
}

static int display_lpc_lcdc_write(const struct device *dev, const uint16_t x, const uint16_t y,
				  const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	uint8_t *dst;
	const uint8_t *src;
	uint16_t row;
	size_t pixel_size = config->bits_per_pixel / 8;
	
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT(buf != NULL, "Buffer pointer is NULL");
	__ASSERT(x + desc->width <= config->width, "Writing outside display width");
	__ASSERT(y + desc->height <= config->height, "Writing outside display height");
	
	if (!data->display_on) {
		return -EPERM;
	}
	
	dst = data->framebuffer + (y * config->width + x) * pixel_size;
	src = buf;
	
	for (row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * pixel_size);
		dst += config->width * pixel_size;
		src += desc->pitch * pixel_size;
	}
	
	return 0;
}

static int display_lpc_lcdc_read(const struct device *dev, const uint16_t x, const uint16_t y,
				 const struct display_buffer_descriptor *desc, void *buf)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	uint8_t *src;
	uint8_t *dst;
	uint16_t row;
	size_t pixel_size = config->bits_per_pixel / 8;
	
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT(buf != NULL, "Buffer pointer is NULL");
	__ASSERT(x + desc->width <= config->width, "Reading outside display width");
	__ASSERT(y + desc->height <= config->height, "Reading outside display height");
	
	if (!data->display_on) {
		return -EPERM;
	}
	
	src = data->framebuffer + (y * config->width + x) * pixel_size;
	dst = buf;
	
	for (row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * pixel_size);
		src += config->width * pixel_size;
		dst += desc->pitch * pixel_size;
	}
	
	return 0;
}

static void *display_lpc_lcdc_get_framebuffer(const struct device *dev)
{
	struct display_lpc_lcdc_data *data = dev->data;
	
	return data->framebuffer;
}

static int display_lpc_lcdc_blanking_on(const struct device *dev)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	
	if (config->backlight_gpio.port) {
		gpio_pin_set_dt(&config->backlight_gpio, 0);
	}
	
	data->display_on = false;
	
	return 0;
}

static int display_lpc_lcdc_blanking_off(const struct device *dev)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	
	if (config->backlight_gpio.port) {
		gpio_pin_set_dt(&config->backlight_gpio, 1);
	}
	
	data->display_on = true;
	
	return 0;
}

static int display_lpc_lcdc_set_brightness(const struct device *dev, const uint8_t brightness)
{
	/* Brightness control would need PWM support */
	LOG_WRN("Brightness control not implemented");
	return -ENOTSUP;
}

static int display_lpc_lcdc_set_contrast(const struct device *dev, const uint8_t contrast)
{
	LOG_WRN("Contrast control not supported");
	return -ENOTSUP;
}

static void display_lpc_lcdc_get_capabilities(const struct device *dev,
					      struct display_capabilities *caps)
{
	const struct display_lpc_lcdc_config *config = dev->config;
	struct display_lpc_lcdc_data *data = dev->data;
	
	memset(caps, 0, sizeof(struct display_capabilities));
	
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = (1 << data->pixel_format);
	caps->current_pixel_format = data->pixel_format;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int display_lpc_lcdc_set_pixel_format(const struct device *dev,
					      const enum display_pixel_format pixel_format)
{
	struct display_lpc_lcdc_data *data = dev->data;
	
	if (pixel_format != data->pixel_format) {
		LOG_ERR("Pixel format change not supported");
		return -ENOTSUP;
	}
	
	return 0;
}

static int display_lpc_lcdc_set_orientation(const struct device *dev,
					     const enum display_orientation orientation)
{
	if (orientation != DISPLAY_ORIENTATION_NORMAL) {
		LOG_ERR("Orientation change not supported");
		return -ENOTSUP;
	}
	
	return 0;
}

static const struct display_driver_api display_lpc_lcdc_api = {
	.blanking_on = display_lpc_lcdc_blanking_on,
	.blanking_off = display_lpc_lcdc_blanking_off,
	.write = display_lpc_lcdc_write,
	.read = display_lpc_lcdc_read,
	.get_framebuffer = display_lpc_lcdc_get_framebuffer,
	.set_brightness = display_lpc_lcdc_set_brightness,
	.set_contrast = display_lpc_lcdc_set_contrast,
	.get_capabilities = display_lpc_lcdc_get_capabilities,
	.set_pixel_format = display_lpc_lcdc_set_pixel_format,
	.set_orientation = display_lpc_lcdc_set_orientation,
};

#define DISPLAY_LPC_LCDC_DEVICE(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct display_lpc_lcdc_config display_lpc_lcdc_config_##inst = {	\
		.base = DT_INST_REG_ADDR(inst),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, backlight_gpios, {0}),\
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}),	\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		.st7701s_spi_init = DT_INST_PROP(inst, st7701s_spi_init),		\
		.cs_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, cs_gpios, {0}),		\
		.sck_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sck_gpios, {0}),		\
		.mosi_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mosi_gpios, {0}),		\
		.width = DT_INST_PROP(inst, width),					\
		.height = DT_INST_PROP(inst, height),					\
		.bits_per_pixel = DT_INST_PROP(inst, bits_per_pixel),			\
		.is_tft = (strcmp(DT_INST_PROP(inst, display_type), "TFT") == 0),	\
		.swap_red_blue = DT_INST_PROP(inst, swap_red_blue),			\
		.clock_frequency = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),	\
					   clock_frequency),				\
		.hsync_len = DT_PROP(DT_INST_PHANDLE(inst, panel_timings), hsync_len),	\
		.hfront_porch = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
					hfront_porch),					\
		.hback_porch = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
				       hback_porch),					\
		.vsync_len = DT_PROP(DT_INST_PHANDLE(inst, panel_timings), vsync_len),	\
		.vfront_porch = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
					vfront_porch),					\
		.vback_porch = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
				       vback_porch),					\
		.hsync_active = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
					hsync_active),					\
		.vsync_active = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),		\
					vsync_active),					\
		.de_active = DT_PROP(DT_INST_PHANDLE(inst, panel_timings), de_active),	\
		.pixelclk_active = DT_PROP(DT_INST_PHANDLE(inst, panel_timings),	\
					   pixelclk_active),				\
	};										\
											\
	static struct display_lpc_lcdc_data display_lpc_lcdc_data_##inst;		\
											\
	DEVICE_DT_INST_DEFINE(inst, display_lpc_lcdc_init, NULL,			\
			      &display_lpc_lcdc_data_##inst,				\
			      &display_lpc_lcdc_config_##inst,				\
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,		\
			      &display_lpc_lcdc_api);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_LPC_LCDC_DEVICE)