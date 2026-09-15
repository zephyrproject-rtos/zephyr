/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/version.h>

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/busy_sim.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
LOG_MODULE_REGISTER(test, 1);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(euart0), okay)
#include <zephyr/drivers/serial/uart_emul.h>
#define TEST_USE_EMUL 1
#define TEST_UART_NODE          DT_NODELABEL(euart0)
#define TEST_UART_TX_BUF_SIZE   DT_PROP(DT_NODELABEL(euart0), tx_fifo_size)
#define TEST_SHELL_DELAY_US     50
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(aux_uart), okay)
#define TEST_USE_HW_LOOPBACK 1
#define TEST_UART_NODE          DT_NODELABEL(shell_uart)
#define TEST_AUX_UART_NODE      DT_NODELABEL(aux_uart)
#define TEST_UART_TX_BUF_SIZE   256
#define TEST_SHELL_DELAY_US     20000
#define TEST_UART_TX_TIMEOUT_US 10000
#define TEST_UART_RX_TIMEOUT_US 1000
#else
#error "Unsupported test UART configuration"
#endif

#define SAMPLE_DATA_SIZE (TEST_UART_TX_BUF_SIZE + 16)

struct shell_backend_uart_fixture {
	const struct device *dev;
#if defined(TEST_USE_HW_LOOPBACK)
	const struct device *aux_dev;
#endif
};

struct shell_backend_uart_stress_fixture {
	const struct device *dev;
#if defined(TEST_USE_HW_LOOPBACK)
	const struct device *aux_dev;
#endif
};

#define CHUNK_DELAY_US                                                  \
	COND_CODE_1(CONFIG_SHELL_BACKEND_SERIAL_ASYNC_RX_TIMEOUT,       \
		(CONFIG_SHELL_BACKEND_SERIAL_ASYNC_RX_TIMEOUT), (1000))

#if defined(TEST_USE_HW_LOOPBACK)
struct test_uart_async_ctx {
	struct k_sem tx_done;
	struct k_sem rx_disabled;
	struct k_sem rx_rdy;
	uint8_t data_buf[4 * TEST_UART_TX_BUF_SIZE];
	uint8_t rx_buf[2][TEST_UART_TX_BUF_SIZE];
	size_t rx_buf_idx;
	size_t rx_total;
};

static struct test_uart_async_ctx aux_async;

static void test_uart_async_callback(const struct device *dev, struct uart_event *evt,
				     void *user_data)
{
	struct test_uart_async_ctx *ctx = user_data;

	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&ctx->tx_done);
		break;
	case UART_RX_RDY:
	{
		size_t len = evt->data.rx.len;

		memcpy(&ctx->data_buf[ctx->rx_total], &evt->data.rx.buf[evt->data.rx.offset], len);
		ctx->rx_total += len;
		break;
	}
	case UART_RX_BUF_REQUEST:
	{
		int rv;
		uint8_t *rx_buf = ctx->rx_buf[ctx->rx_buf_idx++ & 0x1];

		rv = uart_rx_buf_rsp(dev, rx_buf, sizeof(ctx->rx_buf[0]));
		LOG_INF("buf_req: rv=%d", rv);
		zassert_equal(rv, 0, "uart_rx_buf_rsp failed: %d", rv);
		break;
	}
	case UART_RX_DISABLED:
		LOG_INF("rx_disabled");
		k_sem_give(&ctx->rx_disabled);
		break;
	case UART_RX_STOPPED:
		LOG_ERR("rx_stopped");
		break;
	default:
		break;
	}
}

static void test_uart_async_reset_rx(struct test_uart_async_ctx *ctx)
{
	ctx->rx_total = 0;
	memset(ctx->rx_buf[0], 0, sizeof(ctx->rx_buf[0]));
	memset(ctx->rx_buf[1], 0, sizeof(ctx->rx_buf[1]));
	memset(ctx->data_buf, 0, sizeof(ctx->data_buf));
	k_sem_reset(&ctx->rx_rdy);
	k_sem_reset(&ctx->rx_disabled);
}

static void test_uart_async_rx_start(const struct device *aux, struct test_uart_async_ctx *ctx)
{
	int err;

	test_uart_async_reset_rx(ctx);
	err = uart_rx_enable(aux, ctx->rx_buf[0], sizeof(ctx->rx_buf[0]), TEST_UART_RX_TIMEOUT_US);
	zassert_equal(err, 0, "uart_rx_enable failed: %d", err);

	ctx->rx_buf_idx = 1;
	ctx->rx_total = 0;
}

static void test_uart_async_rx_stop(const struct device *aux, struct test_uart_async_ctx *ctx)
{
	uart_rx_disable(aux);
	zassert_equal(k_sem_take(&ctx->rx_disabled, K_MSEC(100)), 0, "RX_DISABLED timeout");
}

static void test_uart_put_rx_data(const struct device *aux, struct test_uart_async_ctx *ctx,
				  const uint8_t *data, size_t len)
{
	int err;

	k_sem_reset(&ctx->tx_done);
	err = uart_tx(aux, (uint8_t *)data, len, TEST_UART_TX_TIMEOUT_US);
	zassert_equal(err, 0, "uart_tx failed: %d", err);
	zassert_equal(k_sem_take(&ctx->tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
}

static uint32_t test_uart_get_tx_data(struct test_uart_async_ctx *ctx, uint8_t **data, size_t len)
{
	size_t copy_len;

	copy_len = MIN(ctx->rx_total, len);
	*data = ctx->data_buf;

	return copy_len;
}

static void test_uart_async_init(const struct device *aux, struct test_uart_async_ctx *ctx)
{
	int err;

	k_sem_init(&ctx->tx_done, 0, 1);
	k_sem_init(&ctx->rx_disabled, 0, 1);
	k_sem_init(&ctx->rx_rdy, 0, 255);

	err = uart_callback_set(aux, test_uart_async_callback, ctx);
	zassert_equal(err, 0, "uart_callback_set failed: %d", err);
}
#endif

static void before(void *f)
{
	struct shell_backend_uart_fixture *fixture = f;

#if defined(TEST_USE_HW_LOOPBACK)
	uart_tx_abort(fixture->aux_dev);
	uart_rx_disable(fixture->aux_dev);
	test_uart_async_rx_start(fixture->aux_dev, &aux_async);
#else
	uart_irq_tx_enable(fixture->dev);
	uart_irq_rx_enable(fixture->dev);
#endif

	uart_err_check(fixture->dev);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_get(fixture->dev);
	}
}

static void after(void *f)
{
	struct shell_backend_uart_fixture *fixture = f;
#if defined(TEST_USE_EMUL) && defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_tx_disable(fixture->dev);
	uart_irq_rx_disable(fixture->dev);

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);
#elif defined(TEST_USE_HW_LOOPBACK)
	test_uart_async_rx_stop(fixture->aux_dev, &aux_async);
	uart_tx_abort(fixture->aux_dev);
#endif

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_put(fixture->dev);
	}
}

static void test_put_shell_cmd(struct shell_backend_uart_fixture *fixture, const char *cmd,
			       size_t len)
{
	if (len == 0) {
		len = strlen(cmd);
	}
#if defined(TEST_USE_EMUL)
	uart_emul_put_rx_data(fixture->dev, cmd, len);
#elif defined(TEST_USE_HW_LOOPBACK)
	test_uart_put_rx_data(fixture->aux_dev, &aux_async, cmd, len);
#endif
}

static uint32_t test_get_shell_response(struct shell_backend_uart_fixture *fixture,
				    uint8_t **tx_content, size_t len)
{
#if defined(TEST_USE_EMUL)
	static uint8_t tx_content_buf[SAMPLE_DATA_SIZE];
	*tx_content = tx_content_buf;

	return uart_emul_get_tx_data(fixture->dev, *tx_content, len);
#elif defined(TEST_USE_HW_LOOPBACK)
	return test_uart_get_tx_data(&aux_async, tx_content, len);
#endif
}

ZTEST(shell_backend_uart, test_backends_count)
{
	/* 2 backends: 1 for zephyr,shell-uart, another 1 is created in the test */
	zassert_equal(shell_backend_count_get(), 2, "Expecting 2, got %d",
		      shell_backend_count_get());
}

static void put_shell_cmd_in_chunks(struct shell_backend_uart_fixture *fixture, const char *cmd,
				    size_t len)
{
	if (len == 0) {
		len = strlen(cmd);
	}

	/* Determine number of chunks: random value from 1 to 4 */
	int num_chunks = (sys_rand32_get() % 4) + 1;

	size_t offset = 0;
	size_t remaining = len;

	for (int i = 0; i < num_chunks; i++) {
		/* If last chunk, send all remaining. Otherwise, random chunk size */
		size_t chunk_size;

		if (i == num_chunks - 1 || remaining == 0) {
			chunk_size = remaining;
		} else {
			/* Ensure at least 1 byte in each chunk */
			size_t max_this_chunk = remaining - (num_chunks - i - 1);

			if (max_this_chunk < 1) {
				chunk_size = 1;
			} else {
				chunk_size = (sys_rand32_get() % max_this_chunk) + 1;
			}
		}
		if (chunk_size == 0) {
			break;
		}

		test_put_shell_cmd(fixture, &cmd[offset], chunk_size);
		offset += chunk_size;
		remaining -= chunk_size;

		if (i != num_chunks - 1) {
			/* Random wait: CHUNK_DELAY_US +- 100us */
			int delta = (int)(sys_rand32_get() % 201) - 100;
			int wait_us = CHUNK_DELAY_US + delta;

			k_usleep(wait_us);
		}
	}
}

static void test_cmd_response(struct shell_backend_uart_fixture *fixture, const char *cmd,
			      const char *expected_response, bool use_chunks, bool lossless)
{
	uint8_t *tx_content;
	uint32_t len;

	if (use_chunks) {
		put_shell_cmd_in_chunks(fixture, cmd, 0);
	} else {
		test_put_shell_cmd(fixture, cmd, 0);
	}
	k_usleep(TEST_SHELL_DELAY_US);
	len = test_get_shell_response(fixture, &tx_content, SAMPLE_DATA_SIZE);
	if (lossless) {
		zassert_true(len > 0, "Expected response from shell backend got:%d", len);
		const char *found = strstr(tx_content, expected_response);

		if (found == NULL) {
			LOG_ERR("err");
			printk("Unexpected response len:%d exp_len:%d\n",
				len, strlen(expected_response));
		}
		zassert_not_null(found, "Expected response to contain '%s' got %s",
				expected_response, tx_content);
	} else {
		if (len == 0) {
			/* Send newline which might got lost. */
			test_put_shell_cmd(fixture, "\n", 1);
			k_usleep(TEST_SHELL_DELAY_US);
			/* Now we expect the response but it might be incomplete - no check here. */
			len = test_get_shell_response(fixture, &tx_content, SAMPLE_DATA_SIZE);
		}
		if (len == 0) {
			LOG_ERR("Expected response from shell backend got:%d", len);
		}
		zassert_true(len > 0, "Expected response from shell backend got:%d", len);

		bool cmd_not_found_present = strstr(tx_content, "command not found") != NULL;
		bool cmd_start_present = memcmp(tx_content, cmd, 4) == 0;

		zassert_true(cmd_not_found_present || cmd_start_present);
	}
}

#if !defined(TEST_USE_EMUL)
/* Stress test attempts to send long command in chunks. If HWFC is disabled then
 * some lost bytes are expected but shell must be operational after series of those
 * long commands. If HWFC is enabled then no lost bytes are expected.
 */
ZTEST_F(shell_backend_uart_stress, test_stress)
{
	static char cmd_buf[CONFIG_SHELL_CMD_BUFF_SIZE + 1];
	static char expected_rsp_buf[CONFIG_SHELL_CMD_BUFF_SIZE];

	int rpt = 100;
	int load = 0;
	bool hwfc = ZTEST_GET_PARAM(bool);
	struct shell_backend_uart_fixture *f = (struct shell_backend_uart_fixture *)fixture;

	for (int i = 0; i < CONFIG_SHELL_CMD_BUFF_SIZE - 1; i++) {
		cmd_buf[i] = 'a' + (i % 26);
	}
	cmd_buf[CONFIG_SHELL_CMD_BUFF_SIZE - 1] = '\n';
	cmd_buf[CONFIG_SHELL_CMD_BUFF_SIZE] = '\0';

	memcpy(expected_rsp_buf, cmd_buf, CONFIG_SHELL_CMD_BUFF_SIZE - 1);
	expected_rsp_buf[CONFIG_SHELL_CMD_BUFF_SIZE - 1] = '\0';

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}
	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_start(50, 20, 200, 40, NULL);
	}

	for (int i = 0; i < rpt; i++) {
		test_cmd_response(f, cmd_buf, expected_rsp_buf, true, hwfc);
		test_uart_async_rx_stop(f->aux_dev, &aux_async);
		test_uart_async_rx_start(f->aux_dev, &aux_async);
	}

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_stop();
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		load = (int)cpu_load_get(true);
		printk("CPU load: %d.%d\n", load / 10, load % 10);
	}

	/* Validate that shell is working ok. */
	test_cmd_response(f, "kernel cycles\n", "cycles: ", false, true);
}
#endif /* !defined(TEST_USE_EMUL) */

ZTEST_F(shell_backend_uart, test_version_cmd)
{
	test_cmd_response(fixture, "kernel version\n", "Zephyr version " KERNEL_VERSION_STRING,
			false, true);
}

ZTEST_F(shell_backend_uart, test_cycles_cmd)
{
	test_cmd_response(fixture, "kernel cycles\n", "cycles: ", false, true);
}

ZTEST_F(shell_backend_uart, test_uptime_cmd)
{
	test_cmd_response(fixture, "kernel uptime\n", "Uptime: ", false, true);
}

static int enable_shell_uart(const struct device *uart, const struct shell *sh)
{
	static const struct shell_backend_config_flags cfg_flags = {0};
	int rv;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}

	rv = shell_init(sh, uart, cfg_flags, false, 0);

	/* Let the shell backend initialize. */
	k_usleep(TEST_SHELL_DELAY_US);

	return rv;
}

SHELL_UART_DEFINE(shell_transport_dut_uart);
SHELL_DEFINE(shell_dut_uart, "", &shell_transport_dut_uart,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

static void *setup_common(void)
{
	static struct shell_backend_uart_fixture fixture = {
		.dev = DEVICE_DT_GET(TEST_UART_NODE),
#if defined(TEST_USE_HW_LOOPBACK)
		.aux_dev = DEVICE_DT_GET(TEST_AUX_UART_NODE),
#endif
	};

	return &fixture;
}

static void *setup(void)
{
	int rv;

	struct shell_backend_uart_fixture *fixture = setup_common();

	zassert_not_null(fixture->dev);
#if defined(TEST_USE_HW_LOOPBACK)
	zassert_not_null(fixture->aux_dev);
	test_uart_async_init(fixture->aux_dev, &aux_async);
#endif

	rv = enable_shell_uart(fixture->dev, &shell_dut_uart);
	zassert_equal(rv, 0, "enable_shell_uart failed rv:%d", rv);

#if defined(TEST_USE_EMUL)
	uint8_t tx_content[SAMPLE_DATA_SIZE] = {0};

	/* get the shell startup newline */
	rv = uart_emul_get_tx_data(fixture->dev, tx_content, SAMPLE_DATA_SIZE);
	zassert_true(rv > 0, "Expected startup banner from shell backend");
	zassert_mem_equal(tx_content, "\r\n", strlen("\r\n"));
#endif

	return fixture;
}

static void shell_uninit_cb(const struct shell *sh, int res)
{
}

static void teardown(void *f)
{
	shell_stop(&shell_dut_uart);

	shell_uninit(&shell_dut_uart, shell_uninit_cb);
	k_msleep(10);
}

static void *setup_stress(void)
{
	return setup_common();
}

static void before_stress(void *f)
{
	int rv;
	struct shell_backend_uart_fixture *fixture = f;
	bool hwfc = ZTEST_GET_PARAM(bool);
	struct uart_config config;

	/* Reconfigue HWFC based on the test parameter. */
	rv = uart_config_get(fixture->dev, &config);
	zassert_equal(rv, 0, "uart_config_get failed rv:%d", rv);
	config.flow_ctrl = hwfc ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;

	rv = uart_configure(fixture->dev, &config);
	zassert_equal(rv, 0, "uart_config_set failed rv:%d", rv);

#if defined(TEST_USE_HW_LOOPBACK)
	/* When other API is used we don't have option to reconfigure. */
	rv = uart_configure(fixture->aux_dev, &config);
	zassert_equal(rv, 0, "uart_configure failed rv:%d", rv);

	test_uart_async_init(fixture->aux_dev, &aux_async);
#endif

	rv = enable_shell_uart(fixture->dev, &shell_dut_uart);
	zassert_equal(rv, 0, "enable_shell_uart failed rv:%d", rv);

	before(f);
}

static void after_stress(void *f)
{
	after(f);

	shell_stop(&shell_dut_uart);

	shell_uninit(&shell_dut_uart, shell_uninit_cb);
	k_msleep(10);
}

ZTEST_SUITE(shell_backend_uart, NULL, setup, before, after, teardown);

ZTEST_DEFINE_PARAM_VALUES(hwfc_vals, bool, false, true);
ZTEST_INSTANTIATE_TEST_SUITE_P(hwfc, shell_backend_uart_stress, test_stress, hwfc_vals);
ZTEST_SUITE(shell_backend_uart_stress, NULL, setup_stress, before_stress, after_stress, NULL);
