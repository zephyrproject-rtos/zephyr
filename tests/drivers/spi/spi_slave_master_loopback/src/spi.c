/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#include <zephyr.h>
#include <misc/printk.h>
#include <string.h>
#include <stdio.h>

#include <spi.h>

#ifdef CONFIG_TEST_SPI_MASTER_CS_GPIO_ENABLE
struct spi_cs_control spi_master_cs = {
	.gpio_pin = CONFIG_TEST_MASTER_CS_GPIO_PIN,
	.delay = 0
};
#define SPI_MASTER_CS (&spi_master_cs)
#else
#define SPI_MASTER_CS NULL
#endif

#ifdef CONFIG_TEST_SPI_SLAVE_CS_IGNORE
/* Ignore CS for slave
 * On STM32, at least, if CS==NULL, it will use the internal CS
 * handler to allow a transfer.
 * If we pass a valid CS pointer with a NULL gpio_dev pointer, it
 * will ignore CS completely and rely only on clock cycles.
 */
struct spi_cs_control spi_slave_cs = {
	.gpio_pin = 0,
	.delay = 0
};
#define SPI_SLAVE_CS (&spi_slave_cs)
#else
#define SPI_SLAVE_CS NULL
#endif

#define BUF_SIZE 17
u8_t buffer_tx[2][BUF_SIZE] = {"0123456789abcdef\0", "ghijklmnopqrstuv\0"};
u8_t buffer_rx[2][BUF_SIZE] = {};

/*
 * We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
u8_t buffer_print_tx[BUF_SIZE * 5 + 1];
u8_t buffer_print_rx[BUF_SIZE * 5 + 1];

static void to_display_format(const u8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

struct spi_config spi_master_slow = {
	.frequency = CONFIG_TEST_MASTER_SLOW_FREQ,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = CONFIG_TEST_SPI_SLAVE,
	.cs = SPI_MASTER_CS,
};

struct spi_config spi_master_fast = {
	.frequency = CONFIG_TEST_MASTER_FAST_FREQ,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = CONFIG_TEST_SPI_SLAVE,
	.cs = SPI_MASTER_CS,
};

struct spi_config spi_slave = {
	.frequency = CONFIG_TEST_SLAVE_FREQ,
	.operation = SPI_OP_MODE_SLAVE | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = CONFIG_TEST_SPI_SLAVE,
	.cs = SPI_SLAVE_CS,
};

#ifdef CONFIG_TEST_SPI_MASTER_CS_GPIO_ENABLE
static int master_cs_ctrl_gpio_config(struct spi_cs_control *cs)
{
	cs->gpio_dev = device_get_binding(CONFIG_TEST_MASTER_CS_GPIO_DRV_NAME);
	if (!cs->gpio_dev) {
		SYS_LOG_ERR("Cannot find %s!\n",
				CONFIG_TEST_MASTER_CS_GPIO_DRV_NAME);
		return -1;
	}

	return 0;
}
#endif

static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
K_THREAD_STACK_DEFINE(spi_async_stack, 256);
static int result = 1;

static void spi_async_call_cb(struct k_poll_event *async_evt,
			      struct k_sem *caller_sem,
			      void *unused)
{
	SYS_LOG_DBG("Polling...");

	while (1) {
		k_poll(async_evt, 1, K_MSEC(100));

		result = async_evt->signal->result;
		k_sem_give(caller_sem);

		/* Reinitializing for next call */
		async_evt->signal->signaled = 0;
		async_evt->state = K_POLL_STATE_NOT_READY;
	}
}

static int spi_async_full_call(struct spi_config *spi_sync_conf,
			       struct spi_config *spi_async_conf)
{
	const struct spi_buf tx_bufs_async[] = {
		{
			.buf = buffer_tx[0],
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf tx_bufs_sync[] = {
		{
			.buf = buffer_tx[1],
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs_async[] = {
		{
			.buf = buffer_rx[0],
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs_sync[] = {
		{
			.buf = buffer_rx[1],
			.len = BUF_SIZE,
		},
	};
	int ret;

	SYS_LOG_INF("Full: Start Async");

	ret = spi_transceive_async(spi_async_conf,
				   tx_bufs_async, ARRAY_SIZE(tx_bufs_async),
				   rx_bufs_async, ARRAY_SIZE(rx_bufs_async),
				   &async_sig);
	if (ret == -ENOTSUP) {
		SYS_LOG_DBG("Async: Not supported");
		return 0;
	}

	if (ret) {
		SYS_LOG_ERR("Async Code %d", ret);
		return -1;
	}

	SYS_LOG_INF("Full: Start Sync");

	ret = spi_transceive(spi_sync_conf,
			     tx_bufs_sync, ARRAY_SIZE(tx_bufs_sync),
			     rx_bufs_sync, ARRAY_SIZE(rx_bufs_sync));
	if (ret) {
		SYS_LOG_ERR("Sync Code %d", ret);
		return ret;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result)  {
		SYS_LOG_ERR("Async Call code %d", result);
		return -1;
	}

	if (memcmp(buffer_tx[0], buffer_rx[1], BUF_SIZE)) {
		to_display_format(buffer_tx[0], BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx[1], BUF_SIZE, buffer_print_rx);
		SYS_LOG_ERR("Async->Sync Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	if (memcmp(buffer_tx[1], buffer_rx[0], BUF_SIZE)) {
		to_display_format(buffer_tx[1], BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx[0], BUF_SIZE, buffer_print_rx);
		SYS_LOG_ERR("Sync->ASync Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_async_call_half_recv(struct spi_config *spi_sync_conf,
				    struct spi_config *spi_async_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx[0],
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf tx_bufs_null[] = {
		{
			.buf = NULL,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx[0],
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs_null[] = {
		{
			.buf = NULL,
			.len = BUF_SIZE,
		},
	};
	int ret;

	SYS_LOG_INF("Half: Start Async TX");

	ret = spi_transceive_async(spi_async_conf,
				   tx_bufs, ARRAY_SIZE(tx_bufs),
				   rx_bufs_null, ARRAY_SIZE(tx_bufs_null),
				   &async_sig);
	if (ret == -ENOTSUP) {
		SYS_LOG_DBG("Async TX: Not supported");
		return 0;
	}

	if (ret) {
		SYS_LOG_ERR("Async TX Code %d", ret);
		return -1;
	}

	SYS_LOG_INF("Half: Start Sync RX");

	ret = spi_transceive(spi_sync_conf,
			     tx_bufs_null, ARRAY_SIZE(tx_bufs_null),
			     rx_bufs, ARRAY_SIZE(rx_bufs));
	if (ret) {
		SYS_LOG_ERR("Sync RX Code %d", ret);
		return ret;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result)  {
		SYS_LOG_ERR("Async TX Call code %d", result);
		return -1;
	}

	if (memcmp(buffer_tx[0], buffer_rx[0], BUF_SIZE)) {
		to_display_format(buffer_tx[0], BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx[0], BUF_SIZE, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_async_call_half_send(struct spi_config *spi_sync_conf,
				    struct spi_config *spi_async_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx[0],
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf tx_bufs_null[] = {
		{
			.buf = NULL,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx[0],
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs_null[] = {
		{
			.buf = NULL,
			.len = BUF_SIZE,
		},
	};
	int ret;

	SYS_LOG_INF("Half: Start Async RX");

	ret = spi_transceive_async(spi_async_conf,
				   tx_bufs_null, ARRAY_SIZE(tx_bufs_null),
				   rx_bufs, ARRAY_SIZE(rx_bufs), &async_sig);
	if (ret == -ENOTSUP) {
		SYS_LOG_DBG("Async RX: Not supported");
		return 0;
	}

	if (ret) {
		SYS_LOG_ERR("Async RX Code %d", ret);
		return -1;
	}

	SYS_LOG_INF("Half: Start Sync TX");

	ret = spi_transceive(spi_sync_conf,
			     tx_bufs, ARRAY_SIZE(tx_bufs),
			     rx_bufs_null, ARRAY_SIZE(rx_bufs_null));
	if (ret) {
		SYS_LOG_ERR("Sync TX Code %d", ret);
		return ret;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result)  {
		SYS_LOG_ERR("Async RX Call code %d", result);
		return -1;
	}

	if (memcmp(buffer_tx[0], buffer_rx[0], BUF_SIZE)) {
		to_display_format(buffer_tx[0], BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx[0], BUF_SIZE, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

void main(void)
{
	struct k_thread async_thread;
	k_tid_t async_thread_id;

	SYS_LOG_INF("Master/Slave SPI test on buffers TX/RX 0:%p/%p 1:%p/%p",
			buffer_tx[0], buffer_rx[0],
			buffer_tx[1], buffer_rx[1]);

#ifdef CONFIG_TEST_SPI_MASTER_CS_GPIO_ENABLE
	/* Only configure SPI CS for master */
	if (master_cs_ctrl_gpio_config(spi_master_slow.cs) ||
	    master_cs_ctrl_gpio_config(spi_master_fast.cs)) {
		return;
	}
#endif

	spi_master_slow.dev = device_get_binding(CONFIG_TEST_MASTER_DRV_NAME);
	if (!spi_master_slow.dev) {
		SYS_LOG_ERR("Cannot find master %s!\n",
			    CONFIG_TEST_MASTER_DRV_NAME);
		return;
	}

	spi_master_fast.dev = spi_master_slow.dev;

	spi_slave.dev = device_get_binding(CONFIG_TEST_SLAVE_DRV_NAME);
	if (!spi_slave.dev) {
		SYS_LOG_ERR("Cannot find slave %s!\n",
			    CONFIG_TEST_SLAVE_DRV_NAME);
		return;
	}

	async_thread_id = k_thread_create(&async_thread, spi_async_stack, 256,
					  (k_thread_entry_t)spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, 0);

	if (spi_async_call_half_send(&spi_master_slow, &spi_slave) ||
	    spi_async_full_call(&spi_master_slow, &spi_slave) ||
	    spi_async_call_half_recv(&spi_master_slow, &spi_slave) ||
	    spi_async_full_call(&spi_master_slow, &spi_slave)) {
		goto end;
	}

	if (spi_async_call_half_send(&spi_master_fast, &spi_slave) ||
	    spi_async_full_call(&spi_master_fast, &spi_slave) ||
	    spi_async_call_half_recv(&spi_master_fast, &spi_slave) ||
	    spi_async_full_call(&spi_master_fast, &spi_slave)) {
		goto end;
	}

	SYS_LOG_INF("All tx/rx passed");
end:
	k_thread_abort(async_thread_id);
}
