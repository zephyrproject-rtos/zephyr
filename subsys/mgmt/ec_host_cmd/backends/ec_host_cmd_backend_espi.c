/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>

LOG_MODULE_REGISTER(host_cmd_espi, CONFIG_EC_HC_LOG_LEVEL);

#define RX_HEADER_SIZE (sizeof(struct ec_host_cmd_request_header))

/* eSPI Host Command state */
enum ec_host_cmd_espi_state {
	/* Interface is disabled */
	ESPI_STATE_DISABLED,
	/* Ready to receive next request */
	ESPI_STATE_READY_TO_RECV,
	/* Processing request */
	ESPI_STATE_PROCESSING,
	/* Processing request */
	ESPI_STATE_SENDING,
	ESPI_STATE_COUNT,
};

struct ec_host_cmd_espi_ctx {
	/* eSPI device instance */
	const struct device *espi_dev;
	/* Context for read operation */
	struct ec_host_cmd_rx_ctx *rx_ctx;
	/* Transmit buffer */
	struct ec_host_cmd_tx_buf *tx;
	/* eSPI callback */
	struct espi_callback espi_cb;
	/* eSPI Host Command state */
	enum ec_host_cmd_espi_state state;
};

#define EC_HOST_CMD_ESPI_DEFINE(_name)                                                             \
	static struct ec_host_cmd_espi_ctx _name##_hc_espi;                                        \
	struct ec_host_cmd_backend _name = {                                                       \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = (struct ec_host_cmd_espi_ctx *)&_name##_hc_espi,                            \
	}

static void espi_handler(const struct device *dev, struct espi_callback *cb,
			 struct espi_event espi_evt)
{
	struct ec_host_cmd_espi_ctx *hc_espi =
		CONTAINER_OF(cb, struct ec_host_cmd_espi_ctx, espi_cb);
	uint16_t event_type = (uint16_t)espi_evt.evt_details;
	/* tx stores the shared memory buf pointer and size, so use it */
	const struct ec_host_cmd_request_header *rx_header = hc_espi->tx->buf;
	const size_t shared_size = hc_espi->tx->len_max;
	const uint16_t rx_valid_data_size = rx_header->data_len + RX_HEADER_SIZE;

	if (event_type != ESPI_PERIPHERAL_EC_HOST_CMD) {
		return;
	}

	/* Make sure we've received a Host Command in a good state not to override buffers for
	 * a Host Command that is currently being processed. There is a moment between sending
	 * a response and setting state to ESPI_STATE_READY_TO_RECV when we can receive a new
	 * host command, so accept the sending state as well.
	 */
	if (hc_espi->state != ESPI_STATE_READY_TO_RECV && hc_espi->state != ESPI_STATE_SENDING) {
		LOG_ERR("Received HC in bad state");
		return;
	}

	/* Only support version 3 and make sure the number of bytes to copy is not
	 * bigger than rx buf size or the shared memory size
	 */
	if (rx_header->prtcl_ver != 3 ||
	    rx_valid_data_size > CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE ||
	    rx_valid_data_size > shared_size) {
		memcpy(hc_espi->rx_ctx->buf, (void *)rx_header, RX_HEADER_SIZE);
		hc_espi->rx_ctx->len = RX_HEADER_SIZE;
	} else {
		memcpy(hc_espi->rx_ctx->buf, (void *)rx_header, rx_valid_data_size);
		hc_espi->rx_ctx->len = rx_valid_data_size;
	}

	/* Even in case of errors, let the general handler send response */
	hc_espi->state = ESPI_STATE_PROCESSING;
	k_sem_give(&hc_espi->rx_ctx->handler_owns);
}

static int ec_host_cmd_espi_init(const struct ec_host_cmd_backend *backend,
				 struct ec_host_cmd_rx_ctx *rx_ctx, struct ec_host_cmd_tx_buf *tx)
{
	struct ec_host_cmd_espi_ctx *hc_espi = (struct ec_host_cmd_espi_ctx *)backend->ctx;

	hc_espi->state = ESPI_STATE_DISABLED;

	if (!device_is_ready(hc_espi->espi_dev)) {
		return -ENODEV;
	}

	hc_espi->rx_ctx = rx_ctx;
	hc_espi->tx = tx;

	espi_init_callback(&hc_espi->espi_cb, espi_handler, ESPI_BUS_PERIPHERAL_NOTIFICATION);
	espi_add_callback(hc_espi->espi_dev, &hc_espi->espi_cb);
	/* Use shared memory as the tx buffer */
	espi_read_lpc_request(hc_espi->espi_dev, ECUSTOM_HOST_CMD_GET_PARAM_MEMORY,
			      (uint32_t *)&tx->buf);
	espi_read_lpc_request(hc_espi->espi_dev, ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE,
			      &tx->len_max);

	hc_espi->state = ESPI_STATE_READY_TO_RECV;

	return 0;
}

static int ec_host_cmd_espi_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_espi_ctx *hc_espi = (struct ec_host_cmd_espi_ctx *)backend->ctx;
	struct ec_host_cmd_response_header *resp_hdr = hc_espi->tx->buf;
	uint32_t result = resp_hdr->result;
	int ret;

	hc_espi->state = ESPI_STATE_SENDING;

	/* Data to transfer are already in the tx buffer (shared memory) */
	ret = espi_write_lpc_request(hc_espi->espi_dev, ECUSTOM_HOST_CMD_SEND_RESULT, &result);
	hc_espi->state = ESPI_STATE_READY_TO_RECV;

	return ret;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = &ec_host_cmd_espi_init,
	.send = &ec_host_cmd_espi_send,
};

EC_HOST_CMD_ESPI_DEFINE(ec_host_cmd_espi);
struct ec_host_cmd_backend *ec_host_cmd_backend_get_espi(const struct device *dev)
{
	((struct ec_host_cmd_espi_ctx *)(ec_host_cmd_espi.ctx))->espi_dev = dev;
	return &ec_host_cmd_espi;
}

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_host_cmd_espi_backend))
static int host_cmd_init(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_host_cmd_backend));

	ec_host_cmd_init(ec_host_cmd_backend_get_espi(dev));
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
#endif
