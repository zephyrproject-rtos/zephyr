/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_mipi_dsi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <am_mcu_apollo.h>
#include <nema_dc_regs.h>
#include <nema_dc_hal.h>
#include <nema_dc_mipi.h>
#include <nema_dc_intern.h>
#include <nema_dc.h>
#include <nema_dc_dsi.h>

LOG_MODULE_REGISTER(dsi_ambiq, CONFIG_MIPI_DSI_LOG_LEVEL);

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), vdd18_gpios)
static const struct gpio_dt_spec vdd18_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(0), vdd18_gpios, {0});

static void mipi_dsi_external_vdd18_switch(bool enable)
{
	int ret;

	LOG_DBG("%s", __func__);
	if (!device_is_ready(vdd18_gpio.port)) {
		LOG_ERR("vdd18 GPIO port not ready!");
		return;
	}

	ret = gpio_pin_configure_dt(&vdd18_gpio, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Could not pull vdd18 switch low! (%d)", ret);
		return;
	}

	ret = gpio_pin_set_dt(&vdd18_gpio, enable ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Could not pull vdd18 to dedicated level! (%d)", ret);
		return;
	}
}
#endif

struct mipi_dsi_ambiq_config {
	uint32_t dbi_width;
	uint32_t phy_clock;
	bool disp_te;
	const struct pinctrl_dev_config *te_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct mipi_dsi_ambiq_data {
	nemadc_layer_t dc_layer;
	nemadc_initial_config_t dc_config;
};

static int mipi_dsi_ambiq_attach(const struct device *dev, uint8_t channel,
				 const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_ambiq_config *config = dev->config;
	struct mipi_dsi_ambiq_data *data = dev->data;
	uint32_t ui32FreqTrim;

	LOG_DBG("%s", __func__);

	if (config->phy_clock % 2000000) {
		ui32FreqTrim = (config->phy_clock / 24000000) | 0x40U;
	} else {
		ui32FreqTrim = config->phy_clock / 24000000;
	}

	(void)pm_device_runtime_get(dev);

	if (am_hal_dsi_para_config(mdev->data_lanes, config->dbi_width, ui32FreqTrim, true) != 0) {
		LOG_ERR("DSI config failed!\n");
		return -EFAULT;
	}

	switch (mdev->pixfmt) {
	case MIPI_DSI_PIXFMT_RGB888:
		if (config->dbi_width == 16) {
			data->dc_config.ui32PixelFormat = MIPICFG_16RGB888_OPT0;
		} else if (config->dbi_width == 8) {
			data->dc_config.ui32PixelFormat = MIPICFG_8RGB888_OPT0;
		}
		data->dc_layer.format = NEMADC_RGB24;
		break;

	case MIPI_DSI_PIXFMT_RGB565:
		if (config->dbi_width == 16) {
			data->dc_config.ui32PixelFormat = MIPICFG_16RGB565_OPT0;
		} else if (config->dbi_width == 8) {
			data->dc_config.ui32PixelFormat = MIPICFG_8RGB565_OPT0;
		}
		data->dc_layer.format = NEMADC_RGB565;
		break;
	default:
		LOG_ERR("Invalid color coding!\n");
		return -ENOTSUP;
	}

	data->dc_config.ui16ResX = mdev->timings.hactive;
	data->dc_config.ui32FrontPorchX = mdev->timings.hfp;
	data->dc_config.ui32BackPorchX = mdev->timings.hbp;
	data->dc_config.ui32BlankingX = mdev->timings.hsync;

	data->dc_config.ui16ResY = mdev->timings.vactive;
	data->dc_config.ui32FrontPorchY = mdev->timings.vfp;
	data->dc_config.ui32BackPorchY = mdev->timings.vbp;
	data->dc_config.ui32BlankingY = mdev->timings.vsync;

	data->dc_config.bTEEnable = config->disp_te;
	data->dc_config.eInterface = DISP_INTERFACE_DBIDSI;

	nemadc_configure(&data->dc_config);

	(void)pm_device_runtime_put(dev);

	data->dc_layer.resx = data->dc_config.ui16ResX;
	data->dc_layer.resy = data->dc_config.ui16ResY;
	data->dc_layer.buscfg = 0;
	data->dc_layer.blendmode = NEMADC_BL_SRC;
	data->dc_layer.stride = nemadc_stride_size(data->dc_layer.format, data->dc_layer.resx);
	data->dc_layer.startx = 0;
	data->dc_layer.starty = 0;
	data->dc_layer.sizex = data->dc_layer.resx;
	data->dc_layer.sizey = data->dc_layer.resy;
	data->dc_layer.alpha = 0xFF;
	data->dc_layer.flipx_en = 0;
	data->dc_layer.flipy_en = 0;
	data->dc_layer.extra_bits = 0;

	return 0;
}

static ssize_t mipi_dsi_ambiq_transfer(const struct device *dev, uint8_t channel,
				       struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_ambiq_data *data = dev->data;
	ssize_t len = 0;
	uint8_t *pointer;
	int ret = 0;

	(void)pm_device_runtime_get(dev);

	switch (msg->type) {
	case MIPI_DSI_DCS_READ:
		ret = nemadc_mipi_cmd_read(msg->cmd, NULL, 0, (uint32_t *)msg->rx_buf,
					   (uint8_t)msg->rx_len, true, false);
		len = msg->rx_len;
		break;
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_DCS_LONG_WRITE:
		if ((msg->cmd == MIPI_DCS_WRITE_MEMORY_START) ||
		    (msg->cmd == MIPI_DCS_WRITE_MEMORY_CONTINUE)) {
			nemadc_timing(data->dc_layer.resx, data->dc_config.ui32FrontPorchX,
				      data->dc_config.ui32BlankingX, data->dc_config.ui32BackPorchX,
				      data->dc_layer.resy, data->dc_config.ui32FrontPorchY,
				      data->dc_config.ui32BlankingY, data->dc_config.ui32BlankingY);
			data->dc_layer.stride =
				nemadc_stride_size(data->dc_layer.format, data->dc_layer.resx);
			data->dc_layer.baseaddr_virt = (void *)msg->tx_buf;
			data->dc_layer.baseaddr_phys = (unsigned int)(data->dc_layer.baseaddr_virt);
			nemadc_set_layer(0, &data->dc_layer);

			if (msg->cmd == MIPI_DCS_WRITE_MEMORY_START) {
				nemadc_transfer_frame_prepare(data->dc_config.bTEEnable);
				if (!data->dc_config.bTEEnable) {
					nemadc_transfer_frame_launch();
				}
			} else {
				nemadc_transfer_frame_continue(false);
				nemadc_transfer_frame_launch();
			}
			nemadc_wait_vsync();
		} else {
			nemadc_mipi_cmd_write(msg->cmd, (uint8_t *)msg->tx_buf,
					      (uint8_t)msg->tx_len, true, false);
			len = msg->tx_len;
			pointer = (uint8_t *)msg->tx_buf;
			if (msg->cmd == MIPI_DCS_SET_COLUMN_ADDRESS) {
				data->dc_layer.resx = (int32_t)((pointer[2] << 8) | pointer[3]) +
						      1 - (int32_t)((pointer[0] << 8) | pointer[1]);
				data->dc_layer.stride = nemadc_stride_size(data->dc_layer.format,
									   data->dc_layer.resx);
			} else if (msg->cmd == MIPI_DCS_SET_PAGE_ADDRESS) {
				data->dc_layer.resy = (int32_t)((pointer[2] << 8) | pointer[3]) +
						      1 - (int32_t)((pointer[0] << 8) | pointer[1]);
			}
		}
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		ret = nemadc_mipi_cmd_read(0, (uint8_t *)msg->tx_buf, (uint8_t)msg->tx_len,
					   (uint32_t *)msg->rx_buf, (uint8_t)msg->rx_len, false,
					   false);
		len = msg->rx_len;
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_GENERIC_LONG_WRITE:
		nemadc_mipi_cmd_write(0, (uint8_t *)msg->tx_buf, (uint8_t)msg->tx_len, false,
				      false);
		len = msg->tx_len;
		break;
	default:
		LOG_ERR("Unsupported message type (%d)", msg->type);
		break;
	}

	(void)pm_device_runtime_put(dev);

	if (ret < 0) {
		LOG_ERR("Failed with error code %d", ret);
		return ret;
	}
	return len;
}

static DEVICE_API(mipi_dsi, dsi_ambiq_api) = {
	.attach = mipi_dsi_ambiq_attach,
	.transfer = mipi_dsi_ambiq_transfer,
};

static int mipi_dsi_ambiq_init(const struct device *dev)
{
	const struct mipi_dsi_ambiq_config *config = dev->config;
	int ret;

	LOG_DBG("%s", __func__);
	/* Select "default" state at initialization time */
	ret = pinctrl_apply_state(config->te_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	am_hal_interrupt_master_enable();

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), vdd18_gpios)
	am_hal_dsi_register_external_vdd18_callback(mipi_dsi_external_vdd18_switch);
#endif

	ret = am_hal_dsi_init();
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("DSI init failed!\n");
		return -EFAULT;
	}

	am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISP);

	ret = nemadc_init();
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("DC init failed!\n");
		return -EFAULT;
	}
#ifdef CONFIG_PM_DEVICE
	/*
	 * To work around software limitations, it causes logic abnormal
	 * if we don't update HAL when PM enabled.
	 */
	ret = am_hal_dsi_para_config(1, 16, AM_HAL_DSI_FREQ_TRIM_X20, true);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("DSI config failed!\n");
		return -EFAULT;
	}
#endif
	config->irq_config_func(dev);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int mipi_dsi_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("Suspending MIPI-DSI device");
		ret = am_hal_dsi_power_control(AM_HAL_SYSCTRL_DEEPSLEEP, true);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("Failed to power down DSI : %d", ret);
			return -EIO;
		}
		ret = nemadc_power_control(AM_HAL_SYSCTRL_DEEPSLEEP, true);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("Failed to power down DC : %d", ret);
			return -EIO;
		}
		break;

	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("Resuming MIPI-DSI device");
		ret = nemadc_power_control(AM_HAL_SYSCTRL_WAKE, true);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("Failed to power on DC : %d", ret);
			return -EIO;
		}
		ret = am_hal_dsi_power_control(AM_HAL_SYSCTRL_WAKE, true);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("Failed to power on DSI : %d", ret);
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

/*
 * Ambiq DC interrupt service routine
 */
extern void am_disp_isr(void);

#define AMBIQ_MIPI_DSI_DEVICE(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void disp_##n##_irq_config_func(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), am_disp_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct mipi_dsi_ambiq_data ambiq_dsi_data_##n;                                      \
	static const struct mipi_dsi_ambiq_config ambiq_dsi_config_##n = {                         \
		.dbi_width = DT_INST_PROP(n, dbi_width),                                           \
		.phy_clock = DT_INST_PROP(n, phy_clock),                                           \
		.disp_te = DT_INST_PROP(n, disp_te),                                               \
		.te_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.irq_config_func = disp_##n##_irq_config_func,                                     \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, mipi_dsi_ambiq_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(n, &mipi_dsi_ambiq_init, PM_DEVICE_DT_INST_GET(n),                   \
			      &ambiq_dsi_data_##n, &ambiq_dsi_config_##n, POST_KERNEL,             \
			      CONFIG_MIPI_DSI_INIT_PRIORITY, &dsi_ambiq_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_MIPI_DSI_DEVICE)
