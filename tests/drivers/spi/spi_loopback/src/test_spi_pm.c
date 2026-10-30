/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/pm.h>

#define SPI_PM_FRAME_SIZE 8192
#define SPI_PM_IDLE_WINDOW_MS 100

#define MODE_LOOP COND_CODE_1(CONFIG_SPI_LOOPBACK_MODE_LOOP, (SPI_MODE_LOOP), (0))
#define SPI_PM_OP                                                                                  \
	(SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | MODE_LOOP | SPI_MODE_CPHA |                      \
	 SPI_WORD_SET(8) | SPI_LINES_SINGLE)

#define SPI_SLOW_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(test_spi_loopback_slow)
static struct spi_dt_spec spi_slow = SPI_DT_SPEC_GET(SPI_SLOW_DEV, SPI_PM_OP);

#ifdef CONFIG_NOCACHE_MEMORY
#define SPI_PM_NOCACHE __attribute__((__section__(".nocache")))
#elif defined(CONFIG_DT_DEFINED_NOCACHE)
#define SPI_PM_NOCACHE __attribute__((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#else
#define SPI_PM_NOCACHE
#endif

#if !defined(CONFIG_NOCACHE_MEMORY) && !defined(CONFIG_DT_DEFINED_NOCACHE) && \
	(CONFIG_DCACHE_LINE_SIZE != 0)
#define SPI_PM_ALIGN __aligned(CONFIG_DCACHE_LINE_SIZE)
#else
#define SPI_PM_ALIGN __aligned(32)
#endif

static SPI_PM_ALIGN uint8_t spi_pm_tx[SPI_PM_FRAME_SIZE] SPI_PM_NOCACHE;
static SPI_PM_ALIGN uint8_t spi_pm_rx[SPI_PM_FRAME_SIZE] SPI_PM_NOCACHE;

static atomic_t spi_pm_standby_entries;
#if defined(CONFIG_SPI_ASYNC)
static int spi_pm_cb_result;
static K_SEM_DEFINE(spi_pm_cb_sem, 0, 1);
#endif

static void spi_pm_state_entry(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		atomic_inc(&spi_pm_standby_entries);
	}
}

static struct pm_notifier spi_pm_notifier = {
	.state_entry = spi_pm_state_entry,
};

#if defined(CONFIG_SPI_ASYNC)
static void spi_pm_cb(const struct device *dev, int result, void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(userdata);

	spi_pm_cb_result = result;
	k_sem_give(&spi_pm_cb_sem);
}
#endif

static void spi_pm_expect_sleep(void)
{
	atomic_clear(&spi_pm_standby_entries);
	k_sleep(K_MSEC(SPI_PM_IDLE_WINDOW_MS));
	zassert_true(atomic_get(&spi_pm_standby_entries) > 0,
		     "System stayed active during the idle window");
}

static void spi_pm_prepare(void)
{
	for (int i = 0; i < SPI_PM_FRAME_SIZE; i++) {
		spi_pm_tx[i] = (uint8_t)i;
	}

	memset(spi_pm_rx, 0, sizeof(spi_pm_rx));
}

static void spi_pm_check(void)
{
	zassert_equal(memcmp(spi_pm_tx, spi_pm_rx, sizeof(spi_pm_tx)), 0,
		      "RX buffer does not match TX");
}

static void spi_pm_transfer(bool asynchronous)
{
	const struct spi_buf tx_buf = {
		.buf = spi_pm_tx,
		.len = sizeof(spi_pm_tx),
	};
	const struct spi_buf rx_buf = {
		.buf = spi_pm_rx,
		.len = sizeof(spi_pm_rx),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int ret;

	spi_pm_prepare();

	if (!asynchronous) {
		ret = spi_transceive_dt(&spi_slow, &tx, &rx);
		zassert_ok(ret, "SPI transceive failed, code %d", ret);
		spi_pm_check();
		return;
	}

#if defined(CONFIG_SPI_ASYNC)
	spi_pm_cb_result = -EIO;
	k_sem_reset(&spi_pm_cb_sem);

	ret = spi_transceive_cb(spi_slow.bus, &spi_slow.config, &tx, &rx, spi_pm_cb, NULL);
	zassert_ok(ret, "SPI async transceive failed, code %d", ret);

	zassert_ok(k_sem_take(&spi_pm_cb_sem, K_MSEC(2000)), "SPI async completion timeout");
	zassert_ok(spi_pm_cb_result, "SPI async transceive failed, code %d", spi_pm_cb_result);
	spi_pm_check();
#endif
}

static void spi_pm_run(bool asynchronous)
{
	zassert_true(device_is_ready(spi_slow.bus), "SPI device is not ready");

	pm_notifier_register(&spi_pm_notifier);

	/* The second pass sleeps after a finished transfer, so the controller
	 * has to come back from light sleep with its configuration intact.
	 */
	for (int cycle = 0; cycle < 2; cycle++) {
		spi_pm_expect_sleep();
		spi_pm_transfer(asynchronous);
	}
}

static void spi_pm_after(void *fixture)
{
	ARG_UNUSED(fixture);

	pm_notifier_unregister(&spi_pm_notifier);
}

ZTEST(spi_pm, test_spi_pm_light_sleep)
{
	spi_pm_run(false);
}

#if defined(CONFIG_SPI_ASYNC)
ZTEST(spi_pm, test_spi_pm_light_sleep_async)
{
	spi_pm_run(true);
}
#endif

ZTEST_SUITE(spi_pm, NULL, NULL, NULL, spi_pm_after, NULL);
