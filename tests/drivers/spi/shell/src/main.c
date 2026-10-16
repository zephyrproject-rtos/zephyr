/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

/* An SPI controller registered without a devicetree node */
static DEVICE_API(spi, nondt_spi_api) = {0};
DEVICE_DEFINE(nondt_spi, "nondt_spi", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_SPI_INIT_PRIORITY, &nondt_spi_api);

#define EMUL_SPI_NAME DEVICE_DT_NAME(DT_NODELABEL(spi0))

/* The emulated controller is an SPI class candidate whether or not its driver
 * is built (CONFIG_EMUL), and the shell resolves the device on first use.
 */
#define EMUL_SPI_EXPECTED (IS_ENABLED(CONFIG_SPI_EMUL) ? 0 : -ENODEV)

static int spi_conf(const char *label)
{
	char cmd[64];

	snprintk(cmd, sizeof(cmd), "spi conf %s 1000000", label);

	return shell_execute_cmd(shell_backend_dummy_get_ptr(), cmd);
}

ZTEST(spi_shell, test_dt_bus_by_name)
{
	zassert_equal(spi_conf(EMUL_SPI_NAME), EMUL_SPI_EXPECTED);
}

ZTEST(spi_shell, test_dt_bus_by_nodelabel)
{
	zassert_equal(spi_conf("spi0"), EMUL_SPI_EXPECTED);
}

ZTEST(spi_shell, test_nondt_device)
{
	const struct device *dev = DEVICE_GET(nondt_spi);

	zassert_true(device_is_ready(dev));
	zassert_equal(spi_conf("nondt_spi"), 0);
}

ZTEST(spi_shell, test_unknown_label)
{
	zassert_equal(spi_conf("no_such_spi"), -ENODEV);
}

ZTEST_SUITE(spi_shell, NULL, NULL, NULL, NULL, NULL);
