/*
 * Copyright (c) 2026 Neoventor AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../test_work.h"
#include <udc_common.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/net_buf.h>
#include <zephyr/usb/usbd.h>

/* Compile the complete driver once, in its usual library. */

#define k_work_submit(work) test_submit_to_queue(&k_sys_work_q, work)
#define k_work_schedule(work, delay) test_schedule_for_queue(&k_sys_work_q, work, delay)
#define k_work_reschedule(work, delay) test_reschedule_for_queue(&k_sys_work_q, work, delay)
#define k_work_submit_to_queue test_submit_to_queue
#define k_work_schedule_for_queue test_schedule_for_queue
#define k_work_reschedule_for_queue test_reschedule_for_queue
#define k_work_cancel test_cancel
#define k_work_cancel_sync test_cancel_sync
#define k_work_cancel_delayable_sync test_cancel_delayable_sync
#include <usbd_cdc_acm.c>
#undef k_work_cancel_delayable_sync
#undef k_work_cancel_sync
#undef k_work_cancel
#undef k_work_reschedule_for_queue
#undef k_work_schedule_for_queue
#undef k_work_submit_to_queue
#undef k_work_reschedule
#undef k_work_schedule
#undef k_work_submit


USBD_CONFIGURATION_DEFINE(rx_fs, 0, 50, NULL);
USBD_CONFIGURATION_DEFINE(rx_hs, 0, 50, NULL);
USBD_DEVICE_DEFINE(rx_device, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);

static const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));

static struct usbd_class_data *rx_class(void)
{
	const struct cdc_acm_uart_config *cfg = uart->config;

	return cfg->c_data;
}

static void release_endpoint(uint8_t ep)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(rx_device.dev, ep);
	struct net_buf *buf;

	zassert_not_null(cfg);
	while ((buf = udc_buf_get(cfg)) != NULL) {
		zassert_ok(usbd_cdc_acm_request(rx_class(), buf, -ECONNABORTED));
	}
}

static void cleanup(void)
{
	struct cdc_acm_uart_data *data = uart->data;
	struct net_buf *buf;

	usbd_cdc_acm_disable(rx_class());
	test_work_reset();
	cdc_acm_irq_callback_set(uart, NULL, NULL);
	cdc_acm_irq_rx_disable(uart);
	cdc_acm_irq_tx_disable(uart);
	release_endpoint(cdc_acm_get_bulk_out(rx_class()));
	release_endpoint(cdc_acm_get_bulk_in(rx_class()));
	while ((buf = k_fifo_get(data->rx_fifo.bufs, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	cleanup();

	zassert_ok(usbd_cdc_acm_init(rx_class()));
	usbd_cdc_acm_enable(rx_class());
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	cleanup();
}

static void *setup(void)
{
	zassert_true(device_is_ready(uart));
	zassert_ok(usbd_add_configuration(&rx_device, USBD_SPEED_FS, &rx_fs));
	zassert_ok(usbd_add_configuration(&rx_device, USBD_SPEED_HS, &rx_hs));
	zassert_ok(usbd_register_class(&rx_device, "cdc_acm_0", USBD_SPEED_FS, 1));
	zassert_ok(usbd_register_class(&rx_device, "cdc_acm_0", USBD_SPEED_HS, 1));
	zassert_ok(usbd_init(&rx_device));
	zassert_ok(usbd_enable(&rx_device));
	zassert_ok(udc_ep_enable(rx_device.dev, cdc_acm_get_bulk_out(rx_class()),
				 USB_EP_TYPE_BULK, cdc_acm_get_bulk_mps(rx_class()), 0));
	zassert_ok(udc_ep_enable(rx_device.dev, cdc_acm_get_bulk_in(rx_class()),
				 USB_EP_TYPE_BULK, cdc_acm_get_bulk_mps(rx_class()), 0));
	return NULL;
}

static uint8_t tx_bytes[USBD_MAX_BULK_MPS * 3U + 7U];
static uint8_t tx_expected[sizeof(tx_bytes) +
	ROUND_UP(DT_PROP(DT_NODELABEL(cdc_acm_uart0), tx_fifo_size), USBD_MAX_BULK_MPS)];
static size_t tx_written;
static bool tx_false_ready;
static bool tx_loop_limit;

static void filling_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	uart_irq_update(dev);
	for (size_t step = 0; step < 16U; step++) {
		int ready = uart_irq_tx_ready(dev);
		int done;

		if (ready <= 0) {
			return;
		}
		done = uart_fifo_fill(dev, &tx_bytes[tx_written],
				     MIN((size_t)ready, sizeof(tx_bytes) - tx_written));
		if (done == 0) {
			tx_false_ready = uart_irq_tx_ready(dev) > 0;
			uart_irq_tx_disable(dev);
			return;
		}
		tx_written += done;
		if (tx_written == sizeof(tx_bytes)) {
			uart_irq_tx_disable(dev);
			return;
		}
	}
	tx_loop_limit = true;
	uart_irq_tx_disable(dev);
}

static size_t receive_tx(struct udc_ep_config *cfg, size_t received, size_t expected)
{
	struct net_buf *buf;

	while ((buf = udc_buf_get(cfg)) != NULL) {
		zassert_true(received + buf->len <= expected);
		zassert_mem_equal(buf->data, &tx_expected[received], buf->len,
				  "host received TX bytes out of order");
		received += buf->len;
		zassert_ok(usbd_cdc_acm_request(rx_class(), buf, 0));
	}
	return received;
}

ZTEST(cdc_acm_work, test_tx_ready_tracks_writable_buffer)
{
	struct cdc_acm_uart_data *data = uart->data;
	struct udc_ep_config *cfg;
	struct net_buf *buf;
	size_t prefix = 0U;
	size_t expected;
	size_t received = 0U;
	size_t written_before_completion;

	cfg = udc_get_ep_cfg(rx_device.dev, cdc_acm_get_bulk_in(rx_class()));
	zassert_not_null(cfg);
	/* No class work runs until test_advance(): snapshot without consuming. */
	for (buf = udc_buf_peek(cfg); buf != NULL;
	     buf = (struct net_buf *)sys_sflist_peek_next((sys_sfnode_t *)buf)) {
		zassert_true(prefix + buf->len <= sizeof(tx_expected) - sizeof(tx_bytes));
		memcpy(&tx_expected[prefix], buf->data, buf->len);
		prefix += buf->len;
	}
	buf = data->tx_fifo.current;
	if (buf != NULL) {
		zassert_true(prefix + buf->len <= sizeof(tx_expected) - sizeof(tx_bytes));
		memcpy(&tx_expected[prefix], buf->data, buf->len);
		prefix += buf->len;
	}
	for (size_t i = 0; i < sizeof(tx_bytes); i++) {
		tx_bytes[i] = (uint8_t)(i + i / USBD_MAX_BULK_MPS);
	}
	memcpy(&tx_expected[prefix], tx_bytes, sizeof(tx_bytes));
	expected = prefix + sizeof(tx_bytes);
	/* Receive queued data and the echo-mitigation ZLP before the callback. */
	test_advance(K_NO_WAIT);
	received = receive_tx(cfg, received, expected);
	data->echo_mitigated = true;
	data->zlp_needed = false;
	tx_written = 0U;
	tx_false_ready = false;
	tx_loop_limit = false;
	zassert_ok(uart_irq_callback_user_data_set(uart, filling_callback, NULL));
	uart_irq_tx_enable(uart);
	/* A successful echo-mitigation ZLP can leave this delay pending. */
	test_advance(K_MSEC(CONFIG_USBD_CDC_ACM_TX_DELAY_MS));
	written_before_completion = prefix + tx_written - received;
	/* Each packet plus a possible ZLP gets a retry and a send turn.
	 * TX retries take 1 ms; leave one final turn to receive the tail.
	 */
	for (size_t step = 0;
	     step < 2U * (DIV_ROUND_UP(expected, USBD_MAX_BULK_MPS) + 1U) + 1U; step++) {
		received = receive_tx(cfg, received, expected);
		if (received == expected) {
			break;
		}
		test_advance(K_MSEC(1));
	}
	zassert_false(tx_false_ready, "ready stayed positive after FIFO rejected data");
	zassert_false(tx_loop_limit, "TX callback exceeded its iteration limit");
	zassert_true(written_before_completion >= USBD_MAX_BULK_MPS * 2U,
		      "TX rotation did not wake callback");
	zassert_equal(tx_written, sizeof(tx_bytes));
	zassert_equal(received, expected, "TX bytes did not reach the host");
}

ZTEST_SUITE(cdc_acm_work, NULL, setup, before, after, NULL);
