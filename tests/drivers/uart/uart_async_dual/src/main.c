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
#define RX_TIMEOUT_BYTES 50

#define MAX_PACKET_LEN 128
#define MIN_PACKET_LEN 10

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

/* Array that contains potential payload. It is used to memcmp against incoming packets. */
static const uint8_t test_buf[256] = {
	255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241,
	240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226,
	225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211,
	210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196,
	195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181,
	180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166,
	165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151,
	150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136,
	135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121,
	120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106,
	105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89,
	88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70,
	69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30,
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,
	9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

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
	TX_CHOPPED,
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
	uint32_t idx;
	uint32_t rx_timeout;
};

enum test_rx_state {
	RX_HDR,
	RX_PAYLOAD
};

enum test_rx_mode {
	RX_CONT,
	RX_DIS,
	RX_ALL,
};

typedef bool (*test_on_rx_rdy_t)(const struct device *dev, uint8_t *buf, size_t len);

struct test_rx_data {
	uint8_t hdr[1];
	uint8_t buf[256];
	uint32_t rx_cnt;
	uint32_t payload_idx;
	enum test_rx_state state;
	enum test_rx_mode mode;
	volatile bool cont;
	bool buf_req;
	struct k_sem sem;
	uint32_t timeout;
	uint32_t buf_idx;
	test_on_rx_rdy_t on_rx_rdy;
};

static struct test_tx_data tx_data;
static struct test_rx_data rx_data;

static void fill_tx(struct test_tx_data *data)
{
	uint8_t *buf;
	uint32_t len;
	int err;

	if (data->mode != TX_BULK) {
		err = k_sem_take(&data->sem, K_MSEC(200));
		if (err < 0 && !data->cont) {
			return;
		}
		zassert_equal(err, 0);

		uint8_t len = sys_rand8_get();

		len = len % MAX_PACKET_LEN;
		len = MAX(MIN_PACKET_LEN, len);

		data->packet_len = len;
		data->idx = 0;
		for (int i = 0; i < len; i++) {
			data->buf[i] = len - i;
		}

		return;
	}

	while ((len = ring_buf_put_claim(&data->rbuf, &buf, 255)) > 0) {
		uint8_t r = (sys_rand8_get() % MAX_PACKET_LEN) % len;
		uint8_t packet_len = MAX(r, MIN_PACKET_LEN);

		packet_len = (len <= MIN_PACKET_LEN) ? len : packet_len;
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

	if (tx_data.mode == TX_PACKETS) {
		uint8_t len = tx_data.packet_len;

		tx_data.packet_len = 0;
		err = uart_tx(dev, tx_data.buf, len, TX_TIMEOUT);
		zassert_equal(err, 0,
				"Unexpected err:%d irq:%d cont:%d\n",
				err, irq, tx_data.cont);
		return;
	}

	if (tx_data.mode == TX_BULK) {
		if (!atomic_cas(&tx_data.busy, 0, 1)) {
			return;
		}

		len = ring_buf_get_claim(&tx_data.rbuf, &buf, 255);
		if (len > 0) {
			err = uart_tx(dev, buf, len, TX_TIMEOUT);
			zassert_equal(err, 0,
					"Unexpected err:%d irq:%d cont:%d\n",
					err, irq, tx_data.cont);
		} else {
			tx_data.busy = 0;
		}
		return;
	}

	zassert_true(tx_data.mode == TX_CHOPPED);

	uint32_t rem = tx_data.packet_len - tx_data.idx;

	if (tx_data.packet_len > 12) {
		len = sys_rand8_get() % (tx_data.packet_len / 4);
	} else {
		len = 0;
	}
	len = MAX(3, len);
	len = MIN(rem, len);

	buf = &tx_data.buf[tx_data.idx];
	tx_data.idx += len;

	err = uart_tx(dev, buf, len, TX_TIMEOUT);
	zassert_equal(err, 0,
			"Unexpected err:%d irq:%d cont:%d\n",
			err, irq, tx_data.cont);
}

static void tx_backoff(uint32_t rx_timeout)
{
	uint32_t delay = (rx_timeout / 2) + (sys_rand32_get() % rx_timeout);

	k_busy_wait(delay);
}

static void on_tx_done(const struct device *dev, struct uart_event *evt)
{
	if (tx_data.mode == TX_PACKETS) {
		k_sem_give(&tx_data.sem);
		return;
	}

	if (tx_data.mode == TX_CHOPPED) {
		if (tx_data.idx == tx_data.packet_len) {
			k_sem_give(&tx_data.sem);
		} else {

			tx_backoff(tx_data.rx_timeout);
			try_tx(dev, true);
		}
		return;
	}

	/* Finish previous data chunk and start new if any pending. */
	ring_buf_get_finish(&tx_data.rbuf, evt->data.tx.len);
	atomic_set(&tx_data.busy, 0);
	try_tx(dev, true);
}

static bool on_rx_rdy_rx_all(const struct device *dev, uint8_t *buf, size_t len)
{
	bool ok;

	if (rx_data.payload_idx == 0) {
		rx_data.payload_idx = buf[0] - 1;
		buf++;
		len--;
	}

	ok = memcmp(buf, &test_buf[256 - rx_data.payload_idx], len) == 0;
	rx_data.payload_idx -= len;

	return ok;
}

static bool on_rx_rdy_hdr(const struct device *dev, uint8_t *buf, size_t len);

static bool on_rx_rdy_payload(const struct device *dev, uint8_t *buf, size_t len)
{
	bool ok;
	int err;

	ok = memcmp(buf, &test_buf[255 - rx_data.payload_idx], len) == 0;
	if (!ok) {
		for (int i = 0; i < len; i++) {
			if (buf[i] != test_buf[255 - rx_data.payload_idx + i]) {
				zassert_true(false, "Byte %d expected: %02x got: %02x",
					i, buf[i], test_buf[255 - rx_data.payload_idx + i]);
			}
		}
		rx_data.cont = false;
		tx_data.cont = false;
		zassert_true(ok);
		return false;
	}

	rx_data.payload_idx -= len;

	if (rx_data.payload_idx == 0) {
		rx_data.state = RX_HDR;
		rx_data.on_rx_rdy = on_rx_rdy_hdr;
		if ((rx_data.mode == RX_CONT) && rx_data.buf_req) {
			rx_data.buf_req = false;
			err = uart_rx_buf_rsp(dev, rx_data.hdr, 1);
			zassert_equal(err, 0);
		}
	}

	return true;
}

static bool on_rx_rdy_hdr(const struct device *dev, uint8_t *buf, size_t len)
{
	int err;

	zassert_equal(buf, rx_data.hdr);
	zassert_equal(len, 1);
	if (rx_data.hdr[0] == 1) {
		/* single byte packet. */
		if ((rx_data.mode == RX_CONT) && rx_data.buf_req) {
			err = uart_rx_buf_rsp(dev, rx_data.hdr, 1);
			zassert_equal(err, 0);
		}
		return true;
	}

	zassert_equal(rx_data.payload_idx, 0);
	rx_data.on_rx_rdy = on_rx_rdy_payload;
	rx_data.payload_idx = rx_data.hdr[0] - 1;
	rx_data.state = RX_PAYLOAD;
	if ((rx_data.mode == RX_CONT) && rx_data.buf_req) {
		size_t l = rx_data.hdr[0] - 1;

		zassert_true(l > 0);
		rx_data.buf_req = false;
		err = uart_rx_buf_rsp(dev, rx_data.buf, buf[0] - 1);
	}

	return true;
}

static void on_rx_buf_req(const struct device *dev)
{
	if (rx_data.mode != RX_ALL) {
		rx_data.buf_req = true;
		return;
	}

	size_t len = sizeof(rx_data.buf) / 2;
	uint8_t *buf = &rx_data.buf[len * rx_data.buf_idx];

	rx_data.buf_idx = (rx_data.buf_idx + 1) & 0x1;
	uart_rx_buf_rsp(dev, buf, len);
}

static void on_rx_dis(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(evt);
	struct test_rx_data *data = user_data;
	int err;
	uint8_t *buf;
	uint32_t len;

	if (data->mode == RX_ALL) {
		buf = data->buf;
		len = sizeof(data->buf) / 2;
	} else {
		buf = (data->state == RX_HDR) ? data->hdr : data->buf;
		len = (data->state == RX_HDR) ? 1 : (data->hdr[0] - 1);
		data->buf_idx = 1;
	}

	data->buf_req = false;

	if (!data->cont) {
		return;
	}

	zassert_true(len > 0);
	err = uart_rx_enable(dev, buf, len, data->timeout);
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
		if (rx_data.cont) {
			rx_data.on_rx_rdy(dev, &evt->data.rx.buf[evt->data.rx.offset],
					  evt->data.rx.len);
			rx_data.rx_cnt += evt->data.rx.len;
		}
		break;
	case UART_RX_BUF_RELEASED:
		zassert_true(dev == rx_dev);
		break;
	case UART_RX_BUF_REQUEST:
		zassert_true(dev == rx_dev);
		on_rx_buf_req(dev);
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

static void config_baudrate(uint32_t rate, bool hwfc)
{
	struct uart_config config;
	int err;

	err = uart_config_get(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	config.flow_ctrl = hwfc ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;
	config.baudrate = rate;

	err = uart_configure(rx_dev, &config);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	if (rx_dev != tx_dev) {
		err = uart_configure(tx_dev, &config);
		zassert_equal(err, 0, "Unexpected err:%d", err);
	}
}

static void report_progress(uint32_t start)
{
	static const uint32_t inc = CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT / 20;
	static uint32_t next;
	static uint32_t progress;

	if ((k_uptime_get_32() - start < inc) && progress) {
		/* Reset state. */
		next = inc;
		progress = 0;
	}

	if (k_uptime_get_32() > (start + next)) {
		progress += 5;
		TC_PRINT("\r%d%%", progress);
		next += inc;
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
static void var_packet(uint32_t baudrate, enum test_tx_mode tx_mode,
			    enum test_rx_mode rx_mode, bool hwfc)
{
	int err;
	uint32_t load = 0;
	uint32_t start = k_uptime_get_32();

	config_baudrate(baudrate, hwfc);

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		uint32_t active_avg = (baudrate == 1000000) ? 5 : 30;
		uint32_t active_delta = (baudrate == 1000000) ? 2 : 10;

		busy_sim_start(active_avg, active_delta, 100, 50, NULL);
	}

	memset(&tx_data, 0, sizeof(tx_data));
	memset(&rx_data, 0, sizeof(rx_data));
	tx_data.cont = true;
	tx_data.mode = tx_mode;
	k_sem_init(&tx_data.sem, (tx_mode != TX_BULK) ? 1 : 0, 1);

	rx_data.timeout = (RX_TIMEOUT_BYTES * 1000000 * 10) / baudrate;
	tx_data.rx_timeout = rx_data.timeout;
	rx_data.cont = true;
	rx_data.rx_cnt = 0;
	rx_data.on_rx_rdy = rx_mode == RX_ALL ? on_rx_rdy_rx_all : on_rx_rdy_hdr;
	rx_data.mode = rx_mode;

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
		report_progress(start);
		try_tx(tx_dev, false);
	}
	TC_PRINT("\n");

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
	(void)uart_rx_enable(rx_dev, rx_data.buf, sizeof(rx_data.buf), rx_data.timeout);
	k_msleep(10);
	(void)uart_rx_disable(rx_dev);
	k_msleep(10);

	TC_PRINT("Received %d bytes for %d ms, CPU load:%d.%d\n",
		 rx_data.rx_cnt, CONFIG_UART_ASYNC_DUAL_TEST_TIMEOUT, load / 10, load % 10);
	zassert_true(rx_data.rx_cnt > 1000, "Unexpected RX cnt: %d", rx_data.rx_cnt);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_dis_hwfc)
{
	/* TX in bulk mode, RX in DIS mode, 115k2 */
	var_packet(115200, TX_BULK, RX_DIS, true);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_cont_hwfc)
{
	/* TX in bulk mode, RX in CONT mode, 115k2  */
	var_packet(115200, TX_BULK, RX_CONT, true);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_dis_hwfc_1m)
{
	/* TX in bulk mode, RX in DIS mode, 1M  */
	var_packet(1000000, TX_BULK, RX_DIS, true);
}

ZTEST(uart_async_dual, test_var_packets_tx_bulk_cont_hwfc_1m)
{
	/* TX in bulk mode, RX in CONT mode, 1M  */
	var_packet(1000000, TX_BULK, RX_CONT, true);
}

ZTEST(uart_async_dual, test_var_packets_dis_hwfc)
{
	/* TX in packet mode, RX in DIS mode, 115k2  */
	var_packet(115200, TX_PACKETS, RX_DIS, true);
}

ZTEST(uart_async_dual, test_var_packets_cont_hwfc)
{
	/* TX in packet mode, RX in CONT mode, 115k2  */
	var_packet(115200, TX_PACKETS, RX_CONT, true);
}

ZTEST(uart_async_dual, test_var_packets_dis_hwfc_1m)
{
	/* TX in packet mode, RX in DIS mode, 1M  */
	var_packet(1000000, TX_PACKETS, RX_DIS, true);
}

ZTEST(uart_async_dual, test_var_packets_cont_hwfc_1m)
{
	/* TX in packet mode, RX in CONT mode, 1M  */
	var_packet(1000000, TX_PACKETS, RX_CONT, true);
}

ZTEST(uart_async_dual, test_var_packets_chopped_all)
{
	if (!IS_ENABLED(CONFIG_TEST_CHOPPED_TX)) {
		ztest_test_skip();
	}

	/* TX in chopped mode, RX in receive ALL mode, 115k2  */
	var_packet(115200, TX_CHOPPED, RX_ALL, false);
}

ZTEST(uart_async_dual, test_var_packets_chopped_all_1m)
{
	if (!IS_ENABLED(CONFIG_TEST_CHOPPED_TX)) {
		ztest_test_skip();
	}

	/* TX in chopped mode, RX in receive ALL mode, 1M  */
	var_packet(1000000, TX_CHOPPED, RX_ALL, false);
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

	err = uart_rx_enable(rx_dev, buf, len, rx_data.timeout);
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
	uint32_t start = k_uptime_get_32();

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
		report_progress(start);
	}
	TC_PRINT("\n");
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

	config_baudrate(baudrate, true);

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
	rx_data.timeout = (RX_TIMEOUT_BYTES * 1000000 * 10) / baudrate;

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

	(void)uart_rx_enable(rx_dev, rx_data.buf, sizeof(rx_data.buf), rx_data.timeout);
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
