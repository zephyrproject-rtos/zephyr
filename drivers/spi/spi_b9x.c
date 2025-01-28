/*
 * Copyright (c) 2021-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_SOC_RISCV_TELINK_B91
#define DT_DRV_COMPAT telink_b91_spi
#elif CONFIG_SOC_RISCV_TELINK_B92
#define DT_DRV_COMPAT telink_b92_spi
#endif

/*  Redefine 'spi_read' and 'spi_write' functions names from HAL */
#define spi_read    hal_spi_read
#define spi_write   hal_spi_write
#include "spi.c"
#undef spi_read
#undef spi_write

#include "clock.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_telink);

#include <zephyr/drivers/spi.h>
#include "spi_context.h"
#include <zephyr/drivers/pinctrl.h>


#define CHIP_SELECT_COUNT               3u
#define SPI_WORD_SIZE                   8u
#define SPI_WR_RD_CHUNK_SIZE_MAX        16u


/* SPI configuration structure */
struct spi_b9x_cfg {
	uint8_t peripheral_id;
	gpio_pin_e cs_pin[CHIP_SELECT_COUNT];
	const struct pinctrl_dev_config *pcfg;
};
#define SPI_CFG(dev)                    ((struct spi_b9x_cfg *) ((dev)->config))

/* SPI data structure */
struct spi_b9x_data {
	struct spi_context ctx;
};
#define SPI_DATA(dev)                   ((struct spi_b9x_data *) ((dev)->data))


/* disable hardware cs flow control */
static void spi_b9x_hw_cs_disable(const struct spi_b9x_cfg *config)
{
	gpio_pin_e pin;

	/* loop through all cs pins (cs0..cs2) */
	for (int i = 0; i < CHIP_SELECT_COUNT; i++) {
		/* get CS pin defined in device tree */
		pin = config->cs_pin[i];

		/* if CS pin is defined in device tree */
		if (pin != 0) {
#if CONFIG_SOC_RISCV_TELINK_B91
			if (config->peripheral_id == PSPI_MODULE) {
				pspi_cs_pin_dis(pin);
			} else {
				hspi_cs_pin_dis(pin);
			}
#elif CONFIG_SOC_RISCV_TELINK_B92
			if (config->peripheral_id == LSPI_MODULE) {
				/* Note: lspi_cs_pin_dis has not added to SPI driver for B92 */
			} else {
				gspi_cs_pin_dis(pin);
			}
#endif
		}
	}
}

/* config cs flow control: hardware or software */
static bool spi_b9x_config_cs(const struct device *dev,
			      const struct spi_config *config)
{
	gpio_pin_e cs_pin = 0;
	const struct spi_b9x_cfg *b9x_config = SPI_CFG(dev);

	/* software flow control */
	if (spi_cs_is_gpio(config)) {
		/* disable all hardware CS pins */
		spi_b9x_hw_cs_disable(b9x_config);
		return true;
	}

	/* hardware flow control */

	/* check for correct slave id */
	if (config->slave >= CHIP_SELECT_COUNT) {
		LOG_ERR("Slave %d not supported (max. %d)", config->slave, CHIP_SELECT_COUNT - 1);
		return false;
	}

	/* loop through all cs pins: cs0, cs1 and cs2 */
	for (int cs_id = 0; cs_id < CHIP_SELECT_COUNT; cs_id++) {
		/* get cs pin defined in device tree */
		cs_pin = b9x_config->cs_pin[cs_id];

		/*  if cs pin is not defined for the selected slave, return error */
		if ((cs_pin == 0) && (cs_id == config->slave)) {
			LOG_ERR("cs%d-pin is not defined in device tree", config->slave);
			return false;
		}

		/* disable cs pin if it is defined and is not requested */
		if ((cs_pin != 0) && (cs_id != config->slave)) {
#if CONFIG_SOC_RISCV_TELINK_B91
			if (b9x_config->peripheral_id == PSPI_MODULE) {
				pspi_cs_pin_dis(cs_pin);
			} else {
				hspi_cs_pin_dis(cs_pin);
			}
#elif CONFIG_SOC_RISCV_TELINK_B92
			if (b9x_config->peripheral_id == LSPI_MODULE) {
				/* Note: lspi_cs_pin_dis has not added to SPI driver for B92 */
			} else {
				gspi_cs_pin_dis(cs_pin);
			}
#endif
		}

		/* enable cs pin if it is defined and is requested */
		if ((cs_pin != 0) && (cs_id == config->slave)) {
#if CONFIG_SOC_RISCV_TELINK_B91
			if (b9x_config->peripheral_id == PSPI_MODULE) {
				pspi_cs_pin_en(cs_pin);
			} else {
				hspi_cs_pin_en(cs_pin);
			}
#elif CONFIG_SOC_RISCV_TELINK_B92
			if (b9x_config->peripheral_id == LSPI_MODULE) {
				/* Note: lspi_cs_pin_en has not added to SPI driver for B92,
				 * lspi_cs_pin_en must call lspi_set_pin_mux
				 */
				lspi_set_pin_mux(cs_pin);
			} else {
				gspi_cs_pin_en(cs_pin);
			}
#endif
		}
	}

	return true;
}

/* get spi transaction length */
static uint32_t spi_b9x_get_txrx_len(const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	uint32_t len_tx = 0;
	uint32_t len_rx = 0;

	/* calculate tx len */
	if (tx_bufs) {
		const struct spi_buf *tx_buf = tx_bufs->buffers;

		for (int i = 0; i < tx_bufs->count; i++) {
			len_tx += tx_buf->len;
			tx_buf++;
		}
	}

	/* calculate rx len */
	if (rx_bufs) {
		const struct spi_buf *rx_buf = rx_bufs->buffers;

		for (int i = 0; i < rx_bufs->count; i++) {
			len_rx += rx_buf->len;
			rx_buf++;
		}
	}

	return MAX(len_tx, len_rx);
}

/* process tx data */
_attribute_ram_code_sec_
static void spi_b9x_tx(uint8_t peripheral_id, struct spi_context *ctx, uint8_t len)
{
	uint8_t tx;

	for (int i = 0; i < len; i++) {
		if (spi_context_tx_buf_on(ctx)) {
			tx = *(uint8_t *)(ctx->tx_buf);
		} else {
			tx = 0;
		}
		spi_context_update_tx(ctx, 1, 1);
		hal_spi_write(peripheral_id, &tx, 1);
	}
}

/* process rx data */
_attribute_ram_code_sec_
static void spi_b9x_rx(uint8_t peripheral_id, struct spi_context *ctx, uint8_t len)
{
	uint8_t rx = 0;

	for (int i = 0; i < len; i++) {
		hal_spi_read(peripheral_id, &rx, 1);
		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rx;
		}
		spi_context_update_rx(ctx, 1, 1);
	}
}

/* SPI transceive internal */
_attribute_ram_code_sec_
static void spi_b9x_txrx(const struct device *dev, uint32_t len)
{
	unsigned int chunk_size = SPI_WR_RD_CHUNK_SIZE_MAX;
	struct spi_b9x_cfg *cfg = SPI_CFG(dev);
	struct spi_context *ctx = &SPI_DATA(dev)->ctx;

	/* prepare SPI module */
	spi_set_transmode(cfg->peripheral_id, SPI_MODE_WRITE_AND_READ);
	spi_set_cmd(cfg->peripheral_id, 0);
	spi_tx_cnt(cfg->peripheral_id, len);
	spi_rx_cnt(cfg->peripheral_id, len);

	/* write and read bytes in chunks */
	for (int i = 0; i < len; i = i + chunk_size) {
		/* check for tail */
		if (chunk_size > (len - i)) {
			chunk_size = len - i;
		}

		/* write bytes */
		spi_b9x_tx(cfg->peripheral_id, ctx, chunk_size);

		/* read bytes */
		if (len <= SPI_WR_RD_CHUNK_SIZE_MAX) {
			/* read all bytes if len is less than chunk size */
			spi_b9x_rx(cfg->peripheral_id, ctx, chunk_size);
		} else if (i == 0) {
			/* head, read 1 byte less than is sent */
			spi_b9x_rx(cfg->peripheral_id, ctx, chunk_size - 1);
		} else if ((len - i) > SPI_WR_RD_CHUNK_SIZE_MAX) {
			/* body, read so many bytes as is sent*/
			spi_b9x_rx(cfg->peripheral_id, ctx, chunk_size);
		} else {
			/* tail, read the rest bytes */
			spi_b9x_rx(cfg->peripheral_id, ctx, chunk_size + 1);
		}

		/* clear TX and RX fifo */
#if CONFIG_SOC_RISCV_TELINK_B91
		BM_SET(reg_spi_fifo_state(cfg->peripheral_id), FLD_SPI_TXF_CLR);
		BM_SET(reg_spi_fifo_state(cfg->peripheral_id), FLD_SPI_RXF_CLR);
#elif CONFIG_SOC_RISCV_TELINK_B92
		BM_SET(reg_spi_status(cfg->peripheral_id), FLD_SPI_TXF_CLR_LEVEL);
		BM_SET(reg_spi_status(cfg->peripheral_id), FLD_SPI_RXF_CLR_LEVEL);
#endif
	}

	/* wait fot SPI is ready */
	while (spi_is_busy(cfg->peripheral_id)) {
	};

	/* context complete */
	spi_context_complete(ctx, dev, 0);
}

/* Check for supported configuration */
static bool spi_b9x_is_config_supported(const struct spi_config *config,
					struct spi_b9x_cfg *b9x_config)
{
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
#if CONFIG_SOC_RISCV_TELINK_B91
		else if (((config->operation & SPI_LINES_MASK) == SPI_LINES_QUAD)
				&& (b9x_config->peripheral_id == PSPI_MODULE)) {
			LOG_ERR("SPI lines Quad is not supported by PSPI");
			return false;
		}
#endif
	}

	/* check for slave configuration */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("SPI Slave is not implemented");
		return -ENOTSUP;
	}

	return true;
}

/* SPI configuration */
static int spi_b9x_config(const struct device *dev,
			  const struct spi_config *config)
{
	int status = 0;
	spi_mode_type_e mode = SPI_MODE0;
	struct spi_b9x_cfg *b9x_config = SPI_CFG(dev);
	struct spi_b9x_data *b9x_data = SPI_DATA(dev);
	const pinctrl_soc_pin_t *pins = b9x_config->pcfg->states->pins;
#if CONFIG_SOC_RISCV_TELINK_B91
	uint8_t clk_src = b9x_config->peripheral_id == PSPI_MODULE ? sys_clk.pclk : sys_clk.hclk;
#elif CONFIG_SOC_RISCV_TELINK_B92
	uint8_t clk_src = sys_clk.pll_clk;
#endif

	/* check for unsupported configuration */
	if (!spi_b9x_is_config_supported(config, b9x_config)) {
		return -ENOTSUP;
	}

	/* config slave selection (CS): hw or sw */
	if (!spi_b9x_config_cs(dev, config)) {
		return -ENOTSUP;
	}

		/* get SPI mode */
	if (((config->operation & SPI_MODE_CPHA) == 0) &&
	    ((config->operation & SPI_MODE_CPOL) == 0)) {
		mode = SPI_MODE0;
	} else if (((config->operation & SPI_MODE_CPHA) == SPI_MODE_CPHA) &&
		   ((config->operation & SPI_MODE_CPOL) == 0)) {
		mode = SPI_MODE1;
	} else if (((config->operation & SPI_MODE_CPHA) == 0) &&
		   ((config->operation & SPI_MODE_CPOL) == SPI_MODE_CPOL)) {
		mode = SPI_MODE2;
	} else if (((config->operation & SPI_MODE_CPHA) == SPI_MODE_CPHA) &&
		   ((config->operation & SPI_MODE_CPOL) == SPI_MODE_CPOL)) {
		mode = SPI_MODE3;
	}

	/* init SPI master */
#if CONFIG_SOC_RISCV_TELINK_B91
	spi_master_init(b9x_config->peripheral_id,
			clk_src * 1000000 / (2 * config->frequency) - 1, mode);
#elif CONFIG_SOC_RISCV_TELINK_B92
	spi_master_init(b9x_config->peripheral_id,
			clk_src * 1000000/config->frequency, mode);
#endif
	spi_master_config(b9x_config->peripheral_id, SPI_NORMAL);

	/* set lines configuration */
	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		uint32_t lines = config->operation & SPI_LINES_MASK;

		if (lines == SPI_LINES_SINGLE) {
			spi_set_io_mode(b9x_config->peripheral_id, SPI_SINGLE_MODE);
		} else if (lines == SPI_LINES_DUAL) {
			spi_set_io_mode(b9x_config->peripheral_id, SPI_DUAL_MODE);
		} else if (lines == SPI_LINES_QUAD) {
#if CONFIG_SOC_RISCV_TELINK_B91
			spi_set_io_mode(b9x_config->peripheral_id, HSPI_QUAD_MODE);
#elif CONFIG_SOC_RISCV_TELINK_B92
			spi_set_io_mode(b9x_config->peripheral_id, SPI_QUAD_MODE);
#endif
		}
	}
	/* The GPIO need to be configured as input to support alternative function */
	for (uint8_t i = 0; i < b9x_config->pcfg->states->pin_cnt; i++) {
		gpio_input_en(B9x_PINMUX_GET_PIN(*pins++));
	}

	/* configure pins */
	status = pinctrl_apply_state(b9x_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure SPI pins");
		return status;
	}

	/* save context config */
	b9x_data->ctx.config = config;

	return 0;
}

/* API implementation: init */
static int spi_b9x_init(const struct device *dev)
{
	int err;
	struct spi_b9x_data *data = SPI_DATA(dev);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* API implementation: transceive */
static int spi_b9x_transceive(const struct device *dev,
			      const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	int status = 0;
	struct spi_b9x_data *data = SPI_DATA(dev);
	uint32_t txrx_len = spi_b9x_get_txrx_len(tx_bufs, rx_bufs);

	/* set configuration */
	status = spi_b9x_config(dev, config);
	if (status) {
		return status;
	}

	/* context setup */
	spi_context_lock(&data->ctx, false, NULL, NULL, config);
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* if cs is defined: software cs control, set active true */
	if (spi_cs_is_gpio(config)) {
		spi_context_cs_control(&data->ctx, true);
	}

	/* transceive data */
	spi_b9x_txrx(dev, txrx_len);

	/* if cs is defined: software cs control, set active false */
	if (spi_cs_is_gpio(config)) {
		spi_context_cs_control(&data->ctx, false);
	}

	/* release context */
	status = spi_context_wait_for_completion(&data->ctx);
	spi_context_release(&data->ctx, status);

	return status;
}

#ifdef CONFIG_SPI_ASYNC
/* API implementation: transceive_async */
static int spi_b9x_transceive_async(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t cb,
				    void *userdata)
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

/* API implementation: release */
static int spi_b9x_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct spi_b9x_data *data = SPI_DATA(dev);

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* SPI driver APIs structure */
static struct spi_driver_api spi_b9x_api = {
	.transceive = spi_b9x_transceive,
	.release = spi_b9x_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_b9x_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
};

/* SPI driver registration */
#define SPI_B9X_INIT(inst)						  \
									  \
	PINCTRL_DT_INST_DEFINE(inst);					  \
									  \
	static struct spi_b9x_data spi_b9x_data_##inst = {		  \
		SPI_CONTEXT_INIT_LOCK(spi_b9x_data_##inst, ctx),	  \
		SPI_CONTEXT_INIT_SYNC(spi_b9x_data_##inst, ctx),	  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)	  \
	};								  \
									  \
	static struct spi_b9x_cfg spi_b9x_cfg_##inst = {		  \
		.peripheral_id = DT_INST_ENUM_IDX(inst, peripheral_id),	  \
		.cs_pin[0] = DT_INST_STRING_TOKEN(inst, cs0_pin),	  \
		.cs_pin[1] = DT_INST_STRING_TOKEN(inst, cs1_pin),	  \
		.cs_pin[2] = DT_INST_STRING_TOKEN(inst, cs2_pin),	  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		  \
	};								  \
									  \
	DEVICE_DT_INST_DEFINE(inst, spi_b9x_init,			  \
			      NULL,					  \
			      &spi_b9x_data_##inst,			  \
			      &spi_b9x_cfg_##inst,			  \
			      POST_KERNEL,				  \
			      CONFIG_SPI_INIT_PRIORITY,			  \
			      &spi_b9x_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_B9X_INIT)
