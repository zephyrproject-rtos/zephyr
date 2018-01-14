/*
 * Copyright (c) 2018 dXplore
 * Copyright (c) 2017 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "spi"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <nrf.h>
#include <nrf5_common.h>
#include <spi.h>

#include "spi_context.h"

#define CS_UNUSED 255

struct spi_nrf52_master_config {
	volatile NRF_SPIM_Type *base;
	void (*irq_config_func)(struct device *dev);
	struct {
		u8_t sck;
		u8_t mosi;
		u8_t miso;
		u8_t cs;
	} psel;
	u8_t orc;
	u32_t max_freq;
};

struct spi_nrf52_master_data {
	struct spi_context ctx;
};

static int spi_configure(struct spi_config *config)
{
	struct spi_nrf52_master_data *data = config->dev->driver_data;
	const struct spi_nrf52_master_config *cfg =
	    config->dev->config->config_info;
	volatile NRF_SPIM_Type *spim = cfg->base;
	u32_t frequency = config->frequency;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		SYS_LOG_ERR("Unsupported word size\n");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_LOOP) {
		SYS_LOG_ERR("Loopback unsupported\n");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		SYS_LOG_ERR("Salve mode unsupported\n");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LINES_DUAL) {
		SYS_LOG_ERR("Dual line mode unsupported\n");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LINES_QUAD) {
		SYS_LOG_ERR("Quad line mode unsupported\n");
		return -ENOTSUP;
	}

	if (frequency > cfg->max_freq) {
		frequency = cfg->max_freq;
	}

	if (frequency < 125000) {
		SYS_LOG_ERR("Unsupported Frequency\n");
		return -ENOTSUP;
	}

	if (frequency < 250000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_K125;
	} else if (frequency < 500000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_K250;
	} else if (frequency < 1000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_K500;
	} else if (frequency < 2000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_M1;
	} else if (frequency < 4000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_M2;
	} else if (frequency < 8000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_M4;
#ifdef CONFIG_SOC_NRF52840
	} else if (frequency < 16000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_M8;
	} else if (frequency < 32000000) {
		frequency = SPIM_FREQUENCY_FREQUENCY_M16;
	} else {
		frequency = SPIM_FREQUENCY_FREQUENCY_M32;
	}
#else
	} else {
		frequency = SPIM_FREQUENCY_FREQUENCY_M8;
	}
#endif

	spim->ENABLE = SPIM_ENABLE_ENABLE_Disabled;

	spim->INTENCLR = 0xffffffff;

	spim->SHORTS = 0;

	spim->ORC = cfg->orc;

	spim->TXD.LIST = 0;
	spim->RXD.LIST = 0;

	spim->TXD.MAXCNT = 0;
	spim->RXD.MAXCNT = 0;

	spim->EVENTS_END = 0;
	spim->EVENTS_ENDTX = 0;
	spim->EVENTS_ENDRX = 0;
	spim->EVENTS_STOPPED = 0;
	spim->EVENTS_STARTED = 0;

	spim->FREQUENCY = frequency;

	spim->CONFIG =
	    (config->operation & SPI_TRANSFER_LSB)
		? (SPIM_CONFIG_ORDER_LsbFirst << SPIM_CONFIG_ORDER_Pos)
		: (SPIM_CONFIG_ORDER_MsbFirst << SPIM_CONFIG_ORDER_Pos);
	spim->CONFIG |=
	    (config->operation & SPI_MODE_CPOL)
		? (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos)
		: (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos);
	spim->CONFIG |=
	    (config->operation & SPI_MODE_CPHA)
		? (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos)
		: (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);

	if ((config->cs != NULL) || (cfg->psel.cs == CS_UNUSED)) {
#ifdef CONFIG_SOC_NRF52840
		/* Disable HW based CS */
		spim->PSEL.CSN |= SPIM_PSEL_CSN_CONNECT_Msk;
#endif
		data->ctx.config = config;
		spi_context_cs_configure(&data->ctx);
	} else {
#ifdef CONFIG_SOC_NRF52840
		/* Enable HW based CS */
		spim->PSEL.CSN &= ~SPIM_PSEL_CSN_CONNECT_Msk;
#endif
		data->ctx.config = NULL;
	}

	spim->INTENSET = SPIM_INTENSET_END_Msk;

	return 0;
}

static int transceive_current_buffer(volatile NRF_SPIM_Type *spim,
				     struct spi_context *ctx)
{
	if (spi_context_tx_on(ctx)) {
		__ASSERT_NO_MSG(ctx->tx_buf);
		if (ctx->tx_len > 255) {
			spim->TXD.MAXCNT = 255;
		} else {
			spim->TXD.MAXCNT = ctx->tx_len;
		}
		spim->TXD.PTR = (u32_t)ctx->tx_buf;
	} else {
		spim->TXD.MAXCNT = 0;
	}

	if (spi_context_rx_on(ctx)) {
		__ASSERT_NO_MSG(ctx->rx_buf);
		if (ctx->rx_len > 255) {
			spim->RXD.MAXCNT = 255;
		} else {
			spim->RXD.MAXCNT = ctx->rx_len;
		}
		spim->RXD.PTR = (u32_t)ctx->rx_buf;
	} else {
		spim->RXD.MAXCNT = 0;
	}

	spim->TASKS_START = 1;
	return 0;
}

static int transceive(struct spi_config *config, const struct spi_buf *tx_bufs,
		      u32_t tx_count, struct spi_buf *rx_bufs, u32_t rx_count,
		      bool asynchronous, struct k_poll_signal *signal)
{
	struct spi_nrf52_master_data *data = config->dev->driver_data;
	const struct spi_nrf52_master_config *cfg =
	    config->dev->config->config_info;
	volatile NRF_SPIM_Type *spim = cfg->base;
	int ret;

	if (!tx_count && !rx_count) {
		return 0;
	}

	spi_context_lock(&data->ctx, asynchronous, signal);

	ret = spi_configure(config);
	if (ret != 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	/* Set buffers info */
	spi_context_buffers_setup(&data->ctx, tx_bufs, tx_count, rx_bufs,
				  rx_count, 1);

	if (spim->ENABLE) {
		return -EALREADY;
	}

	spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled;

	spim->INTENSET = SPIM_INTENSET_END_Msk;

	spi_context_cs_control(&data->ctx, true);

	transceive_current_buffer(spim, &data->ctx);

	ret = spi_context_wait_for_completion(&data->ctx);
	if (ret != 0) {
		SYS_LOG_ERR("error mask 0x%x", ret);
	}

	spi_context_release(&data->ctx, ret);

	return ret;
}

static int transceive_api(struct spi_config *config,
			  const struct spi_buf *tx_bufs, size_t tx_count,
			  struct spi_buf *rx_bufs, size_t rx_count)
{
	return transceive(config, tx_bufs, tx_count, rx_bufs, rx_count, false,
			  NULL);
}

#ifdef CONFIG_POLL
static int transceive_async_api(struct spi_config *config,
				const struct spi_buf *tx_bufs, size_t tx_count,
				struct spi_buf *rx_bufs, size_t rx_count,
				struct k_poll_signal *async)
{
	return transceive(config, tx_bufs, tx_count, rx_bufs, rx_count, true,
			  async);
}
#endif /* CONFIG_POLL */

static int release_api(struct spi_config *config)
{
	struct spi_nrf52_master_data *data = config->dev->driver_data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static const struct spi_driver_api spi_nrf52_master_api = {
	.transceive = transceive_api,
#ifdef CONFIG_POLL
	.transceive_async = transceive_async_api,
#endif /* CONFIG_POLL */
	.release = release_api,
};

static int spi_nrf52_master_init(struct device *dev)
{
	const struct spi_nrf52_master_config *config = dev->config->config_info;
	volatile NRF_SPIM_Type *spi = config->base;
	struct spi_nrf52_master_data *data = dev->driver_data;
	int status;

	SYS_LOG_DBG("Init %s", dev->config->name);

	struct device *gpio_port =
	    device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	status = gpio_pin_configure(gpio_port, config->psel.sck, GPIO_DIR_OUT);
	__ASSERT_NO_MSG(status == 0);

	status = gpio_pin_configure(gpio_port, config->psel.mosi, GPIO_DIR_OUT);
	__ASSERT_NO_MSG(status == 0);

	status = gpio_pin_configure(gpio_port, config->psel.miso, GPIO_DIR_IN);
	__ASSERT_NO_MSG(status == 0);

	spi->PSEL.SCK = config->psel.sck;
	spi->PSEL.MOSI = config->psel.mosi;
	spi->PSEL.MISO = config->psel.miso;

#ifdef CONFIG_SOC_NRF52840
	if (config->psel.cs != CS_UNUSED) {
		status = gpio_pin_configure(gpio_port, config->psel.cs,
					    GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
		__ASSERT_NO_MSG(status == 0);
		spi->PSEL.CSN = config->psel.cs;
	} else {
		spi->PSEL.CSN = CS_UNUSED;
	}
#endif

	config->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static void spi_nrf52_master_isr(void *arg)
{
	struct device *dev = arg;
	const struct spi_nrf52_master_config *config = dev->config->config_info;
	struct spi_nrf52_master_data *data = dev->driver_data;
	volatile NRF_SPIM_Type *spim = config->base;

	if (spim->EVENTS_END) {
		spim->EVENTS_END = 0;
		spi_context_update_tx(&data->ctx, 1, spim->TXD.AMOUNT);
		spi_context_update_rx(&data->ctx, 1, spim->RXD.AMOUNT);

		if (spi_context_tx_on(&data->ctx) ||
		    spi_context_rx_on(&data->ctx)) {
			transceive_current_buffer(spim, &data->ctx);
		} else {
			spi_context_cs_control(&data->ctx, false);
			spim->INTENCLR = 0xffffffff;
			spim->ENABLE = SPIM_ENABLE_ENABLE_Disabled;
			spi_context_complete(&data->ctx, 0);
		}
	}
}

#if defined(CONFIG_SPI0_NRF52_MASTER)
static void spi_nrf52_master_irq_config_0(struct device *dev);

static const struct spi_nrf52_master_config spi_nrf52_master_config_0 = {
	.base = NRF_SPIM0,
	.irq_config_func = spi_nrf52_master_irq_config_0,
	.psel = {
		.sck = CONFIG_SPI0_NRF52_SCK_PIN,
		.mosi = CONFIG_SPI0_NRF52_MOSI_PIN,
		.miso = CONFIG_SPI0_NRF52_MISO_PIN,
		.cs = CS_UNUSED,
	},
	.orc = CONFIG_SPI0_NRF52_ORC,
	.max_freq = 8000000,
};

static struct spi_nrf52_master_data spi_nrf52_master_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_nrf52_master_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_nrf52_master_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_nrf52_master_0, CONFIG_SPI_0_NAME,
		    spi_nrf52_master_init, &spi_nrf52_master_data_0,
		    &spi_nrf52_master_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_nrf52_master_api);

static void spi_nrf52_master_irq_config_0(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI0_TWI0_IRQn, CONFIG_SPI_0_IRQ_PRI,
		    spi_nrf52_master_isr, DEVICE_GET(spi_nrf52_master_0), 0);
	irq_enable(NRF5_IRQ_SPI0_TWI0_IRQn);
}
#endif /* CONFIG_SPI0_NRF52_MASTER */

#if defined(CONFIG_SPI1_NRF52_MASTER)
static void spi_nrf52_master_irq_config_1(struct device *dev);

static const struct spi_nrf52_master_config spi_nrf52_master_config_1 = {
	.base = NRF_SPIM1,
	.irq_config_func = spi_nrf52_master_irq_config_1,
	.psel = {
		.sck = CONFIG_SPI1_NRF52_SCK_PIN,
		.mosi = CONFIG_SPI1_NRF52_MOSI_PIN,
		.miso = CONFIG_SPI1_NRF52_MISO_PIN,
		.cs = CS_UNUSED,
	},
	.orc = CONFIG_SPI1_NRF52_ORC,
	.max_freq = 8000000,
};

static struct spi_nrf52_master_data spi_nrf52_master_data_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_nrf52_master_data_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_nrf52_master_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_nrf52_master_1, CONFIG_SPI_1_NAME,
		    spi_nrf52_master_init, &spi_nrf52_master_data_1,
		    &spi_nrf52_master_config_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_nrf52_master_api);

static void spi_nrf52_master_irq_config_1(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI1_TWI1_IRQn, CONFIG_SPI_1_IRQ_PRI,
		    spi_nrf52_master_isr, DEVICE_GET(spi_nrf52_master_1), 0);
	irq_enable(NRF5_IRQ_SPI1_TWI1_IRQn);
}
#endif /* CONFIG_SPI1_NRF52_MASTER */

#if defined(CONFIG_SPI2_NRF52_MASTER)
static void spi_nrf52_master_irq_config_2(struct device *dev);

static const struct spi_nrf52_master_config spi_nrf52_master_config_2 = {
	.base = NRF_SPIM2,
	.irq_config_func = spi_nrf52_master_irq_config_2,
	.psel = {
		.sck = CONFIG_SPI2_NRF52_SCK_PIN,
		.mosi = CONFIG_SPI2_NRF52_MOSI_PIN,
		.miso = CONFIG_SPI2_NRF52_MISO_PIN,
		.cs = CS_UNUSED,
	},
	.orc = CONFIG_SPI2_NRF52_ORC,
	.max_freq = 8000000,
};

static struct spi_nrf52_master_data spi_nrf52_master_data_2 = {
	SPI_CONTEXT_INIT_LOCK(spi_nrf52_master_data_2, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_nrf52_master_data_2, ctx),
};

DEVICE_AND_API_INIT(spi_nrf52_master_2, CONFIG_SPI_2_NAME,
		    spi_nrf52_master_init, &spi_nrf52_master_data_2,
		    &spi_nrf52_master_config_2, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_nrf52_master_api);

static void spi_nrf52_master_irq_config_2(struct device *dev)
{
	IRQ_CONNECT(NRF52_IRQ_SPIM2_SPIS2_SPI2_IRQn, CONFIG_SPI_2_IRQ_PRI,
		    spi_nrf52_master_isr, DEVICE_GET(spi_nrf52_master_2), 0);
	irq_enable(NRF52_IRQ_SPIM2_SPIS2_SPI2_IRQn);
}
#endif /* CONFIG_SPI2_NRF52_MASTER */


#if defined(CONFIG_SPI3_NRF52_MASTER)
static void spi_nrf52_master_irq_config_3(struct device *dev);

static const struct spi_nrf52_master_config spi_nrf52_master_config_3 = {
	.base = NRF_SPIM3,
	.irq_config_func = spi_nrf52_master_irq_config_3,
	.psel = {
		.sck = CONFIG_SPI3_NRF52_SCK_PIN,
		.mosi = CONFIG_SPI3_NRF52_MOSI_PIN,
		.miso = CONFIG_SPI3_NRF52_MISO_PIN,
		.cs = CONFIG_SPI3_NRF52_CS_PIN,
	},
	.orc = CONFIG_SPI3_NRF52_ORC,
	.max_freq = 32000000,
};

static struct spi_nrf52_master_data spi_nrf52_master_data_3 = {
	SPI_CONTEXT_INIT_LOCK(spi_nrf52_master_data_3, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_nrf52_master_data_3, ctx),
};

DEVICE_AND_API_INIT(spi_nrf52_master_3, CONFIG_SPI_3_NAME,
		    spi_nrf52_master_init, &spi_nrf52_master_data_3,
		    &spi_nrf52_master_config_3, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_nrf52_master_api);

static void spi_nrf52_master_irq_config_3(struct device *dev)
{
	IRQ_CONNECT(NRF52_IRQ_SPIM3_IRQn, CONFIG_SPI_3_IRQ_PRI,
		    spi_nrf52_master_isr, DEVICE_GET(spi_nrf52_master_3), 0);
	irq_enable(NRF52_IRQ_SPIM3_IRQn);
}
#endif /* CONFIG_SPI3_NRF52_MASTER */
