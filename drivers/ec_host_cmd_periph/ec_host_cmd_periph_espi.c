/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ec_host_cmd_periph_espi

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/ec_host_cmd_periph/ec_host_cmd_periph.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/mgmt/ec_host_cmd.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Invalid number of eSPI peripherals");

#define ESPI_BUS    DT_PHANDLE(DT_DRV_INST(0), bus)
#define ESPI_DEVICE DEVICE_DT_GET(ESPI_BUS)

struct ec_host_cmd_periph_espi_data {
	struct k_sem handler_owns;
	struct k_sem dev_owns;
	uint32_t rx_buffer_len;
	struct espi_callback espi_cb;
	uint8_t *espi_shm;
};

static void ec_host_cmd_periph_espi_handler(const struct device *dev, struct espi_callback *cb,
					    struct espi_event espi_evt)
{
	struct ec_host_cmd_periph_espi_data *data =
		CONTAINER_OF(cb, struct ec_host_cmd_periph_espi_data, espi_cb);
	uint16_t event_type = (uint16_t)espi_evt.evt_details;

	if (event_type != ESPI_PERIPHERAL_EC_HOST_CMD) {
		return;
	}

	if (k_sem_take(&data->dev_owns, K_NO_WAIT) != 0) {
		int res = EC_HOST_CMD_IN_PROGRESS;

		espi_write_lpc_request(ESPI_DEVICE, ECUSTOM_HOST_CMD_SEND_RESULT, &res);
		return;
	}

	k_sem_give(&data->handler_owns);
}

int ec_host_cmd_periph_espi_init(const struct device *dev, struct ec_host_cmd_periph_rx_ctx *rx_ctx)
{
	struct ec_host_cmd_periph_espi_data *data = dev->data;

	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	rx_ctx->buf = data->espi_shm;
	rx_ctx->len = &data->rx_buffer_len;
	rx_ctx->dev_owns = &data->dev_owns;
	rx_ctx->handler_owns = &data->handler_owns;

	return 0;
}

int ec_host_cmd_periph_espi_send(const struct device *dev,
				 const struct ec_host_cmd_periph_tx_buf *buf)
{
	struct ec_host_cmd_periph_espi_data *data = dev->data;
	struct ec_host_cmd_response_header *resp_hdr = buf->buf;
	uint32_t result = resp_hdr->result;

	memcpy(data->espi_shm, buf->buf, buf->len);

	return espi_write_lpc_request(ESPI_DEVICE, ECUSTOM_HOST_CMD_SEND_RESULT, &result);
}

static const struct ec_host_cmd_periph_api ec_host_cmd_api = {
	.init = &ec_host_cmd_periph_espi_init,
	.send = &ec_host_cmd_periph_espi_send,
};

static int ec_host_cmd_espi_init(const struct device *dev)
{
	struct ec_host_cmd_periph_espi_data *data = dev->data;

	/* Allow writing to rx buff at startup and block on reading. */
	k_sem_init(&data->handler_owns, 0, 1);
	k_sem_init(&data->dev_owns, 1, 1);

	espi_init_callback(&data->espi_cb, ec_host_cmd_periph_espi_handler,
			   ESPI_BUS_PERIPHERAL_NOTIFICATION);
	espi_add_callback(ESPI_DEVICE, &data->espi_cb);

	espi_read_lpc_request(ESPI_DEVICE, ECUSTOM_HOST_CMD_GET_PARAM_MEMORY,
			      (uint32_t *)&data->espi_shm);
	espi_read_lpc_request(ESPI_DEVICE, ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE,
			      &data->rx_buffer_len);

	return 0;
}

/* Assume only one peripheral */
static struct ec_host_cmd_periph_espi_data espi_data;
DEVICE_DT_INST_DEFINE(0, ec_host_cmd_espi_init, NULL, &espi_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ec_host_cmd_api);
