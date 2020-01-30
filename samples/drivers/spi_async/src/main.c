/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <hal/nrf_gpio.h>
#include <drivers/counter.h>
#if CONFIG_CPU_LOAD
#include <debug/cpu_load.h>
#endif
LOG_MODULE_REGISTER(app);

static struct device *counter;
static struct spi_cs_control spi_cs = {
	.gpio_pin = 28,
	.delay = 0,
};

static struct spi_config cfg = {
	.frequency = 1000000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			SPI_MODE_CPHA | SPI_WORD_SET(8) |
			SPI_LINES_SINGLE,
	.slave = 0,
	.cs = &spi_cs,
};

static void transaction_callback(int res, void *user_data)
{
	struct k_sem *sem = user_data;
	k_sem_give(sem);
}

static void transaction_paused_handler(struct device *dev,
					struct spi_transaction *transaction,
					u8_t msg_idx)
{
	k_busy_wait(100);
	spi_resume(dev);
}

static void spi_new_paused(struct device *spi)
{
	u8_t txbuf[] = {0xff, 0, 0xff};
	u8_t rxbuf[5];

	const struct spi_msg msgs[] = {
		SPI_MSG_TX(txbuf, sizeof(txbuf), SPI_MSG_PAUSE_AFTER),
		SPI_MSG_RX(rxbuf, sizeof(rxbuf), 0)
	};

	struct spi_transaction transaction = {
		.config = &cfg,
		.msgs = msgs,
		.num_msgs = ARRAY_SIZE(msgs),
		.paused_callback = transaction_paused_handler
	};
	struct async_client client;
	int err;
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);

	async_client_init_callback(&client, transaction_callback, &sem);

	spi_set_async_client(&transaction, &client);
	err = spi_schedule(spi, &transaction);
	if (err < 0) {
		LOG_ERR("spi schedule failed (err: %d)", err);
		return;
	}
	k_sem_take(&sem, K_FOREVER);
}

static void spi_new(struct device *spi)
{
	static const u8_t txreg[1] = {0xab};
	static const u8_t txbuf[] = {1,2,3,4,5};
	u8_t rxbuf[10];

	const struct spi_msg msgs[] = {
		SPI_MSG_TXRX(txbuf, sizeof(txbuf), rxbuf, sizeof(rxbuf), 0),
		SPI_MSG_TXTX(txreg, sizeof(txreg), txbuf, sizeof(txbuf), 0),
		SPI_MSG_TX(txbuf, sizeof(txbuf), 0)
	};
	struct spi_transaction transaction = {
		.config = &cfg,
		.msgs = msgs,
		.num_msgs = ARRAY_SIZE(msgs)
	};
	struct async_client client;
	int err;
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);

	async_client_init_callback(&client, transaction_callback, &sem);

	spi_set_async_client(&transaction, &client);
	err = spi_schedule(spi, &transaction);
	if (err < 0) {
		LOG_ERR("spi schedule failed (err: %d)", err);
		return;
	}
	k_sem_take(&sem, K_FOREVER);
}

static void spi_legacy(struct device *spi)
{
	static const u8_t txreg[1] = {0xab};
	static const u8_t tx_buf[] = {1,2,3,4,5};
	u8_t rx_buf[10];
	int err;

	const struct spi_buf txbuf[] = {
		{.buf = (void *)tx_buf, .len = sizeof(tx_buf)},
		{.buf = NULL, .len = 0}
	};
	const struct spi_buf_set tx_set = {
		.buffers = (void *)txbuf,
		.count = ARRAY_SIZE(txbuf)
	};
	const struct spi_buf txreg_buf[] = {
		{.buf = (void *)txreg, .len = sizeof(txreg)},
		{.buf = (void *)tx_buf, .len = sizeof(tx_buf)}
	};
	const struct spi_buf_set txtx_set = {
		.buffers = txreg_buf,
		.count = ARRAY_SIZE(txreg_buf)
	};
	const struct spi_buf_set txonly_set = {
		.buffers = txbuf,
		.count = 1
	};
	const struct spi_buf rxbuf[] = {
		{.buf = NULL, .len = sizeof(tx_buf)},
		{.buf = rx_buf, .len = sizeof(rx_buf)},
	};
	const struct spi_buf_set rx_set = {
		.buffers = rxbuf,
		.count = ARRAY_SIZE(rxbuf)
	};

	err = spi_transceive(spi, &cfg, &tx_set, &rx_set);
	if (err < 0) {
		LOG_ERR("spi transceive failed (err: %d)", err);
		return;
	}
	err = spi_transceive(spi, &cfg, &txtx_set, NULL);
	if (err < 0) {
		LOG_ERR("spi transceive failed (err: %d)", err);
		return;
	}
	err = spi_transceive(spi, &cfg, &txonly_set, NULL);
	if (err < 0) {
		LOG_ERR("spi transceive failed (err: %d)", err);
		return;
	}
}

static void cyccnt_init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	DWT->CYCCNT = 0;
}
typedef void (*test_func_t)(struct device *dev);

static void profile_func(struct device *dev, test_func_t func, u32_t rpt)
{
#if CONFIG_CPU_LOAD
	cpu_load_reset();
#endif
	u32_t cyccnt = DWT->CYCCNT;
	u32_t cnt =counter_read(counter);
	nrf_gpio_pin_set(27);
	for (int i = 0; i < rpt; i++) {
		func(dev);
	}
	nrf_gpio_pin_clear(27);
	cnt = counter_read(counter) - cnt;
	cyccnt = DWT->CYCCNT - cyccnt;
#if CONFIG_CPU_LOAD
	u32_t load = cpu_load_get();
	printk("cpu_load %d.%d%% ", load/1000, load%1000);
#endif
	printk("total time: %d us, cpu time: %d us\n\n", cnt/(rpt*16), cyccnt/(rpt*64));
}

void main(void)
{
	int err;

	cyccnt_init();
	nrf_gpio_cfg_output(27);
#if CONFIG_CPU_LOAD
	err = cpu_load_init();
	if (err < 0) {
		LOG_ERR("cpu_load init failed (err:%d)", err);
	}
#endif
	struct device *spi = device_get_binding("SPI_1");
	if (spi == NULL) {
		LOG_ERR("spi null");
		return;
	}
	spi_cs.gpio_dev = device_get_binding("GPIO_0");
	if (spi_cs.gpio_dev == NULL) {
		LOG_ERR("gpio null");
		return;
	}

	counter = device_get_binding(DT_ALIAS_TIMER_0_LABEL);
	if (counter == NULL) {
		LOG_ERR("counter null");
		return;
	}
	counter_start(counter);

	printk("legacy api\n");
	profile_func(spi, spi_legacy, 10);

	if (IS_ENABLED(CONFIG_SPI_USE_MNGR)) {
		printk("new api\n");
		profile_func(spi, spi_new, 10);

		spi_new_paused(spi);
	}
}
