/*
 * Copyright (c) 2017 Intel Corporation.
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

#define SPI_CS &spi_cs

#if defined(CONFIG_SOC_QUARK_SE_C1000)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 1
#define CS_CTRL_GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME

struct spi_cs_control spi_cs = {
	.gpio_pin = 25,
	.delay = 0
};

#elif defined(CONFIG_SOC_QUARK_SE_C1000_SS)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 0
#define CS_CTRL_GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME

struct spi_cs_control spi_cs = {
	.gpio_pin = 0,
	.delay = 0
};

#elif defined(CONFIG_SOC_EM7D) || defined(CONFIG_SOC_EM9D)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 0

#undef SPI_CS
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""

#elif defined(CONFIG_BOARD_NUCLEO_L432KC) || 	\
      defined(CONFIG_BOARD_DISCO_L475_IOT1) || 	\
      defined(CONFIG_BOARD_NUCLEO_F334R8) || 	\
      defined(CONFIG_BOARD_NUCLEO_F401RE) || 	\
      defined(CONFIG_BOARD_NUCLEO_L476RG)

#define SPI_DRV_NAME CONFIG_SPI_1_NAME
#define SPI_SLAVE 0
#define MIN_FREQ 500000

#undef SPI_CS
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""

#elif defined(CONFIG_BOARD_96B_CARBON)

#define SPI_DRV_NAME CONFIG_SPI_2_NAME
#define SPI_SLAVE 0
#define MIN_FREQ 500000

#define CS_CTRL_GPIO_DRV_NAME "GPIOC"

struct spi_cs_control spi_cs = {
	.gpio_pin = 3,
	.delay = 0,
};

#else
#undef SPI_CS
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""
#endif

#define BUF_SIZE 17
u8_t buffer_tx[] = "0123456789abcdef\0";
u8_t buffer_rx[BUF_SIZE] = {};

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

struct spi_config spi_slow = {
#if defined(MIN_FREQ)
	.frequency = MIN_FREQ,
#else
	.frequency = 128000,
#endif
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

struct spi_config spi_fast = {
	.frequency = 16000000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

static int cs_ctrl_gpio_config(struct spi_cs_control *cs)
{
	if (cs) {
		cs->gpio_dev = device_get_binding(CS_CTRL_GPIO_DRV_NAME);
		if (!cs->gpio_dev) {
			SYS_LOG_ERR("Cannot find %s!\n",
				    CS_CTRL_GPIO_DRV_NAME);
			return -1;
		}
	}

	return 0;
}

static int spi_complete_loop(struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	int ret;

	SYS_LOG_INF("Start");

	ret = spi_transceive(spi_conf, tx_bufs, ARRAY_SIZE(tx_bufs),
			     rx_bufs, ARRAY_SIZE(rx_bufs));
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return ret;
	}

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_start(struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = 8,
		},
	};
	int ret;

	SYS_LOG_INF("Start");

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_conf, tx_bufs, ARRAY_SIZE(tx_bufs),
			     rx_bufs, ARRAY_SIZE(rx_bufs));
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx, buffer_rx, 8)) {
		to_display_format(buffer_tx, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_rx_half_end(struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = 8,
		},
		{
			.buf = buffer_rx,
			.len = 8,
		},
	};
	int ret;

	SYS_LOG_INF("Start");

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_conf, tx_bufs, ARRAY_SIZE(tx_bufs),
			     rx_bufs, ARRAY_SIZE(rx_bufs));
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx+8, buffer_rx, 8)) {
		to_display_format(buffer_tx + 8, 8, buffer_print_tx);
		to_display_format(buffer_rx, 8, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_rx_every_4(struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
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
	int ret;

	SYS_LOG_INF("Start");

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_conf, tx_bufs, ARRAY_SIZE(tx_bufs),
			     rx_bufs, ARRAY_SIZE(rx_bufs));
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx + 4, buffer_rx, 4)) {
		to_display_format(buffer_tx + 4, 4, buffer_print_tx);
		to_display_format(buffer_rx, 4, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	} else if (memcmp(buffer_tx + 12, buffer_rx + 4, 4)) {
		to_display_format(buffer_tx + 12, 4, buffer_print_tx);
		to_display_format(buffer_rx + 4, 4, buffer_print_rx);
		SYS_LOG_ERR("Buffer contents are different: %s",
			    buffer_print_tx);
		SYS_LOG_ERR("                           vs: %s",
			    buffer_print_rx);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

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

static int spi_async_call(struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	int ret;

	SYS_LOG_INF("Start");

	ret = spi_transceive_async(spi_conf, tx_bufs, ARRAY_SIZE(tx_bufs),
				   rx_bufs, ARRAY_SIZE(rx_bufs), &async_sig);
	if (ret == -ENOTSUP) {
		SYS_LOG_DBG("Not supported");
		return 0;
	}

	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	k_sem_take(&caller, K_FOREVER);

	if (result)  {
		SYS_LOG_ERR("Call code %d", ret);
		return -1;
	}

	SYS_LOG_INF("Passed");

	return 0;
}

static int spi_resource_lock_test(struct spi_config *spi_conf_lock,
				   struct spi_config *spi_conf_try)
{
	spi_conf_lock->operation |= SPI_LOCK_ON;

	if (spi_complete_loop(spi_conf_lock)) {
		return -1;
	}

	if (spi_release(spi_conf_lock)) {
		SYS_LOG_ERR("Deadlock now?");
		return -1;
	}

	if (spi_complete_loop(spi_conf_try)) {
		return -1;
	}

	return 0;
}

void main(void)
{
	struct k_thread async_thread;
	k_tid_t async_thread_id;

	SYS_LOG_INF("SPI test on buffers TX/RX %p/%p", buffer_tx, buffer_rx);

	if (cs_ctrl_gpio_config(spi_slow.cs) ||
	    cs_ctrl_gpio_config(spi_fast.cs)) {
		return;
	}

	spi_slow.dev = device_get_binding(SPI_DRV_NAME);
	if (!spi_slow.dev) {
		SYS_LOG_ERR("Cannot find %s!\n", SPI_DRV_NAME);
		return;
	}

	spi_fast.dev = spi_slow.dev;

	async_thread_id = k_thread_create(&async_thread, spi_async_stack, 256,
					  (k_thread_entry_t)spi_async_call_cb,
					  &async_evt, &caller, NULL,
					  K_PRIO_COOP(7), 0, 0);

	if (spi_complete_loop(&spi_slow) ||
	    spi_rx_half_start(&spi_slow) ||
	    spi_rx_half_end(&spi_slow) ||
	    spi_rx_every_4(&spi_slow) ||
	    spi_async_call(&spi_slow)) {
		goto end;
	}

	if (spi_complete_loop(&spi_fast) ||
	    spi_rx_half_start(&spi_fast) ||
	    spi_rx_half_end(&spi_fast) ||
	    spi_rx_every_4(&spi_fast) ||
	    spi_async_call(&spi_fast)) {
		goto end;
	}

	if (spi_resource_lock_test(&spi_slow, &spi_fast)) {
		goto end;
	}

	SYS_LOG_INF("All tx/rx passed");
end:
	k_thread_abort(async_thread_id);
}
