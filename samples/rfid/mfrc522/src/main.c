/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rfid/mfrc522.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

LOG_MODULE_REGISTER(mfrc522_sample, CONFIG_RFID_LOG_LEVEL);

#define PICC_CMD_REQA    0x26
#define PICC_CMD_SEL_CL1 0x93

#define PICC_ANTICOLL 0x20
#define PICC_SELECT   0x70

#define POLL_FREQ_MS 100

#define UID_MAX_LEN 10

static const struct device *rfid_dev = DEVICE_DT_GET(DT_ALIAS(rfid));

static int detect_tag(const struct device *dev, uint16_t *atq)
{
	NET_BUF_SIMPLE_DEFINE(tx_buf, 1);
	NET_BUF_SIMPLE_DEFINE(rx_buf, 2);
	int ret;

	net_buf_simple_add_u8(&tx_buf, PICC_CMD_REQA);

	LOG_HEXDUMP_DBG(tx_buf.data, tx_buf.len, "-> TX");

	ret = mfrc522_transceive(dev, tx_buf.data, tx_buf.len, rx_buf.data, rx_buf.size, 7);
	if (ret < 0) {
		return ret;
	}
	rx_buf.len = ret;

	LOG_HEXDUMP_DBG(rx_buf.data, rx_buf.len, "<- RX");

	*atq = (rx_buf.data[1] << 4U) | rx_buf.data[0];

	return 0;
}

static int read_uid(const struct device *dev, uint8_t *uid, uint8_t *uid_len)
{
	NET_BUF_SIMPLE_DEFINE(tx_buf, 2);
	NET_BUF_SIMPLE_DEFINE(rx_buf, 5);
	int ret;

	net_buf_simple_add_u8(&tx_buf, PICC_CMD_SEL_CL1);
	net_buf_simple_add_u8(&tx_buf, PICC_ANTICOLL);

	LOG_HEXDUMP_DBG(tx_buf.data, tx_buf.len, "-> TX");

	ret = mfrc522_transceive(dev, tx_buf.data, tx_buf.len, rx_buf.data, rx_buf.size, 0);
	if (ret < 0) {
		return ret;
	}
	rx_buf.len = ret;

	LOG_HEXDUMP_DBG(rx_buf.data, rx_buf.len, "<- RX");

	if (rx_buf.len > UID_MAX_LEN) {
		LOG_ERR("rx length %d overflows UID max length %d", rx_buf.len, UID_MAX_LEN);
		return -ENOBUFS;
	}

	memcpy(uid, rx_buf.data, rx_buf.len);
	*uid_len = rx_buf.len;

	return 0;
}

int main(void)
{
	uint8_t uid[UID_MAX_LEN];
	uint8_t uid_len;
	uint16_t atq;
	int ret;

	if (!device_is_ready(rfid_dev)) {
		LOG_ERR("mfrc522 dev is not ready");
		return -ENODEV;
	}

	mfrc522_enable(rfid_dev, true);

	LOG_INF("starting poll for rfid tags");
	while (true) {
		ret = detect_tag(rfid_dev, &atq);
		if (ret < 0) {
			/* no tag, keep polling */
			goto poll_next;
		}

		ret = read_uid(rfid_dev, uid, &uid_len);
		if (ret < 0) {
			LOG_ERR("failed to read uid");
			goto poll_next;
		}

		LOG_HEXDUMP_INF(uid, uid_len, "UID");

poll_next:
		k_msleep(POLL_FREQ_MS);
	}

	return 0;
}
