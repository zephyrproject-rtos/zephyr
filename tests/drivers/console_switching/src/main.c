/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/console/console.h>
#include <zephyr/drivers/misc/devmux/devmux.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define BUF_SIZE 32

/* array of const struct device* */
#define PHANDLE_TO_DEVICE(node_id, prop, idx) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))
static const struct device *devs[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(devmux0), devices, PHANDLE_TO_DEVICE, (,))};

/* array of names, e.g. "euart0" */
#define PHANDLE_TO_NAME(node_id, prop, idx) DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(node_id, prop, idx))
static const char *const name[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(devmux0), devices, PHANDLE_TO_NAME, (,))};

/* array of greetings, e.g. "Hello, euart0!" */
#define PHANDLE_TO_TEXT(node_id, prop, idx)                                                        \
	"Hello, " DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(node_id, prop, idx)) "!"
static const char *const text[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(devmux0), devices, PHANDLE_TO_TEXT, (,))};

ZTEST(console_switching, test_write)
{
	size_t normal_uart = DT_PROP(DT_NODELABEL(devmux0), selected);
	struct device *const devmux_dev = DEVICE_DT_GET(DT_NODELABEL(devmux0));

	/* for each uart_emul device */
	for (size_t i = 0, j = 0, N = ARRAY_SIZE(devs); i < 2 * N; i++, j++, j %= N) {
		if (j == normal_uart) {
			/* skip testing non-emul uart */
			continue;
		}

		int ret[4];
		char buf[BUF_SIZE] = {0};

		/* write text[j] to dev[j] */
		ret[0] = devmux_select_set(devmux_dev, j);
		printk("%s", text[j]);
		ret[1] = uart_emul_get_tx_data(devs[j], buf, ARRAY_SIZE(buf));
		ret[2] = devmux_select_set(devmux_dev, normal_uart);

		zassert_ok(ret[0], "Failed to select devmux %zu", j);
		zassert_ok(ret[2], "Switching back to selection %zu failed", normal_uart);

		/* verify that text[j] was written to dev[j] */
		TC_PRINT("wrote '%s' to %s\n", buf, name[j]);

		zassert_equal(ret[1], strlen(text[j]), "Only wrote %zu/%zu bytes of '%s'",
			      ret[1], strlen(text[j]), text[j]);
		zassert_equal(0, strcmp(text[j], buf), "Strings '%s' and '%s' do not match",
			      text[j], buf);
	}
}

ZTEST(console_switching, test_read)
{
	size_t normal_uart = DT_PROP(DT_NODELABEL(devmux0), selected);
	struct device *const devmux_dev = DEVICE_DT_GET(DT_NODELABEL(devmux0));

	/* for each uart_emul device */
	for (size_t i = 0, j = 0, N = ARRAY_SIZE(devs); i < 2 * N; i++, j++, j %= N) {
		if (j == normal_uart) {
			/* skip testing non-emul uart */
			continue;
		}

		int ret[4];
		char buf[BUF_SIZE] = {0};

		/* read text[j] from dev[j] */
		ret[0] = devmux_select_set(devmux_dev, j);
		console_getline_init();
		ret[1] = uart_emul_put_rx_data(devs[j], (uint8_t *)text[j], strlen(text[j]));
		ret[3] = uart_emul_put_rx_data(devs[j], "\n", 1);
		snprintf(buf, BUF_SIZE, "%s", console_getline());
		ret[2] = devmux_select_set(devmux_dev, normal_uart);

		zassert_ok(ret[0], "Failed to select devmux %zu", j);
		zassert_ok(ret[2], "Switching back to selection %zu failed", normal_uart);

		/* verify that text[j] was written to dev[j] */
		TC_PRINT("read '%s' from %s\n", buf, name[j]);

		zassert_equal(ret[1], strlen(text[j]), "Only put %zu/%zu bytes of '%s'",
			      ret[1], strlen(text[j]), text[j]);
		zassert_equal(0, strcmp(text[j], buf), "Strings '%s' and '%s' do not match",
			      text[j], buf);
	}
}

static void *setup(void)
{
	size_t selected = DT_PROP(DT_NODELABEL(devmux1), selected);
	struct device *const devmux_dev = DEVICE_DT_GET(DT_NODELABEL(devmux1));

	/* ensure that non-default initial selection via DT works */
	zassert_equal(devmux_select_get(devmux_dev), selected);

	return NULL;
}

static void before(void *arg)
{
	struct device *const devmux_dev = DEVICE_DT_GET(DT_NODELABEL(devmux0));

	zassert_ok(devmux_select_set(devmux_dev, 0));
	zassert_ok(devmux_select_get(devmux_dev));

	for (size_t i = 1; i < ARRAY_SIZE(devs); ++i) {
		uart_emul_flush_tx_data(devs[i]);
	}
}

ZTEST_SUITE(console_switching, NULL, setup, before, NULL, NULL);
