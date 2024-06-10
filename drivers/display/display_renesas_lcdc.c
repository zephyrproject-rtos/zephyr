/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT  renesas_smartbond_display

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/dma.h>
#include <da1469x_lcdc.h>
#include <DA1469xAB.h>
#include <da1469x_pd.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smartbond_display, CONFIG_DISPLAY_LOG_LEVEL);

#define SMARTBOND_IRQN		DT_INST_IRQN(0)
#define SMARTBOND_IRQ_PRIO  DT_INST_IRQ(0, priority)

#define LCDC_SMARTBOND_CLK_DIV(_freq)                           \
	((32000000U % (_freq)) ? (96000000U / (_freq)) : (32000000U / (_freq)))

#define LCDC_SMARTBOND_IS_PLL_REQUIRED \
	!!(32000000U % DT_PROP(DT_INST_CHILD(0, display_timings), clock_frequency))

#define DISPLAY_SMARTBOND_IS_DMA_PREFETCH_ENABLED \
	DT_INST_ENUM_IDX_OR(0, dma_prefetch, 0)

#define LCDC_LAYER0_OFFSETX_REG_SET_FIELD(_field, _var, _val)\
	((_var)) =	\
	((_var) & ~(LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Msk)) |	\
	(((_val) << LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Pos) &	\
	LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Msk)

#define DISPLAY_SMARTBOND_PIXEL_SIZE(inst)	\
	(DISPLAY_BITS_PER_PIXEL(DT_INST_PROP(inst, pixel_format)) / 8)

#if CONFIG_DISPLAY_RENESAS_LCDC_BUFFER_PSRAM
#define DISPLAY_BUFFER_LINKER_SECTION \
	Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(psram)))
#else
#define DISPLAY_BUFFER_LINKER_SECTION
#endif

struct display_smartbond_data {
	/* Provide mutual exclusion when a display operation is requested. */
	struct k_sem device_sem;
	/* Frame update synchronization token */
	struct k_sem sync_sem;
	/* Flag indicating whether or not an underflow took place */
	volatile bool underflow_flag;
	/* Layer settings */
	lcdc_smartbond_layer_cfg layer;
	/* Frame buffer */
	uint8_t *buffer;
	/* DMA device */
	const struct device *dma;
	/* DMA configuration structures */
	struct dma_config dma_cfg;
	struct dma_block_config dma_block_cfg;
	/* DMA memory transfer synchronization token */
	struct k_sem dma_sync_sem;
	/* Granted DMA channel used for memory transfers */
	int dma_channel;
#if defined(CONFIG_PM_DEVICE)
	ATOMIC_DEFINE(pm_policy_state_flag, 1);
#endif
};

struct display_smartbond_config {
	/* Reference to device instance's pinctrl configurations */
	const struct pinctrl_dev_config *pcfg;
	/* Display ON/OFF GPIO */
	const struct gpio_dt_spec disp;
	/* Host controller's timing settings */
	lcdc_smartbond_timing_cfg timing_cfg;
	/* Parallel interface settings */
	lcdc_smartbond_mode_cfg mode;
	/* Background default color configuration */
	lcdc_smartbond_bgcolor_cfg bgcolor_cfg;
	/* Display dimensions */
	const uint16_t x_res;
	const uint16_t y_res;
	/* Pixel size in bytes */
	uint8_t pixel_size;
	enum display_pixel_format pixel_format;
};

static inline void lcdc_smartbond_pm_policy_state_lock_get(struct display_smartbond_data *data)
{
#ifdef CONFIG_PM_DEVICE
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, 0) == 0) {
		/*
		 * Prevent the SoC from etering the normal sleep state as PDC does not support
		 * waking up the application core following LCDC events.
		 */
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
}

static inline void lcdc_smartbond_pm_policy_state_lock_put(struct display_smartbond_data *data)
{
#ifdef CONFIG_PM_DEVICE
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, 0) == 1) {
		/* Allow the SoC to enter the nornmal sleep state once LCDC is inactive */
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
}

/* Display pixel to layer color format translation */
static uint8_t lcdc_smartbond_pixel_to_lcm(enum display_pixel_format pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		return (uint8_t)LCDC_SMARTBOND_L0_RGB565;
	case PIXEL_FORMAT_ARGB_8888:
		return (uint8_t)LCDC_SMARTBOND_L0_ARGB8888;
	default:
		LOG_ERR("Unsupported pixel format");
		return 0;
	};
}

static int display_smartbond_configure(const struct device *dev)
{
	uint8_t clk_div =
		LCDC_SMARTBOND_CLK_DIV(DT_PROP(DT_INST_CHILD(0, display_timings), clock_frequency));

	const struct display_smartbond_config *config = dev->config;
	struct display_smartbond_data *data = dev->data;

	int ret = 0;

	/* First enable the controller so registers can be written. */
	da1469x_lcdc_set_status(true, LCDC_SMARTBOND_IS_PLL_REQUIRED, clk_div);

	if (!da1469x_lcdc_check_id()) {
		LOG_ERR("Invalid LCDC ID");
		da1469x_lcdc_set_status(false, false, 0);
		return -EINVAL;
	}

	da1469x_lcdc_parallel_interface_configure((lcdc_smartbond_mode_cfg *)&config->mode);
	da1469x_lcdc_bgcolor_configure((lcdc_smartbond_bgcolor_cfg *)&config->bgcolor_cfg);

	/*
	 * Partial update is not supported and so timing and layer settings can be configured
	 * once at initialization.
	 */
	ret = da1469x_lcdc_timings_configure(config->x_res, config->y_res,
						(lcdc_smartbond_timing_cfg *)&config->timing_cfg);
	if (ret < 0) {
		LOG_ERR("Unable to configure timing settings");
		da1469x_lcdc_set_status(false, false, 0);
		return ret;
	}

	/*
	 * Stride should be updated at the end of a frame update (typically in ISR context).
	 * It's OK to update stride here as continuous mode should not be enabled yet.
	 */
	data->layer.color_format =
		lcdc_smartbond_pixel_to_lcm(config->pixel_format);
	data->layer.stride =
		da1469x_lcdc_stride_calculation(data->layer.color_format, config->x_res);

	ret = da1469x_lcdc_layer_configure(&data->layer);
	if (ret < 0) {
		LOG_ERR("Unable to configure layer settings");
		da1469x_lcdc_set_status(false, false, 0);
	}

	LCDC_LAYER0_OFFSETX_REG_SET_FIELD(LCDC_L0_DMA_PREFETCH,
		LCDC->LCDC_LAYER0_OFFSETX_REG, DISPLAY_SMARTBOND_IS_DMA_PREFETCH_ENABLED);

	LCDC->LCDC_MODE_REG |= LCDC_LCDC_MODE_REG_LCDC_MODE_EN_Msk;

	return ret;
}

static void smartbond_display_isr(const void *arg)
{
	struct display_smartbond_data *data =  ((const struct device *)arg)->data;

	data->underflow_flag = LCDC_STATUS_REG_GET_FIELD(LCDC_STICKY_UNDERFLOW);

	/*
	 * Underflow sticky bit will remain high until cleared by writing
	 * any value to LCDC_INTERRUPT_REG.
	 */
	LCDC->LCDC_INTERRUPT_REG &= ~LCDC_LCDC_INTERRUPT_REG_LCDC_VSYNC_IRQ_EN_Msk;

	/* Notify that current frame update is completed */
	k_sem_give(&data->sync_sem);
}

static void display_smartbond_dma_cb(const struct device *dma, void *arg,
				uint32_t id, int status)
{
	struct display_smartbond_data *data = arg;

	if (status < 0) {
		LOG_WRN("DMA transfer did not complete");
	}

	k_sem_give(&data->dma_sync_sem);
}

static int display_smartbond_dma_config(const struct device *dev)
{
	struct display_smartbond_data *data = dev->data;

	data->dma = DEVICE_DT_GET(DT_NODELABEL(dma));
	if (!device_is_ready(data->dma)) {
		LOG_ERR("DMA device is not ready");
		return -ENODEV;
	}

	data->dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	data->dma_cfg.user_data = data;
	data->dma_cfg.dma_callback = display_smartbond_dma_cb;
	data->dma_cfg.block_count = 1;
	data->dma_cfg.head_block = &data->dma_block_cfg;
	data->dma_cfg.error_callback_dis = 1;

	/* Request an arbitrary DMA channel */
	data->dma_channel = dma_request_channel(data->dma, NULL);
	if (data->dma_channel < 0) {
		LOG_ERR("Could not acquire a DMA channel");
		return -EIO;
	}

	return 0;
}

static int display_smartbond_resume(const struct device *dev)
{
	const struct display_smartbond_config *config = dev->config;
	int ret;

	/* Select default state */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not apply LCDC pins' default state (%d)", ret);
		return -EIO;
	}

#if LCDC_SMARTBOND_IS_PLL_REQUIRED
	const struct device *clock_dev = DEVICE_DT_GET(DT_NODELABEL(osc));

	if (!device_is_ready(clock_dev)) {
		LOG_WRN("Clock device is not ready");
		return -ENODEV;
	}

	ret = z_smartbond_select_sys_clk(SMARTBOND_CLK_PLL96M);
	if (ret < 0) {
		LOG_WRN("Could not switch to PLL");
		return -EIO;
	}
#endif

	ret = display_smartbond_dma_config(dev);
	if (ret < 0) {
		return ret;
	}

	return display_smartbond_configure(dev);
}

#if defined(CONFIG_PM_DEVICE)
static void display_smartbond_dma_deconfig(const struct device *dev)
{
	struct display_smartbond_data *data = dev->data;

	__ASSERT(data->dma, "DMA should be already initialized");

	dma_release_channel(data->dma, data->dma_channel);
}

static int display_smartbond_suspend(const struct device *dev)
{
	const struct display_smartbond_config *config = dev->config;
	int ret;

	/* Select sleep state; it's OK if settings fails for any reason */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_WRN("Could not apply DISPLAY pins' sleep state");
	}

	/* Disable host controller to minimize power consumption */
	da1469x_lcdc_set_status(false, false, 0);

	display_smartbond_dma_deconfig(dev);

	return 0;
}
#endif

static int display_smartbond_init(const struct device *dev)
{
	const struct display_smartbond_config *config = dev->config;
	struct display_smartbond_data *data = dev->data;
	int ret;

	/* Device should be ready to be acquired */
	k_sem_init(&data->device_sem, 1, 1);
	/* Event should be signaled by LCDC ISR */
	k_sem_init(&data->sync_sem, 0, 1);
	/* Event should be signaled by DMA ISR */
	k_sem_init(&data->dma_sync_sem, 0, 1);

	/* As per docs, display port should be enabled by default. */
	if (gpio_is_ready_dt(&config->disp)) {
		ret = gpio_pin_configure_dt(&config->disp, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not activate display port");
			return -EIO;
		}
	}

	IRQ_CONNECT(SMARTBOND_IRQN, SMARTBOND_IRQ_PRIO, smartbond_display_isr,
								DEVICE_DT_INST_GET(0), 0);

	/*
	 * Currently, there is no API to explicitly enable/disable the display controller.
	 * At the same time, the controller is set to continuous mode meaning that
	 * as long as a display panel is turned on, frame updates should happen all
	 * the time (otherwise contents on the display pane will be lost as the latter
	 * does not integrate an SDRAM memory to keep its frame).
	 * As such, resume/suspend operations are bound to blanking operations.
	 * That is, when the display is blanked on we can safely consider that display
	 * is no longer functional and thus, the controller can be suspended (allowing the
	 * SoC to enter the sleep state). Once the display is blanked off, then we consider
	 * that the controller should be resumed and sleep should be prevented at all
	 * (this is because the controller is powered by the same power domain used to
	 * power the application core). Side effect of the above is that the controller
	 * should be configured at initialization phase as display operations might
	 * be requested before the display is blanked off for the very first time.
	 */
	ret = display_smartbond_resume(dev);
	if (ret == 0) {
		/* Display port should be enabled at this moment and so sleep is not allowed. */
		lcdc_smartbond_pm_policy_state_lock_get(data);
	}

	return ret;
}

static int display_smartbond_blanking_on(const struct device *dev)
{
	const struct display_smartbond_config *config = dev->config;
	struct display_smartbond_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->device_sem, K_FOREVER);

	/*
	 * This bit will force LCD controller's output to blank that is,
	 * the controller will keep operating without outputting any
	 * pixel data.
	 */
	LCDC->LCDC_MODE_REG |= LCDC_LCDC_MODE_REG_LCDC_FORCE_BLANK_Msk;

	/* If enabled, disable display port. */
	if (gpio_is_ready_dt(&config->disp)) {
		ret = gpio_pin_configure_dt(&config->disp, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_WRN("Display port could not be de-activated");
		}
	}

	/*
	 * At this moment the display panel should be turned off and so the device
	 * can enter the suspend state.
	 */
	lcdc_smartbond_pm_policy_state_lock_put(data);

	k_sem_give(&data->device_sem);

	return ret;
}

static int display_smartbond_blanking_off(const struct device *dev)
{
	const struct display_smartbond_config *config = dev->config;
	struct display_smartbond_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->device_sem, K_FOREVER);

	/* If used, enable display port */
	if (gpio_is_ready_dt(&config->disp)) {
		ret = gpio_pin_configure_dt(&config->disp, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_WRN("Display port could not be activated");
		}
	}

	/*
	 * This bit will force LCD controller's output to blank that is,
	 * the controller will keep operating without outputting any
	 * pixel data.
	 */
	LCDC->LCDC_MODE_REG &= ~LCDC_LCDC_MODE_REG_LCDC_FORCE_BLANK_Msk;

	/*
	 * At this moment the display should be turned on and so the device
	 * cannot enter the suspend state.
	 */
	lcdc_smartbond_pm_policy_state_lock_get(data);

	k_sem_give(&data->device_sem);

	return ret;
}

static void *display_smartbond_get_framebuffer(const struct device *dev)
{
	struct display_smartbond_data *data = dev->data;

	return ((void *)data->buffer);
}

static void display_smartbond_get_capabilities(const struct device *dev,
			struct display_capabilities *capabilities)
{
	memset(capabilities, 0, sizeof(*capabilities));

	/*
	 * Multiple color formats should be supported by LCDC. Currently, RGB56 and ARGB888
	 * exposed by display API are supported. In the future we should consider supporting
	 * more color formats which should require changes in LVGL porting.
	 * Here, only one color format should be supported as the frame buffer is accessed
	 * directly by LCDC and is allocated statically during device initialization. The color
	 * format is defined based on the pixel-format property dictated by lcd-controller
	 * bindings.
	 */
	capabilities->supported_pixel_formats = DT_INST_PROP(0, pixel_format);
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->current_pixel_format = DT_INST_PROP(0, pixel_format);
	capabilities->x_resolution = DT_INST_PROP(0, width);
	capabilities->y_resolution = DT_INST_PROP(0, height);
}

static int display_smartbond_read(const struct device *dev,
			const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	struct display_smartbond_data *data = dev->data;
	const struct display_smartbond_config *config = dev->config;
	uint8_t *dst = buf;
	const uint8_t *src = data->buffer;

	k_sem_take(&data->device_sem, K_FOREVER);

	/* pointer to upper left pixel of the rectangle */
	src += (x * config->pixel_size);
	src += (y * data->layer.stride);

	data->dma_block_cfg.block_size = desc->width * config->pixel_size;
	/*
	 * Source and destination base address is word aligned.
	 * Data size should be selected based on color depth as
	 * cursor is shifted multiple of pixel color depth.
	 */
	data->dma_cfg.source_data_size = data->dma_cfg.dest_data_size =
			!(config->pixel_size & 3) ? 4 :
			!(config->pixel_size & 1) ? 2 : 1;

	data->dma_cfg.dest_burst_length = data->dma_cfg.source_burst_length =
		!((data->dma_block_cfg.block_size / data->dma_cfg.source_data_size) & 7) ? 8 :
		!((data->dma_block_cfg.block_size / data->dma_cfg.source_data_size) & 3) ? 4 : 1;

	for (int row = 0; row < desc->height; row++) {

		data->dma_block_cfg.dest_address = (uint32_t)dst;
		data->dma_block_cfg.source_address = (uint32_t)src;

		if (dma_config(data->dma, data->dma_channel, &data->dma_cfg)) {
			LOG_ERR("Could not configure DMA");
			k_sem_give(&data->device_sem);
			return -EIO;
		}

		if (dma_start(data->dma, data->dma_channel)) {
			LOG_ERR("Could not start DMA");
			k_sem_give(&data->device_sem);
			return -EIO;
		}

		k_sem_take(&data->dma_sync_sem, K_FOREVER);

		src += data->layer.stride;
		dst += (desc->pitch * config->pixel_size);
	}

	if (dma_stop(data->dma, data->dma_channel)) {
		LOG_WRN("Could not stop DMA");
	}

	k_sem_give(&data->device_sem);

	return 0;
}

static int display_smartbond_write(const struct device *dev,
				const uint16_t x, const uint16_t y,
				const struct display_buffer_descriptor *desc,
				const void *buf)
{
	struct display_smartbond_data *data = dev->data;
	const struct display_smartbond_config *config = dev->config;
	uint8_t *dst = data->buffer;
	const uint8_t *src = buf;

	k_sem_take(&data->device_sem, K_FOREVER);

	/* pointer to upper left pixel of the rectangle */
	dst += (x * config->pixel_size);
	dst += (y * data->layer.stride);

	/*
	 * Wait for the current frame to finish. Do not disable continuous mode as this
	 * will have visual artifacts.
	 */
	LCDC->LCDC_INTERRUPT_REG |= LCDC_LCDC_INTERRUPT_REG_LCDC_VSYNC_IRQ_EN_Msk;
	k_sem_take(&data->sync_sem, K_FOREVER);

	data->dma_block_cfg.block_size = desc->width * config->pixel_size;
	/*
	 * Source and destination base address is word aligned.
	 * Data size should be selected based on color depth as
	 * cursor is shifted multiple of pixel color depth.
	 */
	data->dma_cfg.source_data_size = data->dma_cfg.dest_data_size =
			!(config->pixel_size & 3) ? 4 :
			!(config->pixel_size & 1) ? 2 : 1;

	data->dma_cfg.dest_burst_length = data->dma_cfg.source_burst_length =
		!((data->dma_block_cfg.block_size / data->dma_cfg.source_data_size) & 7) ? 8 :
		!((data->dma_block_cfg.block_size / data->dma_cfg.source_data_size) & 3) ? 4 : 1;

	for (int row = 0; row < desc->height; row++) {

		data->dma_block_cfg.dest_address = (uint32_t)dst;
		data->dma_block_cfg.source_address = (uint32_t)src;

		if (dma_config(data->dma, data->dma_channel, &data->dma_cfg)) {
			LOG_ERR("Could not configure DMA");
			k_sem_give(&data->device_sem);
			return -EIO;
		}

		if (dma_start(data->dma, data->dma_channel)) {
			LOG_ERR("Could not start DMA");
			k_sem_give(&data->device_sem);
			return -EIO;
		}

		k_sem_take(&data->dma_sync_sem, K_FOREVER);

		dst += data->layer.stride;
		src += (desc->pitch * config->pixel_size);
	}

	if (dma_stop(data->dma, data->dma_channel)) {
		LOG_WRN("Could not stop DMA");
	}

	k_sem_give(&data->device_sem);

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int display_smartbond_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* A non-zero value should not affect sleep */
		(void)display_smartbond_suspend(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * The resume error code should not be taken into consideration
		 * by the PM subsystem
		 */
		ret = display_smartbond_resume(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif

static const struct display_driver_api display_smartbond_driver_api = {
	.write =  display_smartbond_write,
	.read = display_smartbond_read,
	.get_framebuffer = display_smartbond_get_framebuffer,
	.get_capabilities = display_smartbond_get_capabilities,
	.blanking_off = display_smartbond_blanking_off,
	.blanking_on = display_smartbond_blanking_on
};

#define SMARTBOND_DISPLAY_INIT(inst)	\
	PINCTRL_DT_INST_DEFINE(inst);	\
	PM_DEVICE_DT_INST_DEFINE(inst, display_smartbond_pm_action);	\
									\
	__aligned(4) static uint8_t buffer_ ## inst[(((DT_INST_PROP(inst, width) * \
	DISPLAY_SMARTBOND_PIXEL_SIZE(inst)) + 0x3) & ~0x3) *	\
	DT_INST_PROP(inst, height)] DISPLAY_BUFFER_LINKER_SECTION;	\
                                            \
	static const struct display_smartbond_config display_smartbond_config_## inst = {  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	\
		.disp = GPIO_DT_SPEC_INST_GET_OR(inst, disp_gpios, {}), \
		.timing_cfg.vsync_len =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_len),	\
		.timing_cfg.hsync_len =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), hsync_len),	\
		.timing_cfg.hfront_porch =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), hfront_porch),	\
		.timing_cfg.vfront_porch =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), vfront_porch),	\
		.timing_cfg.hback_porch =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), hback_porch),	\
		.timing_cfg.vback_porch =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), vback_porch),	\
		.bgcolor_cfg = {0xFF, 0xFF, 0xFF, 0}, \
		.x_res = DT_INST_PROP(inst, width), \
		.y_res = DT_INST_PROP(inst, height),	\
		.pixel_size = DISPLAY_SMARTBOND_PIXEL_SIZE(inst),	\
		.pixel_format = DT_INST_PROP(0, pixel_format),	\
		.mode.vsync_pol =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_active) ? 0 : 1,	\
		.mode.hsync_pol =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_active) ? 0 : 1,	\
		.mode.de_pol =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), de_active) ? 0 : 1,	\
		.mode.pixelclk_pol =	\
			DT_PROP(DT_INST_CHILD(inst, display_timings), pixelclk_active) ? 0 : 1,	\
	};							\
								\
	static struct display_smartbond_data display_smartbond_data_## inst = {	\
		.buffer = buffer_ ##inst,	\
		.layer.start_x = 0, \
		.layer.start_y = 0, \
		.layer.size_x = DT_INST_PROP(inst, width),	\
		.layer.size_y = DT_INST_PROP(inst, height),	\
		.layer.frame_buf = (uint32_t)buffer_ ## inst, \
	}; \
							\
							\
	DEVICE_DT_INST_DEFINE(inst, display_smartbond_init, PM_DEVICE_DT_INST_GET(inst), \
				&display_smartbond_data_## inst,	\
				&display_smartbond_config_## inst,	\
				POST_KERNEL,	\
				CONFIG_DISPLAY_INIT_PRIORITY,	\
				&display_smartbond_driver_api);

SMARTBOND_DISPLAY_INIT(0);
