/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_lcdc

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <soc.h>
#include <fsl_lcdc.h>

LOG_MODULE_REGISTER(display_mcux_lcdc, CONFIG_DISPLAY_LOG_LEVEL);

/*
 * The driver owns two framebuffers in the devicetree memory-region and scans
 * out one while writes go to the other. A write that completes a frame
 * queues the back buffer and waits for the controller to latch it at the
 * next vertical blank before the buffers swap roles.
 */
struct mcux_lcdc_config {
	LCD_Type *base;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	struct gpio_dt_spec backlight_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	struct pwm_dt_spec backlight_pwm;
#endif
	lcdc_config_t lcdc_config;
	uint8_t *fb[2];
	size_t fb_size;
	uint16_t panel_width;
	uint16_t panel_height;
	enum display_pixel_format pixel_format;
	uint8_t pixel_bytes;
};

struct mcux_lcdc_data {
	struct k_mutex lock;
	struct k_sem sem;
	volatile bool frame_pending;
	uint8_t back_idx;
	bool back_dirty;
	bool blanked;
	uint8_t brightness;
};

static int mcux_lcdc_backlight_apply(const struct device *dev)
{
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	/* A PWM backlight takes precedence over the backlight GPIO. */
	if (config->backlight_pwm.dev != NULL) {
		uint32_t pulse = 0U;

		if (!data->blanked) {
			/* Scale the 0-255 brightness linearly onto the PWM period. */
			pulse = (uint32_t)(((uint64_t)config->backlight_pwm.period *
					    data->brightness) /
					   255U);
		}

		return pwm_set_pulse_dt(&config->backlight_pwm, pulse);
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (config->backlight_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->backlight_gpio, data->blanked ? 0 : 1);
	}
#endif

	ARG_UNUSED(config);
	ARG_UNUSED(data);

	return -ENOSYS;
}

/* Queue the back buffer and wait until the controller has latched it. */
static void mcux_lcdc_present(const struct device *dev)
{
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;
	uint8_t *back = config->fb[data->back_idx];

	/* The copy must reach memory before the controller fetches it. */
	barrier_dmem_fence_full();

	LCDC_SetPanelAddr(config->base, kLCDC_UpperPanel, (uint32_t)back);

	/*
	 * The base-address-update flag is set at every vertical blank, also
	 * while masked. Clear it after the address write, so only a latch of
	 * the new address releases the wait (at worst one frame late).
	 */
	data->frame_pending = true;
	LCDC_ClearInterruptsStatus(config->base, kLCDC_BaseAddrUpdateInterrupt);
	LCDC_EnableInterrupts(config->base, kLCDC_BaseAddrUpdateInterrupt);
	k_sem_take(&data->sem, K_FOREVER);

	data->back_idx ^= 1U;
	data->back_dirty = false;
}

static int mcux_lcdc_write(const struct device *dev, const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;
	const size_t pb = config->pixel_bytes;
	const uint8_t *src = buf;
	uint8_t *back;
	bool full_frame;

	__ASSERT((pb * desc->pitch * desc->height) <= desc->buf_size, "Input buffer too small");

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((desc->pitch < desc->width) || ((x + desc->width) > config->panel_width) ||
	    ((y + desc->height) > config->panel_height)) {
		LOG_ERR("Write outside the panel");
		return -EINVAL;
	}

	full_frame = (x == 0U) && (y == 0U) && (desc->width == config->panel_width) &&
		     (desc->height == config->panel_height);

	k_mutex_lock(&data->lock, K_FOREVER);

	back = config->fb[data->back_idx];

	/* A partial update starts from the frame that is on screen. */
	if (!full_frame && !data->back_dirty) {
		memcpy(back, config->fb[data->back_idx ^ 1U], config->fb_size);
	}

	if (full_frame && (desc->pitch == desc->width)) {
		memcpy(back, src, config->fb_size);
	} else {
		for (uint16_t row = 0U; row < desc->height; row++) {
			memcpy(back + ((((size_t)y + row) * config->panel_width) + x) * pb,
			       src + ((size_t)row * desc->pitch * pb), (size_t)desc->width * pb);
		}
	}

	data->back_dirty = true;

	if (!desc->frame_incomplete) {
		mcux_lcdc_present(dev);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_lcdc_set_blanked(const struct device *dev, bool blanked)
{
	struct mcux_lcdc_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->blanked = blanked;
	ret = mcux_lcdc_backlight_apply(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mcux_lcdc_blanking_off(const struct device *dev)
{
	return mcux_lcdc_set_blanked(dev, false);
}

static int mcux_lcdc_blanking_on(const struct device *dev)
{
	return mcux_lcdc_set_blanked(dev, true);
}

static int mcux_lcdc_set_brightness(const struct device *dev, const uint8_t brightness)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;
	int ret;

	if (config->backlight_pwm.dev == NULL) {
		return -ENOSYS;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->brightness = brightness;
	ret = mcux_lcdc_backlight_apply(dev);
	k_mutex_unlock(&data->lock);

	return ret;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(brightness);
	return -ENOSYS;
#endif
}

static int mcux_lcdc_set_pixel_format(const struct device *dev,
				      const enum display_pixel_format pixel_format)
{
	const struct mcux_lcdc_config *config = dev->config;

	/* The format is fixed by the devicetree; only accept a no-op. */
	if (pixel_format != config->pixel_format) {
		LOG_ERR("Pixel format is fixed to the devicetree value");
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_lcdc_set_orientation(const struct device *dev,
				     const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void mcux_lcdc_get_capabilities(const struct device *dev,
				       struct display_capabilities *capabilities)
{
	const struct mcux_lcdc_config *config = dev->config;

	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void mcux_lcdc_isr(const struct device *dev)
{
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;
	uint32_t status;

	status = LCDC_GetInterruptsPendingStatus(config->base);
	LCDC_ClearInterruptsStatus(config->base, status);

	if (data->frame_pending && ((status & kLCDC_BaseAddrUpdateInterrupt) != 0U)) {
		LCDC_DisableInterrupts(config->base, kLCDC_BaseAddrUpdateInterrupt);
		data->frame_pending = false;
		k_sem_give(&data->sem);
	}
}

static int mcux_lcdc_backlight_init(const struct mcux_lcdc_config *config)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	if (config->backlight_pwm.dev != NULL) {
		if (!pwm_is_ready_dt(&config->backlight_pwm)) {
			LOG_ERR("Backlight PWM not ready");
			return -ENODEV;
		}

		/* The display starts blanked. */
		return pwm_set_pulse_dt(&config->backlight_pwm, 0U);
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (config->backlight_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->backlight_gpio)) {
			LOG_ERR("Backlight GPIO not ready");
			return -ENODEV;
		}

		return gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_INACTIVE);
	}
#endif

	return 0;
}

static int mcux_lcdc_init(const struct device *dev)
{
	const struct mcux_lcdc_config *config = dev->config;
	struct mcux_lcdc_data *data = dev->data;
	uint32_t src_clock_hz;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->sem, 0, 1);
	data->blanked = true;
	data->brightness = UINT8_MAX;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = mcux_lcdc_backlight_init(config);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &src_clock_hz);
	if (ret != 0) {
		return ret;
	}

	/* The framebuffer region is not zeroed at boot; start from a black frame. */
	memset(config->fb[0], 0, config->fb_size);
	memset(config->fb[1], 0, config->fb_size);

	config->irq_config_func(dev);

	if (LCDC_Init(config->base, &config->lcdc_config, src_clock_hz) != kStatus_Success) {
		LOG_ERR("LCDC init failed");
		return -EIO;
	}

	/* Scan out the black frame in fb[1]; writes go to fb[0] first. */
	data->back_idx = 0U;
	barrier_dmem_fence_full();
	LCDC_SetPanelAddr(config->base, kLCDC_UpperPanel, (uint32_t)config->fb[1]);
	LCDC_Start(config->base);
	LCDC_PowerUp(config->base);

	LOG_DBG("LCDC up: %ux%u, src clock %u Hz", config->panel_width, config->panel_height,
		src_clock_hz);

	return 0;
}

static DEVICE_API(display, mcux_lcdc_api) = {
	.blanking_on = mcux_lcdc_blanking_on,
	.blanking_off = mcux_lcdc_blanking_off,
	.set_brightness = mcux_lcdc_set_brightness,
	.write = mcux_lcdc_write,
	.get_capabilities = mcux_lcdc_get_capabilities,
	.set_pixel_format = mcux_lcdc_set_pixel_format,
	.set_orientation = mcux_lcdc_set_orientation,
};

#define LCDC_TIMING(id, cell) DT_PROP(DT_INST_CHILD(id, display_timings), cell)

#define LCDC_FORMAT(id) DT_INST_PROP(id, pixel_format)

#define LCDC_FORMAT_IS_32BPP(id)                                                                   \
	((LCDC_FORMAT(id) == PANEL_PIXEL_FORMAT_ARGB_8888) ||                                      \
	 (LCDC_FORMAT(id) == PANEL_PIXEL_FORMAT_XRGB_8888))

/* Full-screen framebuffer size in bytes for this instance. */
#define LCDC_FB_SIZE(id)                                                                           \
	(DT_INST_PROP(id, width) * DT_INST_PROP(id, height) *                                      \
	 (DISPLAY_BITS_PER_PIXEL(LCDC_FORMAT(id)) / BITS_PER_BYTE))

/* Linker section for the framebuffers, taken from the memory-region phandle. */
#define LCDC_FB_SECTION(id) LINKER_DT_NODE_REGION_NAME_TOKEN(DT_INST_PHANDLE(id, memory_region))

/* 24 bpp mode fetches one 32 bit word per pixel. */
#define LCDC_BPP(id) (LCDC_FORMAT_IS_32BPP(id) ? kLCDC_24BPP : kLCDC_16BPP565)

/*
 * LCDC polarity flags are "invert" bits: a set bit makes the signal active-low
 * (HSYNC/VSYNC/DE) or drives data on the falling clock edge. The panel-timing
 * "*-active" properties use 1 for active-high / rising edge, so invert when 0.
 */
#define LCDC_POLARITY(id)                                                                          \
	((LCDC_TIMING(id, hsync_active) ? 0 : kLCDC_InvertHsyncPolarity) |                         \
	 (LCDC_TIMING(id, vsync_active) ? 0 : kLCDC_InvertVsyncPolarity) |                         \
	 (LCDC_TIMING(id, de_active) ? 0 : kLCDC_InvertDePolarity) |                               \
	 (LCDC_TIMING(id, pixelclk_active) ? 0 : kLCDC_InvertClkPolarity))

#define LCDC_IN_RANGE(val, max) (((val) >= 1) && ((val) <= (max)))

/* Limits of the TIMH/TIMV fields, which hold each value minus one. */
#define LCDC_CHECK_TIMINGS(id)                                                                     \
	BUILD_ASSERT((LCDC_FORMAT(id) == PANEL_PIXEL_FORMAT_RGB_565) || LCDC_FORMAT_IS_32BPP(id),  \
		     "LCDC supports RGB_565, ARGB_8888 and XRGB_8888");                            \
	BUILD_ASSERT((DT_INST_PROP(id, width) % 16) == 0, "LCDC width must be a multiple of 16");  \
	BUILD_ASSERT(LCDC_IN_RANGE(DT_INST_PROP(id, width), 1024), "LCDC width out of range");     \
	BUILD_ASSERT(LCDC_IN_RANGE(DT_INST_PROP(id, height), 1024), "LCDC height out of range");   \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, hsync_len), 256), "hsync-len out of range");    \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, hfront_porch), 256),                            \
		     "hfront-porch out of range");                                                 \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, hback_porch), 256),                             \
		     "hback-porch out of range");                                                  \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, vsync_len), 64), "vsync-len out of range");     \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, vfront_porch), 256),                            \
		     "vfront-porch out of range");                                                 \
	BUILD_ASSERT(LCDC_IN_RANGE(LCDC_TIMING(id, vback_porch), 256), "vback-porch out of range")

#define MCUX_LCDC_DEVICE_INIT(id)                                                                  \
	LCDC_CHECK_TIMINGS(id);                                                                    \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static void mcux_lcdc_config_func_##id(const struct device *dev);                          \
	static uint8_t mcux_lcdc_fb_##id[2][LCDC_FB_SIZE(id)] __aligned(8)                         \
		Z_GENERIC_SECTION(LCDC_FB_SECTION(id));                                            \
	static const struct mcux_lcdc_config mcux_lcdc_config_##id = {                             \
		.base = (LCD_Type *)DT_INST_REG_ADDR(id),                                          \
		.irq_config_func = mcux_lcdc_config_func_##id,                                     \
		.fb = {mcux_lcdc_fb_##id[0], mcux_lcdc_fb_##id[1]},                                \
		.fb_size = LCDC_FB_SIZE(id),                                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),             \
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios),                      \
			(.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(id, backlight_gpios, {0}),))   \
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms),                                 \
			(.backlight_pwm = PWM_DT_SPEC_INST_GET_OR(id, {0}),))                      \
		.panel_width = DT_INST_PROP(id, width),                                            \
		.panel_height = DT_INST_PROP(id, height),                                          \
		.pixel_format = LCDC_FORMAT(id),                                                   \
		.pixel_bytes = DISPLAY_BITS_PER_PIXEL(LCDC_FORMAT(id)) / BITS_PER_BYTE,            \
		.lcdc_config = {                                                                   \
			.panelClock_Hz = LCDC_TIMING(id, clock_frequency),                         \
			.ppl = DT_INST_PROP(id, width),                                            \
			.hsw = LCDC_TIMING(id, hsync_len),                                         \
			.hfp = LCDC_TIMING(id, hfront_porch),                                      \
			.hbp = LCDC_TIMING(id, hback_porch),                                       \
			.lpp = DT_INST_PROP(id, height),                                           \
			.vsw = LCDC_TIMING(id, vsync_len),                                         \
			.vfp = LCDC_TIMING(id, vfront_porch),                                      \
			.vbp = LCDC_TIMING(id, vback_porch),                                       \
			.acBiasFreq = 1,                                                           \
			.polarityFlags = LCDC_POLARITY(id),                                        \
			.upperPanelAddr = 0,                                                       \
			.lowerPanelAddr = 0,                                                       \
			.bpp = LCDC_BPP(id),                                                       \
			.dataFormat = kLCDC_LittleEndian,                                          \
			/* Red in the high bits, as in every Zephyr format supported here. */     \
			.swapRedBlue = true,                                                       \
			.display = kLCDC_DisplayTFT,                                               \
		},                                                                                 \
	};                                                                                         \
	static struct mcux_lcdc_data mcux_lcdc_data_##id;                                          \
	DEVICE_DT_INST_DEFINE(id, mcux_lcdc_init, NULL, &mcux_lcdc_data_##id,                      \
			      &mcux_lcdc_config_##id, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,   \
			      &mcux_lcdc_api);                                                     \
	static void mcux_lcdc_config_func_##id(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lcdc_isr,            \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_LCDC_DEVICE_INIT)
