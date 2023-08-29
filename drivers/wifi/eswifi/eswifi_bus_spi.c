/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT inventek_eswifi
#include "eswifi_log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "eswifi.h"

#define ESWIFI_SPI_THREAD_STACK_SIZE 1024
K_KERNEL_STACK_MEMBER(eswifi_spi_poll_stack, ESWIFI_SPI_THREAD_STACK_SIZE);

#define SPI_READ_CHUNK_SIZE 32

struct eswifi_spi_config {
	struct gpio_dt_spec dr;
	struct spi_dt_spec bus;
};

struct eswifi_spi_data {
	const struct eswifi_spi_config *cfg;
	struct k_thread poll_thread;
};

static const struct eswifi_spi_config eswifi_config_spi0 = {
	.dr = GPIO_DT_SPEC_INST_GET(0, data_gpios),
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |
				    SPI_WORD_SET(16) | SPI_HOLD_ON_CS |
				    SPI_LOCK_ON, 1000U),
};

static struct eswifi_spi_data eswifi_spi0;

static bool eswifi_spi_cmddata_ready(struct eswifi_spi_data *spi)
{
	return gpio_pin_get_dt(&spi->cfg->dr) > 0;
}

static int eswifi_spi_wait_cmddata_ready(struct eswifi_spi_data *spi)
{
	unsigned int max_retries = 60 * 1000; /* 1 minute */

	do {
		/* allow other threads to be scheduled */
		k_sleep(K_MSEC(1));
	} while (!eswifi_spi_cmddata_ready(spi) && --max_retries);

	return max_retries ? 0 : -ETIMEDOUT;
}

static int eswifi_spi_write(struct eswifi_dev *eswifi, char *data, size_t dlen)
{
	struct eswifi_spi_data *spi = eswifi->bus_data;
	struct spi_buf spi_tx_buf[1];
	struct spi_buf_set spi_tx;
	int status;

	spi_tx_buf[0].buf = data;
	spi_tx_buf[0].len = dlen;
	spi_tx.buffers = spi_tx_buf;
	spi_tx.count = ARRAY_SIZE(spi_tx_buf);

	status = spi_write_dt(&spi->cfg->bus, &spi_tx);
	if (status) {
		LOG_ERR("SPI write error %d", status);
	} else {
		status = dlen;
	}

	return status;
}

static int eswifi_spi_read(struct eswifi_dev *eswifi, char *data, size_t dlen)
{
	struct eswifi_spi_data *spi = eswifi->bus_data;
	struct spi_buf spi_rx_buf[1];
	struct spi_buf_set spi_rx;
	int status;

	spi_rx_buf[0].buf = data;
	spi_rx_buf[0].len = dlen;
	spi_rx.buffers = spi_rx_buf;
	spi_rx.count = ARRAY_SIZE(spi_rx_buf);

	status = spi_read_dt(&spi->cfg->bus, &spi_rx);
	if (status) {
		LOG_ERR("SPI read error %d", status);
	} else {
		status = dlen;
	}

	return status;
}

static int eswifi_spi_request(struct eswifi_dev *eswifi, char *cmd, size_t clen,
			      char *rsp, size_t rlen)
{
	struct eswifi_spi_data *spi = eswifi->bus_data;
	unsigned int offset = 0U, to_read = SPI_READ_CHUNK_SIZE;
	char tmp[2];
	int err;

	LOG_DBG("cmd=%p (%u byte), rsp=%p (%u byte)", cmd, clen, rsp, rlen);

	/*
	 * CMD/DATA protocol:
	 * 1. Module raises data-ready when ready for **command phase**
	 * 2. Host announces command start by lowering chip-select
	 * 3. Host write the command (possibly several spi transfers)
	 * 4. Host announces end of command by raising chip-select
	 * 5. Module lowers data-ready signal
	 * 6. Module raises data-ready to signal start of the **data phase**
	 * 7. Host lowers chip-select
	 * 8. Host fetch data as long as data-ready pin is up
	 * 9. Module lowers data-ready to signal the end of the data Phase
	 * 10. Host raises chip-select
	 *
	 * Note:
	 * All commands to the eS-WiFi module must be post-padded with
	 * 0x0A (Line Feed) to an even number of bytes.
	 * All data from eS-WiFi module are post-padded with 0x15(NAK) to an
	 * even number of bytes.
	 */

	if (!cmd) {
		goto data;
	}

	/* CMD/DATA READY signals the Command Phase */
	err = eswifi_spi_wait_cmddata_ready(spi);
	if (err) {
		LOG_ERR("CMD ready timeout\n");
		return err;
	}

	if (clen % 2) { /* Add post-padding if necessary */
		/* cmd is a string so cmd[clen] is 0x00 */
		cmd[clen] = 0x0a;
		clen++;
	}

	eswifi_spi_write(eswifi, cmd, clen);

	/* Our device is flagged with SPI_HOLD_ON_CS|SPI_LOCK_ON, release */
	spi_release_dt(&spi->cfg->bus);

data:
	/* CMD/DATA READY signals the Data Phase */
	err = eswifi_spi_wait_cmddata_ready(spi);
	if (err) {
		LOG_ERR("DATA ready timeout\n");
		return err;
	}

	while (eswifi_spi_cmddata_ready(spi) && to_read) {
		to_read = MIN(rlen - offset, to_read);
		memset(rsp + offset, 0, to_read);
		eswifi_spi_read(eswifi, rsp + offset, to_read);
		offset += to_read;
		k_yield();
	}

	/* Flush remaining data if receiving buffer not large enough */
	while (eswifi_spi_cmddata_ready(spi)) {
		eswifi_spi_read(eswifi, tmp, 2);
		k_sleep(K_MSEC(1));
	}

	/* Our device is flagged with SPI_HOLD_ON_CS|SPI_LOCK_ON, release */
	spi_release_dt(&spi->cfg->bus);

	LOG_DBG("success");

	return offset;
}

static void eswifi_spi_read_msg(struct eswifi_dev *eswifi)
{
	const char startstr[] = "[SOMA]";
	const char endstr[] = "[EOMA]";
	char cmd[] = "MR\r";
	size_t msg_len;
	char *rsp;
	int ret;

	LOG_DBG("");

	eswifi_lock(eswifi);

	ret = eswifi_at_cmd_rsp(eswifi, cmd, &rsp);
	if (ret < 0) {
		LOG_ERR("Unable to read msg %d", ret);
		eswifi_unlock(eswifi);
		return;
	}

	if (strncmp(rsp, startstr, sizeof(endstr) - 1)) {
		LOG_ERR("Malformed async msg");
		eswifi_unlock(eswifi);
		return;
	}

	/* \r\n[SOMA]...[EOMA]\r\nOK\r\n> */
	msg_len = ret - (sizeof(startstr) - 1) - (sizeof(endstr) - 1);
	if (msg_len > 0) {
		eswifi_async_msg(eswifi, rsp + sizeof(endstr) - 1, msg_len);
	}

	eswifi_unlock(eswifi);
}

static void eswifi_spi_poll_thread(void *p1)
{
	struct eswifi_dev *eswifi = p1;

	while (1) {
		k_sleep(K_MSEC(1000));
		eswifi_spi_read_msg(eswifi);
	}
}

int eswifi_spi_init(struct eswifi_dev *eswifi)
{
	struct eswifi_spi_data *spi = &eswifi_spi0; /* Static instance */
	const struct eswifi_spi_config *cfg = &eswifi_config_spi0; /* Static instance */

	/* SPI DATA READY PIN */
	if (!gpio_is_ready_dt(&cfg->dr)) {
		LOG_ERR("device %s is not ready", cfg->dr.port->name);
		return -ENODEV;
	}
	gpio_pin_configure_dt(&cfg->dr, GPIO_INPUT);

	/* SPI BUS */
	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	};

	spi->cfg = cfg;

	eswifi->bus_data = spi;

	LOG_DBG("success");

	k_thread_create(&spi->poll_thread, eswifi_spi_poll_stack,
			ESWIFI_SPI_THREAD_STACK_SIZE,
			(k_thread_entry_t)eswifi_spi_poll_thread, eswifi, NULL,
			NULL, K_PRIO_COOP(CONFIG_WIFI_ESWIFI_THREAD_PRIO), 0,
			K_NO_WAIT);

	return 0;
}

static struct eswifi_bus_ops eswifi_bus_ops_spi = {
	.init = eswifi_spi_init,
	.request = eswifi_spi_request,
};

struct eswifi_bus_ops *eswifi_get_bus(void)
{
	return &eswifi_bus_ops_spi;
}
