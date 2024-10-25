/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <ipc/ipc_based_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_telink);

#include "spi_context.h"

/* Driver dts compatibility: telink,w91_spi */
#define DT_DRV_COMPAT telink_w91_spi

enum {
	IPC_DISPATCHER_SPI_INIT = IPC_DISPATCHER_SPI,
	IPC_DISPATCHER_SPI_CONFIG,
	IPC_DISPATCHER_SPI_MASTER_TX_RX,
};

enum {
	SPI0_INST,
	SPI1_INST,
	SPI2_INST,
};

#define SPI_WORD_SIZE          8u
#define SPI_TX_RX_BUFFER_SIZE  512u

#define SPI_CLK_XTAL       40000000u
#define SPI0_SPI1_CLK_PLL  240000000u
#define SPI2_CLK_PLL       80000000u

enum spi_clk_src {
	/* spi0, spi1 and spi2 use XTAL 40Mhz */
	SPI_CLK_SRC_XTAL,
	/* spi0 and spi1 use 240Mhz, spi2 uses 80Mhz */
	SPI_CLK_SRC_PLL,
};

enum spi_mode {
	SPI_MODE_0, /* active high, odd edge sampling */
	SPI_MODE_1, /* active high, even edge sampling */
	SPI_MODE_2, /* active low, odd edge sampling */
	SPI_MODE_3, /* active low, even edge sampling */
};

enum spi_data_io_format {
	SPI_DATA_IO_FORMAT_SINGLE,
	SPI_DATA_IO_FORMAT_DUAL,
	SPI_DATA_IO_FORMAT_QUAD,
};

enum spi_bit_order {
	SPI_BIT_MSB_FIRST,
	SPI_BIT_LSB_FIRST,
};

/* SPI configuration structure */
struct spi_w91_config {
	const struct pinctrl_dev_config *pcfg;
	uint8_t instance_id;
};

/* SPI data structure */
struct spi_w91_data {
	struct spi_context ctx;
	struct spi_config config;
	struct k_mutex mutex;
	struct ipc_based_driver ipc; /* ipc driver part */
};

struct spi_w91_config_req {
	uint8_t role;
	uint8_t clk_src;
	uint8_t clk_div_2mul;
	uint8_t mode;
	uint8_t data_io_format;
	uint8_t bit_order;
};

struct spi_w91_master_tx_rx_req {
	uint32_t rx_len;
	uint32_t tx_len;
	uint8_t *tx_buffer;
};

struct spi_w91_master_tx_rx_resp {
	int err;
	uint32_t rx_len;
	uint8_t *rx_buffer;
};

#define SPI_TX_RX_MAX_SIZE_IN_PACK                                                              \
((CONFIG_PBUF_RX_READ_BUF_SIZE) - sizeof(uint32_t) - sizeof(uint32_t) - sizeof(uint32_t))

/* Check for supported configuration */
static bool spi_w91_is_config_supported(const struct spi_config *config)
{
	/* check for half-duplex */
	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return false;
	}

	/* check for loop back */
	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loop back mode not supported");
		return false;
	}

	/* check for transfer LSB first */
	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return false;
	}

	/* check word size */
	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return false;
	}

	/* check for CS active high */
	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported for HW flow control");
		return false;
	}

	/* check for lines configuration */
	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		if ((config->operation & SPI_LINES_MASK) == SPI_LINES_OCTAL) {
			LOG_ERR("SPI lines Octal is not supported");
			return false;
		}
	}

	/* check for slave configuration */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("SPI Slave is not implemented");
		return false;
	}

	return true;
}

/* Check if it is new configuration */
static bool spi_w91_is_new_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_w91_data *data = dev->data;

	/* check for frequency */
	if (data->config.frequency != config->frequency) {
		return true;
	}

	/* check for operation */
	if (data->config.operation != config->operation) {
		return true;
	}

	return false;
}

/* Save configuration */
static void spi_w91_save_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_w91_data *data = dev->data;

	data->config.frequency = config->frequency;
	data->config.operation = config->operation;
}

/* APIs implementation: SPI configure */
static size_t pack_spi_w91_ipc_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct spi_w91_config_req *p_config_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_config_req->role) +
		sizeof(p_config_req->clk_src) + sizeof(p_config_req->clk_div_2mul) +
		sizeof(p_config_req->mode) + sizeof(p_config_req->data_io_format) +
		sizeof(p_config_req->bit_order);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_SPI_CONFIG, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->role);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->clk_src);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->clk_div_2mul);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->mode);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->data_io_format);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->bit_order);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(spi_w91_ipc_configure);

static int spi_w91_config(const struct device *dev, const struct spi_config *config)
{
	int err = -ETIMEDOUT;
	struct spi_w91_config_req config_req;
	uint8_t inst = ((struct spi_w91_config *)dev->config)->instance_id;

	/* check if it is new configuration */
	if (!spi_w91_is_new_config(dev, config)) {
		return 0;
	}

	/* check for unsupported configuration */
	if (!spi_w91_is_config_supported(config)) {
		return -ENOTSUP;
	}

	/* set SPI role */
	config_req.role = SPI_OP_MODE_GET(config->operation);

	/* set SPI clock source */
	uint32_t spi_clk;

	if ((inst == SPI0_INST) || (inst == SPI1_INST)) {
		if (config->frequency <= (SPI_CLK_XTAL / 2)) {
			config_req.clk_src = SPI_CLK_SRC_XTAL;
			spi_clk = SPI_CLK_XTAL;
		} else {
			config_req.clk_src = SPI_CLK_SRC_PLL;
			spi_clk = SPI0_SPI1_CLK_PLL;
		}
	} else if (inst == SPI2_INST) {
		if (config->frequency <= (SPI_CLK_XTAL / 2)) {
			config_req.clk_src = SPI_CLK_SRC_XTAL;
			spi_clk = SPI_CLK_XTAL;
		} else {
			config_req.clk_src = SPI_CLK_SRC_PLL;
			spi_clk = SPI2_CLK_PLL;
		}
	} else {
		LOG_ERR("SPI inst is invalid");
		return -EINVAL;
	}

	/* set SPI clock divider */
	config_req.clk_div_2mul = spi_clk / (2 * config->frequency);
	if (!config_req.clk_div_2mul) {
		LOG_ERR("SPI frequency (%u) is invalid: clock divider cannot be set",
			config->frequency);
		return -EINVAL;
	}

	/* set SPI mode */
	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			config_req.mode = SPI_MODE_3;
		} else {
			config_req.mode = SPI_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			config_req.mode = SPI_MODE_1;
		} else {
			config_req.mode = SPI_MODE_0;
		}
	}

	/* set SPI lines configuration */
	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		if ((config->operation & SPI_LINES_MASK) == SPI_LINES_DUAL) {
			config_req.data_io_format = SPI_DATA_IO_FORMAT_DUAL;
		} else if ((config->operation & SPI_LINES_MASK) == SPI_LINES_QUAD) {
			config_req.data_io_format = SPI_DATA_IO_FORMAT_QUAD;
		} else {
			config_req.data_io_format = SPI_DATA_IO_FORMAT_SINGLE;
		}
	} else {
		config_req.data_io_format = SPI_DATA_IO_FORMAT_SINGLE;
	}

	/* Set SPI bit order */
	if (config->operation & SPI_TRANSFER_LSB) {
		config_req.bit_order = SPI_BIT_LSB_FIRST;
	} else {
		config_req.bit_order = SPI_BIT_MSB_FIRST;
	}

	struct ipc_based_driver *ipc_data = &((struct spi_w91_data *)dev->data)->ipc;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		spi_w91_ipc_configure, &config_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (!err) {
		spi_w91_save_config(dev, config);
	}

	return err;
}

/* API implementation: get max SPI tx/rx len */
static uint32_t spi_w91_get_tx_rx_len(struct spi_context *ctx)
{
	uint32_t len_tx = 0;
	uint32_t len_rx = 0;

	const struct spi_buf *tx_buf = ctx->current_tx;
	const struct spi_buf *rx_buf = ctx->current_rx;

	if (tx_buf) {
		for (int i = 0; i < ctx->tx_count; i++) {
			len_tx += tx_buf->len;
			tx_buf++;
		}
	}

	if (rx_buf) {
		for (int i = 0; i < ctx->rx_count; i++) {
			len_rx += rx_buf->len;
			rx_buf++;
		}
	}

	return MAX(len_tx, len_rx);
}

/* API implementation: set SPI tx context */
static void spi_w91_context_tx_set(struct spi_context *ctx, uint8_t *tx_buf, uint32_t len)
{
	uint32_t tx_len = 0;

	for (int i = 0; i < len; i += tx_len) {
		if (spi_context_tx_buf_on(ctx)) {
			memcpy(tx_buf + i, ctx->tx_buf, ctx->tx_len);
			tx_len = ctx->tx_len;
		} else {
			if (ctx->tx_len) {
				memset(tx_buf + i, 0, ctx->tx_len);
				tx_len = ctx->tx_len;
			} else {
				*(tx_buf + i) = 0;
				tx_len = 1;
			}
		}

		spi_context_update_tx(ctx, 1, tx_len);
	}
}

/* API implementation: set SPI rx context */
static void spi_w91_context_rx_set(struct spi_context *ctx, uint8_t *rx_buf, uint32_t len)
{
	uint32_t rx_len = 0;

	for (int i = 0; i < len; i += rx_len) {
		if (spi_context_rx_buf_on(ctx)) {
			memcpy(ctx->rx_buf, rx_buf + i, ctx->rx_len);
			rx_len = ctx->rx_len;
		} else {
			if (ctx->rx_len) {
				rx_len = ctx->rx_len;
			} else {
				rx_len = 1;
			}
		}

		spi_context_update_rx(ctx, 1, rx_len);
	}
}

/* APIs implementation: SPI master tx/rx */
static size_t pack_spi_w91_master_tx_rx(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct spi_w91_master_tx_rx_req *p_master_tx_rx_req = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_master_tx_rx_req->rx_len) +
		sizeof(p_master_tx_rx_req->tx_len) + p_master_tx_rx_req->tx_len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_SPI_MASTER_TX_RX, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_master_tx_rx_req->rx_len);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_master_tx_rx_req->tx_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_master_tx_rx_req->tx_buffer,
			p_master_tx_rx_req->tx_len);
	}

	return pack_data_len;
}

static void unpack_spi_w91_master_tx_rx(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct spi_w91_master_tx_rx_resp *p_master_tx_rx_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_master_tx_rx_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_master_tx_rx_resp->rx_len);

	size_t expect_len = sizeof(uint32_t) + sizeof(p_master_tx_rx_resp->err) +
		sizeof(p_master_tx_rx_resp->rx_len) + p_master_tx_rx_resp->rx_len;

	if (expect_len != pack_data_len) {
		p_master_tx_rx_resp->err = -EINVAL;
		return;
	}

	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_master_tx_rx_resp->rx_buffer,
		p_master_tx_rx_resp->rx_len);
}

static int spi_w91_master_tx_rx(const struct device *dev)
{
	struct spi_w91_data *data = dev->data;
	struct spi_w91_master_tx_rx_req master_tx_rx_req;
	struct spi_w91_master_tx_rx_resp master_tx_rx_resp;

	uint32_t len = spi_w91_get_tx_rx_len(&data->ctx);

	if ((len > SPI_TX_RX_BUFFER_SIZE) || (len > SPI_TX_RX_MAX_SIZE_IN_PACK)) {
		LOG_ERR("Incorrect SPI master tx/rx len: %u (spi tx/rx buff: %u, max ipc pack: %u)",
			len, SPI_TX_RX_BUFFER_SIZE, SPI_TX_RX_MAX_SIZE_IN_PACK);
		return -EINVAL;
	}

	uint8_t *spi_tx_rx_buffer = malloc(len);

	if (!spi_tx_rx_buffer) {
		LOG_ERR("SPI master tx/rx operation failed (no memory)");
		return -ENOMEM;
	}

	spi_w91_context_tx_set(&data->ctx, spi_tx_rx_buffer, len);

	master_tx_rx_req.tx_buffer = spi_tx_rx_buffer;
	master_tx_rx_req.tx_len = len;
	master_tx_rx_resp.rx_buffer = spi_tx_rx_buffer;
	master_tx_rx_req.rx_len = len;

	struct ipc_based_driver *ipc_data = &((struct spi_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct spi_w91_config *)dev->config)->instance_id;

	master_tx_rx_resp.err = -ETIMEDOUT;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		spi_w91_master_tx_rx, &master_tx_rx_req, &master_tx_rx_resp,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (master_tx_rx_resp.err) {
		LOG_ERR("SPI tx/rx operation failed (ERR type = %d)", master_tx_rx_resp.err);
	}

	if (!master_tx_rx_resp.err && (master_tx_rx_req.rx_len != master_tx_rx_resp.rx_len)) {
		master_tx_rx_resp.err = -EINVAL;
		LOG_ERR("Incorrect SPI rx len: req len(%u) != (%u)resp len",
			master_tx_rx_req.rx_len, master_tx_rx_resp.rx_len);
	}

	spi_w91_context_rx_set(&data->ctx, spi_tx_rx_buffer, len);

	free(spi_tx_rx_buffer);

	return master_tx_rx_resp.err;
}

/* API implementation: transceive */
static int spi_w91_transceive(const struct device *dev, const struct spi_config *config,
	const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	int err;
	struct spi_w91_data *data = dev->data;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	err = spi_w91_config(dev, config);
	if (err) {
		LOG_ERR("An error occurred in the SPI configuration");
		spi_context_release(&data->ctx, err);
		return err;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	err = spi_w91_master_tx_rx(dev);

	spi_context_release(&data->ctx, err);

	return err;
}

/* API implementation: release */
static int spi_w91_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_w91_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
/* API implementation: transceive_async */
static int spi_w91_transceive_async(const struct device *dev,
	const struct spi_config *config, const struct spi_buf_set *tx_bufs,
	const struct spi_buf_set *rx_bufs, spi_callback_t cb, void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	ARG_UNUSED(tx_bufs);
	ARG_UNUSED(rx_bufs);
	ARG_UNUSED(cb);
	ARG_UNUSED(userdata);

	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

/* APIs implementation: SPI initialization */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(spi_w91_init, IPC_DISPATCHER_SPI_INIT);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(spi_w91_init);

static int spi_w91_init(const struct device *dev)
{
	int err;
	struct spi_w91_data *data = dev->data;
	const struct spi_w91_config *cfg = dev->config;

	ipc_based_driver_init(&data->ipc);

	/* configure pins */
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure SPI pins");
		return err;
	}

	uint8_t inst = ((struct spi_w91_config *)dev->config)->instance_id;
	struct ipc_based_driver *ipc_data = &((struct spi_w91_data *)dev->data)->ipc;

	err = -ETIMEDOUT;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		spi_w91_init, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (err) {
		LOG_ERR("Failed to init SPI");
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct spi_driver_api spi_w91_api = {
	.transceive = spi_w91_transceive,
	.release = spi_w91_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_w91_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

#define SPI_W91_INIT(n)                                         \
	                                                            \
	PINCTRL_DT_INST_DEFINE(n);                                  \
	                                                            \
	static struct spi_w91_data spi_w91_data_##n = {             \
		SPI_CONTEXT_INIT_LOCK(spi_w91_data_##n, ctx),           \
		SPI_CONTEXT_INIT_SYNC(spi_w91_data_##n, ctx),           \
	};                                                          \
	                                                            \
	const static struct spi_w91_config spi_w91_config_##n = {   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),              \
		.instance_id = DT_INST_PROP(n, instance_id),            \
	};                                                          \
	                                                            \
	DEVICE_DT_INST_DEFINE(n, spi_w91_init,                      \
			      NULL,                                         \
			      &spi_w91_data_##n,                            \
			      &spi_w91_config_##n,                          \
			      POST_KERNEL,                                  \
			      CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,  \
			      &spi_w91_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_W91_INIT)
