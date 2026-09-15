/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sdio_device.h>
#include <zephyr/sd/sdio_stream.h>
#include <zephyr/drivers/sdio_dc.h>
#include <zephyr/drivers/sdio_dc_virtual.h>

#define DC_NODE   DT_NODELABEL(sdio_dc0)
#define FIFO_REG  0x0

static struct sdio_device endpoint;
static struct sdio_stream_function sf;

/* Host-side interrupt bookkeeping. */
static volatile uint32_t irq_count;

static void irq_cb(const struct device *dc, enum sdio_func_num func, void *user)
{
	ARG_UNUSED(dc);
	ARG_UNUSED(func);
	ARG_UNUSED(user);

	irq_count++;
}

static int host_access(enum sdio_dc_dir dir, uint8_t *data, uint32_t len)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);
	struct sdio_dc_xfer xfer = {
		.func = SDIO_FUNC_NUM_1,
		.dir = dir,
		.reg = FIFO_REG,
		.increment = false,
		.data = data,
		.len = len,
	};

	return sdio_dc_virtual_access(dc, &xfer);
}

static void *setup(void)
{
	const struct device *dc = DEVICE_DT_GET(DC_NODE);

	zassert_true(device_is_ready(dc), "controller not ready");
	zassert_ok(sdio_device_init(&endpoint, dc, NULL));
	zassert_ok(sdio_stream_function_init(&sf, SDIO_FUNC_NUM_1, FIFO_REG));
	zassert_ok(sdio_device_register_function(&endpoint, &sf.base));
	zassert_ok(sdio_device_enable(&endpoint));
	sdio_dc_virtual_set_irq_cb(dc, irq_cb, NULL);

	return NULL;
}

/* A host FIFO write is delivered to the RX path and read out by the device. */
ZTEST(sdio_stream, test_rx_from_host)
{
	uint8_t tx[32];
	uint8_t rx[32];
	int n;

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(0xF0 ^ i);
	}

	zassert_ok(host_access(SDIO_DC_DIR_WRITE, tx, sizeof(tx)));

	n = sdio_stream_read(&sf, rx, sizeof(rx), K_MSEC(100));
	zassert_equal(n, (int)sizeof(tx), "wrong rx length: %d", n);
	zassert_mem_equal(rx, tx, sizeof(tx), "rx payload mismatch");
}

/* poll() reports readable once a host write has landed. */
ZTEST(sdio_stream, test_poll_in)
{
	uint8_t tx[4] = {1, 2, 3, 4};
	uint8_t rx[4];
	uint32_t revents = 0;

	zassert_ok(host_access(SDIO_DC_DIR_WRITE, tx, sizeof(tx)));

	zassert_ok(sdio_stream_poll(&sf, SDIO_STREAM_POLLIN, &revents,
				    K_MSEC(100)));
	zassert_true(revents & SDIO_STREAM_POLLIN, "POLLIN not set");

	/* drain */
	zassert_equal(sdio_stream_read(&sf, rx, sizeof(rx), K_NO_WAIT),
		      (int)sizeof(tx));
}

/* A device write queues a TX packet, raises the interrupt, and the host
 * retrieves the payload by reading the function data port.
 */
ZTEST(sdio_stream, test_tx_to_host)
{
	uint8_t tx[24];
	uint8_t rx[24];
	uint32_t before = irq_count;

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(i + 3);
	}

	zassert_ok(sdio_stream_write(&sf, tx, sizeof(tx)));
	zassert_equal(irq_count, before + 1, "host not interrupted");

	memset(rx, 0, sizeof(rx));
	zassert_ok(host_access(SDIO_DC_DIR_READ, rx, sizeof(rx)));
	zassert_mem_equal(rx, tx, sizeof(tx), "tx payload mismatch");
}

ZTEST_SUITE(sdio_stream, NULL, setup, NULL, NULL, NULL);

/* ----- Zero-copy path (second controller instance) ---------------------- */

#define DC_ZC_NODE DT_NODELABEL(sdio_dc1)

static struct sdio_device zc_endpoint;
static struct sdio_stream_function zc_sf;

static int zc_fifo(enum sdio_dc_dir dir, uint8_t *data, uint32_t len)
{
	const struct device *dc = DEVICE_DT_GET(DC_ZC_NODE);
	struct sdio_dc_xfer xfer = {
		.func = SDIO_FUNC_NUM_1,
		.dir = dir,
		.reg = FIFO_REG,
		.increment = false,
		.data = data,
		.len = len,
	};

	return sdio_dc_virtual_access(dc, &xfer);
}

static void *zc_setup(void)
{
	const struct device *dc = DEVICE_DT_GET(DC_ZC_NODE);

	zassert_true(device_is_ready(dc), "controller not ready");
	zassert_ok(sdio_device_init(&zc_endpoint, dc, NULL));
	zassert_ok(sdio_stream_function_init(&zc_sf, SDIO_FUNC_NUM_1, FIFO_REG));
	zassert_ok(sdio_device_register_function(&zc_endpoint, &zc_sf.base));
	zassert_ok(sdio_device_enable(&zc_endpoint));
	zassert_true(sdio_device_is_zero_copy(&zc_endpoint), "no zero-copy path");
	zassert_ok(sdio_stream_function_start(&zc_sf));
	return NULL;
}

/* An inbound frame lands directly in the pool packet the consumer reads. */
ZTEST(sdio_stream_zc, test_zero_copy_rx)
{
	const struct device *dc = DEVICE_DT_GET(DC_ZC_NODE);
	uint8_t tx[40];
	struct sdio_pkt *pkt;

	for (int i = 0; i < (int)sizeof(tx); i++) {
		tx[i] = (uint8_t)(0x11 * i);
	}

	zassert_ok(zc_fifo(SDIO_DC_DIR_WRITE, tx, sizeof(tx)));

	pkt = sdio_stream_read_pkt(&zc_sf, K_MSEC(100));
	zassert_not_null(pkt, "no packet received");
	zassert_equal(pkt->len, sizeof(tx), "wrong length");
	zassert_mem_equal(pkt->data, tx, sizeof(tx), "payload mismatch");
	/* Zero-copy: the buffer the consumer holds is the one the controller
	 * filled -- no copy took place in the subsystem.
	 */
	zassert_equal_ptr(pkt->data, sdio_dc_virtual_last_rx(dc),
			  "RX was not zero-copy");
	sdio_pkt_free(pkt);
}

/* An outbound packet is handed to the controller without copying. */
ZTEST(sdio_stream_zc, test_zero_copy_tx)
{
	const struct device *dc = DEVICE_DT_GET(DC_ZC_NODE);
	struct sdio_pkt *pkt = sdio_pkt_alloc(SDIO_PKT_TX);
	uint8_t rx[24];
	uint8_t *sent;

	zassert_not_null(pkt, "alloc failed");
	for (int i = 0; i < (int)sizeof(rx); i++) {
		pkt->data[i] = (uint8_t)(0xC0 + i);
	}
	pkt->len = sizeof(rx);
	sent = pkt->data;

	zassert_ok(sdio_stream_write_pkt(&zc_sf, pkt));

	/* Host reads the data port. */
	memset(rx, 0, sizeof(rx));
	zassert_ok(zc_fifo(SDIO_DC_DIR_READ, rx, sizeof(rx)));
	for (int i = 0; i < (int)sizeof(rx); i++) {
		zassert_equal(rx[i], (uint8_t)(0xC0 + i), "payload mismatch");
	}
	/* Zero-copy: the controller sent the exact buffer we submitted. */
	zassert_equal_ptr(sdio_dc_virtual_last_tx(dc), sent,
			  "TX was not zero-copy");
}

ZTEST_SUITE(sdio_stream_zc, NULL, zc_setup, NULL, NULL, NULL);
