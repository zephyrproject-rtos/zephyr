/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_uart
 * @{
 * @defgroup t_uart_mix_fifo_poll test_uart_mix_fifo_poll
 * @}
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/random/random.h>
/* RX and TX pins have to be connected together*/

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#elif defined(CONFIG_BOARD_SAMD21_XPRO)
#define UART_NODE DT_NODELABEL(sercom1)
#elif defined(CONFIG_BOARD_SAMR21_XPRO)
#define UART_NODE DT_NODELABEL(sercom3)
#elif defined(CONFIG_BOARD_SAME54_XPRO)
#define UART_NODE DT_NODELABEL(sercom1)
#else
#define UART_NODE DT_CHOSEN(zephyr_console)
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(counter_dev))
#define COUNTER_NODE DT_NODELABEL(counter_dev)
#else
#define COUNTER_NODE DT_NODELABEL(timer0)
#endif

struct rx_source {
	int cnt;
	uint8_t prev;
};

struct dut_data {
	const struct device *dev;
	const char *name;
};

static struct dut_data duts[] = {
	{
		.dev = DEVICE_DT_GET(UART_NODE),
		.name = DT_NODE_FULL_NAME(UART_NODE),
	},
#if DT_NODE_EXISTS(DT_NODELABEL(dut2))
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(dut2)),
		.name = DT_NODE_FULL_NAME(DT_NODELABEL(dut2)),
	},
#endif
};

#define BUF_SIZE 16

/* Buffer used for polling. */
static uint8_t txbuf[3][BUF_SIZE];

/* Buffer used for async or interrupt driven apis.
 * One of test configurations checks if RO buffer works with the driver.
 */
static IF_ENABLED(TEST_CONST_BUFFER, (const)) uint8_t txbuf3[16] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
};

struct test_data {
	const uint8_t *buf;
	volatile int cnt;
	int max;
	struct k_sem sem;
};

static struct rx_source source[4];
static struct test_data test_data[3];
static struct test_data int_async_data;

static const struct device *const counter_dev =
	DEVICE_DT_GET(COUNTER_NODE);
static const struct device *uart_dev;

static bool async;
static bool int_driven;
static volatile bool async_rx_enabled;
static struct k_sem async_tx_sem;

static void int_driven_callback(const struct device *dev, void *user_data);
static void async_callback(const struct device *dev,
			   struct uart_event *evt, void *user_data);

static void process_byte(uint8_t b)
{
	int base = b >> 4;
	struct rx_source *src = &source[base];
	bool ok;

	b &= 0x0F;
	src->cnt++;

	if (src->cnt == 1) {
		src->prev = b;
		return;
	}

	ok = ((b - src->prev) == 1) || (!b && (src->prev == 0x0F));

	zassert_true(ok, "Unexpected byte received:0x%02x, prev:0x%02x",
			(base << 4) | b, (base << 4) | src->prev);
	src->prev = b;
}

static void counter_top_handler(const struct device *dev, void *user_data)
{
	static bool enable = true;
	static uint8_t async_rx_buf[4];

	if (async && !async_rx_enabled) {
		int err;

		err = uart_rx_enable(uart_dev, async_rx_buf,
				     sizeof(async_rx_buf), 1 * USEC_PER_MSEC);
		zassert_true(err >= 0);
		async_rx_enabled = true;
	} else if (int_driven) {
		if (enable) {
			uart_irq_rx_enable(uart_dev);
		} else {
			uart_irq_rx_disable(uart_dev);
		}

		enable = !enable;
	} else if (!async && !int_driven) {
		uint8_t c;

		while (uart_poll_in(uart_dev, &c) >= 0) {
			process_byte(c);
		}
	}
}

static void init_test(int idx)
{
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = counter_top_handler,
		.user_data = NULL,
		.flags = 0
	};

	memset(source, 0, sizeof(source));
	async_rx_enabled = false;
	uart_dev = duts[idx].dev;
	TC_PRINT("UART instance:%s\n", duts[idx].name);

	zassert_true(device_is_ready(uart_dev), "uart device is not ready");

	if (uart_callback_set(uart_dev, async_callback, NULL) == 0) {
		async = true;
	} else {
		async = false;
		int_driven = uart_irq_tx_complete(uart_dev) >= 0;
		if (int_driven) {
			uart_irq_callback_set(uart_dev, int_driven_callback);
		}
	}

	/* Setup counter which will periodically enable/disable UART RX,
	 * Disabling RX should lead to flow control being activated.
	 */
	zassert_true(device_is_ready(counter_dev));

	top_cfg.ticks = counter_us_to_ticks(counter_dev, 1000);

	err = counter_set_top_value(counter_dev, &top_cfg);
	zassert_true(err >= 0);

	err = counter_start(counter_dev);
	zassert_true(err >= 0);
}

static void rx_isr(void)
{
	uint8_t buf[64];
	int len;

	do {
		len = uart_fifo_read(uart_dev, buf, BUF_SIZE);
		for (int i = 0; i < len; i++) {
			process_byte(buf[i]);
		}
	} while (len);
}

static void tx_isr(void)
{
	const uint8_t *buf = &int_async_data.buf[int_async_data.cnt & 0xF];
	int len = uart_fifo_fill(uart_dev, buf, 1);

	int_async_data.cnt += len;

	k_busy_wait(len ? 4 : 2);
	uart_irq_tx_disable(uart_dev);
}

static void int_driven_callback(const struct device *dev, void *user_data)
{
	while (uart_irq_is_pending(uart_dev)) {
		if (uart_irq_rx_ready(uart_dev)) {
			rx_isr();
		}
		if (uart_irq_tx_ready(uart_dev)) {
			tx_isr();
		}
	}
}

static void async_callback(const struct device *dev,
			   struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&async_tx_sem);
		break;
	case UART_RX_RDY:
		for (int i = 0; i < evt->data.rx.len; i++) {
			process_byte(evt->data.rx.buf[evt->data.rx.offset + i]);
		}
		break;
	case UART_RX_DISABLED:
		async_rx_enabled = false;
		break;
	default:
		break;

	}
}

static void bulk_poll_out(struct test_data *data, int wait_base, int wait_range)
{
	for (int i = 0; i < data->max; i++) {

		data->cnt++;
		uart_poll_out(uart_dev, data->buf[i % BUF_SIZE]);
		if (wait_base) {
			int r = sys_rand32_get();

			k_sleep(K_USEC(wait_base + (r % wait_range)));
		}
	}

	k_sem_give(&data->sem);
}

static void poll_out_thread(void *data, void *unused0, void *unused1)
{
	bulk_poll_out((struct test_data *)data, 200, 600);
}

K_THREAD_STACK_DEFINE(high_poll_out_thread_stack, 1024);
static struct k_thread high_poll_out_thread;

K_THREAD_STACK_DEFINE(int_async_thread_stack, 1024);
static struct k_thread int_async_thread;

static void int_async_thread_func(void *p_data, void *base, void *range)
{
	struct test_data *data = p_data;
	int wait_base = (int)base;
	int wait_range = (int)range;

	k_sem_init(&async_tx_sem, 1, 1);

	while (data->cnt < data->max) {
		if (async) {
			int err;

			err = k_sem_take(&async_tx_sem, K_MSEC(1000));
			zassert_true(err >= 0);

			int idx = data->cnt & 0xF;
			size_t len = (idx < BUF_SIZE / 2) ? 5 : 1; /* Try various lengths */
			len = MIN(len, data->max - data->cnt);

			data->cnt += len;
			err = uart_tx(uart_dev, &int_async_data.buf[idx],
				      len, 1000 * USEC_PER_MSEC);
			zassert_true(err >= 0,
					"Unexpected err:%d", err);
		} else {
			uart_irq_tx_enable(uart_dev);
		}

		int r = sys_rand32_get();

		k_sleep(K_USEC(wait_base + (r % wait_range)));
	}

	k_sem_give(&data->sem);
}

static void poll_out_timer_handler(struct k_timer *timer)
{
	struct test_data *data = k_timer_user_data_get(timer);

	uart_poll_out(uart_dev, data->buf[data->cnt % BUF_SIZE]);

	data->cnt++;
	if (data->cnt == data->max) {
		k_timer_stop(timer);
		k_sem_give(&data->sem);
	} else {
		k_timer_start(timer, K_USEC(250 + (sys_rand16_get() % 800)),
				K_NO_WAIT);
	}
}

K_TIMER_DEFINE(poll_out_timer, poll_out_timer_handler, NULL);

static void init_buf(uint8_t *buf, int len, int idx)
{
	for (int i = 0; i < len; i++) {
		buf[i] = i | (idx << 4);
	}
}

static void init_test_data(struct test_data *data, const uint8_t *buf, int repeat)
{
	k_sem_init(&data->sem, 0, 1);
	data->buf = buf;
	data->cnt = 0;
	data->max = repeat;
}

ZTEST(uart_mix_fifo_poll, test_mixed_uart_access)
{
	int repeat = CONFIG_STRESS_TEST_REPS;
	int err;
	int num_of_contexts = ARRAY_SIZE(test_data);

	for (int i = 0; i < ARRAY_SIZE(test_data); i++) {
		init_buf(txbuf[i], sizeof(txbuf[i]), i);
		init_test_data(&test_data[i], txbuf[i], repeat);
	}
	(void)k_thread_create(&high_poll_out_thread,
			      high_poll_out_thread_stack, 1024,
			      poll_out_thread, &test_data[0], NULL, NULL,
			      1, 0, K_NO_WAIT);


	if (async || int_driven) {
		init_test_data(&int_async_data, txbuf3, repeat);
		(void)k_thread_create(&int_async_thread,
				int_async_thread_stack, 1024,
				int_async_thread_func,
				&int_async_data, (void *)300, (void *)400,
				2, 0, K_NO_WAIT);
	}

	k_timer_user_data_set(&poll_out_timer, &test_data[1]);
	k_timer_start(&poll_out_timer, K_USEC(250), K_NO_WAIT);

	bulk_poll_out(&test_data[2], 300, 500);

	k_msleep(1);

	for (int i = 0; i < num_of_contexts; i++) {
		err = k_sem_take(&test_data[i].sem, K_MSEC(10000));
		zassert_equal(err, 0);
	}

	if (async || int_driven) {
		err = k_sem_take(&int_async_data.sem, K_MSEC(10000));
		zassert_equal(err, 0);
	}

	k_msleep(10);

	for (int i = 0; i < (num_of_contexts + (async || int_driven ? 1 : 0)); i++) {
		zassert_equal(source[i].cnt, repeat,
				"%d: Unexpected rx bytes count (%d/%d)",
				i, source[i].cnt, repeat);
	}
}

void *uart_mix_setup(void)
{
	static int idx;

	init_test(idx++);

	return NULL;
}

ZTEST_SUITE(uart_mix_fifo_poll, NULL, uart_mix_setup,
		NULL, NULL, NULL);

void test_main(void)
{
	/* Run all suites for each dut UART. Setup function for each suite is picking
	 * next UART from the array.
	 */
	ztest_run_all(NULL, false, ARRAY_SIZE(duts), 1);
	ztest_verify_all_test_suites_ran();
}
