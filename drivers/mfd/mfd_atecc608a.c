/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atecc608a

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/atecc608a.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "mfd_atecc608a.h"

LOG_MODULE_REGISTER(mfd_atecc608a, CONFIG_MFD_LOG_LEVEL);

/* tWHI after a wake token, ATECC608A datasheet (1500 us minimum). */
#define ATECC608A_TWHI_US 1500U
#define ATECC608A_WAKE_RETRIES 3U
#define ATECC608A_POLL_MS 2U

#define ATECC608A_PKT_MAX 40U
#define ATECC608A_TX_DATA_MAX 32U

/* CRC-16 polynomial 0x8005, initial value 0, ATECC608A datasheet. */
#define ATECC608A_CRC_POLY 0x8005U

struct mfd_atecc608a_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset;
};

struct mfd_atecc608a_data {
	struct k_sem lock;
	uint8_t pkt[ATECC608A_PKT_MAX] __aligned(4);
	uint8_t rsp[ATECC608A_PKT_MAX] __aligned(4);
};

static uint16_t atecc608a_crc16(const uint8_t *buf, size_t len)
{
	uint16_t crc = 0U;

	for (size_t i = 0U; i < len; i++) {
		uint8_t shift;

		for (shift = 0x01U; shift != 0U; shift <<= 1) {
			uint8_t data_bit = (buf[i] & shift) ? 1U : 0U;
			uint8_t crc_bit = (crc >> 15) & 0x01U;

			crc <<= 1;
			if (data_bit != crc_bit) {
				crc ^= ATECC608A_CRC_POLY;
			}
		}
	}

	return crc;
}

static int atecc608a_idle(const struct mfd_atecc608a_config *cfg)
{
	uint8_t word = ATECC608A_WA_IDLE;

	return i2c_write_dt(&cfg->i2c, &word, sizeof(word));
}

static int atecc608a_wake(const struct mfd_atecc608a_config *cfg)
{
	uint8_t token = 0x00U;
	uint8_t status[4];
	int ret;

	for (uint8_t i = 0U; i < ATECC608A_WAKE_RETRIES; i++) {
		/*
		 * A write to address 0x00 holds SDA low long enough to form a
		 * wake token. The device NACKs this transfer.
		 */
		(void)i2c_write(cfg->i2c.bus, &token, sizeof(token), 0x00);
		k_busy_wait(ATECC608A_TWHI_US);

		ret = i2c_read_dt(&cfg->i2c, status, sizeof(status));
		if (ret == 0) {
			return 0;
		}

		k_msleep(1);
	}

	return -EIO;
}

static int atecc608a_wait_rsp(const struct mfd_atecc608a_config *cfg, uint8_t *rx, size_t want,
			      k_timeout_t min_wait, k_timeout_t max_wait)
{
	k_timepoint_t end = sys_timepoint_calc(max_wait);
	int ret = -ETIMEDOUT;

	if (want < 4U || want > ATECC608A_PKT_MAX) {
		return -EINVAL;
	}

	if (!K_TIMEOUT_EQ(min_wait, K_NO_WAIT)) {
		k_sleep(min_wait);
	}

	memset(rx, 0, ATECC608A_PKT_MAX);

	/*
	 * Read the exact group length into a word-aligned buffer. Some I2C
	 * hosts drop unread bytes on NACK or require aligned RX buffers.
	 */
	do {
		ret = i2c_read_dt(&cfg->i2c, rx, want);
		if (ret == 0) {
			return 0;
		}

		k_msleep(ATECC608A_POLL_MS);
	} while (!sys_timepoint_expired(end));

	return -ETIMEDOUT;
}

static int atecc608a_do_execute(const struct device *dev, uint8_t opcode, uint8_t param1,
				uint16_t param2, const uint8_t *tx, size_t tx_len, uint8_t *rx,
				size_t rx_len, k_timeout_t min_wait, k_timeout_t max_wait)
{
	const struct mfd_atecc608a_config *cfg = dev->config;
	struct mfd_atecc608a_data *data = dev->data;
	uint8_t *pkt = data->pkt;
	uint8_t *rsp = data->rsp;
	uint16_t crc;
	uint8_t count;
	size_t payload;
	size_t want;
	int ret;

	if ((tx_len > 0U && tx == NULL) || tx_len > ATECC608A_TX_DATA_MAX) {
		return -EINVAL;
	}

	count = (uint8_t)(7U + tx_len);
	if ((size_t)count + 1U > ATECC608A_PKT_MAX) {
		return -EINVAL;
	}

	ret = atecc608a_wake(cfg);
	if (ret < 0) {
		LOG_ERR("wake failed (%d)", ret);
		return ret;
	}

	pkt[0] = ATECC608A_WA_COMMAND;
	pkt[1] = count;
	pkt[2] = opcode;
	pkt[3] = param1;
	pkt[4] = (uint8_t)(param2 & 0xffU);
	pkt[5] = (uint8_t)(param2 >> 8);
	if (tx_len > 0U) {
		memcpy(&pkt[6], tx, tx_len);
	}

	crc = atecc608a_crc16(&pkt[1], (size_t)count - 2U);
	pkt[count - 1U] = (uint8_t)(crc & 0xffU);
	pkt[count] = (uint8_t)(crc >> 8);

	ret = i2c_write_dt(&cfg->i2c, pkt, (size_t)count + 1U);
	if (ret < 0) {
		LOG_ERR("command write failed (%d)", ret);
		(void)atecc608a_idle(cfg);
		return ret;
	}

	want = (rx_len == 0U) ? 4U : (rx_len + 3U);
	ret = atecc608a_wait_rsp(cfg, rsp, want, min_wait, max_wait);
	if (ret < 0) {
		LOG_ERR("command 0x%02x timed out (want %u)", opcode, (unsigned int)want);
		(void)atecc608a_idle(cfg);
		return ret;
	}

	if (rsp[0] < 4U || rsp[0] > ATECC608A_PKT_MAX) {
		LOG_ERR("invalid response count %u", rsp[0]);
		(void)atecc608a_idle(cfg);
		return -EIO;
	}

	crc = atecc608a_crc16(rsp, (size_t)rsp[0] - 2U);
	if (rsp[rsp[0] - 2U] != (uint8_t)(crc & 0xffU) ||
	    rsp[rsp[0] - 1U] != (uint8_t)(crc >> 8)) {
		LOG_ERR("response CRC mismatch (count %u)", rsp[0]);
		LOG_HEXDUMP_ERR(rsp, rsp[0], "rsp");
		(void)atecc608a_idle(cfg);
		return -EIO;
	}

	if (rsp[0] == 4U) {
		if (rsp[1] != ATECC608A_STATUS_SUCCESS) {
			LOG_ERR("command 0x%02x status 0x%02x", opcode, rsp[1]);
			(void)atecc608a_idle(cfg);
			return -EIO;
		}

		payload = 0U;
	} else {
		payload = (size_t)rsp[0] - 3U;
	}

	if (payload != rx_len) {
		LOG_ERR("command 0x%02x payload %u, expected %u (count %u status 0x%02x)", opcode,
			(unsigned int)payload, (unsigned int)rx_len, rsp[0], rsp[1]);
		(void)atecc608a_idle(cfg);
		return -EIO;
	}

	if (payload > 0U) {
		if (rx == NULL) {
			(void)atecc608a_idle(cfg);
			return -EINVAL;
		}

		memcpy(rx, &rsp[1], payload);
	}

	(void)atecc608a_idle(cfg);

	return 0;
}

int mfd_atecc608a_execute(const struct device *dev, uint8_t opcode, uint8_t param1,
			  uint16_t param2, const uint8_t *tx, size_t tx_len, uint8_t *rx,
			  size_t rx_len, k_timeout_t min_wait, k_timeout_t max_wait)
{
	struct mfd_atecc608a_data *data = dev->data;
	int ret;

	(void)k_sem_take(&data->lock, K_FOREVER);
	ret = atecc608a_do_execute(dev, opcode, param1, param2, tx, tx_len, rx, rx_len, min_wait,
				   max_wait);
	k_sem_give(&data->lock);

	return ret;
}

static int mfd_atecc608a_init(const struct device *dev)
{
	const struct mfd_atecc608a_config *cfg = dev->config;
	struct mfd_atecc608a_data *data = dev->data;
	uint8_t info[ATECC608A_INFO_LEN];
	int ret;

	k_sem_init(&data->lock, 1, 1);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->reset.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			return ret;
		}

		k_msleep(1);
		ret = gpio_pin_set_dt(&cfg->reset, 0);
		if (ret < 0) {
			return ret;
		}

		k_msleep(2);
	}

	ret = mfd_atecc608a_execute(dev, ATECC608A_OP_INFO, ATECC608A_INFO_REVISION, 0U, NULL, 0U,
				    info, sizeof(info), K_MSEC(2), K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("Info(Revision) failed (%d)", ret);
		return ret;
	}

	if (info[2] != ATECC608A_DEVTYPE_608) {
		LOG_ERR("unexpected device type 0x%02x%02x%02x%02x", info[0], info[1], info[2],
			info[3]);
		return -ENODEV;
	}

	LOG_INF("ATECC608 present, revision 0x%02x%02x%02x%02x", info[0], info[1], info[2],
		info[3]);

	return 0;
}

#define MFD_ATECC608A_DEFINE(inst)                                                                 \
	static const struct mfd_atecc608a_config config##inst = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
	};                                                                                         \
	static struct mfd_atecc608a_data data##inst;                                               \
	DEVICE_DT_INST_DEFINE(inst, mfd_atecc608a_init, NULL, &data##inst, &config##inst,          \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_ATECC608A_DEFINE)
