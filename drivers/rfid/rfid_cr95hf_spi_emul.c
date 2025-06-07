/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT st_cr95hf

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_emul_cr95hf, LOG_LEVEL_DBG);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/kernel.h>

struct cr95hf_emul_data {
	uint8_t next_response[256];
	size_t next_resp_len;
	size_t rest;
	size_t offset;
};

struct rfid_cr95hf_emul_config {
	const struct gpio_dt_spec irq_out;
};

static int cr95hf_emul_io(const struct emul *cr95hf_emul, const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct cr95hf_emul_data *data = cr95hf_emul->data;
	const struct rfid_cr95hf_emul_config *cfg = cr95hf_emul->cfg;
	const struct gpio_dt_spec *irq_out = &cfg->irq_out;

	uint8_t transmit[256] = {0};
	size_t trans_len = 0;

	uint8_t response[256] = {0};
	size_t resp_len = 0;

	if (data->rest > 0) {
		const struct spi_buf *buf = &rx_bufs->buffers[0];
		size_t len = MIN(buf->len, data->rest);

		memcpy(buf->buf, data->next_response + data->offset, len);

		LOG_DBG("CR95HF TX: %02x %02x (len=%d)",
			len > 0 ? data->next_response[data->offset] : 0,
			len > 1 ? data->next_response[data->offset + 1] : 0, len);

		if (len != data->rest) {
			data->rest = data->rest - len;
			data->offset = data->offset + len;
		} else {
			data->rest = 0;
			data->offset = 0;
		}
		return 0;
	}

	if (tx_bufs) {
		if (tx_bufs->count == 1) {
			memcpy(transmit, tx_bufs->buffers[0].buf, tx_bufs->buffers[0].len);
			trans_len = tx_bufs->buffers[0].len;
		}

		LOG_DBG("CR95HF RX: %02x %02x (len=%d)", trans_len > 0 ? transmit[0] : 0,
			trans_len > 1 ? transmit[1] : 0, trans_len);
	}

	if (trans_len == 1 && transmit[0] == 0x01) {
		/* Reset Received -> No response*/
		resp_len = 0;
	} else if (trans_len == 2 && transmit[0] == 0x00 && transmit[1] == 0x55) {
		/* Echo Request received -> Response Read with Echo 0x55*/
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x55;
		data->next_resp_len = 2;
		resp_len = 0;
	} else if (trans_len == 1 && transmit[0] == 0x02) {
		/* Read Command received -> Response prepared data*/
		memcpy(response, data->next_response, data->next_resp_len);
		resp_len = data->next_resp_len;
		data->next_resp_len = 0;
	} else if (trans_len == 17 && transmit[0] == 0x00 && transmit[1] == 0x07) {
		/* Tag Detector Mode */
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x00;
		data->next_response[2] = 0x01;
		data->next_response[3] = 0x01;
		data->next_resp_len = 4;
		resp_len = 0;
		/* Set irq_out to low to indicate a wakeup */
		gpio_pin_set_dt(irq_out, 1);
	} else if (trans_len == 7 && transmit[0] == 0x00 && transmit[1] == 0x02) {
		/* ISO14443-A selected */
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x00;
		data->next_response[2] = 0x00;
		data->next_resp_len = 3;
		resp_len = 0;
	} else if (trans_len == 5 && transmit[0] == 0x00 && transmit[3] == 0x26) {
		/* REQA received -> send ATQA */
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x80;
		data->next_response[2] = 0x05;
		data->next_response[3] = 0x04;
		data->next_response[4] = 0x00;
		data->next_response[5] = 0x28;
		data->next_response[6] = 0x00;
		data->next_response[7] = 0x00;
		data->next_resp_len = 8;
		resp_len = 0;
	} else if (trans_len == 6 && transmit[0] == 0x00 && transmit[3] == 0x93) {
		/* CL1 Received  */
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x80;
		data->next_response[2] = 0x08;
		data->next_response[3] = 0x08;
		data->next_response[4] = 0x19;
		data->next_response[5] = 0x2D;
		data->next_response[6] = 0xA2;
		data->next_response[7] = 0x9E;
		data->next_response[8] = 0x28;
		data->next_response[9] = 0x00;
		data->next_response[10] = 0x00;
		data->next_resp_len = 11;
		resp_len = 0;
	} else if (trans_len == 11 && transmit[0] == 0x00 && transmit[3] == 0x93) {
		/* CL1 Complete */
		data->next_response[0] = 0x00; /* Dummy Byte while read command*/
		data->next_response[1] = 0x80;
		data->next_response[2] = 0x06;
		data->next_response[3] = 0x20;
		data->next_response[4] = 0xFC;
		data->next_response[5] = 0x70;
		data->next_response[6] = 0x08;
		data->next_response[7] = 0x00;
		data->next_response[8] = 0x00;
		data->next_resp_len = 9;
		resp_len = 0;
	} else {
		LOG_ERR("Answer for this request is not yet implemented!");
	}

	if (rx_bufs && resp_len > 0) {
		const struct spi_buf *buf = &rx_bufs->buffers[0];
		size_t len = MIN(buf->len, resp_len);

		memcpy(buf->buf, response, len);

		if (len != resp_len) {
			data->rest = resp_len - len;
			data->offset = len;
		} else {
			data->rest = 0;
			data->offset = 0;
		}

		LOG_DBG("CR95HF TX: %02x %02x (len=%d)", len > 0 ? response[0] : 0,
			len > 1 ? response[1] : 0, len);
	}

	return 0;
}

static struct spi_emul_api cr95hf_emul_api = {
	.io = cr95hf_emul_io,
};

static int cr95hf_emul_init(const struct emul *cr95hf_emul, const struct device *parent)
{
	struct cr95hf_emul_data *data = cr95hf_emul->data;
	const struct rfid_cr95hf_emul_config *config = cr95hf_emul->cfg;
	const struct gpio_dt_spec *irq_out = &config->irq_out;

	data->next_resp_len = 0;
	memset(data->next_response, 0, sizeof(data->next_response));
	data->rest = 0;

	int ret = gpio_pin_configure_dt(irq_out, GPIO_OUTPUT_INACTIVE);

	if (ret) {
		LOG_ERR("Failed to configure IRQ GPIO (err %d)", ret);
		return ret;
	}
	LOG_INF("CR95HF Emulator initialized");
	return 0;
}

#define DEFINE_SPI_EMUL(n)                                                                         \
	static struct cr95hf_emul_data cr95hf_emul_data_##n;                                       \
	static const struct rfid_cr95hf_emul_config rfid_device_prv_config_##n = {                 \
		.irq_out = GPIO_DT_SPEC_INST_GET_OR(n, irq_out_gpios, {0}),                        \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, &cr95hf_emul_init, &cr95hf_emul_data_##n,                           \
			    &rfid_device_prv_config_##n, &cr95hf_emul_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SPI_EMUL)
