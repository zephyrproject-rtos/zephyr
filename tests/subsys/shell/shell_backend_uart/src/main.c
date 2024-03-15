/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <version.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/ztest.h>

#define EMUL_UART_NODE(i)         DT_NODELABEL(euart##i)
#define EMUL_UART_TX_FIFO_SIZE(i) DT_PROP(DT_NODELABEL(euart##i), tx_fifo_size)
#define SAMPLE_DATA_SIZE          EMUL_UART_TX_FIFO_SIZE(0)

struct shell_backend_uart_fixture {
	const struct device *dev;
};

static void before(void *f)
{
	struct shell_backend_uart_fixture *fixture = f;

	uart_irq_tx_enable(fixture->dev);
	uart_irq_rx_enable(fixture->dev);

	uart_err_check(fixture->dev);
}

static void after(void *f)
{
	struct shell_backend_uart_fixture *fixture = f;

	uart_irq_tx_disable(fixture->dev);
	uart_irq_rx_disable(fixture->dev);

	uart_emul_flush_rx_data(fixture->dev);
	uart_emul_flush_tx_data(fixture->dev);
}

ZTEST(shell_backend_uart, test_backends_count)
{
	/* 2 backends: 1 for zephyr,shell-uart, another 1 is created in the test */
	zassert_equal(shell_backend_count_get(), 2, "Expecting 2, got %d",
		      shell_backend_count_get());
}

ZTEST_F(shell_backend_uart, test_backend_euart0_version)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE] = {0};

	uart_emul_put_rx_data(fixture->dev, "kernel version\n", sizeof("kernel version\n"));

	/* Let the shell to run */
	k_usleep(50);

	uart_emul_get_tx_data(fixture->dev, tx_content, SAMPLE_DATA_SIZE);
	zassert_mem_equal(tx_content, "Zephyr version " KERNEL_VERSION_STRING,
			  strlen("Zephyr version " KERNEL_VERSION_STRING));
}

ZTEST_F(shell_backend_uart, test_backend_euart0_cycles)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE] = {0};

	uart_emul_put_rx_data(fixture->dev, "kernel cycles\n", sizeof("kernel cycles\n"));

	/* Let the shell to run */
	k_usleep(50);

	uart_emul_get_tx_data(fixture->dev, tx_content, SAMPLE_DATA_SIZE);
	zassert_mem_equal(tx_content, "cycles: ", strlen("cycles: "));
}

ZTEST_F(shell_backend_uart, test_backend_euart0_uptime)
{
	uint8_t tx_content[SAMPLE_DATA_SIZE] = {0};

	uart_emul_put_rx_data(fixture->dev, "kernel uptime\n", sizeof("kernel uptime\n"));

	/* Let the shell to run */
	k_usleep(50);

	uart_emul_get_tx_data(fixture->dev, tx_content, SAMPLE_DATA_SIZE);
	zassert_mem_equal(tx_content, "Uptime: ", strlen("Uptime: "));
}

static int enable_shell_euart0(const struct device *euart0, const struct shell *sh)
{
	static const struct shell_backend_config_flags cfg_flags = {0};

	if (!device_is_ready(euart0)) {
		return -ENODEV;
	}

	return shell_init(sh, euart0, cfg_flags, false, 0);
}

SHELL_UART_DEFINE(shell_transport_euart0);
SHELL_DEFINE(shell_euart0, "", &shell_transport_euart0,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

static void *setup(void)
{
	static struct shell_backend_uart_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(euart0)),
	};

	zassert_not_null(fixture.dev);
	enable_shell_euart0(fixture.dev, &shell_euart0);

	/* Let the shell backend initialize. */
	k_usleep(10);

	return &fixture;
}

ZTEST_SUITE(shell_backend_uart, NULL, setup, before, after, NULL);
