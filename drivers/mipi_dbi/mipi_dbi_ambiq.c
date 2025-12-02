/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_mipi_dbi

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <am_mcu_apollo.h>
#include <nema_dc_hal.h>
#include <nema_dc_mipi.h>
#include <nema_dc.h>

LOG_MODULE_REGISTER(mipi_dbi_ambiq, CONFIG_MIPI_DBI_LOG_LEVEL);

/* MIPI DBI Type-B (Intel 8080) interface implementation for Ambiq */

struct mipi_dbi_ambiq_config {
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
};

struct mipi_dbi_ambiq_data {
	uint32_t mode;
	enum display_pixel_format pixfmt;
	nemadc_layer_t dc_layer;
	nemadc_initial_config_t dc_config;
};

static int mipi_dbi_ambiq_command_write(const struct device *dev,
					const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					const uint8_t *data_buf, size_t len)
{
	struct mipi_dbi_ambiq_data *data = dev->data;
	int ret = 0;

	/* HAL expects non-const buffer pointer */
	ret = nemadc_mipi_cmd_write(cmd, (uint8_t *)data_buf, len, false, false);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to write command: %d", ret);
		return ret;
	}

	if (cmd == MIPI_DCS_SET_COLUMN_ADDRESS) {
		if (len < 4) {
			LOG_ERR("Invalid argument.");
			return -EINVAL;
		}
		data->dc_layer.resx = (int32_t)((data_buf[2] << 8) | data_buf[3]) -
				      (int32_t)((data_buf[0] << 8) | data_buf[1]);
	} else if (cmd == MIPI_DCS_SET_PAGE_ADDRESS) {
		if (len < 4) {
			LOG_ERR("Invalid argument.");
			return -EINVAL;
		}
		data->dc_layer.resy = (int32_t)((data_buf[2] << 8) | data_buf[3]) -
				      (int32_t)((data_buf[0] << 8) | data_buf[1]);
	}

	return ret;
}

static int mipi_dbi_ambiq_command_read(const struct device *dev,
				       const struct mipi_dbi_config *dbi_config, uint8_t *cmds,
				       size_t num_cmds, uint8_t *response, size_t len)
{
	int ret = 0;

	if (cmds == NULL) {
		LOG_ERR("Invalid argument.");
		return -EINVAL;
	}

	if (len > 4) {
		LOG_ERR("The maximum read length is 4 bytes.");
		return -EINVAL;
	}

	ret = nemadc_mipi_cmd_read(cmds[0], NULL, num_cmds, (uint32_t *)response, len, false,
				   false);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to read command: %d", ret);
		return ret;
	}
	return ret;
}

static int mipi_dbi_ambiq_write_display(const struct device *dev,
					const struct mipi_dbi_config *dbi_config,
					const uint8_t *framebuf,
					struct display_buffer_descriptor *desc,
					enum display_pixel_format pixfmt)
{
	struct mipi_dbi_ambiq_data *data = dev->data;

	if (data->mode != dbi_config->mode) {

		data->dc_config.ui32PixelFormat = 0;

		switch (dbi_config->mode & 0xF) {
		case MIPI_DBI_MODE_8080_BUS_16_BIT:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_DBI16;
			break;
		case MIPI_DBI_MODE_8080_BUS_9_BIT:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_DBI9;
			break;
		case MIPI_DBI_MODE_8080_BUS_8_BIT:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_DBI8;
			break;
		default:
			LOG_ERR("Invalid data bus width!\n");
			return -ENOTSUP;
		}

		switch (dbi_config->mode & (0x7 << 4)) {
		case MIPI_DBI_MODE_RGB332:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT0 | MIPI_DCS_RGB332;
			break;
		case MIPI_DBI_MODE_RGB444:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT0 | MIPI_DCS_RGB444;
			break;
		case MIPI_DBI_MODE_RGB565:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT0 | MIPI_DCS_RGB565;
			break;
		case MIPI_DBI_MODE_RGB666_1:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT0 | MIPI_DCS_RGB666;
			break;
		case MIPI_DBI_MODE_RGB666_2:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT1 | MIPI_DCS_RGB666;
			break;
		case MIPI_DBI_MODE_RGB888_1:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT0 | MIPI_DCS_RGB888;
			break;
		case MIPI_DBI_MODE_RGB888_2:
			data->dc_config.ui32PixelFormat |= MIPICFG_PF_OPT1 | MIPI_DCS_RGB888;
			break;
		default:
			LOG_ERR("Invalid color format!\n");
			return -ENOTSUP;
		}

		nemadc_configure(&data->dc_config);
		data->mode = dbi_config->mode;
	}

	if (data->pixfmt != pixfmt) {
		switch (pixfmt) {
		case PIXEL_FORMAT_RGB_888:
			data->dc_layer.format = NEMADC_RGB24;
			break;
		case PIXEL_FORMAT_ARGB_8888:
			data->dc_layer.format = NEMADC_ARGB8888;
			break;
		case PIXEL_FORMAT_RGB_565:
			data->dc_layer.format = NEMADC_RGB565;
			break;
		case PIXEL_FORMAT_BGR_565:
			data->dc_layer.format = NEMADC_BGR565;
			break;
		case PIXEL_FORMAT_L_8:
			data->dc_layer.format = NEMADC_L8;
			break;
		case PIXEL_FORMAT_AL_88:
			data->dc_layer.format = NEMADC_AL88;
			break;
		default:
			LOG_ERR("Invalid pixel format!\n");
			return -ENOTSUP;
		}
		data->pixfmt = pixfmt;
	}

	nemadc_timing(data->dc_layer.resx, data->dc_config.ui32FrontPorchX,
		      data->dc_config.ui32BlankingX, data->dc_config.ui32BackPorchX,
		      data->dc_layer.resy, data->dc_config.ui32FrontPorchY,
		      data->dc_config.ui32BlankingY, data->dc_config.ui32BlankingY);
	data->dc_layer.stride = nemadc_stride_size(data->dc_layer.format, data->dc_layer.resx);

	data->dc_layer.baseaddr_virt = (void *)framebuf;
	data->dc_layer.baseaddr_phys = (unsigned int)(data->dc_layer.baseaddr_virt);

	nemadc_set_layer(0, &data->dc_layer);

	nemadc_transfer_frame_prepare(false);

	nemadc_transfer_frame_launch();

	nemadc_wait_vsync();

	return 0;
}

static int mipi_dbi_ambiq_init(const struct device *dev)
{
	const struct mipi_dbi_ambiq_config *config = dev->config;
	struct mipi_dbi_ambiq_data *data = dev->data;
	int ret;

	/* Select "default" state at initialization time */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable display peripheral power */
	ret = am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISP);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable display peripheral power: %d", ret);
		return -EIO;
	}

	/* Configure clock source, the frequency is up to 192MHz */
	ret = am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DISPCLKSEL_HFRC192, NULL);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to configure display clock: %d", ret);
		return -EIO;
	}

	ret = am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DCCLK_ENABLE, NULL);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable DC clock: %d", ret);
		return -EIO;
	}

	/* Initialize NemaDC */
	ret = nemadc_init();
	if (ret != 0) {
		LOG_ERR("DC init failed!\n");
		return -EFAULT;
	}

	/* Enable global interrupts */
	am_hal_interrupt_master_enable();

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Set default the pixel format */
	data->dc_config.ui32PixelFormat = MIPICFG_PF_DBI16 | MIPICFG_PF_OPT0 | MIPI_DCS_RGB565;
	data->dc_config.bTEEnable = false;
	data->dc_config.eInterface = DISP_INTERFACE_DBI;
	/* The WRX frequency is the half of format clock. */
	data->dc_config.fCLKMaxFreq = (float)config->clock_frequency / 1000000 * 2;

	nemadc_configure(&data->dc_config);

	data->mode = 0;
	data->pixfmt = 0;

	/* The partial pixel format of DC */
	switch (data->dc_config.ui32PixelFormat & 0x7) {
	case MIPI_DCS_RGB565:
		data->pixfmt = PIXEL_FORMAT_RGB_565;
		data->dc_layer.format = NEMADC_RGB565;
		break;
	case MIPI_DCS_RGB888:
		data->pixfmt = PIXEL_FORMAT_RGB_888;
		data->dc_layer.format = NEMADC_RGB24;
		break;
	default:
		LOG_ERR("Invalid color coding!\n");
		return -ENOTSUP;
	}

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

	return ret;
}

static DEVICE_API(mipi_dbi, mipi_dbi_ambiq_driver_api) = {
	.command_write = mipi_dbi_ambiq_command_write,
	.command_read = mipi_dbi_ambiq_command_read,
	.write_display = mipi_dbi_ambiq_write_display,
};

/*
 * Ambiq DC interrupt service routine
 */
extern void am_disp_isr(void);

#define AMBIQ_MIPI_DBI_DEVICE(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void disp_##n##_irq_config_func(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), am_disp_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct mipi_dbi_ambiq_config mipi_dbi_ambiq_config_##n = {                    \
		.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 0),                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = disp_##n##_irq_config_func,                                     \
	};                                                                                         \
	static struct mipi_dbi_ambiq_data mipi_dbi_ambiq_data_##n = {                              \
		.dc_config = {.ui16ResX = DT_INST_PROP_OR(n, hactive, 0),                          \
			      .ui32FrontPorchX = DT_INST_PROP_OR(n, hfp, 1),                       \
			      .ui32BackPorchX = DT_INST_PROP_OR(n, hbp, 1),                        \
			      .ui32BlankingX = DT_INST_PROP_OR(n, hsync, 1),                       \
			      .ui16ResY = DT_INST_PROP_OR(n, vactive, 0),                          \
			      .ui32FrontPorchY = DT_INST_PROP_OR(n, vfp, 1),                       \
			      .ui32BackPorchY = DT_INST_PROP_OR(n, vbp, 1),                        \
			      .ui32BlankingY = DT_INST_PROP_OR(n, vsync, 1)}};                     \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_ambiq_init, NULL, &mipi_dbi_ambiq_data_##n,              \
			      &mipi_dbi_ambiq_config_##n, POST_KERNEL,                             \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_MIPI_DBI_DEVICE)
