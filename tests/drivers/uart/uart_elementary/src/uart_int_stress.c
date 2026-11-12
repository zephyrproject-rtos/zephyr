/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Stress test for two UART instances connected with pins and used with the interrupt
 * driven API.
 *
 * Transmitter (dut_aux) mimics the way how the modem UART ISR backend
 * (subsys/modem/backends/modem_backend_uart_isr.c) is sending data: data is put by
 * a thread into a ring buffer (with TX interrupt disabled around the put) and the
 * TX interrupt handler feeds the UART FIFO from that ring buffer. When the ring buffer
 * becomes empty the TX interrupt handler disables the TX interrupt.
 * Data is put at random intervals, in random chunks.
 *
 * Receiver (dut) is reading data from the FIFO in its interrupt handler and verifies
 * that the incoming stream is exactly what has been sent (a continuous counter pattern).
 *
 * When device runtime PM is enabled, test verifies that:
 * - the transmitter is suspended when it has nothing to send (checked at random moments
 *   during the test, whenever the transmission is idle),
 * - the receiver is active as long as its RX interrupt is enabled,
 * - both devices are suspended once the test is finished.
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#if DT_NODE_EXISTS(DT_NODELABEL(dut)) && DT_NODE_EXISTS(DT_NODELABEL(dut_aux))
#define RX_NODE DT_NODELABEL(dut)
#define TX_NODE DT_NODELABEL(dut_aux)
#else
#error "dut and dut_aux must be defined"
#endif

/* Size of the transmit ring buffer. */
#define TX_RB_SIZE 1024
/* Same as in the modem backend, data is not accepted if buffer is filled above 75%. */
#define TX_PUT_LIMIT (TX_RB_SIZE - (TX_RB_SIZE / 4))
/* Maximum number of bytes put to the ring buffer at once. */
#define MAX_CHUNK 200
/* Probability (in percent) that after a put the test waits until the transmission is
 * finished and verifies PM state.
 */
#define IDLE_CHECK_PERCENT 4
/* Time in which the receiver must get everything that was sent. */
#define DRAIN_TIMEOUT_MS 2000
/* Time in which the device must be suspended after it became idle. */
#define PM_SUSPEND_TIMEOUT_MS 200
/* Minimum amount of data that must be transferred to consider test as valid. */
#define MIN_BYTES 2000

static const struct device *const rx_dev = DEVICE_DT_GET(RX_NODE);
static const struct device *const tx_dev = DEVICE_DT_GET(TX_NODE);

static uint8_t tx_rb_buf[TX_RB_SIZE];
static struct ring_buf tx_rb;

/* Total number of bytes accepted by the transmitter ring buffer. Modified only by the
 * thread. The transmitted byte value is a low byte of the byte index.
 */
static uint32_t tx_put_total;
/* Receiver state, modified only by the RX ISR. */
static volatile uint32_t rx_total;
static volatile uint32_t rx_errors;
static volatile uint32_t rx_first_err_idx;
static volatile uint8_t rx_first_err_exp;
static volatile uint8_t rx_first_err_got;
static uint8_t rx_next;
/* Statistics. */
static volatile uint32_t tx_isr_cnt;
static volatile uint32_t rx_isr_cnt;

static void tx_irq_handler(const struct device *dev, void *user_data)
{
	uint32_t size;
	uint8_t *buffer;
	int ret;

	ARG_UNUSED(user_data);

	tx_isr_cnt++;
	uart_irq_update(dev);

	if (!uart_irq_tx_ready(dev)) {
		return;
	}

	if (ring_buf_is_empty(&tx_rb)) {
		uart_irq_tx_disable(dev);
		return;
	}

	size = ring_buf_get_ptr(&tx_rb, &buffer, 0);
	ret = uart_fifo_fill(dev, buffer, size);
	if (ret > 0) {
		ring_buf_consume(&tx_rb, (uint32_t)ret);
	}
}

static void rx_irq_handler(const struct device *dev, void *user_data)
{
	uint8_t buf[16];
	int len;

	ARG_UNUSED(user_data);

	rx_isr_cnt++;
	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while ((len = uart_fifo_read(dev, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < len; i++) {
			if (buf[i] != rx_next) {
				if (rx_errors == 0) {
					rx_first_err_idx = rx_total + i;
					rx_first_err_exp = rx_next;
					rx_first_err_got = buf[i];
				}
				rx_errors++;
				/* Resynchronize to limit the number of reported errors. */
				rx_next = buf[i];
			}
			rx_next++;
		}
		rx_total += len;
	}
}

/* Verify the device PM state. Not applicable if runtime PM is disabled.
 * If device is expected to be off then give it some time to get there since
 * suspending is asynchronous.
 */
static void pm_check(const struct device *dev, bool exp_on, int line)
{
	enum pm_device_state state = PM_DEVICE_STATE_OFF;
	int cnt = -1;
	int64_t end = k_uptime_get() + PM_SUSPEND_TIMEOUT_MS;
	int err;

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		return;
	}

	do {
		cnt = pm_device_runtime_usage(dev);
		err = pm_device_state_get(dev, &state);
		zassert_equal(err, 0, "line:%d", line);

		if (exp_on || (state == PM_DEVICE_STATE_SUSPENDED)) {
			break;
		}
		k_msleep(1);
	} while (k_uptime_get() < end);

	if (exp_on) {
		zassert_not_equal(cnt, 0, "%s: wrong PM usage count:%d, line:%d", dev->name, cnt,
				  line);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "%s: wrong PM state %s, line:%d",
			      dev->name, pm_device_state_str(state), line);
		return;
	}

	zassert_equal(cnt, 0, "%s: wrong PM usage count:%d, line:%d", dev->name, cnt, line);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "%s: wrong PM state %s, line:%d",
		      dev->name, pm_device_state_str(state), line);
}

#define PM_CHECK(dev, exp_on) pm_check(dev, exp_on, __LINE__)

static uint32_t config_baudrate(uint32_t rate)
{
	struct uart_config config;
	uint32_t baudrate;
	int err;

	err = uart_config_get(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	config.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	baudrate = config.baudrate;
	config.baudrate = rate;

	err = uart_configure(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	err = uart_configure(tx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	return baudrate;
}

/* Put random amount of data to the transmit buffer and (re)start the transmission.
 * Same sequence as in modem_backend_uart_isr_transmit_chain().
 */
static void tx_put(void)
{
	uint8_t buf[MAX_CHUNK];
	uint32_t len = 1 + (sys_rand32_get() % MAX_CHUNK);
	uint32_t put;

	if (ring_buf_size_get(&tx_rb) > TX_PUT_LIMIT) {
		return;
	}

	for (uint32_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(tx_put_total + i);
	}

	uart_irq_tx_disable(tx_dev);
	put = ring_buf_put(&tx_rb, buf, len);
	uart_irq_tx_enable(tx_dev);

	tx_put_total += put;
}

static bool wait_for_rx_all(uint32_t timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;

	while ((rx_total != tx_put_total) && (rx_errors == 0)) {
		if (k_uptime_get() > end) {
			return false;
		}
		k_msleep(1);
	}

	return rx_errors == 0;
}

static void check_rx_errors(void)
{
	zassert_equal(rx_errors, 0,
		"RX data corrupted: %u errors, first at byte %u (expected %02x, got %02x), "
		"received %u of %u bytes",
		rx_errors, rx_first_err_idx, rx_first_err_exp, rx_first_err_got, rx_total,
		tx_put_total);
}

static uint32_t random_gap_us(uint32_t baudrate)
{
	/* Mean time needed to send an average chunk. Random gap is up to twice that so on
	 * average the transmitter is loaded at about the link capacity. Ring buffer gets
	 * filled up and emptied in turns.
	 */
	uint32_t chunk_us = ((MAX_CHUNK / 2) * 10 * 1000000ULL) / baudrate;

	/* One in five times there is no gap at all. */
	if ((sys_rand32_get() % 5) == 0) {
		return 0;
	}

	return sys_rand32_get() % (2 * chunk_us);
}

static void stress(uint32_t baudrate)
{
	int64_t end;
	uint32_t tx_puts_cnt = 0;
	uint32_t idle_checks = 0;
	uint32_t prev_baudrate;
	int err;

	prev_baudrate = config_baudrate(baudrate);

	/* Before the test both devices are idle. */
	PM_CHECK(rx_dev, false);
	PM_CHECK(tx_dev, false);

	ring_buf_init(&tx_rb, sizeof(tx_rb_buf), tx_rb_buf);
	tx_put_total = 0;
	rx_total = 0;
	rx_errors = 0;
	rx_next = 0;
	tx_isr_cnt = 0;
	rx_isr_cnt = 0;

	uart_irq_tx_disable(tx_dev);
	uart_irq_rx_disable(rx_dev);
	uart_irq_callback_user_data_set(tx_dev, tx_irq_handler, NULL);
	uart_irq_callback_user_data_set(rx_dev, rx_irq_handler, NULL);

	/* Receiver is enabled for the whole test. */
	uart_irq_rx_enable(rx_dev);
	PM_CHECK(rx_dev, true);
	/* Transmitter is active only when it has something to send. */
	PM_CHECK(tx_dev, false);

	end = k_uptime_get() + CONFIG_UART_INT_STRESS_TEST_TIMEOUT;
	while ((k_uptime_get() < end) && (rx_errors == 0)) {
		uint32_t gap = random_gap_us(baudrate);

		tx_put();
		tx_puts_cnt++;

		if ((sys_rand32_get() % 100) < IDLE_CHECK_PERCENT) {
			/* Wait until everything is sent and received and then expect that
			 * transmitter is powered down while receiver is still on.
			 */
			err = wait_for_rx_all(DRAIN_TIMEOUT_MS);
			check_rx_errors();
			zassert_true(err, "Not all data received, got %u of %u", rx_total,
				     tx_put_total);
			PM_CHECK(tx_dev, false);
			PM_CHECK(rx_dev, true);
			idle_checks++;
			/* Random idle period before new transfer. */
			k_msleep(sys_rand32_get() % 50);
		} else if (gap > 0) {
			k_usleep(gap);
		}
	}

	check_rx_errors();

	err = wait_for_rx_all(DRAIN_TIMEOUT_MS);
	check_rx_errors();
	zassert_true(err, "Not all data received, got %u of %u", rx_total, tx_put_total);

	/* Wait to confirm that nothing more is coming. */
	k_msleep(20);
	check_rx_errors();
	zassert_equal(rx_total, tx_put_total, "Received %u, sent %u", rx_total, tx_put_total);

	TC_PRINT("Sent/received %u bytes in %u tx puts, %u idle checks, tx isr:%u, rx isr:%u\n",
		 rx_total, tx_puts_cnt, idle_checks, tx_isr_cnt, rx_isr_cnt);
	zassert_true(rx_total > MIN_BYTES, "Too little data transferred: %u", rx_total);

	/* Transmission is done, transmitter must be off, receiver is still on. */
	PM_CHECK(tx_dev, false);
	PM_CHECK(rx_dev, true);

	/* Once the receiver is disabled the whole system must be powered down. */
	uart_irq_rx_disable(rx_dev);
	PM_CHECK(rx_dev, false);
	PM_CHECK(tx_dev, false);

	/* Restore the original baudrate. */
	config_baudrate(prev_baudrate);
}

ZTEST(uart_int_stress, test_stress_115200)
{
	stress(115200);
}

ZTEST(uart_int_stress, test_stress_1m)
{
	stress(1000000);
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(rx_dev), "RX device not ready");
	zassert_true(device_is_ready(tx_dev), "TX device not ready");

	return NULL;
}

ZTEST_SUITE(uart_int_stress, NULL, suite_setup, NULL, NULL, NULL);
