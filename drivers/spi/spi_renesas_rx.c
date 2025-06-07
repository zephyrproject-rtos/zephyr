/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_rspi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_rspi_rx_if.h"
#include "iodefine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rx_rspi);

#include "spi_context.h"

#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)
typedef enum {
	/* Values will be used as bit flags.*/
	RSPI_DO_TX = 0x1,
	RSPI_DO_RX = 0x2,
	RSPI_DO_TX_RX = 0x3
} rspi_operation_t;

typedef struct rx_rspi_tcb_s {
	rspi_operation_t transfer_mode;
	rspi_str_tranmode_t data_tran_mode;
	uint16_t tx_count;
	uint16_t rx_count;
	uint16_t xfr_length;
	uint8_t bytes_per_transfer;
	bool do_rx_now;
	bool do_tx;
} rx_rspi_tcb_t;

static uint8_t rspi_get_data_type(rspi_command_word_t command_word)
{
	switch (command_word.bit_length) {
	case RSPI_SPCMD_BIT_LENGTH_8:
		return RSPI_BYTE_DATA;

	case RSPI_SPCMD_BIT_LENGTH_9:
	case RSPI_SPCMD_BIT_LENGTH_10:
	case RSPI_SPCMD_BIT_LENGTH_11:
	case RSPI_SPCMD_BIT_LENGTH_12:
	case RSPI_SPCMD_BIT_LENGTH_13:
	case RSPI_SPCMD_BIT_LENGTH_14:
	case RSPI_SPCMD_BIT_LENGTH_15:
	case RSPI_SPCMD_BIT_LENGTH_16:
		return RSPI_WORD_DATA;

	case RSPI_SPCMD_BIT_LENGTH_20:
	case RSPI_SPCMD_BIT_LENGTH_24:
	case RSPI_SPCMD_BIT_LENGTH_32:
		return RSPI_LONG_DATA;

	default:
		return 0;
	}
} /* End of function rspi_get_data_type */

#endif /* CONFIG_SPI_RENESAS_RX_INTERRUPT */

struct rx_rspi_data {
	struct spi_context ctx;
	int channel_id;
	int ssl_assert;
	volatile struct st_rspi *preg;
	rspi_handle_t rspi;
	rspi_chnl_settings_t channel_setting;
	rspi_command_word_t command_word;
	rspi_callback_data_t callback_data;
	uint8_t dfs;
#if CONFIG_SPI_RENESAS_RX_INTERRUPT
	rx_rspi_tcb_t tcb;
	uint32_t rxdata;
	uint32_t data_len;
#endif
};

struct rx_rspi_config {
	const struct pinctrl_dev_config *pcfg;
};

static void spi_cb(void *p_args)
{
	struct device *dev = (struct device *)p_args;
	struct rx_rspi_data *data = dev->data;

	switch (data->callback_data.event_code) {
	case RSPI_EVT_TRANSFER_COMPLETE:
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, 0);
		break;
	case RSPI_EVT_TRANSFER_ABORTED:
	case RSPI_EVT_ERR_MODE_FAULT:
	case RSPI_EVT_ERR_READ_OVF:
	case RSPI_EVT_ERR_PARITY:
	case RSPI_EVT_ERR_UNDER_RUN:
	case RSPI_EVT_ERR_UNDEF:
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, dev, -EIO);
		break;
	default:
		break;
	}
}

#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)
static void transmit_data(struct rx_rspi_data *data, uint16_t tx_count)
{
	rx_rspi_tcb_t *rspi_tcb = &(data->tcb);
	void *psrc = (void *)data->ctx.tx_buf;
	uint8_t data_size = rspi_tcb->bytes_per_transfer;

	if (rspi_tcb->do_tx) {
		if (RSPI_BYTE_DATA == data_size) {
			data->preg->SPDR.LONG = ((uint8_t *)psrc)[tx_count];
		} else if (RSPI_WORD_DATA == data_size) {
			data->preg->SPDR.LONG = ((uint16_t *)psrc)[tx_count];
		} else {
			data->preg->SPDR.LONG = ((uint32_t *)psrc)[tx_count];
		}
	} else {
		data->preg->SPDR.LONG = 0;
	}
}

void rx_rspi_spti_sub(const struct device *dev)
{
	struct rx_rspi_data *data = dev->data;
	rx_rspi_tcb_t *rspi_tcb = &(data->tcb);
	uint16_t tx_count = rspi_tcb->tx_count;

	/* If transmit only, enable SPII interrupt in transmit */
	if ((!spi_context_rx_on(&data->ctx)) && (tx_count == rspi_tcb->xfr_length - 1)) {
		data->preg->SPCR2.BIT.SPIIE = 1;

		/* If the SPI is in slave mode */
		if (spi_context_is_slave(&data->ctx)) {
			/* Disable RSPI */
			data->preg->SPCR.BIT.SPE = 0;

			/**
			 * Transfer complete. Call the user callback function passing
			 * pointer to the result structure.
			 */
			if (data->rspi->pcallback != NULL) {
				data->callback_data.handle = data->rspi;
				data->callback_data.event_code = RSPI_EVT_TRANSFER_COMPLETE;
				data->rspi->pcallback((void *)dev);
			}
		}
	}

	/* Service the hardware first to keep it busy. */
	/* Feed the TX. */
	if (tx_count < rspi_tcb->xfr_length) {
		transmit_data(data, tx_count);
		rspi_tcb->tx_count++;
	} else {
		if (spi_context_is_slave(&data->ctx)) {
			spi_context_update_tx(&data->ctx, data->dfs, data->data_len);
		}
	}
}
#endif

static int rx_rspi_configure(const struct device *dev, const struct spi_config *config)
{
	struct rx_rspi_data *data = dev->data;
	rspi_err_t err;

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	err = R_RSPI_Close(data->rspi);

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		data->channel_setting.master_slave_mode = RSPI_MS_MODE_SLAVE;
	} else {
		data->channel_setting.master_slave_mode = RSPI_MS_MODE_MASTER;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		data->command_word.cpol = RSPI_SPCMD_CPOL_IDLE_HI;
	} else {
		data->command_word.cpol = RSPI_SPCMD_CPOL_IDLE_LO;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		data->command_word.cpha = RSPI_SPCMD_CPHA_SAMPLE_EVEN;
	} else {
		if (data->channel_setting.master_slave_mode == RSPI_MS_MODE_MASTER) {
			data->command_word.cpha = RSPI_SPCMD_CPHA_SAMPLE_ODD;
		} else {
			/* In slave mode cpha must be 1 */
			LOG_ERR("Invalid clock phase");
			return -EINVAL;
		}
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		data->command_word.bit_order = RSPI_SPCMD_ORDER_LSB_FIRST;
	} else {
		data->command_word.bit_order = RSPI_SPCMD_ORDER_MSB_FIRST;
	}

	if (spi_cs_is_gpio(config) || !IS_ENABLED(CONFIG_SPI_RENESAS_RX_USE_HW_SS)) {
		data->channel_setting.gpio_ssl = RSPI_IF_MODE_3WIRE;
	} else {
		data->channel_setting.gpio_ssl = RSPI_IF_MODE_4WIRE;
		switch (data->ssl_assert) {
		case 0:
			data->command_word.ssl_assert = RSPI_SPCMD_ASSERT_SSL0;
			break;
		case 1:
			data->command_word.ssl_assert = RSPI_SPCMD_ASSERT_SSL1;
			break;
		case 2:
			data->command_word.ssl_assert = RSPI_SPCMD_ASSERT_SSL2;
			break;
		case 3:
			data->command_word.ssl_assert = RSPI_SPCMD_ASSERT_SSL3;
			break;
		default:
			LOG_ERR("Invalid SSL");
			return -EINVAL;
		}
	}

	data->channel_setting.bps_target = config->frequency;
	data->channel_setting.tran_mode = RSPI_TRANS_MODE_SW;

	uint8_t array_rspi_bit_length[] = {
		RSPI_SPCMD_BIT_LENGTH_8,  RSPI_SPCMD_BIT_LENGTH_9,  RSPI_SPCMD_BIT_LENGTH_10,
		RSPI_SPCMD_BIT_LENGTH_11, RSPI_SPCMD_BIT_LENGTH_12, RSPI_SPCMD_BIT_LENGTH_13,
		RSPI_SPCMD_BIT_LENGTH_14, RSPI_SPCMD_BIT_LENGTH_15, RSPI_SPCMD_BIT_LENGTH_16};

	uint16_t bit_frame_size = SPI_WORD_SIZE_GET(config->operation);

	if (bit_frame_size >= 8 && bit_frame_size <= 16) {
		data->command_word.bit_length = array_rspi_bit_length[bit_frame_size - 8];
	} else {
		switch (bit_frame_size) {
		case 20:
			data->command_word.bit_length = RSPI_SPCMD_BIT_LENGTH_20;
			break;
		case 24:
			data->command_word.bit_length = RSPI_SPCMD_BIT_LENGTH_24;
			break;
		case 32:
			data->command_word.bit_length = RSPI_SPCMD_BIT_LENGTH_32;
			break;
		default:
			return -ENOTSUP;
		}
	}

	err = R_RSPI_Open(data->channel_id, &data->channel_setting, data->command_word, spi_cb,
			  &data->rspi);
	if (err != RSPI_SUCCESS) {
		LOG_ERR("R_RSPI_Open error: %d", err);
		return -EINVAL;
#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)
	} else {
		data->tcb.data_tran_mode = data->channel_setting.tran_mode;
#endif
	}
	/* Manually set these bits, because the Open function not */
	data->preg->SPCMD0.BIT.CPHA = data->command_word.cpha;
	data->preg->SPCMD0.BIT.LSBF = data->command_word.bit_order;
	data->preg->SPCMD0.BIT.SSLA = data->command_word.ssl_assert;

	/* Set the ctx config = config for next time enter the function */
	data->ctx.config = config;

	return 0;
}

static bool rx_spi_transfer_ongoing(struct rx_rspi_data *data)
{
#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
#else
	if (spi_context_total_tx_len(&data->ctx) < spi_context_total_rx_len(&data->ctx)) {
		return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
	} else {
		return (spi_context_tx_on(&data->ctx) && spi_context_rx_on(&data->ctx));
	}
#endif
}

#ifndef CONFIG_SPI_RENESAS_RX_INTERRUPT
static int rx_rspi_transceive_slave(struct rx_rspi_data *data)
{
	if (data->preg->SPSR.BIT.SPTEF && spi_context_tx_on(&data->ctx)) {
		uint32_t tx;

		if (data->ctx.tx_buf != NULL) {
			if (data->dfs > 2) {
				tx = UNALIGNED_GET((uint32_t *)(data->ctx.tx_buf));
			} else if (data->dfs > 1) {
				tx = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
			} else {
				tx = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
			}
		} else {
			tx = 0;
		}
		/* Write a specific number of frame clear the SPTEF bit */
		data->preg->SPDR.LONG = tx;
		spi_context_update_tx(&data->ctx, data->dfs, 1);
	} else {
		data->preg->SPCR.BIT.SPTIE = 0;
	}

	uint32_t rx;

	if (data->preg->SPSR.BIT.SPRF && spi_context_rx_buf_on(&data->ctx)) {
		/* Read data from the Data Reg make the receive full flag being cleared */
		rx = data->preg->SPDR.LONG;
		if (data->dfs > 2) {
			UNALIGNED_PUT(rx, (uint32_t *)data->ctx.rx_buf);
		} else if (data->dfs > 1) {
			UNALIGNED_PUT(rx, (uint16_t *)data->ctx.rx_buf);
		} else {
			UNALIGNED_PUT(rx, (uint8_t *)data->ctx.rx_buf);
		}

		spi_context_update_rx(&data->ctx, data->dfs, 1);
	}

	return 0;
}

static int rx_rspi_transceive_master(struct rx_rspi_data *data)
{
	uint32_t tx;
	uint32_t rx;

	if (spi_context_tx_buf_on(&data->ctx)) {
		if (data->dfs > 2) {
			tx = UNALIGNED_GET((uint32_t *)(data->ctx.tx_buf));
		} else if (data->dfs > 1) {
			tx = UNALIGNED_GET((uint16_t *)(data->ctx.tx_buf));
		} else {
			tx = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		}
	} else {
		tx = 0U;
	}

	while (!data->preg->SPSR.BIT.SPTEF) {
	}

	data->preg->SPDR.LONG = tx;

	spi_context_update_tx(&data->ctx, data->dfs, 1);

	if (spi_context_rx_on(&data->ctx)) {
		while (!data->preg->SPSR.BIT.SPRF) {
		}
		rx = data->preg->SPDR.LONG;

		if (spi_context_rx_buf_on(&data->ctx)) {
			if (data->dfs > 2) {
				UNALIGNED_PUT(rx, (uint32_t *)data->ctx.rx_buf);
			} else if (data->dfs > 1) {
				UNALIGNED_PUT(rx, (uint16_t *)data->ctx.rx_buf);
			} else {
				UNALIGNED_PUT(rx, (uint8_t *)data->ctx.rx_buf);
			}
		}
		spi_context_update_rx(&data->ctx, data->dfs, 1);
	} else {
		/* If there no rx and the tx still send, read and drop the data */
		if (data->preg->SPSR.BIT.SPRF) {
			/* In case there no rx drop the incoming data*/
			rx = data->preg->SPDR.LONG;
		}
	}

	return 0;
}

static int rx_rspi_transceive_data(struct rx_rspi_data *data)
{
	uint16_t operation = data->ctx.config->operation;

	if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
		rx_rspi_transceive_master(data);
	} else {
		rx_rspi_transceive_slave(data);
	}

	return 0;
}
#endif

static int transceive(const struct device *dev, const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct rx_rspi_data *data = dev->data;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	if (!IS_ENABLED(CONFIG_SPI_RENESAS_RX_INTERRUPT) && asynchronous) {
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	ret = rx_rspi_configure(dev, spi_cfg);
	if (ret) {
		goto end;
	}

	data->dfs = ((SPI_WORD_SIZE_GET(spi_cfg->operation) - 1) / 8) + 1;

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);
	spi_context_cs_control(&data->ctx, true);

	if ((!spi_context_tx_buf_on(&data->ctx)) && (!spi_context_rx_buf_on(&data->ctx))) {
		/* If current buffer has no data, do nothing */
		goto end;
	}

#ifdef CONFIG_SPI_RENESAS_RX_INTERRUPT

	if (data->ctx.rx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? (spi_context_total_tx_len(&data->ctx) / data->dfs)
					 : data->ctx.tx_len;
	} else if (data->ctx.tx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? (spi_context_total_rx_len(&data->ctx) / data->dfs)
					 : data->ctx.rx_len;
	} else {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? (MAX(spi_context_total_tx_len(&data->ctx),
						spi_context_total_rx_len(&data->ctx)) /
					    data->dfs)
					 : MIN(data->ctx.tx_len, data->ctx.rx_len);
	}

	data->tcb.xfr_length = data->data_len;
	data->tcb.tx_count = 0;
	data->tcb.rx_count = 0;
	data->tcb.do_rx_now = false;
	data->tcb.do_tx = true;
	data->tcb.bytes_per_transfer = rspi_get_data_type(data->command_word);
	if (data->tcb.bytes_per_transfer == 0) {
		LOG_ERR("Invalid bit length");
		ret = -EINVAL;
		goto end;
	}

	if (data->ctx.rx_buf == NULL) {
		data->tcb.transfer_mode = RSPI_DO_TX;
		rspi_err_t err = R_RSPI_Write(data->rspi, data->command_word,
					      (void *)data->ctx.tx_buf, data->data_len);
		if (err != 0) {
			ret = -EINVAL;
			goto end;
		}
	} else if (data->ctx.tx_buf == NULL) {
		data->tcb.transfer_mode = RSPI_DO_RX;
		data->tcb.do_tx = false;
		rspi_err_t err = R_RSPI_Read(data->rspi, data->command_word,
					     (void *)data->ctx.rx_buf, data->data_len);
		if (err != 0) {
			ret = -EINVAL;
			goto end;
		}
	} else {
		data->tcb.transfer_mode = RSPI_DO_TX_RX;
		rspi_err_t err =
			R_RSPI_WriteRead(data->rspi, data->command_word, (void *)data->ctx.tx_buf,
					 (void *)data->ctx.rx_buf, data->data_len);
		if (err != 0) {
			ret = -EINVAL;
			goto end;
		}
	}
	ret = spi_context_wait_for_completion(&data->ctx);

#else
	data->preg->SPCR.BIT.TXMD = 0x0; /* tx - rx */
	if (!spi_context_rx_on(&data->ctx)) {
		data->preg->SPCR.BIT.TXMD = 0x1; /* tx only */
	}

	/* Enable the SPI Transfer */
	data->preg->SPCMD0.BIT.SPB = data->command_word.bit_length;
	data->preg->SPCR.BIT.SPE = 1;

	do {
		rx_rspi_transceive_data(data);
	} while (rx_spi_transfer_ongoing(data));

	if (SPI_OP_MODE_GET(data->ctx.config->operation) == SPI_OP_MODE_MASTER) {
		/* Wait for transmision complete */
		while (data->preg->SPSR.BIT.IDLNF) {
			if (data->preg->SPSR.BIT.SPRF) {
				/* Drop the incoming data because there are no rx */
				uint32_t trash_can;

				trash_can = data->preg->SPDR.LONG;
			}
		}
	}

	/* Disable the SPI Transfer. */
	data->preg->SPCR.BIT.SPE = 0;

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_RENESAS_RX_INTERRUPT */

end:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int rx_rspi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int rx_rspi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				    void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int rx_rspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct rx_rspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, rx_spi_api) = {
	.transceive = rx_rspi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = rx_rspi_transceive_async,
#endif
	.release = rx_rspi_release,
};

static int rspi_rx_init(const struct device *dev)
{
	const struct rx_rspi_config *config = dev->config;
	struct rx_rspi_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)

static void rx_rspi_retransmit(struct rx_rspi_data *data)
{
	if (data->ctx.rx_len == 0) {
		data->data_len = data->ctx.tx_len;
		data->tcb.transfer_mode = RSPI_DO_TX;
		data->tcb.do_tx = true;
	} else if (data->ctx.tx_len == 0) {
		data->data_len = data->ctx.rx_len;
		data->tcb.transfer_mode = RSPI_DO_RX;
		data->tcb.do_tx = false;
	} else {
		data->data_len = MIN(data->ctx.tx_len, data->ctx.rx_len);
		data->tcb.transfer_mode = RSPI_DO_TX_RX;
		data->tcb.do_tx = true;
	}

	data->tcb.do_rx_now = false;
	data->tcb.tx_count = 0;
	data->tcb.rx_count = 0;
	data->tcb.xfr_length = data->data_len;

	/* Execute the transmit of first byte here to start transmit on the new buffer */
	rx_rspi_tcb_t *rspi_tcb = &(data->tcb);
	uint16_t tx_count = rspi_tcb->tx_count;

	if (tx_count < rspi_tcb->xfr_length) {
		transmit_data(data, tx_count);
		rspi_tcb->tx_count++;
	}
}

static void rx_rspi_spri_isr(const struct device *dev)
{
	struct rx_rspi_data *data = dev->data;
	rx_rspi_tcb_t *rspi_tcb = &(data->tcb);
	uint32_t *rxdata = &(data->rxdata);

	if (RSPI_TRANS_MODE_SW == rspi_tcb->data_tran_mode) {
		*rxdata = data->preg->SPDR.LONG;
		rspi_tcb->rx_count++;
#if RSPI_CFG_HIGH_SPEED_READ == 0
		rx_rspi_spti_sub(dev);
#endif
		void *pdest = (void *)data->ctx.rx_buf;
		uint16_t rx_count = rspi_tcb->rx_count;
		uint8_t data_size = rspi_tcb->bytes_per_transfer;

		if (rspi_tcb->do_rx_now) {
			if (RSPI_BYTE_DATA == data_size) {
				((uint8_t *)pdest)[rx_count - 1] = *((uint8_t *)rxdata);
			} else if (RSPI_WORD_DATA == data_size) {
				((uint16_t *)pdest)[rx_count - 1] = *((uint16_t *)rxdata);
			} else {
				((uint32_t *)pdest)[rx_count - 1] = *rxdata;
			}
		}
		if (rx_count == rspi_tcb->xfr_length) {
			data->preg->SPCR2.BIT.SPIIE = 1;

			/* If the SPI is in slave mode */
			if (spi_context_is_slave(&data->ctx)) {
				spi_context_update_rx(&data->ctx, data->dfs, data->data_len);
				/* Disable RSPI */
				data->preg->SPCR.BIT.SPE = 0;

				/**
				 * Transfer complete. Call the user callback function passing
				 * pointer to the result structure.
				 */
				if (data->rspi->pcallback != NULL) {
					data->callback_data.handle = data->rspi;
					data->callback_data.event_code = RSPI_EVT_TRANSFER_COMPLETE;
					data->rspi->pcallback((void *)dev);
				}
			}
		}
	} else {
		R_RSPI_IntSpriIerClear(data->rspi);
		R_RSPI_DisableRSPI(data->rspi);

		/**
		 * Transfer complete. Call the user callback function passing pointer to the result
		 * structure.
		 */
		if (data->rspi->pcallback != NULL) {
			data->callback_data.handle = data->rspi;
			data->callback_data.event_code = RSPI_EVT_TRANSFER_COMPLETE;

			data->rspi->pcallback((void *)dev);
		}
	}
}

static void rx_rspi_spti_isr(const struct device *dev)
{
	struct rx_rspi_data *data = dev->data;
	uint32_t *rxdata = &(data->rxdata);
	rx_rspi_tcb_t *rspi_tcb = &(data->tcb);

	if (RSPI_TRANS_MODE_SW == rspi_tcb->data_tran_mode) {
		if (0 == rspi_tcb->tx_count) {
			*rxdata = data->preg->SPDR.LONG;
		}

/**
 * If master mode then disable further SPTI interrupts on first transmit.
 * If slave mode then we do two transmits to fill the double buffer,
 * then disable SPTI interrupts.
 * The receive interrupt will handle any remaining data.
 */
#if RSPI_CFG_HIGH_SPEED_READ == 0
		if ((data->preg->SPCR.BIT.MSTR) || (rspi_tcb->txcount > 0)) {
			data->preg->SPCR.BIT.SPTIE = 0;
		}
#endif
		rx_rspi_spti_sub(dev);

		if (rspi_tcb->transfer_mode & RSPI_DO_RX) {
			/* Count was incremented in the call to rx_rspi_spti_sub. */
			if ((data->preg->SPCR.BIT.MSTR) || (rspi_tcb->tx_count > 1)) {
				/* Enables saving of receive data on next receive interrupt. */
				rspi_tcb->do_rx_now = true;
			}
		}
	} else {
		R_RSPI_DisableSpti(data->rspi);
		R_RSPI_IntSptiIerClear(data->rspi);
	}
}

static void rx_rspi_spii_isr(const struct device *dev)
{
	struct rx_rspi_data *data = dev->data;

	if (data->tcb.rx_count >= data->tcb.xfr_length) {
		spi_context_update_rx(&data->ctx, data->dfs, data->data_len);
	}
	if (data->tcb.tx_count >= data->tcb.xfr_length) {
		spi_context_update_tx(&data->ctx, data->dfs, data->data_len);
	}
	if (rx_spi_transfer_ongoing(data)) {
		data->preg->SPCR2.BIT.SPIIE = 0;
		rx_rspi_retransmit(data);
	} else {
		uint8_t status_flags = data->preg->SPSR.BYTE;
		rspi_evt_t event = RSPI_EVT_ERR_UNDEF;
		rspi_callback_data_t *rspi_cb_data = &(data->callback_data);

		rspi_cb_data->event_code = event;
		if ((status_flags & RSPI_SPSR_IDLNF) == 0x00) {
			/* Disable idle interrupt requests of the RSPI. */
			data->preg->SPCR2.BIT.SPIIE = 0;

			/* Disable RSPI */
			data->preg->SPCR.BIT.SPE = 0;

			/**
			 * Transfer complete. Call the user callback function passing pointer to the
			 * result structure.
			 */
			if (data->rspi->pcallback != NULL) {
				rspi_cb_data->handle = data->rspi;
				rspi_cb_data->event_code = RSPI_EVT_TRANSFER_COMPLETE;
				data->rspi->pcallback((void *)dev);
			}
		}
	}
}

static void rx_rspi_spei_isr(const struct device *dev)
{
	struct rx_rspi_data *data = dev->data;
	uint8_t status_flags = data->preg->SPSR.BYTE;
	rspi_evt_t event = RSPI_EVT_ERR_UNDEF;
	rspi_callback_data_t *rspi_cb_data = &(data->callback_data);

	/* Identify and clear error condition. */
	if (status_flags & RSPI_SPSR_OVRF) {
		event = RSPI_EVT_ERR_READ_OVF;
		/* Clear error source: OVRF flag. */
		data->preg->SPSR.BIT.OVRF = 0;
		goto error_callback;
	}

	if (status_flags & RSPI_SPSR_MODF) {
		if (status_flags & RSPI_SPSR_UDRF) {
			event = RSPI_EVT_ERR_UNDER_RUN;
			/* Clear error source: MODF flag and UDRF. */
			data->preg->SPSR.BYTE &= RSPI_SPSR_MODF_UDRF_MASK;
		} else {
			event = RSPI_EVT_ERR_MODE_FAULT;
			/* Clear error source: MODF flag. */
			data->preg->SPSR.BIT.MODF = 0;
		}
		goto error_callback;
	}

	if (status_flags & RSPI_SPSR_PERF) {
		event = RSPI_EVT_ERR_PARITY;
		/* Clear error source: PERF flag. */
		data->preg->SPSR.BIT.PERF = 0;
		goto error_callback;
	}
error_callback:
	rspi_cb_data->event_code = event;

	/* Disable the RSPI operation. */
	data->preg->SPCR.BYTE &= (uint8_t)(~((RSPI_SPCR_SPTIE | RSPI_SPCR_SPRIE) | RSPI_SPCR_SPE));

	/* Disable idle interrupt requests of the RSPI. */
	data->preg->SPCR2.BIT.SPIIE = 0;

	/* Call the user callback function passing pointer to the result structure. */
	if (data->rspi->pcallback != NULL) {
		rspi_cb_data->handle = data->rspi;
		data->rspi->pcallback((void *)dev);
	}
}

#endif

#if defined(CONFIG_SPI_RENESAS_RX_INTERRUPT)

#define RX_RSPI_IRQ_CONFIG_INIT(n)                                                                 \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, spri, irq), DT_INST_IRQ_BY_NAME(n, spri, priority),     \
		    rx_rspi_spri_isr, DEVICE_DT_INST_GET(n), 0);                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, spti, irq), DT_INST_IRQ_BY_NAME(n, spti, priority),     \
		    rx_rspi_spti_isr, DEVICE_DT_INST_GET(n), 0);                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, spii, irq), DT_INST_IRQ_BY_NAME(n, spii, priority),     \
		    rx_rspi_spii_isr, DEVICE_DT_INST_GET(n), 0);                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, spei, irq), DT_INST_IRQ_BY_NAME(n, spei, priority),     \
		    rx_rspi_spei_isr, DEVICE_DT_INST_GET(n), 0);                                   \
                                                                                                   \
	irq_enable(DT_INST_IRQ_BY_NAME(n, spri, irq));                                             \
	irq_enable(DT_INST_IRQ_BY_NAME(n, spti, irq));                                             \
	irq_enable(DT_INST_IRQ_BY_NAME(n, spei, irq));

#else

#define RX_RSPI_IRQ_CONFIG_INIT(n)

#endif

#define RX_RSPI_INIT(n)                                                                            \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct rx_rspi_config rx_rspi_config_##n = {                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
	static struct rx_rspi_data rx_rspi_data_##n = {                                            \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
			SPI_CONTEXT_INIT_LOCK(rx_rspi_data_##n, ctx),                              \
		SPI_CONTEXT_INIT_SYNC(rx_rspi_data_##n, ctx),                                      \
		.preg = (struct st_rspi *)DT_INST_REG_ADDR(n),                                     \
		.channel_id = DT_INST_PROP(n, channel),                                            \
		.ssl_assert = DT_INST_PROP(n, ssl_assert),                                         \
	};                                                                                         \
	static int rspi_rx_init##n(const struct device *dev)                                       \
	{                                                                                          \
		int err = rspi_rx_init(dev);                                                       \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		RX_RSPI_IRQ_CONFIG_INIT(n);                                                        \
		return 0;                                                                          \
	}                                                                                          \
	SPI_DEVICE_DT_INST_DEFINE(n, rspi_rx_init##n, PM_DEVICE_DT_INST_GET(n), &rx_rspi_data_##n, \
				  &rx_rspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,      \
				  &rx_spi_api);

DT_INST_FOREACH_STATUS_OKAY(RX_RSPI_INIT)
