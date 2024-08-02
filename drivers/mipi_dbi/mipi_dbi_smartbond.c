/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT  renesas_smartbond_mipi_dbi

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <DA1469xAB.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi.h>
#include <da1469x_lcdc.h>
#include <da1469x_pd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smartbond_mipi_dbi, CONFIG_MIPI_DBI_LOG_LEVEL);

#define SMARTBOND_IRQN		DT_INST_IRQN(0)
#define SMARTBOND_IRQ_PRIO  DT_INST_IRQ(0, priority)

#define PINCTRL_STATE_READ  PINCTRL_STATE_PRIV_START

#define MIPI_DBI_SMARTBOND_IS_READ_SUPPORTED \
				DT_INST_NODE_HAS_PROP(0, spi_dev)

#define LCDC_SMARTBOND_CLK_DIV(_freq)                           \
	((32000000U % (_freq)) ? (96000000U / (_freq)) : (32000000U / (_freq)))

#define MIPI_DBI_SMARTBOND_IS_PLL_REQUIRED \
			!!(32000000U % DT_PROP(DT_CHOSEN(zephyr_display), mipi_max_frequency))

#define MIPI_DBI_SMARTBOND_IS_TE_ENABLED \
			DT_INST_PROP_OR(0, te_enable, 0)

#define MIPI_DBI_SMARTBOND_IS_DMA_PREFETCH_ENABLED \
			DT_INST_ENUM_IDX_OR(0, dma_prefetch, 0)

#define MIPI_DBI_SMARTBOND_IS_RESET_AVAILABLE \
			DT_INST_NODE_HAS_PROP(0, reset_gpios)

#define LCDC_LAYER0_OFFSETX_REG_SET_FIELD(_field, _var, _val) \
	((_var)) = \
	((_var) & ~(LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Msk)) |	\
	(((_var) << LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Pos) & \
	LCDC_LCDC_LAYER0_OFFSETX_REG_ ## _field ## _Msk)

struct mipi_dbi_smartbond_data {
	/* Provide mutual exclusion when a display operation is requested. */
	struct k_sem device_sem;
	/* Provide synchronization between task return and ISR firing */
	struct k_sem sync_sem;
	/* Flag indicating whether or not an underflow took place */
	volatile bool underflow_flag;
	/* Layer settings */
	lcdc_smartbond_layer_cfg layer;
};

struct mipi_dbi_smartbond_config {
	/* Reference to device instance's pinctrl configurations */
	const struct pinctrl_dev_config *pcfg;
	/* Reset GPIO */
	const struct gpio_dt_spec reset;
	/* Host controller's timing settings */
	lcdc_smartbond_timing_cfg timing_cfg;
	/* Background default color configuration */
	lcdc_smartbond_bgcolor_cfg bgcolor_cfg;
};

/* Mark the device is progress and so it's not allowed to enter the sleep state. */
static inline void mipi_dbi_smartbond_pm_policy_state_lock_get(void)
{
	/*
	 * Prevent the SoC from etering the normal sleep state as PDC does not support
	 * waking up the application core following LCDC events.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

/* Mark that device is inactive and so it's allowed to enter the sleep state */
static inline void mipi_dbi_smartbond_pm_policy_state_lock_put(void)
{
	/* Allow the SoC to enter the nornmal sleep state once LCDC is inactive */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

/* Helper function to trigger the LCDC fetching data from frame buffer to the connected display */
static void mipi_dbi_smartbond_send_single_frame(const struct device *dev)
{
	struct mipi_dbi_smartbond_data *data = dev->data;

#if MIPI_DBI_SMARTBOND_IS_TE_ENABLED
	da1469x_lcdc_te_set_status(true, DT_INST_PROP_OR(0, te_polarity, false));
	/*
	 * Wait for the TE signal to be asserted so display's refresh status can be synchronized
	 * with the current frame update.
	 */
	k_sem_take(&data->sync_sem, K_FOREVER);
#endif

	LCDC->LCDC_INTERRUPT_REG |= LCDC_LCDC_INTERRUPT_REG_LCDC_VSYNC_IRQ_EN_Msk;

	/* Setting this bit will enable the host to start outputing pixel data */
	LCDC->LCDC_MODE_REG |= LCDC_LCDC_MODE_REG_LCDC_SFRAME_UPD_Msk;

	/* Wait for frame update to complete */
	k_sem_take(&data->sync_sem, K_FOREVER);

	if (data->underflow_flag) {
		LOG_WRN("Underflow took place");
		data->underflow_flag = false;
	}
}

#if MIPI_DBI_SMARTBOND_IS_RESET_AVAILABLE
static int mipi_dbi_smartbond_reset(const struct device *dev, uint32_t delay)
{
	const struct mipi_dbi_smartbond_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->reset)) {
		LOG_ERR("Reset signal not available");
		return -ENODEV;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		LOG_ERR("Cannot drive reset signal");
		return ret;
	}
	k_msleep(delay);

	return gpio_pin_set_dt(&config->reset, 0);
}
#endif

/* Display pixel to output color format translation */
static inline uint8_t lcdc_smartbond_pixel_to_ocm(enum display_pixel_format pixfmt)
{
	switch (pixfmt) {
	case PIXEL_FORMAT_RGB_565:
		return (uint8_t)LCDC_SMARTBOND_OCM_RGB565;
	case PIXEL_FORMAT_RGB_888:
		return (uint8_t)LCDC_SMARTBOND_OCM_RGB888;
	case PIXEL_FORMAT_MONO10:
		return (uint8_t)LCDC_SMARTBOND_L0_L1;
	default:
		LOG_ERR("Unsupported pixel format");
		return 0;
	};
}

static inline uint8_t lcdc_smartbond_line_mode_translation(uint8_t mode)
{
	switch (mode) {
	case MIPI_DBI_MODE_SPI_3WIRE:
		return (uint8_t)LCDC_SMARTBOND_MODE_SPI3;
	case MIPI_DBI_MODE_SPI_4WIRE:
		return (uint8_t)LCDC_SMARTBOND_MODE_SPI4;
	default:
		LOG_ERR("Unsupported SPI mode");
		return 0;
	}
}

static inline uint8_t lcdc_smartbond_pixel_to_lcm(enum display_pixel_format pixfmt)
{
	switch (pixfmt) {
	case PIXEL_FORMAT_RGB_565:
		return (uint8_t)LCDC_SMARTBOND_L0_RGB565;
	case PIXEL_FORMAT_ARGB_8888:
		return (uint8_t)LCDC_SMARTBOND_L0_ARGB8888;
	default:
		LOG_ERR("Unsupported pixel format");
		return 0;
	};
}

static void lcdc_smartbond_mipi_dbi_translation(const struct mipi_dbi_config *dbi_config,
			lcdc_smartbond_mipi_dbi_cfg *mipi_dbi_cfg,
			enum display_pixel_format pixfmt)
{
	mipi_dbi_cfg->cpha = dbi_config->config.operation & SPI_MODE_CPHA;
	mipi_dbi_cfg->cpol = dbi_config->config.operation & SPI_MODE_CPOL;
	mipi_dbi_cfg->cs_active_high = dbi_config->config.operation & SPI_CS_ACTIVE_HIGH;
	mipi_dbi_cfg->line_mode = lcdc_smartbond_line_mode_translation(dbi_config->mode);
	mipi_dbi_cfg->color_mode = lcdc_smartbond_pixel_to_ocm(pixfmt);
}

#if MIPI_DBI_SMARTBOND_IS_READ_SUPPORTED
static int mipi_dbi_smartbond_command_read(const struct device *dev,
				const struct mipi_dbi_config *dbi_config,
				uint8_t *cmd, size_t num_cmds,
				uint8_t *response, size_t len)
{
	struct mipi_dbi_smartbond_data *data = dev->data;
	const struct mipi_dbi_smartbond_config *config = dev->config;
	int ret = 0;
	lcdc_smartbond_mipi_dbi_cfg mipi_dbi_cfg;

	k_sem_take(&data->device_sem, K_FOREVER);

	mipi_dbi_smartbond_pm_policy_state_lock_get();

	/*
	 * Add an arbitrary valid color format to satisfy subroutine. The MIPI DBI command/data
	 * engine should not be affected.
	 */
	lcdc_smartbond_mipi_dbi_translation(dbi_config, &mipi_dbi_cfg, PIXEL_FORMAT_RGB_565);
	ret = da1469x_lcdc_mipi_dbi_interface_configure(&mipi_dbi_cfg);
	if (ret < 0) {
		goto _mipi_dbi_read_exit;
	}

	/* Check if the cmd/data engine is busy since the #CS line will be overruled. */
	if (da1469x_lcdc_is_busy()) {
		LOG_WRN("MIPI DBI host is busy");
		ret = -EBUSY;
		goto _mipi_dbi_read_exit;
	}

	/* Force CS line to low. Typically, command and data are bound in the same #CS assertion */
	da1469x_lcdc_force_cs_line(true, mipi_dbi_cfg.cs_active_high);

	da1469x_lcdc_send_cmd_data(true, cmd, num_cmds);

	if (len) {
		const struct device *spi_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, spi_dev));

		struct spi_buf buffer = {
			.buf = (void *)response,
			.len = len,
		};
		struct spi_buf_set buf_set = {
			.buffers = &buffer,
			.count = 1,
		};

		if (!device_is_ready(spi_dev)) {
			LOG_ERR("SPI device is not ready");
			ret = -ENODEV;
			goto _mipi_dbi_read_exit;
		}

		/* Overwrite CLK and enable DI lines. CS is driven forcefully. */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_READ);
		if (ret < 0) {
			LOG_ERR("Could not apply MIPI DBI pins' SPI read state (%d)", ret);
			goto _mipi_dbi_read_exit;
		}

		/* Get response */
		ret = spi_read(spi_dev, &dbi_config->config, &buf_set);
		if (ret < 0) {
			LOG_ERR("Could not read data from SPI");
			goto _mipi_dbi_read_exit;
		}
	}

_mipi_dbi_read_exit:

	/* Restore #CS line */
	da1469x_lcdc_force_cs_line(false, mipi_dbi_cfg.cs_active_high);

	/* Make sure default LCDC pins are applied upon exit */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not apply MIPI DBI pins' default state (%d)", ret);
	}

	mipi_dbi_smartbond_pm_policy_state_lock_put();

	k_sem_give(&data->device_sem);

	return ret;
}
#endif

static int mipi_dbi_smartbond_command_write(const struct device *dev,
				const struct mipi_dbi_config *dbi_config,
				uint8_t cmd, const uint8_t *data_buf,
				size_t len)
{
	struct mipi_dbi_smartbond_data *data = dev->data;
	int ret = 0;
	lcdc_smartbond_mipi_dbi_cfg mipi_dbi_cfg;

	k_sem_take(&data->device_sem, K_FOREVER);

	mipi_dbi_smartbond_pm_policy_state_lock_get();

	/*
	 * Add an arbitrary valid color format to satisfy subroutine. The MIPI DBI command/data
	 * engine should not be affected.
	 */
	lcdc_smartbond_mipi_dbi_translation(dbi_config, &mipi_dbi_cfg, PIXEL_FORMAT_RGB_565);
	ret = da1469x_lcdc_mipi_dbi_interface_configure(&mipi_dbi_cfg);
	if (ret < 0) {
		goto finish;
	}

	/* Command and accompanied data should be transmitted via the DBIB interface */
	da1469x_lcdc_send_cmd_data(true, &cmd, 1);

	if (len) {
		/* Data should be transmitted via the DBIB interface */
		da1469x_lcdc_send_cmd_data(false, data_buf, len);
	}

finish:
	mipi_dbi_smartbond_pm_policy_state_lock_put();

	k_sem_give(&data->device_sem);

	return ret;
}

static int mipi_dbi_smartbond_write_display(const struct device *dev,
					const struct mipi_dbi_config *dbi_config,
					const uint8_t *framebuf,
					struct display_buffer_descriptor *desc,
					enum display_pixel_format pixfmt)
{
	struct mipi_dbi_smartbond_data *data = dev->data;
	const struct mipi_dbi_smartbond_config *config = dev->config;
	lcdc_smartbond_layer_cfg *layer = &data->layer;
	int ret = 0;
	lcdc_smartbond_mipi_dbi_cfg mipi_dbi_cfg;
	uint8_t layer_color = lcdc_smartbond_pixel_to_lcm(pixfmt);

	if (desc->width * desc->height * (DISPLAY_BITS_PER_PIXEL(pixfmt) / 8) !=
		desc->buf_size) {
		LOG_ERR("Incorrect buffer size for given width and height");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	mipi_dbi_smartbond_pm_policy_state_lock_get();

	/*
	 * Mainly check if the frame generator is busy with a pending frame update (might happen
	 * when two frame updates take place one after the other and the display interface is
	 * quite slow). VSYNC interrupt line should be asserted when the last line is being
	 * outputed.
	 */
	if (da1469x_lcdc_is_busy()) {
		LOG_WRN("MIPI DBI host is busy");
		ret = -EBUSY;
		goto _mipi_dbi_write_exit;
	}

	lcdc_smartbond_mipi_dbi_translation(dbi_config, &mipi_dbi_cfg, pixfmt);
	ret = da1469x_lcdc_mipi_dbi_interface_configure(&mipi_dbi_cfg);
	if (ret < 0) {
		goto _mipi_dbi_write_exit;
	}

	ret = da1469x_lcdc_timings_configure(desc->width, desc->height,
			(lcdc_smartbond_timing_cfg *)&config->timing_cfg);
	if (ret < 0) {
		goto _mipi_dbi_write_exit;
	}

	LCDC_SMARTBOND_LAYER_CONFIG(layer, framebuf, 0, 0, desc->width, desc->height,
					layer_color,
					da1469x_lcdc_stride_calculation(layer_color, desc->width));
	ret = da1469x_lcdc_layer_configure(layer);
	if (ret < 0) {
		goto _mipi_dbi_write_exit;
	}

	/* Trigger single frame update via the LCDC-DMA engine */
	mipi_dbi_smartbond_send_single_frame(dev);

_mipi_dbi_write_exit:

	mipi_dbi_smartbond_pm_policy_state_lock_put();

	k_sem_give(&data->device_sem);

	return ret;
}

static int mipi_dbi_smartbond_configure(const struct device *dev)
{
	uint8_t clk_div =
		LCDC_SMARTBOND_CLK_DIV(DT_PROP(DT_CHOSEN(zephyr_display), mipi_max_frequency));
	const struct mipi_dbi_smartbond_config *config = dev->config;

	/*
	 * First enable the controller so registers can be written. In serial interfaces
	 * clock divider is further divided by 2.
	 */
	da1469x_lcdc_set_status(true, MIPI_DBI_SMARTBOND_IS_PLL_REQUIRED,
		(clk_div >= 2 ? clk_div / 2 : clk_div));

	if (!da1469x_lcdc_check_id()) {
		LOG_ERR("Mismatching LCDC ID");
		da1469x_lcdc_set_status(false, 0, 0);
		return -EINVAL;
	}

	da1469x_lcdc_te_set_status(false, DT_INST_PROP_OR(0, te_polarity, false));

	da1469x_lcdc_bgcolor_configure((lcdc_smartbond_bgcolor_cfg *)&config->bgcolor_cfg);

	LCDC_LAYER0_OFFSETX_REG_SET_FIELD(LCDC_L0_DMA_PREFETCH,
			LCDC->LCDC_LAYER0_OFFSETX_REG, MIPI_DBI_SMARTBOND_IS_DMA_PREFETCH_ENABLED);

	return 0;
}

static void smartbond_mipi_dbi_isr(const void *arg)
{
	struct mipi_dbi_smartbond_data *data =  ((const struct device *)arg)->data;

	/*
	 * Underflow sticky bit will remain high until cleared by writing
	 * any value to LCDC_INTERRUPT_REG.
	 */
	data->underflow_flag = LCDC_STATUS_REG_GET_FIELD(LCDC_STICKY_UNDERFLOW);

	/* Default interrupt mode is level triggering so interrupt should be cleared */
	da1469x_lcdc_te_set_status(false, DT_INST_PROP_OR(0, te_polarity, false));

	k_sem_give(&data->sync_sem);
}

static int mipi_dbi_smartbond_resume(const struct device *dev)
{
	const struct mipi_dbi_smartbond_config *config = dev->config;
	int ret;

	/* Select default state */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not apply LCDC pins' default state (%d)", ret);
		return -EIO;
	}

#if MIPI_DBI_SMARTBOND_IS_PLL_REQUIRED
	const struct device *clock_dev = DEVICE_DT_GET(DT_NODELABEL(osc));

	if (!device_is_ready(clock_dev)) {
		LOG_WRN("Clock device is not available; PLL cannot be used");
	} else {
		ret = z_smartbond_select_sys_clk(SMARTBOND_CLK_PLL96M);
		if (ret < 0) {
			LOG_WRN("Could not switch to PLL. Requested speed should not be achieved.");
		}
	}
#endif

	return mipi_dbi_smartbond_configure(dev);
}

#if defined(CONFIG_PM_DEVICE)
static int mipi_dbi_smartbond_suspend(const struct device *dev)
{
	const struct mipi_dbi_smartbond_config *config = dev->config;
	int ret;

	/* Select sleep state; it's OK if settings fails for any reason. */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_WRN("Could not apply MIPI DBI pins' sleep state");
	}

	/* Disable host controller to minimize power consumption. */
	da1469x_lcdc_set_status(false, false, 0);

	return 0;
}

static int mipi_dbi_smartbond_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* A non-zero value should not affect sleep */
		(void)mipi_dbi_smartbond_suspend(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * The resume error code should not be taken into consideration
		 * by the PM subsystem.
		 */
		ret = mipi_dbi_smartbond_resume(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int mipi_dbi_smartbond_init(const struct device *dev)
{
	__unused const struct mipi_dbi_smartbond_config *config = dev->config;
	struct mipi_dbi_smartbond_data *data = dev->data;
	int ret;

	/* Device should be ready to be acquired */
	k_sem_init(&data->device_sem, 1, 1);
	/* Event should be signaled by LCDC ISR */
	k_sem_init(&data->sync_sem, 0, 1);

#if MIPI_DBI_SMARTBOND_IS_RESET_AVAILABLE
	if (gpio_is_ready_dt(&config->reset)) {
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset line (%d)", ret);
			return -EIO;
		}
	}
#endif

	IRQ_CONNECT(SMARTBOND_IRQN, SMARTBOND_IRQ_PRIO, smartbond_mipi_dbi_isr,
						DEVICE_DT_INST_GET(0), 0);

	ret = mipi_dbi_smartbond_resume(dev);

	return ret;
}

static const struct mipi_dbi_driver_api mipi_dbi_smartbond_driver_api = {
#if MIPI_DBI_SMARTBOND_IS_RESET_AVAILABLE
	.reset = mipi_dbi_smartbond_reset,
#endif
	.command_write = mipi_dbi_smartbond_command_write,
	.write_display = mipi_dbi_smartbond_write_display,
#if MIPI_DBI_SMARTBOND_IS_READ_SUPPORTED
	.command_read = mipi_dbi_smartbond_command_read,
#endif
};

#define SMARTBOND_MIPI_DBI_INIT(inst)	\
	PINCTRL_DT_INST_DEFINE(inst);	\
                                    \
	static const struct mipi_dbi_smartbond_config mipi_dbi_smartbond_config_## inst = {	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),	\
		.timing_cfg = { 0 },	\
		.bgcolor_cfg = { 0xFF, 0xFF, 0xFF, 0 },	\
	};	\
												\
	static struct mipi_dbi_smartbond_data mipi_dbi_smartbond_data_## inst;	\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, mipi_dbi_smartbond_pm_action);	\
												\
	DEVICE_DT_INST_DEFINE(inst, mipi_dbi_smartbond_init,	\
						PM_DEVICE_DT_INST_GET(inst),	\
						&mipi_dbi_smartbond_data_## inst,	\
						&mipi_dbi_smartbond_config_## inst,	\
						POST_KERNEL,	\
						CONFIG_MIPI_DBI_INIT_PRIORITY,	\
						&mipi_dbi_smartbond_driver_api);

SMARTBOND_MIPI_DBI_INIT(0);
