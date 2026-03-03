/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <sample_usbd.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
static const struct device *uart_host;

/* Host TX and RX buffers, at least two bulk endpoint MPS. */
#define TEST_BUF_SIZE (2 * 512)

/*
 * Device side (IRQ/FIFO API driven) loopback. Everything received from the
 * host is echoed back.
 */
#define RING_BUF_SIZE TEST_BUF_SIZE
static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf ringbuf;
static bool rx_throttled;

static void dev_interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (true) {
		uint8_t *data;
		uint32_t len;
		int ret;

		uart_irq_update(dev);
		if (uart_irq_is_pending(dev) <= 0) {
			break;
		}

		/* Read straight from the RX FIFO into the ring buffer. */
		if (!rx_throttled && uart_irq_rx_ready(dev)) {
			len = ring_buf_put_claim(&ringbuf, &data, sizeof(ring_buffer));
			if (len == 0) {
				/* Throttle because ring buffer is full */
				uart_irq_rx_disable(dev);
				rx_throttled = true;
			} else {
				ret = uart_fifo_read(dev, data, len);
				ring_buf_put_finish(&ringbuf, ret);
				if (ret > 0) {
					uart_irq_tx_enable(dev);
				}
			}
		}

		/* Fill the TX FIFO straight from the ring buffer. */
		if (uart_irq_tx_ready(dev)) {
			len = ring_buf_get_claim(&ringbuf, &data, sizeof(ring_buffer));
			if (len == 0) {
				uart_irq_tx_disable(dev);
			} else {
				ret = uart_fifo_fill(dev, data, len);
				ring_buf_get_finish(&ringbuf, ret);

				if (rx_throttled) {
					uart_irq_rx_enable(dev);
					rx_throttled = false;
				}
			}
		}
	}
}

/* Host side (UART ASYNC API) test helper. */
static K_SEM_DEFINE(tx_done, 0, 1);
static K_SEM_DEFINE(tx_aborted, 0, 1);
static K_SEM_DEFINE(rx_rdy, 0, 1);
static K_SEM_DEFINE(rx_disabled, 0, 1);

UDC_STATIC_BUF_DEFINE(tx_buf, TEST_BUF_SIZE);

/* Ping-pong RX buffers and the accumulated received data. */
UDC_STATIC_BUF_DEFINE(rx_buf0, TEST_BUF_SIZE);
UDC_STATIC_BUF_DEFINE(rx_buf1, TEST_BUF_SIZE);
static uint8_t rx_data[TEST_BUF_SIZE];
static size_t rx_data_len;

/* Total number of bytes received, used by the throughput test. */
static uint32_t rx_total;
/* Length used for each RX buffer, equal to the transfer length under test. */
static size_t rx_buf_len;
/* Released RX buffer available to be supplied again for the ping-pong. */
static uint8_t *rx_free_buf;

static void uart_cb_async_handler(const struct device *const dev,
				  struct uart_event *const evt, void *const app_data)
{
	ARG_UNUSED(app_data);

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("TX_DONE, data %p length %zu",
			(void *)evt->data.tx.buf, evt->data.tx.len);
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		LOG_INF("TX_ABORTED");
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		LOG_DBG("RX_RDY offset %zu length %zu",
			evt->data.rx.offset, evt->data.rx.len);
		rx_total += evt->data.rx.len;
		if (rx_data_len + evt->data.rx.len <= sizeof(rx_data)) {
			memcpy(&rx_data[rx_data_len],
			       &evt->data.rx.buf[evt->data.rx.offset],
			       evt->data.rx.len);
			rx_data_len += evt->data.rx.len;
		}

		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_REQUEST:
		if (rx_free_buf != NULL) {
			uint8_t *const buf = rx_free_buf;

			rx_free_buf = NULL;
			uart_rx_buf_rsp(dev, buf, rx_buf_len);
		}
		break;
	case UART_RX_BUF_RELEASED:
		rx_free_buf = evt->data.rx_buf.buf;
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		LOG_WRN("Unhandled UART event type: %d", evt->type);
		break;
	}
}

static void test_reset(void)
{
	rx_data_len = 0;
	rx_total = 0;
	rx_free_buf = NULL;
	memset(rx_data, 0, sizeof(rx_data));
	k_sem_reset(&tx_done);
	k_sem_reset(&tx_aborted);
	k_sem_reset(&rx_rdy);
	k_sem_reset(&rx_disabled);
}

ZTEST(cdc_acm_test, test_ready)
{
	zassert_true(device_is_ready(uart_dev),
		     "CDC ACM device %s not ready", uart_dev->name);

	zassert_true(device_is_ready(uart_host),
		     "CDC ACM device %s not ready", uart_host->name);

	LOG_INF("Host UART %s <-> Device UART %s", uart_host->name, uart_dev->name);
}

/*
 * Send len bytes from tx and verify that they are echoed back unchanged into
 * the rx0/rx1 ping-pong buffers. The RX buffer length is set equal to len so
 * that a transfer that is a multiple of the bulk MPS completes on a full buffer
 * instead of waiting for a (never sent) short packet. Released buffers are
 * supplied again so reception can span several buffers regardless of how the
 * device chunks the echo.
 */
static void cdc_acm_loopback_buf(uint8_t *const tx, uint8_t *const rx0,
				 uint8_t *const rx1, const size_t len)
{
	int err;

	test_reset();

	for (size_t i = 0; i < len; i++) {
		tx[i] = (uint8_t)i;
	}

	rx_buf_len = len;
	rx_free_buf = rx1;

	err = uart_rx_enable(uart_host, rx0, len, SYS_FOREVER_US);
	zassert_ok(err, "uart_rx_enable() failed with %d (len %zu)", err, len);

	err = uart_tx(uart_host, tx, len, SYS_FOREVER_US);
	zassert_ok(err, "uart_tx() failed with %d (len %zu)", err, len);

	/* A second transfer while the first is still pending is rejected. */
	err = uart_tx(uart_host, tx, len, SYS_FOREVER_US);
	zassert_equal(err, -EBUSY, "Concurrent uart_tx() not rejected (%d)", err);

	zassert_ok(k_sem_take(&tx_done, K_MSEC(1000)), "TX_DONE timeout (len %zu)", len);

	/* Wait for the echoed data to be received back. */
	while (rx_data_len < len) {
		zassert_ok(k_sem_take(&rx_rdy, K_MSEC(1000)),
			   "RX_RDY timeout (len %zu)", len);
	}

	zassert_equal(rx_data_len, len,
		      "Received %zu bytes, expected %zu", rx_data_len, len);
	zassert_mem_equal(rx_data, tx, len, "Loopback data mismatch (len %zu)", len);

	err = uart_rx_disable(uart_host);
	zassert_ok(err, "uart_rx_disable() failed with %d (len %zu)", err, len);
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(1000)),
		   "RX_DISABLED timeout (len %zu)", len);
}

static void cdc_acm_loopback(const size_t len)
{
	cdc_acm_loopback_buf(tx_buf, rx_buf0, rx_buf1, len);
}

/*
 * Send len bytes and verify that they are echoed back unchanged, for several
 * transfer sizes.
 */
ZTEST(cdc_acm_test, test_loopback_aligned)
{
	/* Below, equal to and above the bulk endpoint MPS, and a multiple of it. */
	static const size_t sizes[] = {1, 63, 64, 512, 513, 1024};

	ARRAY_FOR_EACH(sizes, i) {
		cdc_acm_loopback(sizes[i]);
	}
}

/*
 * Exercise unaligned (non DMA-able) buffers by offsetting one byte into the
 * aligned static buffers. With bounce buffers enabled the driver copies through
 * an aligned buffer; otherwise the request is rejected with -ENOTSUP.
 */
ZTEST(cdc_acm_test, test_loopback_unaligned)
{
	uint8_t *const tx = &tx_buf[1];
	uint8_t *const rx0 = &rx_buf0[1];
	uint8_t *const rx1 = &rx_buf1[1];
	const size_t len = 64;

	if (!IS_ENABLED(CONFIG_USBH_CDC_ACM_BOUNCE)) {
		int err;

		test_reset();
		rx_buf_len = len;
		rx_free_buf = rx1;

		err = uart_rx_enable(uart_host, rx0, len, SYS_FOREVER_US);
		zassert_equal(err, -ENOTSUP,
			      "Unaligned uart_rx_enable() returned %d", err);
		return;
	}

	cdc_acm_loopback_buf(tx, rx0, rx1, len);
}

/*
 * Measure the loopback throughput. With RX enabled once, send a buffer of len
 * bytes and wait for it to be echoed back, iterations times, and report the
 * achieved data rate.
 */
static void cdc_acm_throughput(const size_t len, const unsigned int iterations)
{
	const uint32_t total = iterations * (uint32_t)len;
	int64_t start, elapsed;
	int err;

	test_reset();

	for (size_t i = 0; i < len; i++) {
		tx_buf[i] = (uint8_t)i;
	}

	rx_buf_len = len;
	rx_free_buf = rx_buf1;

	err = uart_rx_enable(uart_host, rx_buf0, len, SYS_FOREVER_US);
	zassert_ok(err, "uart_rx_enable() failed with %d", err);

	start = k_uptime_get();

	/*
	 * Keep a single TX transfer in flight (ASYNC UART API allows only one)
	 * and let the echo be received in parallel, so the OUT and IN endpoints
	 * are both busy each frame instead of alternating.
	 */
	for (unsigned int n = 0; n < iterations; n++) {
		k_sem_reset(&tx_done);

		err = uart_tx(uart_host, tx_buf, len, SYS_FOREVER_US);
		zassert_ok(err, "uart_tx() failed with %d", err);
		zassert_ok(k_sem_take(&tx_done, K_MSEC(1000)), "TX_DONE timeout");
	}

	/* Wait for all the echoed data to be received back. */
	while (rx_total < total) {
		zassert_ok(k_sem_take(&rx_rdy, K_MSEC(1000)), "RX_RDY timeout");
	}

	elapsed = k_uptime_delta(&start);

	err = uart_rx_disable(uart_host);
	zassert_ok(err, "uart_rx_disable() failed with %d", err);
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(1000)), "RX_DISABLED timeout");

	zassert_equal(rx_total, total, "Received %u of %u bytes", rx_total, total);

	TC_PRINT("Throughput (len %zu, %u iterations): %u bytes in %lld ms",
		 len, iterations, total, elapsed);
	if (elapsed > 0) {
		TC_PRINT(", %u B/s\n",
			 (uint32_t)(((uint64_t)total * 1000U) / (uint64_t)elapsed));
	} else {
		TC_PRINT("\n");
	}
}

ZTEST(cdc_acm_test, test_throughput)
{
	cdc_acm_throughput(TEST_BUF_SIZE, 1024);
}

/*
 * Test UART poll_in/poll_out implementation. poll_out a byte, let the device
 * echo it back, and poll in the echo. The poll API is only usable without an
 * ASYNC API callback, so it is removed for the duration of the test and
 * restored afterwards.
 */
ZTEST(cdc_acm_test, test_poll)
{
	unsigned char rc;
	int err;

	/* poll_in/poll_out are rejected while an ASYNC API callback is set. */
	rc = '!';
	zassert_equal(uart_poll_in(uart_host, &rc), -EBUSY,
		      "poll_in() not rejected while async callback is set");

	err = uart_callback_set(uart_host, NULL, NULL);
	zassert_ok(err, "uart_callback_set(NULL) failed with %d", err);

	for (unsigned char c = '0'; c <= 'Z'; c++) {
		uart_poll_out(uart_host, c);

		err = uart_poll_in(uart_host, &rc);
		zassert_ok(err, "uart_poll_in() failed with %d", err);
		zassert_equal(rc, c, "Polled 0x%02x, expected 0x%02x", rc, c);
	}

	/* Restore the ASYNC API callback for the remaining tests. */
	err = uart_callback_set(uart_host, uart_cb_async_handler, NULL);
	zassert_ok(err, "uart_callback_set() failed with %d", err);
}

/*
 * Test the device side poll_in/poll_out. Disable the interrupt-driven loopback
 * so the device poll API drains the RX FIFO, and echo each byte back to the
 * host with uart_poll_out(). Both ends use the poll API.
 */
ZTEST(cdc_acm_test, test_poll_device)
{
	unsigned char rc;
	int err;

	err = uart_callback_set(uart_host, NULL, NULL);
	zassert_ok(err, "uart_callback_set(NULL) failed with %d", err);

	/* Stop the interrupt-driven echo so poll_in drains the device RX FIFO. */
	uart_irq_rx_disable(uart_dev);
	uart_irq_callback_set(uart_dev, NULL);

	for (unsigned char c = '0'; c <= 'Z'; c++) {
		unsigned char dc = 0;

		uart_poll_out(uart_host, c);

		/* Device poll_in is non-blocking; wait for the byte to arrive. */
		for (int i = 0; i < 1000; i++) {
			if (uart_poll_in(uart_dev, &dc) == 0) {
				break;
			}

			k_msleep(1);
		}

		zassert_equal(dc, c, "Device polled 0x%02x, expected 0x%02x", dc, c);

		uart_poll_out(uart_dev, dc);

		err = uart_poll_in(uart_host, &rc);
		zassert_ok(err, "Host uart_poll_in() failed with %d", err);
		zassert_equal(rc, c, "Host polled 0x%02x, expected 0x%02x", rc, c);
	}

	/* Restore the interrupt-driven echo and the host ASYNC API callback. */
	uart_irq_callback_set(uart_dev, dev_interrupt_handler);
	uart_irq_rx_enable(uart_dev);

	err = uart_callback_set(uart_host, uart_cb_async_handler, NULL);
	zassert_ok(err, "uart_callback_set() failed with %d", err);
}

/*
 * Drain and discard any data the device still echoes back, leaving the RX pipe
 * empty so that the tests do not depend on the execution order.
 */
static void cdc_acm_drain_rx(void)
{
	int err;

	test_reset();
	rx_buf_len = TEST_BUF_SIZE;
	rx_free_buf = rx_buf1;

	err = uart_rx_enable(uart_host, rx_buf0, TEST_BUF_SIZE, SYS_FOREVER_US);
	zassert_ok(err, "uart_rx_enable() failed with %d", err);

	/* Consume buffers until RX pipe stays quiet. */
	while (k_sem_take(&rx_rdy, K_MSEC(100)) == 0) {
	}

	err = uart_rx_disable(uart_host);
	zassert_ok(err, "uart_rx_disable() failed with %d", err);
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(1000)), "RX_DISABLED timeout");
}

/* Larger than the device RX FIFO so the transfer stalls and can be aborted. */
UDC_STATIC_BUF_DEFINE(tx_abort_buf, 2 * 1024);

ZTEST(cdc_acm_test, test_tx_abort)
{
	int err;

	test_reset();

	/* Aborting with no transmission in progress reports -EFAULT. */
	err = uart_tx_abort(uart_host);
	zassert_equal(err, -EFAULT, "uart_tx_abort() returned %d", err);

	/*
	 * Stop the device UART from draining its RX FIFO so that a transfer
	 * larger than the TX FIFO stays in progress and can actually be aborted.
	 */
	uart_irq_rx_disable(uart_dev);

	err = uart_tx(uart_host, tx_abort_buf, sizeof(tx_abort_buf), SYS_FOREVER_US);
	zassert_ok(err, "uart_tx() failed with %d", err);

	/* Let the transfer start and stall on the device side. */
	k_msleep(100);

	err = uart_tx_abort(uart_host);
	zassert_ok(err, "uart_tx_abort() failed with %d", err);

	zassert_ok(k_sem_take(&tx_aborted, K_MSEC(1000)), "TX_ABORTED timeout");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), -EAGAIN,
		      "Unexpected TX_DONE after abort");

	/* Re-enable the device echo and drain the data the device absorbed */
	uart_irq_rx_enable(uart_dev);
	cdc_acm_drain_rx();
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/*
 * Apply a line coding on the host, verify it is reported back by the host, and
 * verify that the device side reflects the same settings (the host issues a
 * SET_LINE_CODING request, which the device applies to its UART config).
 */
static void host_configure(const uint8_t parity, const uint8_t stop_bits,
			   const uint8_t data_bits, const uint32_t baudrate)
{
	const struct uart_config cfg = {
		.baudrate = baudrate,
		.parity = parity,
		.stop_bits = stop_bits,
		.data_bits = data_bits,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	struct uart_config rb_cfg;
	int err;

	err = uart_configure(uart_host, &cfg);
	zassert_ok(err, "uart_configure() failed with %d (baud %u parity %u stop %u data %u)",
		   err, baudrate, parity, stop_bits, data_bits);

	err = uart_config_get(uart_host, &rb_cfg);
	zassert_ok(err, "uart_config_get() failed with %d", err);
	zassert_equal(rb_cfg.baudrate, baudrate, "Host baudrate %u != %u",
		      rb_cfg.baudrate, baudrate);
	zassert_equal(rb_cfg.parity, parity, "Host parity %u != %u",
		      rb_cfg.parity, parity);
	zassert_equal(rb_cfg.stop_bits, stop_bits, "Host stop bits %u != %u",
		      rb_cfg.stop_bits, stop_bits);
	zassert_equal(rb_cfg.data_bits, data_bits, "Host data bits %u != %u",
		      rb_cfg.data_bits, data_bits);

	memset(&rb_cfg, 0, sizeof(rb_cfg));
	/* The device applies the line coding slightly after the request completes. */
	for (int i = 0; i < 100; i++) {
		err = uart_config_get(uart_dev, &rb_cfg);
		zassert_ok(err, "device uart_config_get() failed with %d", err);
		if (rb_cfg.baudrate == baudrate && rb_cfg.parity == parity &&
		    rb_cfg.stop_bits == stop_bits && rb_cfg.data_bits == data_bits) {
			break;
		}

		k_msleep(1);
	}

	zassert_equal(rb_cfg.baudrate, baudrate, "Device baudrate %u != %u",
		      rb_cfg.baudrate, baudrate);
	zassert_equal(rb_cfg.parity, parity, "Device parity %u != %u",
		      rb_cfg.parity, parity);
	zassert_equal(rb_cfg.stop_bits, stop_bits, "Device stop bits %u != %u",
		      rb_cfg.stop_bits, stop_bits);
	zassert_equal(rb_cfg.data_bits, data_bits, "Device data bits %u != %u",
		      rb_cfg.data_bits, data_bits);

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		uint32_t tmp = 0;

		err = uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_BAUD_RATE, &tmp);
		zassert_ok(err, "Device uart_line_ctrl_get() failed with %d", err);
		zassert_equal(tmp, baudrate, "Device line control baudrate %u != %u",
			      tmp, baudrate);
	}
}

ZTEST(cdc_acm_test, test_configure)
{
	static const uint8_t parities[] = {
		UART_CFG_PARITY_NONE, UART_CFG_PARITY_ODD, UART_CFG_PARITY_EVEN,
		UART_CFG_PARITY_MARK, UART_CFG_PARITY_SPACE,
	};
	static const uint8_t stop_bits[] = {
		UART_CFG_STOP_BITS_1, UART_CFG_STOP_BITS_1_5, UART_CFG_STOP_BITS_2,
	};
	static const uint8_t data_bits[] = {
		UART_CFG_DATA_BITS_5, UART_CFG_DATA_BITS_6,
		UART_CFG_DATA_BITS_7, UART_CFG_DATA_BITS_8,
	};
	static const uint32_t baudrates[] = {9600, 115200};

	ARRAY_FOR_EACH(parities, i) {
		host_configure(parities[i], UART_CFG_STOP_BITS_1,
			       UART_CFG_DATA_BITS_8, 115200);
	}

	ARRAY_FOR_EACH(stop_bits, i) {
		host_configure(UART_CFG_PARITY_NONE, stop_bits[i],
			       UART_CFG_DATA_BITS_8, 115200);
	}

	ARRAY_FOR_EACH(data_bits, i) {
		host_configure(UART_CFG_PARITY_NONE, UART_CFG_STOP_BITS_1,
			       data_bits[i], 115200);
	}

	ARRAY_FOR_EACH(baudrates, i) {
		host_configure(UART_CFG_PARITY_NONE, UART_CFG_STOP_BITS_1,
			       UART_CFG_DATA_BITS_8, baudrates[i]);
	}
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_LINE_CTRL
/*
 * Set a line control signal on the device UART and verify that it is reported
 * to the host UART through a CDC SerialState notification.
 */
static void verify_serial_state(const uint32_t ctrl, const uint32_t val)
{
	uint32_t tmp = !val;
	int err;

	err = uart_line_ctrl_set(uart_dev, ctrl, val);
	zassert_ok(err, "Device uart_line_ctrl_set() failed with %d", err);

	/* The notification is delivered to the host asynchronously. */
	for (int i = 0; i < 100; i++) {
		err = uart_line_ctrl_get(uart_host, ctrl, &tmp);
		zassert_ok(err, "Host uart_line_ctrl_get() failed with %d", err);
		if (tmp == val) {
			break;
		}

		k_msleep(1);
	}

	zassert_equal(tmp, val, "Host line ctrl 0x%x is %u, expected %u", ctrl, tmp, val);
}

ZTEST(cdc_acm_test, test_serial_state)
{
	/* DCD (RxCarrier) and DSR (TxCarrier) are carried by the SerialState. */
	verify_serial_state(UART_LINE_CTRL_DCD, 1);
	verify_serial_state(UART_LINE_CTRL_DSR, 1);
	verify_serial_state(UART_LINE_CTRL_DCD, 0);
	verify_serial_state(UART_LINE_CTRL_DSR, 0);
}

/*
 * Set a control line signal on the host UART and verify that it is reported
 * back by the host and applied on the device UART, which the host conveys with
 * a SET_CONTROL_LINE_STATE request.
 */
static void verify_control_line_state(const uint32_t ctrl, const uint32_t val)
{
	uint32_t tmp;
	int err;

	err = uart_line_ctrl_set(uart_host, ctrl, val);
	zassert_ok(err, "Host uart_line_ctrl_set() failed with %d", err);

	err = uart_line_ctrl_get(uart_host, ctrl, &tmp);
	zassert_ok(err, "Host uart_line_ctrl_get() failed with %d", err);
	zassert_equal(tmp, val, "Host line ctrl 0x%x is %u, expected %u", ctrl, tmp, val);

	/* The device applies the control line state slightly after the request. */
	tmp = !val;
	for (int i = 0; i < 100; i++) {
		err = uart_line_ctrl_get(uart_dev, ctrl, &tmp);
		zassert_ok(err, "Device uart_line_ctrl_get() failed with %d", err);
		if (tmp == val) {
			break;
		}

		k_msleep(1);
	}

	zassert_equal(tmp, val, "Device line ctrl 0x%x is %u, expected %u", ctrl, tmp, val);
}

ZTEST(cdc_acm_test, test_control_line_state)
{
	/* DTR and RTS are carried by the SET_CONTROL_LINE_STATE request. */
	verify_control_line_state(UART_LINE_CTRL_DTR, 1);
	verify_control_line_state(UART_LINE_CTRL_RTS, 1);
	verify_control_line_state(UART_LINE_CTRL_DTR, 0);
	verify_control_line_state(UART_LINE_CTRL_RTS, 0);
}
#endif /* CONFIG_UART_LINE_CTRL */

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

struct usbh_context *const uhs_ctx = &test_uhs_ctx;

void *cdc_acm_test_enable(void)
{
	int ret;

	/* Setup device UART */
	ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);
	uart_irq_callback_set(uart_dev, dev_interrupt_handler);
	uart_irq_rx_enable(uart_dev);

	/* Setup host UART */
	uart_host = device_get_binding("usbh_cdc_acm_0");
	zassert_not_null(uart_host, "No USB host CDC ACM instance available");
	uart_callback_set(uart_host, uart_cb_async_handler, NULL);

	ret = usbh_init(uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus reset");

	ret = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus resume");

	ret = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(ret, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	ret = usbd_init(test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Allow the host time to enumerate and configure the device. */
	k_msleep(500);

	return NULL;
}

void cdc_acm_test_shutdown(void *f)
{
	int ret;

	ret = usbd_disable(test_usbd);
	zassert_ok(ret, "Failed to disable device support");

	ret = usbd_shutdown(test_usbd);
	zassert_ok(ret, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	ret = usbh_disable(uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(cdc_acm_test, NULL, cdc_acm_test_enable, NULL, NULL, cdc_acm_test_shutdown);
