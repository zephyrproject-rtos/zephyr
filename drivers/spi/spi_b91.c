/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_spi

/*  Redefine 'spi_read' and 'spi_write' functions names from HAL */
#define spi_read    hal_spi_read
#define spi_write   hal_spi_write
#include "spi.c"
#undef spi_read
#undef spi_write

#include "clock.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(spi_telink);

#include <drivers/spi.h>
#include "spi_context.h"


#define SPI_WORD_SIZE                   8u
#define CHIP_SELECT_COUNT               3u


/* SPI configuration structure */
struct spi_b91_cfg {
	uint8_t peripheral_id;
	gpio_pin_e cs_pin[CHIP_SELECT_COUNT];
};
#define SPI_CFG(dev)                    ((struct spi_b91_cfg *) ((dev)->config))

/* SPI data structure */
struct spi_b91_data {
	struct spi_context ctx;
};
#define SPI_DATA(dev)                   ((struct spi_b91_data *) ((dev)->data))


/* disable hardware cs flow control */
static void spi_b91_hw_cs_disable(const struct spi_b91_cfg *config)
{
	gpio_pin_e pin;

	/* loop through all cs pins (cs0..cs2) */
	for (int i = 0; i < CHIP_SELECT_COUNT; i++) {
		/* get CS pin defined in device tree */
		pin = config->cs_pin[i];

		/* if CS pin is defined in device tree */
		if (pin != 0) {
			if (config->peripheral_id == PSPI_MODULE) {
				/* disable CS pin for PSPI */
				pspi_cs_pin_dis(pin);
			} else {
				/* disable CS pin for MSPI */
				hspi_cs_pin_dis(pin);
			}
		}
	}
}

/* config cs flow control: hardware or software */
static bool spi_b91_config_cs(const struct device *dev,
			      const struct spi_config *config)
{
	pspi_csn_pin_def_e cs_pin = 0;
	const struct spi_b91_cfg *b91_config = SPI_CFG(dev);

	/* software flow control */
	if (config->cs) {
		/* disable all hardware CS pins */
		spi_b91_hw_cs_disable(b91_config);
		return true;
	}

	/* hardware flow control */

	/* check for correct slave id */
	if (config->slave >= CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d is not supported (max. %d)", config->slave, CHIP_SELECT_COUNT - 1);
		return false;
	}

	/* loop through all cs pins: cs0, cs1 and cs2 */
	for (int cs_id = 0; cs_id < CHIP_SELECT_COUNT; cs_id++) {
		/* get cs pin defined in device tree */
		cs_pin = b91_config->cs_pin[cs_id];

		/*  if cs pin is not defined for the selected slave, return error */
		if ((cs_pin == 0) && (cs_id == config->slave)) {
			LOG_ERR("cs%d-pin is not defined in device tree", config->slave);
			return false;
		}

		/* disable cs pin if it is defined and is not requested */
		if ((cs_pin != 0) && (cs_id != config->slave)) {
			if (b91_config->peripheral_id == PSPI_MODULE) {
				pspi_cs_pin_dis(cs_pin);
			} else {
				hspi_cs_pin_dis(cs_pin);
			}
		}

		/* enable cs pin if it is defined and is requested */
		if ((cs_pin != 0) && (cs_id == config->slave)) {
			if (b91_config->peripheral_id == PSPI_MODULE) {
				pspi_set_pin_mux(cs_pin);
				pspi_cs_pin_en(cs_pin);
			} else {
				hspi_set_pin_mux(cs_pin);
				hspi_cs_pin_en(cs_pin);
			}
		}
	}

	return true;
}

/* spi master read (implemented here because it is not present in hal_telink) */
static void spi_master_read(spi_sel_e spi_sel, unsigned char *data, unsigned int len)
{
	spi_rx_fifo_clr(spi_sel);
	spi_rx_cnt(spi_sel, len);
	spi_set_transmode(spi_sel, SPI_MODE_READ_ONLY);
	spi_set_cmd(spi_sel, 0x00);
	hal_spi_read(spi_sel, (unsigned char *)data, len);
	while (spi_is_busy(spi_sel));
}

/* SPI transceive internal */
static void spi_b91_txrx(const struct device *dev)
{
	struct spi_b91_cfg *cfg = SPI_CFG(dev);
	struct spi_b91_data *data = SPI_DATA(dev);
	struct spi_context *ctx = &data->ctx;

	/* loop through all tx and rx buffers */
	while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
		if (!spi_context_rx_buf_on(ctx)) {
			/* write only */
			spi_master_write(cfg->peripheral_id,
					 (unsigned char *)ctx->tx_buf, ctx->tx_len);

			/* move to the next tx buffer */
			spi_context_update_tx(ctx, 1, ctx->tx_len);
		} else if (!spi_context_tx_buf_on(ctx)) {
			/* read only */
			spi_master_read(cfg->peripheral_id,
					(unsigned char *)ctx->rx_buf, ctx->rx_len);

			/* move to the next rx buffer */
			spi_context_update_rx(ctx, 1, ctx->rx_len);
		} else {
			/* write read */
			spi_master_write_read(cfg->peripheral_id,
					      (unsigned char *)ctx->tx_buf, ctx->tx_len,
					      (unsigned char *)ctx->rx_buf, ctx->rx_len);

			/* move to the next tx/rx buffers */
			spi_context_update_tx(ctx, 1, ctx->tx_len);
			spi_context_update_rx(ctx, 1, ctx->rx_len);
		}
	}

	/* context complate */
	spi_context_complete(&data->ctx, 0);
}

/* Check for supported configuration */
static bool spi_b91_is_config_supported(const struct spi_config *config,
					struct spi_b91_cfg *b91_config)
{
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

	/* check for CS lock on */
	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return false;
	}

	/* check for CS active hich */
	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported for HW flow control");
		return false;
	}

	/* check for lines configuration */
	if ((config->operation & SPI_LINES_MASK) == SPI_LINES_OCTAL) {
		LOG_ERR("SPI lines Octal configuration is not supported");
		return false;
	} else if (((config->operation & SPI_LINES_MASK) == SPI_LINES_QUAD) &&
		   (b91_config->peripheral_id == PSPI_MODULE)) {
		LOG_ERR("SPI lines Quad configuration is not supported by PSPI");
		return false;
	}

	/* check for slave configuration */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("SPI Slave is not implemented");
		return -ENOTSUP;
	}

	return true;
}

/* SPI configuration */
static int spi_b91_config(const struct device *dev,
			  const struct spi_config *config)
{
	spi_mode_type_e mode = SPI_MODE0;
	struct spi_b91_cfg *b91_config = SPI_CFG(dev);
	struct spi_b91_data *b91_data = SPI_DATA(dev);
	uint8_t clk_src = b91_config->peripheral_id == PSPI_MODULE ? sys_clk.pclk : sys_clk.hclk;

	/* check for unsupported configuration */
	if (!spi_b91_is_config_supported(config, b91_config)) {
		return -ENOTSUP;
	}

	/* config slave selection (CS): hw or sw */
	if (!spi_b91_config_cs(dev, config)) {
		return -ENOTSUP;
	}

	/* get SPI mode */
	if (((config->operation & SPI_MODE_CPHA) == 0) &&
	    ((config->operation & SPI_MODE_CPOL) == 0)) {
		mode = SPI_MODE0;
	} else if (((config->operation & SPI_MODE_CPHA) == 0) &&
		   ((config->operation & SPI_MODE_CPOL) == SPI_MODE_CPOL)) {
		mode = SPI_MODE1;
	} else if (((config->operation & SPI_MODE_CPHA) == SPI_MODE_CPHA) &&
		   ((config->operation & SPI_MODE_CPOL) == 0)) {
		mode = SPI_MODE2;
	} else if (((config->operation & SPI_MODE_CPHA) == SPI_MODE_CPHA) &&
		   ((config->operation & SPI_MODE_CPOL) == SPI_MODE_CPOL)) {
		mode = SPI_MODE3;
	}

	/* init SPI master */
	spi_master_init(b91_config->peripheral_id,
			clk_src * 1000000 / (2 * config->frequency) - 1, mode);
	spi_master_config(b91_config->peripheral_id, SPI_NOMAL);

	/* set lines configuration */
	if ((config->operation & SPI_LINES_MASK) == SPI_LINES_SINGLE) {
		spi_set_io_mode(b91_config->peripheral_id, SPI_SINGLE_MODE);
	} else if ((config->operation & SPI_LINES_MASK) == SPI_LINES_DUAL) {
		spi_set_io_mode(b91_config->peripheral_id, SPI_DUAL_MODE);
	} else if ((config->operation & SPI_LINES_MASK) == SPI_LINES_QUAD) {
		spi_set_io_mode(b91_config->peripheral_id, HSPI_QUAD_MODE);
	}

	/* save context config */
	b91_data->ctx.config = config;

	/* config software CS control if enabled */
	if (config->cs != NULL) {
		spi_context_cs_configure(&b91_data->ctx);
	}

	return 0;
}

/* API implementation: init */
static int spi_b91_init(const struct device *dev)
{
	struct spi_b91_data *data = SPI_DATA(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* API implementation: transceive */
static int spi_b91_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	int status = 0;
	struct spi_b91_data *data = SPI_DATA(dev);

	/* set configuration */
	status = spi_b91_config(dev, config);
	if (status) {
		return status;
	}

	/* get context */
	spi_context_lock(&data->ctx, false, NULL, config);

	/* setup context buffers */
	if ((tx_bufs != NULL) && (tx_bufs->count == 0)) {
		/* if tx buffer is not NULL and count is 0: read only */
		tx_bufs = NULL;
	} else if ((rx_bufs != NULL) && (rx_bufs->count == 0)) {
		/* if rx buffer is not NULL and count is 0: write only */
		rx_bufs = NULL;
	}
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* if cs is defined: software cs control, set active true */
	if (config->cs) {
		spi_context_cs_control(&data->ctx, true);
	}

	/* transceive data */
	spi_b91_txrx(dev);

	/* if cs is defined: software cs control, set active false */
	if (config->cs) {
		spi_context_cs_control(&data->ctx, false);
	}

	/* release context */
	status = spi_context_wait_for_completion(&data->ctx);
	spi_context_release(&data->ctx, status);

	return status;
}

#ifdef CONFIG_SPI_ASYNC
/* API implementation: transceive_async */
static int spi_b91_transceive_async(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    struct k_poll_signal *async)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	ARG_UNUSED(tx_bufs);
	ARG_UNUSED(rx_bufs);
	ARG_UNUSED(async);

	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

/* API implementation: release */
static int spi_b91_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct spi_b91_data *data = SPI_DATA(dev);

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* SPI driver APIs structure */
static struct spi_driver_api spi_b91_api = {
	.transceive = spi_b91_transceive,
	.release = spi_b91_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_b91_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

/* SPI driver registration */
#define SPI_B91_INIT(inst)							\
										\
	static struct spi_b91_data spi_b91_data_##inst = {			\
		SPI_CONTEXT_INIT_LOCK(spi_b91_data_##inst, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_b91_data_##inst, ctx),		\
	};									\
										\
	static struct spi_b91_cfg spi_b91_cfg_##inst = {			\
		.peripheral_id = DT_ENUM_IDX(DT_DRV_INST(inst), peripheral_id),	\
		.cs_pin[0] = DT_ENUM_TOKEN(DT_DRV_INST(inst), cs0_pin),		\
		.cs_pin[1] = DT_ENUM_TOKEN(DT_DRV_INST(inst), cs1_pin),		\
		.cs_pin[2] = DT_ENUM_TOKEN(DT_DRV_INST(inst), cs2_pin),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, spi_b91_init,				\
			      NULL,				\
			      &spi_b91_data_##inst,				\
			      &spi_b91_cfg_##inst,				\
			      POST_KERNEL,					\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &spi_b91_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_B91_INIT)
