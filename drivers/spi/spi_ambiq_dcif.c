/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_spi_dcif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_ambiq_dcif, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dt-bindings/spi/spi.h>

#include <stdlib.h>
#include <errno.h>
#include "spi_context.h"
#include <soc.h>

#include <am_mcu_apollo.h>
#include <nema_dc_regs.h>
#include <nema_dc_hal.h>
#include <nema_dc_mipi.h>
#include <nema_dc_intern.h>
#include <nema_dc.h>

#define SPI_WORD_SIZE         8
#define SPI_COLOR_FORMAT_MASK 0x7

struct spi_ambiq_config {
	const struct pinctrl_dev_config *pcfg;
	bool disp_te;
	void (*irq_config_func)(const struct device *dev);
};

struct spi_ambiq_data {
	struct spi_context ctx;
	nemadc_layer_t dc_layer;
	nemadc_initial_config_t dc_config;
	bool isframe;
	uint8_t command;
};

/* Helper to configure hardware based on spi_config */
static int spi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_ambiq_config *cfg = dev->config;
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &(data->ctx);
	uint32_t ui32Config;

	if (!config) {
		LOG_ERR("SPI config is NULL");
		return -EINVAL;
	}

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	ui32Config = data->dc_config.ui32PixelFormat & SPI_COLOR_FORMAT_MASK;

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Operational mode must be SPI_OP_MODE_MASTER");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		LOG_ERR("SPI_MODE_LOOP is not available");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		ui32Config |= MIPICFG_SPI_CPOL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		ui32Config |= MIPICFG_SPI_CPHA;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB-first transfer is not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if (!(config->operation & SPI_HALF_DUPLEX)) {
		LOG_ERR("The full-duplex mode is not available");
		return -ENOTSUP;
	}

	if (config->operation & SPI_HOLD_ON_CS) {
		ui32Config |= MIPICFG_FRC_CSX_0;
	} else {
		ui32Config &= ~MIPICFG_FRC_CSX_0;
	}

	ui32Config |= MIPICFG_SPI4 | MIPICFG_PF_OPT0;
	if ((config->operation & SPI_LINES_MASK) == SPI_LINES_QUAD) {
		ui32Config |= MIPICFG_QSPI | MIPICFG_PF_QSPI;
		data->dc_config.eInterface = DISP_INTERFACE_QSPI;
	} else if ((config->operation & SPI_LINES_MASK) == SPI_LINES_DUAL) {
		ui32Config |= MIPICFG_DSPI | MIPICFG_PF_DSPI;
		data->dc_config.eInterface = DISP_INTERFACE_DSPI;
	} else if ((config->operation & SPI_LINES_MASK) == SPI_LINES_SINGLE) {
		ui32Config |= MIPICFG_PF_SPI;
		data->dc_config.eInterface = DISP_INTERFACE_SPI4;
	} else {
		LOG_ERR("Unsupported SPI lines mode");
		return -ENOTSUP;
	}

	if (!(config->operation & SPI_CS_ACTIVE_HIGH)) {
		ui32Config |= MIPICFG_SPI_CSX_V;
	}

	if (config->operation & SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame format is not supported");
		return -ENOTSUP;
	}

	if (config->frequency == 0) {
		LOG_ERR("Invalid SPI frequency");
		return -ENOTSUP;
	}

	data->dc_config.fCLKMaxFreq = (float)config->frequency / 1000000;

	if ((ui32Config & SPI_COLOR_FORMAT_MASK) == MIPI_DCS_RGB888) {
		data->dc_layer.format = NEMADC_RGB24;
	} else if ((ui32Config & SPI_COLOR_FORMAT_MASK) == MIPI_DCS_RGB565) {
		data->dc_layer.format = NEMADC_RGB565;
	} else {
		LOG_ERR("Unsupported color format");
		return -ENOTSUP;
	}

	data->dc_config.ui32PixelFormat = ui32Config | MIPICFG_DBI_EN | MIPICFG_RESX;

	data->dc_config.bTEEnable = cfg->disp_te;

	nemadc_configure(&data->dc_config);

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

	data->isframe = false;
	data->command = 0;

	ctx->config = config;
	return 0;
}

static int spi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &(data->ctx);
	int ret = 0;
	uint8_t *pointer;

	if (ctx->tx_len) {
		if (data->isframe) {
			data->isframe = false;
			nemadc_timing(data->dc_layer.resx, data->dc_config.ui32FrontPorchX,
				      data->dc_config.ui32BlankingX, data->dc_config.ui32BackPorchX,
				      data->dc_layer.resy, data->dc_config.ui32FrontPorchY,
				      data->dc_config.ui32BlankingY, data->dc_config.ui32BlankingY);
			data->dc_layer.stride =
				nemadc_stride_size(data->dc_layer.format, data->dc_layer.resx);
			data->dc_layer.baseaddr_virt = (void *)ctx->tx_buf;
			data->dc_layer.baseaddr_phys = (unsigned int)(data->dc_layer.baseaddr_virt);
			nemadc_set_layer(0, &data->dc_layer);
			if (data->command == MIPI_DCS_WRITE_MEMORY_START) {
				nemadc_transfer_frame_prepare(data->dc_config.bTEEnable);
				if (!data->dc_config.bTEEnable) {
					nemadc_transfer_frame_launch();
				}
			} else {
				nemadc_transfer_frame_continue(false);
				nemadc_transfer_frame_launch();
			}
			nemadc_wait_vsync();
			goto spi_complete;
		}
		data->command = ctx->tx_buf[0];
		if (((data->command == MIPI_DCS_WRITE_MEMORY_START) ||
		     (data->command == MIPI_DCS_WRITE_MEMORY_CONTINUE)) &&
		    (ctx->tx_len == 1)) {
			data->isframe = true;
		} else {
			ret = nemadc_mipi_cmd_write(data->command, (uint8_t *)&(ctx->tx_buf[1]),
						    ctx->tx_len - 1, true, false);
			pointer = (uint8_t *)&(ctx->tx_buf[1]);
			if (data->command == MIPI_DCS_SET_COLUMN_ADDRESS) {
				if (ctx->tx_len < 5) {
					LOG_ERR("Invalid column address data length");
					return -EINVAL;
				}
				data->dc_layer.resx = (int32_t)((pointer[2] << 8) | pointer[3]) +
						      1 - (int32_t)((pointer[0] << 8) | pointer[1]);
				data->dc_layer.stride = nemadc_stride_size(data->dc_layer.format,
									   data->dc_layer.resx);
			} else if (data->command == MIPI_DCS_SET_PAGE_ADDRESS) {
				if (ctx->tx_len < 5) {
					LOG_ERR("Invalid page address data length");
					return -EINVAL;
				}
				data->dc_layer.resy = (int32_t)((pointer[2] << 8) | pointer[3]) +
						      1 - (int32_t)((pointer[0] << 8) | pointer[1]);
			}
		}
	}

spi_complete:
	spi_context_complete(ctx, dev, 0);

	return ret;
}

static int spi_ambiq_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_ambiq_data *data = dev->data;
	int ret = 0;

	ret = spi_configure(dev, config);
	if (ret) {
		LOG_ERR("SPI configuration failed: %d", ret);
		return ret;
	}

	if (!tx_bufs && !rx_bufs) {
		LOG_WRN("No buffers provided for transceive");
		return 0;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ret = spi_ambiq_xfer(dev, config);
	if (ret) {
		LOG_ERR("SPI transfer failed: %d", ret);
	}

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_ambiq_transceive_async(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				      void *userdata)
{
	LOG_ERR("Asynchronous SPI not supported");
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_ambiq_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_ambiq_driver_api) = {
	.transceive = spi_ambiq_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_ambiq_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_ambiq_release,
};

static int spi_ambiq_init(const struct device *dev)
{
	const struct spi_ambiq_config *config = dev->config;
	int ret;

	/* Apply default pinmux configuration */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	/* Enable display peripheral power */
	ret = am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISP);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable display peripheral power: %d", ret);
		return -EIO;
	}

	/* Configure display clock */
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
	if (nemadc_init() != 0) {
		LOG_ERR("NemaDC initialization failed");
		return -EFAULT;
	}

	/* Enable global interrupts */
	am_hal_interrupt_master_enable();

	/* Configure interrupts */
	config->irq_config_func(dev);

	return ret;
}

/*
 * Ambiq DC interrupt service routine
 */
extern void am_disp_isr(void);

/* Macro to define instances */
#define SPI_AMBIQ_DEFINE(id)                                                                       \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static void disp_##id##_irq_config_func(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), am_disp_isr,              \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}                                                                                          \
	static const struct spi_ambiq_config spi_ambiq_cfg_##id = {                                \
		.disp_te = DT_INST_PROP(id, disp_te),                                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
		.irq_config_func = disp_##id##_irq_config_func,                                    \
	};                                                                                         \
	static struct spi_ambiq_data spi_ambiq_data_##id = {                                       \
		.dc_config = {.ui16ResX = DT_INST_PROP_OR(id, hactive, 0),                         \
			      .ui32FrontPorchX = DT_INST_PROP_OR(id, hfp, 1),                      \
			      .ui32BackPorchX = DT_INST_PROP_OR(id, hbp, 1),                       \
			      .ui32BlankingX = DT_INST_PROP_OR(id, hsync, 1),                      \
			      .ui16ResY = DT_INST_PROP_OR(id, vactive, 0),                         \
			      .ui32FrontPorchY = DT_INST_PROP_OR(id, vfp, 1),                      \
			      .ui32BackPorchY = DT_INST_PROP_OR(id, vbp, 1),                       \
			      .ui32BlankingY = DT_INST_PROP_OR(id, vsync, 1),                      \
			      .ui32PixelFormat = DT_INST_ENUM_IDX(id, pixfmt)}};                   \
	DEVICE_DT_INST_DEFINE(id, spi_ambiq_init, NULL, &spi_ambiq_data_##id, &spi_ambiq_cfg_##id, \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_AMBIQ_DEFINE)
