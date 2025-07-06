/*
 * Copyright (c) 2024 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * IEEE 802.15.4 HDLC RCP interface - serial communication interface (UART)
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */
#include <zephyr/drivers/uart.h>
#include <openthread/platform/radio.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/hdlc_rcp_if/hdlc_rcp_if.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/openthread.h>
#include <zephyr/sys/ring_buffer.h>

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */

#define DT_DRV_COMPAT uart_hdlc_rcp_if

#define LOG_MODULE_NAME hdlc_rcp_if_uart
#define LOG_LEVEL       CONFIG_HDLC_RCP_IF_DRIVER_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct openthread_uart {
	struct k_work work;
	struct ring_buf *rx_ringbuf;
	struct ring_buf *tx_ringbuf;
	const struct device *dev;
	atomic_t tx_busy;

	hdlc_rx_callback_t cb;
	void *param;
};

#define OT_UART_DEFINE(_name, _ringbuf_rx_size, _ringbuf_tx_size)  \
	RING_BUF_DECLARE(_name##_rx_ringbuf, _ringbuf_rx_size); \
	RING_BUF_DECLARE(_name##_tx_ringbuf, _ringbuf_tx_size); \
	static struct openthread_uart _name = { \
		.rx_ringbuf = &_name##_rx_ringbuf, \
		.tx_ringbuf = &_name##_tx_ringbuf, \
	}
OT_UART_DEFINE(ot_uart, CONFIG_OPENTHREAD_HDLC_RCP_IF_UART_RX_RING_BUFFER_SIZE,
	       CONFIG_OPENTHREAD_HDLC_RCP_IF_UART_TX_RING_BUFFER_SIZE);

static struct ot_hdlc_rcp_context {
	struct net_if *iface;
	struct openthread_context *ot_context;
} ot_hdlc_rcp_ctx;

/* -------------------------------------------------------------------------- */
/*                             Private functions                              */
/* -------------------------------------------------------------------------- */

static void ot_uart_rx_cb(struct k_work *item)
{
	struct openthread_uart *otuart =
		CONTAINER_OF(item, struct openthread_uart, work);
	uint8_t *data;
	uint32_t len;

	len = ring_buf_get_claim(otuart->rx_ringbuf, &data,
				 otuart->rx_ringbuf->size);
	if (len > 0) {
		otuart->cb(data, len, otuart->param);
		ring_buf_get_finish(otuart->rx_ringbuf, len);
	}
}

static void uart_tx_handle(const struct device *dev)
{
	uint32_t tx_len = 0, len;
	uint8_t *data;

	len = ring_buf_get_claim(
		ot_uart.tx_ringbuf, &data,
		ot_uart.tx_ringbuf->size);
	if (len > 0) {
		tx_len = uart_fifo_fill(dev, data, len);
		int err = ring_buf_get_finish(ot_uart.tx_ringbuf, tx_len);
		(void)err;
		__ASSERT_NO_MSG(err == 0);
	} else {
		uart_irq_tx_disable(dev);
	}
}

static void uart_rx_handle(const struct device *dev)
{
	uint32_t rd_len = 0, len;
	uint8_t *data;

	len = ring_buf_put_claim(
		ot_uart.rx_ringbuf, &data,
		ot_uart.rx_ringbuf->size);
	if (len > 0) {
		rd_len = uart_fifo_read(dev, data, len);

		int err = ring_buf_put_finish(ot_uart.rx_ringbuf, rd_len);
		(void)err;
		__ASSERT_NO_MSG(err == 0);
	}
}

static void uart_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_rx_handle(dev);
		}

		if (uart_irq_tx_ready(dev)) {
			uart_tx_handle(dev);
		}
	}

	if (ring_buf_size_get(ot_uart.rx_ringbuf) > 0) {
		k_work_submit(&ot_uart.work);
	}
}

static void hdlc_iface_init(struct net_if *iface)
{
	struct ot_hdlc_rcp_context *ctx = net_if_get_device(iface)->data;
	otExtAddress eui64;

	ot_uart.dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ot_uart));

	if (!device_is_ready(ot_uart.dev)) {
		LOG_ERR("UART device not ready");
	}

	uart_irq_callback_user_data_set(ot_uart.dev,
					uart_callback,
					(void *)&ot_uart);

	ctx->iface = iface;
	ieee802154_init(iface);
	ctx->ot_context = net_if_l2_data(iface);

	otPlatRadioGetIeeeEui64(ctx->ot_context->instance, eui64.m8);
	net_if_set_link_addr(iface, eui64.m8, OT_EXT_ADDRESS_SIZE,
			     NET_LINK_IEEE802154);
}

static int hdlc_register_rx_cb(hdlc_rx_callback_t hdlc_rx_callback, void *param)
{
	ot_uart.cb = hdlc_rx_callback;
	ot_uart.param = param;

	k_work_init(&ot_uart.work, ot_uart_rx_cb);
	uart_irq_rx_enable(ot_uart.dev);

	return 0;
}

static int hdlc_send(const uint8_t *frame, uint16_t length)
{
	uint32_t ret;

	if (frame == NULL) {
		return -EIO;
	}

	ret = ring_buf_put(ot_uart.tx_ringbuf, frame, length);
	uart_irq_tx_enable(ot_uart.dev);

	if (ret < length) {
		LOG_WRN("Cannot store full frame to RB (%d < %d)", ret, length);
		return -EIO;
	}

	return 0;
}

static int hdlc_deinit(void)
{
	uart_irq_tx_disable(ot_uart.dev);
	uart_irq_rx_disable(ot_uart.dev);

	ring_buf_reset(ot_uart.rx_ringbuf);
	ring_buf_reset(ot_uart.tx_ringbuf);

	return 0;
}

static const struct hdlc_api uart_hdlc_api = {
	.iface_api.init = hdlc_iface_init,
	.register_rx_cb = hdlc_register_rx_cb,
	.send = hdlc_send,
	.deinit = hdlc_deinit,
};

#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)

#define MTU 1280

NET_DEVICE_DT_INST_DEFINE(0, NULL,                             /* Initialization Function */
			  NULL,                                /* No PM API support */
			  &ot_hdlc_rcp_ctx,                    /* HDLC RCP context data */
			  NULL,                                /* Configuration info */
			  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, /* Initial priority */
			  &uart_hdlc_api,                       /* API interface functions */
			  OPENTHREAD_L2,                       /* Openthread L2 */
			  NET_L2_GET_CTX_TYPE(OPENTHREAD_L2),  /* Openthread L2 context type */
			  MTU);                                /* MTU size */
