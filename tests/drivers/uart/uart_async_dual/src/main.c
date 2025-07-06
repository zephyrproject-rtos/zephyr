/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/ztest.h>
#include <zephyr/busy_sim.h>
#include <zephyr/logging/log.h>
#include <zephyr/debug/cpu_load.h>
LOG_MODULE_REGISTER(test);

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define DUT_NODE DT_NODELABEL(dut)
#else
#error "dut not defined"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dut_aux))
#define DUT_AUX_NODE DT_NODELABEL(dut_aux)
#else
#define DUT_AUX_NODE DT_NODELABEL(dut)
#endif

#define TX_TIMEOUT 100000
#define RX_TIMEOUT 2000

#define MAX_PACKET_LEN 128

struct dut_data {
	const struct device *dev;
	const struct device *dev_aux;
	const char *name;
	const char *name_aux;
};

ZTEST_DMEM struct dut_data duts[] = {
	{
		.dev = DEVICE_DT_GET(DUT_NODE),
		.dev_aux = DT_SAME_NODE(DUT_NODE, DUT_AUX_NODE) ?
			NULL : DEVICE_DT_GET(DUT_AUX_NODE),
		.name = DT_NODE_FULL_NAME(DUT_NODE),
		.name_aux = DT_SAME_NODE(DUT_NODE, DUT_AUX_NODE) ?
			NULL : DT_NODE_FULL_NAME(DUT_AUX_NODE),
	},
#if DT_NODE_EXISTS(DT_NODELABEL(dut2)) && DT_NODE_HAS_STATUS(DT_NODELABEL(dut2), okay)
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(dut2)),
		.name = DT_NODE_FULL_NAME(DT_NODELABEL(dut2)),
#if DT_NODE_EXISTS(DT_NODELABEL(dut_aux2)) && DT_NODE_HAS_STATUS(DT_NODELABEL(dut_aux2), okay)
		.dev_aux = DEVICE_DT_GET(DT_NODELABEL(dut_aux2)),
		.name_aux = DT_NODE_FULL_NAME(DT_NODELABEL(dut_aux2)),
#endif
	},
#endif
};

static void pm_check(const struct device *dev, const struct device *second_dev, bool exp_on,
		     int line)
{
	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		return;
	}

	if (dev == second_dev) {
		return;
	}

	enum pm_device_state state;
	int err;
	int cnt;

	cnt = pm_device_runtime_usage(dev);
	err = pm_device_state_get(dev, &state);
	zassert_equal(err, 0);

	if (exp_on) {
		zassert_not_equal(cnt, 0, "Wrong PM cnt:%d, line:%d", cnt, line);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE,
			"Wrong PM state %s, line:%d", pm_device_state_str(state), line);
		return;
	}

	/* Expect device to be off. */
	zassert_equal(cnt, 0, "Wrong PM count:%d, line:%d", cnt, line);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED,
			"Wrong PM state %s, line:%d", pm_device_state_str(state), line);
}

#define PM_CHECK(dev, second_dev, exp_on) pm_check(dev, second_dev, exp_on, __LINE__)

static const struct device *rx_dev;
static const struct device *tx_dev;

enum test_tx_mode {
	TX_BULK,
	TX_PACKETS,
};

struct test_tx_data {
	uint8_t buf[512];
	struct ring_buf rbuf;
	atomic_t busy;
	uint8_t packet_len;
	uint8_t cnt;
	volatile bool cont;
	volatile enum test_tx_mode mode;
	struct k_sem sem;
};

enum test_rx_state {
	RX_HDR,
	RX_PAYLOAD
};

enum test_rx_mode {
	RX_CONT,
	RX_DIS,
};

struct test_rx_data {
	uint8_t hdr[1];
	uint8_t buf[256];
	uint32_t rx_cnt;
	enum test_rx_state state;
	enum test_rx_mode mode;
	volatile bool cont;
	bool buf_req;
	struct k_sem sem;
};

static struct test_tx_data tx_data;
static struct test_rx_data rx_data;

static void fill_tx(struct test_tx_data *data)
{
	uint8_t *buf;
	uint32_t len;
	int err;

	if (data->mode == TX_PACKETS) {
		err = k_sem_take(&data->sem, K_MSEC(100));
		if (err < 0 && !data->cont) {
			return;
		}
		zassert_equal(err, 0);

		uint8_t len = sys_rand8_get();

		len = len % MAX_PACKET_LEN;
		len = MAX(2, len);

		data->packet_len = len;
		for (int i = 0; i < len; i++) {
			data->buf[i] = len - i;
		}

		return;
	}

	while ((len = ring_buf_put_claim(&data->rbuf, &buf, 255)) > 1) {
		uint8_t r = (sys_rand8_get() % MAX_PACKET_LEN) % len;
		uint8_t packet_len = MAX(r, 2);
		uint8_t rem = len - packet_len;

		packet_len = (rem < 3) ? len : packet_len;
		buf[0] = packet_len;
		for (int i = 1; i < packet_len; i++) {
			buf[i] = packet_len - i;
		}

		ring_buf_put_finish(&data->rbuf, packet_len);
	}
}

static void try_tx(const struct device *dev, bool irq)
{
	uint8_t *buf;
	uint32_t len;
	int err;

	if (!tx_data.cont) {
		rx_data.cont = false;
		return;
	}

	if ((tx_data.mode == TX_PACKETS) && (tx_data.packet_len > 0)) {
		uint8_t len = tx_data.packet_len;

		tx_data.packet_len = 0;
		err = uart_tx(dev, tx_data.buf, len, TX_TIMEOUT);
		zassert_equal(err, 0,
				"Unexpected err:%d irq:%d cont:%d\n",
				err, irq, tx_data.cont);
		return;
	}
	zassert_true(tx_data.mode == TX_BULK);

	if (!atomic_cas(&tx_data.busy, 0, 1)) {
		return;
	}

	len = ring_buf_get_claim(&tx_data.rbuf, &buf, 255);
	if (len > 0) {
		err = uart_tx(dev, buf, len, TX_TIMEOUT);
		zassert_equal(err, 0,
				"Unexpected err:%d irq:%d cont:%d\n",
				err, irq, tx_data.cont);
	}
}

static void on_tx_done(const struct device *dev, struct uart_event *evt)
{
	if (tx_data.mode == TX_PACKETS) {
		k_sem_give(&tx_data.sem);
		return;
	}

	/* Finish previous data chunk and start new if any pending. */
	ring_buf_get_finish(&tx_data.rbuf, evt->data.tx.len);
	atomic_set(&tx_data.busy, 0);
	try_tx(dev, true);
}

static void on_rx_rdy(const struct device *dev, struct uart_event *evt)
{
	uint32_t len = evt->data.rx.len;
	uint32_t off = evt->data.rx.offset;
	int err;

	if (!rx_data.cont) {
		return;
	}

	rx_data.rx_cnt += evt->data.rx.len;
	if (evt->data.rx.buf == rx_data.hdr) {
		rx_data.state = RX_PAYLOAD;
		if ((rx_data.mode == RX_CONT) && rx_data.buf_req) {
			size_t l = rx_data.hdr[0] - 1;

			zassert_true(l > 0);
			rx_data.buf_req = false;
			err = uart_rx_buf_rsp(dev, rx_data.buf, rx_data.hdr[0] - 1);
		}
	} else {
		/* Payload received */
		rx_data.state = RX_HDR;
		zassert_equal(len, rx_data.hdr[0] - 1);

		for (int i = 0; i < len; i++) {
			bool ok = evt->data.rx.buf[off + i] == (uint8_t)(len - i);

			if (!ok) {
				LOG_ERR("Unexpected data at %d, exp:%02x got:%02x",
					i, len - i, evt->data.rx.buf[off + i]);
			}

			zassert_true(ok, "Unexpected data at %d, exp:%02x got:%02x",
					i, len - i, evt->data.rx.buf[off + i]);
			if (!ok) {
				rx_data.cont = false;
				tx_data.cont = false;
				/* Avoid flood of errors as we are in the interrupt and ztest
				 * cannot abort from here.
				 */
				return;
			}
		}
		if ((rx_data.mode == RX_CONT) && rx_data.buf_req) {
			rx_data.buf_req = false;
			err = uart_rx_buf_rsp(dev, rx_data.hdr, 1);
		}
	}
}

static void on_rx_dis(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(evt);
	struct test_rx_data *data = user_data;
	int err;
	uint8_t *buf = (data->state == RX_HDR) ? data->hdr : data->buf;
	uint32_t len = (data->state == RX_HDR) ? 1 : (data->hdr[0] - 1);

	data->buf_req = false;

	if (!data->cont) {
		return;
	}


	zassert_true(len > 0);
	err = uart_rx_enable(dev, buf, len, RX_TIMEOUT);
	zassert_equal(err, 0, "Unexpected err:%d", err);
}

static void test_end(void)
{
	tx_data.cont = false;
}

static void test_timeout(struct k_timer *timer)
{
	test_end();
}

static K_TIMER_DEFINE(test_timer, test_timeout, NULL);

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		zassert_true(dev == tx_dev);
		on_tx_done(dev, evt);
		break;
	case UART_TX_ABORTED:
		zassert_true(dev == tx_dev);
		zassert_false(tx_data.cont,
			      "Unexpected TX abort, receiver not reading data on time");
		break;
	case UART_RX_RDY:
		zassert_true(dev == rx_dev);
		on_rx_rdy(dev, evt);
		break;
	case UART_RX_BUF_RELEASED:
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_BUF_REQUEST:
		rx_data.buf_req = true;
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_DISABLED:
		zassert_true(dev == rx_dev);
		on_rx_dis(dev, evt, &rx_data);
		break;
	case UART_RX_STOPPED:
		zassert_true(false);
		break;
	default:
		zassert_true(false);
		break;
	}
}

static void config_baudrate(uint32_t rate)
{
	struct uart_config config;
	int err;

	err = uart_config_get(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	config.baudrate = rate;

	err = uart_configure(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	if (rx_dev != tx_dev) {
		err = uart_configure(tx_dev, &config);
		zassert_equal(err, 0, "Unexpected err:%d", err);
	}
}

/* Test is running following scenario. Transmitter is sending packets which
 * has 1 byte header with length followed by the payload. Transmitter can send
 * packets in two modes: bulk where data is send in chunks without gaps between
 * packets or in packet mode where transmitter sends packet by packet.
 * Receiver receives the header and sets reception of the payload based on the
 * length in the header. It can also work in two modes: continuous (CONT) where new
 * transfer is started from UART_RX_RDY event or slower mode (DIS) where reception is
 * started from UART_RX_DISABLED event.
 *
 * Test has busy simulator running if it is enabled in the configuration.
 */
static void var_packet_hwfc(uint32_t baudrate, bool tx_packets, bool cont)
{
	int err;
	uint32_t load = 0;

	config_baudrate(baudrate);

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		uint32_t active_avg = (baudrate == 1000000) ? 5 : 30;
		uint32_t active_delta = (baudrate == 1000000) ? 2 : 10;

		busy_sim_start(active_avg, active_delta, 100, 50, NULL);
	}

	memset(&tx_data, 0, sizeof(tx_data));
	memset(&rx_data, 0, sizeof(rx_data));
	tx_data.cont = true;
	tx_data.mode = tx_packets ? TX_PACKETS : TX_BULK;
	k_sem_init(&tx_data.sem, tx_packets ? 1 : 0, 1);

	rx_data.cont = true;
	rx_data.rx_cnt = 0;
	rx_data.state = RX_HDR;
	rx_data.mode = cont ? RX_CONT : RX_DIS;

	ring_buf_init(&tx_data.rbuf, sizeof(tx_data.buf), tx_data.buf);

	k_timer_start(&test_timer, K_MSEC(CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT), K_NO_WAIT);

	err = uart_callback_set(rx_dev, uart_callback, &rx_data);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	err = uart_callback_set(tx_dev, uart_callback, &tx_data);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	on_rx_dis(rx_dev, NULL, &rx_data);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}

	while (tx_data.cont || rx_data.cont) {
		fill_tx(&tx_data);
		k_msleep(1);
		try_tx(tx_dev, false);
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		load = cpu_load_get(true);
	}

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_stop();
	}

	(void)uart_tx_abort(tx_dev);
	(void)uart_rx_disable(rx_dev);

	/* Flush all TX data that may be already started. */
	k_msleep(10);
	(void)uart_rx_enable(rx_dev, rx_data.buf, sizeof(rx_data.buf), RX_TIMEOUT);
	k_msleep(10);
	(void)uart_rx_disable(rx_dev);
	k_msleep(10);

	TC_PRINT("Received %d bytes for %d ms, CPU load:%d.%d\n",
		 rx_data.rx_cnt, CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT, load / 10, load % 10);
	zassert_true(rx_data.rx_cnt > 1000, "Unexected RX cnt: %d", rx_data.rx_cnt);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_dis_hwfc)
{
	/* TX in bulk mode, RX in DIS mode, 115k2 */
	var_packet_hwfc(115200, false, false);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_cont_hwfc)
{
	/* TX in bulk mode, RX in CONT mode, 115k2  */
	var_packet_hwfc(115200, false, true);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_dis_hwfc_1m)
{
	/* TX in bulk mode, RX in DIS mode, 1M  */
	var_packet_hwfc(1000000, false, false);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_cont_hwfc_1m)
{
	/* TX in bulk mode, RX in CONT mode, 1M  */
	var_packet_hwfc(1000000, false, true);
}

ZTEST(uart_async_dual, test_var_packets_dis_hwfc)
{
	/* TX in packet mode, RX in DIS mode, 115k2  */
	var_packet_hwfc(115200, true, false);
}

ZTEST(uart_async_dual, test_var_packets_cont_hwfc)
{
	/* TX in packet mode, RX in CONT mode, 115k2  */
	var_packet_hwfc(115200, true, true);
}

ZTEST(uart_async_dual, test_var_packets_dis_hwfc_1m)
{
	/* TX in packet mode, RX in DIS mode, 1M  */
	var_packet_hwfc(1000000, true, false);
}

ZTEST(uart_async_dual, test_var_packets_cont_hwfc_1m)
{
	/* TX in packet mode, RX in CONT mode, 1M  */
	var_packet_hwfc(1000000, true, true);
}

static void hci_like_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:

		zassert_true(dev == tx_dev);
		if (IS_ENABLED(CONFIG_PM_RUNTIME_IN_TEST)) {
			pm_device_runtime_put_async(tx_dev, K_NO_WAIT);
		}
		k_sem_give(&tx_data.sem);
		break;
	case UART_TX_ABORTED:
		zassert_true(dev == tx_dev);
		if (IS_ENABLED(CONFIG_PM_RUNTIME_IN_TEST)) {
			pm_device_runtime_put_async(tx_dev, K_NO_WAIT);
		}
		zassert_false(tx_data.cont,
				"Unexpected TX abort, receiver not reading data on time");
		break;
	case UART_RX_RDY:
		rx_data.rx_cnt += evt->data.rx.len;
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_BUF_RELEASED:
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_BUF_REQUEST:
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_DISABLED:
		zassert_true(dev == rx_dev);
		k_sem_give(&rx_data.sem);
		break;
	case UART_RX_STOPPED:
		zassert_true(false);
		break;
	default:
		zassert_true(false);
		break;
	}
}

static bool rx(uint8_t *buf, size_t len)
{
	int err;

	err = uart_rx_enable(rx_dev, buf, len, RX_TIMEOUT);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	err = k_sem_take(&rx_data.sem, K_MSEC(100));
	if ((err < 0) || !tx_data.cont) {
		zassert_false(tx_data.cont);
		err = uart_rx_disable(rx_dev);
		if (err == 0) {
			err = k_sem_take(&rx_data.sem, K_MSEC(100));
			zassert_equal(err, 0, "Unexpected err:%d", err);
		}

		return false;
	}

	return true;
}

static void check_pre_hdr(uint8_t *buf, uint8_t last_hdr)
{
	uint8_t exp_idx = (last_hdr + 1) & 0x7F;

	zassert_true(buf[0] & 0x80);
	zassert_equal(exp_idx, buf[0] & 0x7F);
}

static uint8_t get_len(uint8_t *buf)
{
	static const uint8_t exp_buf[] = {'a', 'b', 'c'};
	int rv;

	rv = memcmp(buf, exp_buf, sizeof(exp_buf));

	zassert_equal(rv, 0, "exp: %02x %02x %02x, got: %02x %02x %02x",
			exp_buf[0], exp_buf[1], exp_buf[2], buf[0], buf[1], buf[2]);

	return buf[sizeof(exp_buf)];
}

static void check_payload(uint8_t *buf, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		uint8_t exp_val = len - i;
		bool ok = buf[i] == exp_val;

		zassert_true(ok, "Unexpected byte at %d, got:%02x exp:%02x", i, buf[i], exp_val);
		if (!ok) {
			test_end();
			return;
		}
	}
}

static void hci_like_tx_prepare(struct test_tx_data *tx_data)
{
	int idx = tx_data->cnt & 0x1 ? sizeof(tx_data->buf) / 2 : 0;
	uint8_t *buf = &tx_data->buf[idx];
	uint8_t len = sys_rand8_get() & 0x1F;

	len = MAX(1, len);

	*buf++ = 0x80 | (tx_data->cnt & 0x7F);
	*buf++ = 'a';
	*buf++ = 'b';
	*buf++ = 'c';
	*buf++ = len;

	for (int i = 0; i < len; i++) {
		*buf++ = len - i;
	}

	tx_data->cnt++;
}

static void hci_like_tx(struct test_tx_data *tx_data)
{
	int idx = tx_data->cnt & 0x1 ? 0 : sizeof(tx_data->buf) / 2;
	uint8_t *buf = &tx_data->buf[idx];
	uint8_t len = buf[4] + 5;
	int err;

	if (IS_ENABLED(CONFIG_PM_RUNTIME_IN_TEST)) {
		pm_device_runtime_get(tx_dev);
	}

	err = uart_tx(tx_dev, buf, len, TX_TIMEOUT);
	zassert_equal(err, 0, "Unexpected err:%d", err);
}

static void hci_like_tx_thread_entry(void *p1, void *p2, void *p3)
{
	int err;

	while (tx_data.cont) {
		hci_like_tx_prepare(&tx_data);

		err = k_sem_take(&tx_data.sem, K_MSEC(500));
		if (err < 0) {
			break;
		}
		zassert_equal(err, 0, "Unexpected err:%d", err);

		hci_like_tx(&tx_data);
	}
}


static void hci_like_rx(void)
{
	uint8_t last_hdr = 0xff;
	uint8_t len;
	bool cont;
	bool explicit_pm = IS_ENABLED(CONFIG_PM_RUNTIME_IN_TEST);

	while (1) {
		if (explicit_pm) {
			pm_device_runtime_get(rx_dev);
		}

		cont = rx(rx_data.buf, 1);
		if (!cont) {
			if (explicit_pm) {
				pm_device_runtime_put(rx_dev);
			}
			break;
		}
		check_pre_hdr(rx_data.buf, last_hdr);
		last_hdr = rx_data.buf[0];

		/* If explicitly requested device to stay on in should be on.
		 * If application did not touch PM, device should be off.
		 */
		PM_CHECK(rx_dev, tx_dev, explicit_pm);

		cont = rx(rx_data.buf, 4);
		if (!cont) {
			if (explicit_pm) {
				pm_device_runtime_put(rx_dev);
			}
			break;
		}
		len = get_len(rx_data.buf);

		/* If explicitly requested device to stay on in should be on.
		 * If application did not touch PM, device should be off.
		 */
		PM_CHECK(rx_dev, tx_dev, explicit_pm);

		cont = rx(rx_data.buf, len);
		if (!cont) {
			if (explicit_pm) {
				pm_device_runtime_put(rx_dev);
			}
			break;
		}

		if (explicit_pm) {
			pm_device_runtime_put(rx_dev);
		}

		/* Device shall be released and off. */
		PM_CHECK(rx_dev, tx_dev, false);

		check_payload(rx_data.buf, len);
	}
}

#define HCI_LIKE_TX_STACK_SIZE 2048
static K_THREAD_STACK_DEFINE(hci_like_tx_thread_stack, HCI_LIKE_TX_STACK_SIZE);
static struct k_thread hci_like_tx_thread;

/* Test is emulating behavior that is implemented in the bluetooth HCI controller sample
 * which is using UART asynchronous API. Each packet consists of 3 parts: 1 byte header
 * indicating a type, 4 byte second header with length of the variable part which is
 * a third part.
 *
 * Receiver sets reception of each part after receiver is disabled.
 *
 * If enabled, busy simulator is used in the test.
 */
static void hci_like_test(uint32_t baudrate)
{
	int err;
	uint32_t load = 0;

	config_baudrate(baudrate);

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		uint32_t active_avg = (baudrate == 1000000) ? 10 : 50;
		uint32_t active_delta = (baudrate == 1000000) ? 5 : 20;

		busy_sim_start(active_avg, active_delta, 100, 50, NULL);
	}

	memset(&tx_data, 0, sizeof(tx_data));
	memset(&rx_data, 0, sizeof(rx_data));
	tx_data.cnt = 0;
	tx_data.cont = true;
	rx_data.cont = true;

	k_sem_init(&tx_data.sem, 1, 1);
	k_sem_init(&rx_data.sem, 0, 1);

	k_timer_start(&test_timer, K_MSEC(CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT), K_NO_WAIT);

	err = uart_callback_set(rx_dev, hci_like_callback, NULL);
	zassert_equal(err, 0);

	err = uart_callback_set(tx_dev, hci_like_callback, NULL);
	zassert_equal(err, 0);

	k_thread_create(&hci_like_tx_thread, hci_like_tx_thread_stack,
			K_THREAD_STACK_SIZEOF(hci_like_tx_thread_stack),
			hci_like_tx_thread_entry, NULL, NULL, NULL,
			K_PRIO_COOP(7), 0, K_MSEC(10));

	k_msleep(1);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}
	hci_like_rx();

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		load = cpu_load_get(true);
	}

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_stop();
	}

	/* Flush data. */
	(void)uart_tx_abort(tx_dev);
	k_msleep(10);
	PM_CHECK(tx_dev, rx_dev, false);

	(void)uart_rx_enable(rx_dev, rx_data.buf, sizeof(rx_data.buf), RX_TIMEOUT);
	k_msleep(1);
	(void)uart_rx_disable(rx_dev);

	k_thread_abort(&hci_like_tx_thread);
	k_msleep(10);

	TC_PRINT("Received %d bytes for %d ms CPU load:%d.%d\n",
		 rx_data.rx_cnt, CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT, load / 10, load % 10);
}

ZTEST(uart_async_dual, test_hci_like_115k)
{
	/* HCI like test at 115k2 */
	hci_like_test(115200);
}

ZTEST(uart_async_dual, test_hci_like_1M)
{
	/* HCI like test at 1M */
	hci_like_test(1000000);
}

static void *setup(void)
{
	static int idx;

	rx_dev = duts[idx].dev;
	if (duts[idx].dev_aux == NULL) {
		TC_PRINT("Single UART test on instance:%s\n", duts[idx].name);
		tx_dev = rx_dev;
	} else {
		TC_PRINT("Dual UART test on instances:%s and %s\n",
			 duts[idx].name, duts[idx].name_aux);
		tx_dev = duts[idx].dev_aux;
	}
	idx++;

	zassert_true(device_is_ready(rx_dev));
	zassert_true(device_is_ready(tx_dev));

	return NULL;
}

ZTEST_SUITE(uart_async_dual, NULL, setup, NULL, NULL, NULL);

void test_main(void)
{
	ztest_run_all(NULL, false, ARRAY_SIZE(duts), 1);
	ztest_verify_all_test_suites_ran();
}
