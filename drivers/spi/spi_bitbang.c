/*
 * Copyright (c) 2021 Marc Reilly - Creative Product Design
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_spi_bitbang

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_bitbang);

#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/spi.h>
#include "spi_context.h"

struct spi_bitbang_data {
	struct spi_context ctx;
	int bits;
	int wait_us;
	int dfs;
};

struct spi_bitbang_config {
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec mosi_gpio;
	struct gpio_dt_spec miso_gpio;
};

static int spi_bitbang_configure(const struct spi_bitbang_config *info,
			    struct spi_bitbang_data *data,
			    const struct spi_config *config)
{
	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_TRANSFER_LSB | SPI_LINES_DUAL
			| SPI_LINES_QUAD)) {
		LOG_ERR("Unsupported configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits > 16) {
		LOG_ERR("Word sizes > 16 bits not supported");
		return -ENOTSUP;
	}

	data->bits = bits;
	data->dfs = ((data->bits - 1) / 8) + 1;
	if (config->frequency > 0) {
		/* convert freq to period, the extra /2 is due to waiting
		 * twice in each clock cycle. The '2000' is an upscale factor.
		 */
		data->wait_us = (1000000ul * 2000ul / config->frequency) / 2000ul;
		data->wait_us /= 2;
	} else {
		data->wait_us = 8 / 2; /* 125 kHz */
	}

	data->ctx.config = config;

	return 0;
}

static int spi_bitbang_transceive(const struct device *dev,
			      const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	const struct spi_bitbang_config *info = dev->config;
	struct spi_bitbang_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;
	const struct gpio_dt_spec *miso = NULL;
	const struct gpio_dt_spec *mosi = NULL;
	gpio_flags_t mosi_flags = GPIO_OUTPUT_INACTIVE;

	rc = spi_bitbang_configure(info, data, spi_cfg);
	if (rc < 0) {
		return rc;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		if (!info->mosi_gpio.port) {
			LOG_ERR("No MOSI pin specified in half duplex mode");
			return -EINVAL;
		}

		if (tx_bufs && rx_bufs) {
			LOG_ERR("Both RX and TX specified in half duplex mode");
			return -EINVAL;
		} else if (tx_bufs && !rx_bufs) {
			/* TX mode */
			mosi = &info->mosi_gpio;
		} else if (!tx_bufs && rx_bufs) {
			/* RX mode */
			mosi_flags = GPIO_INPUT;
			miso = &info->mosi_gpio;
		}
	} else {
		if (info->mosi_gpio.port) {
			mosi = &info->mosi_gpio;
		}

		if (info->miso_gpio.port) {
			miso = &info->miso_gpio;
		}
	}

	if (info->mosi_gpio.port) {
		rc = gpio_pin_configure_dt(&info->mosi_gpio, mosi_flags);
		if (rc < 0) {
			LOG_ERR("Couldn't configure MOSI pin: %d", rc);
			return rc;
		}
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);

	int clock_state = 0;
	int cpha = 0;
	bool loop = false;

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		clock_state = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		cpha = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		loop = true;
	}

	/* set the initial clock state before CS */
	gpio_pin_set_dt(&info->clk_gpio, clock_state);

	spi_context_cs_control(ctx, true);

	const uint32_t wait_us = data->wait_us;

	while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
		uint16_t w = 0;

		if (ctx->tx_len) {
			switch (data->dfs) {
			case 2:
				w = *(uint16_t *)(ctx->tx_buf);
				break;
			case 1:
				w = *(uint8_t *)(ctx->tx_buf);
				break;
			}
		}

		int shift = data->bits - 1;
		uint16_t r = 0;
		int b = 0;
		bool do_read = false;

		if (miso && spi_context_rx_buf_on(ctx)) {
			do_read = true;
		}

		while (shift >= 0) {
			const int d = (w >> shift) & 0x1;

			b = 0;

			/* setup data out first thing */
			if (mosi) {
				gpio_pin_set_dt(mosi, d);
			}

			k_busy_wait(wait_us);

			/* first clock edge */
			gpio_pin_set_dt(&info->clk_gpio, !clock_state);

			if (!loop && do_read && !cpha) {
				b = gpio_pin_get_dt(miso);
			}

			k_busy_wait(wait_us);

			/* second clock edge */
			gpio_pin_set_dt(&info->clk_gpio, clock_state);

			if (!loop && do_read && cpha) {
				b = gpio_pin_get_dt(miso);
			}

			if (loop) {
				b = d;
			}

			r = (r << 1) | (b ? 0x1 : 0x0);

			--shift;
		}

		if (spi_context_rx_buf_on(ctx)) {
			switch (data->dfs) {
			case 2:
				*(uint16_t *)(ctx->rx_buf) = r;
				break;
			case 1:
				*(uint8_t *)(ctx->rx_buf) = r;
				break;
			}
		}

		LOG_DBG(" w: %04x, r: %04x , do_read: %d", w, r, do_read);

		spi_context_update_tx(ctx, data->dfs, 1);
		spi_context_update_rx(ctx, data->dfs, 1);
	}

	spi_context_cs_control(ctx, false);

	spi_context_complete(ctx, dev, 0);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spi_bitbang_transceive_async(const struct device *dev,
				    const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

int spi_bitbang_release(const struct device *dev,
			  const struct spi_config *config)
{
	struct spi_bitbang_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);
	return 0;
}

static struct spi_driver_api spi_bitbang_api = {
	.transceive = spi_bitbang_transceive,
	.release = spi_bitbang_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_bitbang_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

int spi_bitbang_init(const struct device *dev)
{
	const struct spi_bitbang_config *config = dev->config;
	struct spi_bitbang_data *data = dev->data;
	int rc;

	if (!gpio_is_ready_dt(&config->clk_gpio)) {
		LOG_ERR("GPIO port for clk pin is not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&config->clk_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Couldn't configure clk pin; (%d)", rc);
		return rc;
	}

	if (config->mosi_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->mosi_gpio)) {
			LOG_ERR("GPIO port for mosi pin is not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(&config->mosi_gpio,
				GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("Couldn't configure mosi pin; (%d)", rc);
			return rc;
		}
	}

	if (config->miso_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->miso_gpio)) {
			LOG_ERR("GPIO port for miso pin is not ready");
			return -ENODEV;
		}


		rc = gpio_pin_configure_dt(&config->miso_gpio, GPIO_INPUT);
		if (rc < 0) {
			LOG_ERR("Couldn't configure miso pin; (%d)", rc);
			return rc;
		}
	}

	rc = spi_context_cs_configure_all(&data->ctx);
	if (rc < 0) {
		LOG_ERR("Failed to configure CS pins: %d", rc);
		return rc;
	}

	return 0;
}

#define SPI_BITBANG_INIT(inst)						\
	static struct spi_bitbang_config spi_bitbang_config_##inst = {	\
		.clk_gpio = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),	\
		.mosi_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mosi_gpios, {0}),	\
		.miso_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, miso_gpios, {0}),	\
	};								\
									\
	static struct spi_bitbang_data spi_bitbang_data_##inst = {	\
		SPI_CONTEXT_INIT_LOCK(spi_bitbang_data_##inst, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_bitbang_data_##inst, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    spi_bitbang_init,				\
			    NULL,					\
			    &spi_bitbang_data_##inst,			\
			    &spi_bitbang_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spi_bitbang_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_BITBANG_INIT)
