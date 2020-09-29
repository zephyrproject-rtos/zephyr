/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <ztest.h>

#include <drivers/spi.h>

#define SPI_DRV_NAME	CONFIG_SPI_LOOPBACK_DRV_NAME
#define SPI_SLAVE	CONFIG_SPI_LOOPBACK_SLAVE_NUMBER
#define SLOW_FREQ	CONFIG_SPI_LOOPBACK_SLOW_FREQ
#define FAST_FREQ	CONFIG_SPI_LOOPBACK_FAST_FREQ

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
#define CS_CTRL_GPIO_DRV_NAME CONFIG_SPI_LOOPBACK_CS_CTRL_GPIO_DRV_NAME
struct spi_cs_control spi_cs = {
	.gpio_pin = CONFIG_SPI_LOOPBACK_CS_CTRL_GPIO_PIN,
	.gpio_dt_flags = GPIO_ACTIVE_LOW,
	.delay = 0,
};
#define SPI_CS (&spi_cs)
#else
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""
#endif

/* to run this test, connect MOSI pin to the MISO of the SPI */

#define STACK_SIZE 512
#define BUF_SIZE 17
uint8_t buffer_tx[] = "0123456789abcdef\0";
uint8_t buffer_rx[BUF_SIZE] = {};

#define BUF2_SIZE 36
uint8_t buffer2_tx[] = "Thequickbrownfoxjumpsoverthelazydog\0";
uint8_t buffer2_rx[BUF2_SIZE] = {};

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
uint8_t buffer_print_tx[BUF_SIZE * 5 + 1];
uint8_t buffer_print_rx[BUF_SIZE * 5 + 1];

uint8_t buffer_print_tx2[BUF2_SIZE * 5 + 1];
uint8_t buffer_print_rx2[BUF2_SIZE * 5 + 1];

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

struct spi_config spi_cfg_slow = {
	.frequency = SLOW_FREQ,
#if CONFIG_SPI_LOOPBACK_MODE_LOOP
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_LOOP |
#else
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
#endif
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

struct spi_config spi_cfg_fast = {
	.frequency = FAST_FREQ,
#if CONFIG_SPI_LOOPBACK_MODE_LOOP
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_LOOP |
#else
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
#endif
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
static int cs_ctrl_gpio_config(void)
{
	spi_cs.gpio_dev = device_get_binding(CS_CTRL_GPIO_DRV_NAME);
	if (!spi_cs.gpio_dev) {
		LOG_ERR("Cannot find %s!", CS_CTRL_GPIO_DRV_NAME);
		zassert_not_null(spi_cs.gpio_dev, "Invalid gpio device");
		return -1;
	}

	return 0;
}
#endif /* CONFIG_SPI_LOOPBACK_CS_GPIO */

/* test transferring different buffers on the same dma channels */
static int spi_complete_multiple(const struct device *dev,
				 struct spi_config *spi_conf)
{
	struct spi_buf tx_bufs[2];
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	tx_bufs[0].buf = buffer_tx;
	tx_bufs[0].len = BUF_SIZE;

	tx_bufs[1].buf = buffer2_tx;
	tx_bufs[1].len = BUF2_SIZE;


	struct spi_buf rx_bufs[2];
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	rx_bufs[0].buf = buffer_rx;
	rx_bufs[0].len = BUF_SIZE;

	rx_bufs[1].buf = buffer2_rx;
	rx_bufs[1].len = BUF2_SIZE;

	int ret;

	LOG_INF("Start complete multiple");

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
	}

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	if (memcmp(buffer2_tx, buffer2_rx, BUF2_SIZE)) {
		to_display_format(buffer2_tx, BUF2_SIZE, buffer_print_tx2);
		to_display_format(buffer2_rx, BUF2_SIZE, buffer_print_rx2);
		LOG_ERR("Buffer 2 contents are different: %s",
			    buffer_print_tx2);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx2);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_complete_loop(const struct device *dev,
			     struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	int ret;

	LOG_INF("Start complete loop");

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
	}

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_null_tx_buf(const struct device *dev,
			   struct spi_config *spi_conf)
{
	static const uint8_t EXPECTED_NOP_RETURN_BUF[BUF_SIZE] = { 0 };
	(void)memset(buffer_rx, 0x77, BUF_SIZE);

	const struct spi_buf tx_bufs[] = {
		{
	       /*
		* According to documentation, when sending NULL tx buf -
		*  NOP frames should be sent on MOSI line
		*/
			.buf = NULL,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	int ret;

	LOG_INF("Start null tx");

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return ret;
	}


	if (memcmp(buffer_rx, EXPECTED_NOP_RETURN_BUF, BUF_SIZE)) {
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		LOG_ERR("Rx Buffer should contain NOP frames but got: %s",
			buffer_print_rx);
		zassert_false(1, "Buffer not as expected");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_start(const struct device *dev,
			     struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = 8,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};
	int ret;

	LOG_INF("Start half start");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

	if (memcmp(buffer_tx, buffer_rx, 8)) {
		to_display_format(buffer_tx, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_end(const struct device *dev,
			   struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = 8,
		},
		{
			.buf = buffer_rx,
			.len = 8,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};
	int ret;

	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip half end");
		return 0;
	}

	LOG_INF("Start half end");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

	if (memcmp(buffer_tx+8, buffer_rx, 8)) {
		to_display_format(buffer_tx + 8, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

static int spi_rx_every_4(const struct device *dev,
			  struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = 4,
		},
		{
			.buf = buffer_rx,
			.len = 4,
		},
		{
			.buf = NULL,
			.len = 4,
		},
		{
			.buf = buffer_rx + 4,
			.len = 4,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};
	int ret;

	if (IS_ENABLED(CONFIG_SPI_STM32_DMA)) {
		LOG_INF("Skip every 4");
		return 0;
	}

	LOG_INF("Start every 4");

	(void)memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

	if (memcmp(buffer_tx + 4, buffer_rx, 4)) {
		to_display_format(buffer_tx + 4, 4, buffer_print_tx);
		to_display_format(buffer_rx, 4, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	} else if (memcmp(buffer_tx + 12, buffer_rx + 4, 4)) {
		to_display_format(buffer_tx + 12, 4, buffer_print_tx);
		to_display_format(buffer_rx + 4, 4, buffer_print_rx);
		LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		zassert_false(1, "Buffer contents are different");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}

#if (CONFIG_SPI_ASYNC)
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
K_THREAD_STACK_DEFINE(spi_async_stack, STACK_SIZE);
static int result = 1;

static void spi_async_call_cb(struct k_poll_event *async_evt,
			      struct k_sem *caller_sem,
			      void *unused)
{
	int ret;

	LOG_DBG("Polling...");

	while (1) {
		ret = k_poll(async_evt, 1, K_MSEC(200));
		zassert_false(ret, "one or more events are not ready");

		result = async_evt->signal->result;
		k_sem_give(caller_sem);

		/* Reinitializing for next call */
		async_evt->signal->signaled = 0U;
		async_evt->state = K_POLL_STATE_NOT_READY;
	}
}

static int spi_async_call(const struct device *dev,
			  struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};
	int ret;

	LOG_INF("Start async call");

	ret = spi_transceive_async(dev, spi_conf, &tx, &rx, &async_sig);
	if (ret == -ENOTSUP) {
		LOG_DBG("Not supported");
		return 0;
	}

	if (ret) {
		LOG_ERR("Code %d", ret);
		zassert_false(ret, "SPI transceive failed");
		return -1;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result)  {
		LOG_ERR("Call code %d", ret);
		zassert_false(result, "SPI transceive failed");
		return -1;
	}

	LOG_INF("Passed");

	return 0;
}
#endif

static int spi_resource_lock_test(const struct device *lock_dev,
				  struct spi_config *spi_conf_lock,
				  const struct device *try_dev,
				  struct spi_config *spi_conf_try)
{
	spi_conf_lock->operation |= SPI_LOCK_ON;

	if (spi_complete_loop(lock_dev, spi_conf_lock)) {
		return -1;
	}

	if (spi_release(lock_dev, spi_conf_lock)) {
		LOG_ERR("Deadlock now?");
		zassert_false(1, "SPI release failed");
		return -1;
	}

	if (spi_complete_loop(try_dev, spi_conf_try)) {
		return -1;
	}

	return 0;
}

void test_spi_loopback(void)
{
#if (CONFIG_SPI_ASYNC)
	struct k_thread async_thread;
	k_tid_t async_thread_id;
#endif
	const struct device *spi_slow;
	const struct device *spi_fast;

	LOG_INF("SPI test on buffers TX/RX %p/%p", buffer_tx, buffer_rx);

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
	if (cs_ctrl_gpio_config()) {
		return;
	}
#endif /* CONFIG_SPI_LOOPBACK_CS_GPIO */

	spi_slow = device_get_binding(SPI_DRV_NAME);
	if (!spi_slow) {
		LOG_ERR("Cannot find %s!\n", SPI_DRV_NAME);
		zassert_not_null(spi_slow, "Invalid SPI device");
		return;
	}

	spi_fast = spi_slow;

#if (CONFIG_SPI_ASYNC)
	async_thread_id = k_thread_create(&async_thread,
					  spi_async_stack, STACK_SIZE,
					  (k_thread_entry_t)spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, K_NO_WAIT);
#endif

	LOG_INF("SPI test slow config");

	if (spi_complete_multiple(spi_slow, &spi_cfg_slow) ||
	    spi_complete_loop(spi_slow, &spi_cfg_slow) ||
	    spi_null_tx_buf(spi_slow, &spi_cfg_slow) ||
	    spi_rx_half_start(spi_slow, &spi_cfg_slow) ||
	    spi_rx_half_end(spi_slow, &spi_cfg_slow) ||
	    spi_rx_every_4(spi_slow, &spi_cfg_slow)
#if (CONFIG_SPI_ASYNC)
	    || spi_async_call(spi_slow, &spi_cfg_slow)
#endif
	    ) {
		goto end;
	}

	LOG_INF("SPI test fast config");

	if (spi_complete_multiple(spi_fast, &spi_cfg_fast) ||
	    spi_complete_loop(spi_fast, &spi_cfg_fast) ||
	    spi_null_tx_buf(spi_fast, &spi_cfg_fast) ||
	    spi_rx_half_start(spi_fast, &spi_cfg_fast) ||
	    spi_rx_half_end(spi_fast, &spi_cfg_fast) ||
	    spi_rx_every_4(spi_fast, &spi_cfg_fast)
#if (CONFIG_SPI_ASYNC)
	    || spi_async_call(spi_fast, &spi_cfg_fast)
#endif
	    ) {
		goto end;
	}

	if (spi_resource_lock_test(spi_slow, &spi_cfg_slow,
				   spi_fast, &spi_cfg_fast)) {
		goto end;
	}

	LOG_INF("All tx/rx passed");
end:
#if (CONFIG_SPI_ASYNC)
	k_thread_abort(async_thread_id);
#else
	;
#endif
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_spi, ztest_unit_test(test_spi_loopback));
	ztest_run_test_suite(test_spi);
}
