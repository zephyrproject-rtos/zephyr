/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/nfc.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

LOG_MODULE_REGISTER(rc522_sample);

#define PICC_CMD_REQA    0x26
#define PICC_CMD_SEL_CL1 0x93

#define PICC_ANTICOLL 0x20
#define PICC_SELECT   0x70

#define POLL_FREQ_MS 250

static const struct device *nfc_dev = DEVICE_DT_GET(DT_ALIAS(nfc));

static int detect_tag(const struct device *dev, struct nfc_target *tag)
{
	NET_BUF_SIMPLE_DEFINE(tx_buf, 1);
	NET_BUF_SIMPLE_DEFINE(rx_buf, 2);
	int ret;

	net_buf_simple_add_u8(&tx_buf, PICC_CMD_REQA);

	struct nfc_comm comm = {.tx_buf = &tx_buf, .rx_buf = &rx_buf, .valid_bits = 7};

	LOG_HEXDUMP_DBG(comm.tx_buf->data, comm.tx_buf->len, "-> TX");

	ret = nfc_transceive(dev, &comm);
	if (ret < 0) {
		return ret;
	}

	LOG_HEXDUMP_DBG(comm.rx_buf->data, comm.rx_buf->len, "<- RX");

	tag->atq = (rx_buf.data[1] << 4U) | rx_buf.data[0];

	return 0;
}

static int read_uid(const struct device *dev, struct nfc_target *tag)
{
	NET_BUF_SIMPLE_DEFINE(tx_buf, 2);
	NET_BUF_SIMPLE_DEFINE(rx_buf, 5);
	int ret;

	net_buf_simple_add_u8(&tx_buf, PICC_CMD_SEL_CL1);
	net_buf_simple_add_u8(&tx_buf, PICC_ANTICOLL);

	struct nfc_comm comm = {.tx_buf = &tx_buf, .rx_buf = &rx_buf};

	LOG_HEXDUMP_DBG(comm.tx_buf->data, comm.tx_buf->len, "-> TX");

	ret = nfc_transceive(dev, &comm);
	if (ret < 0) {
		return ret;
	}

	LOG_HEXDUMP_DBG(comm.rx_buf->data, comm.rx_buf->len, "<- RX");

	memcpy(tag->nfc_id, rx_buf.data, rx_buf.len);
	tag->nfc_id_len = rx_buf.len;

	return 0;
}

int main(void)
{
	struct nfc_target tag;
	int ret;

	if (!device_is_ready(nfc_dev)) {
		LOG_ERR("nfc phy dev is not ready");
		return -ENODEV;
	}

	nfc_enable(nfc_dev, true);

	LOG_INF("starting poll for nfc tags");
	while (true) {
		ret = detect_tag(nfc_dev, &tag);
		if (ret < 0) {
			goto poll_next;
		}

		ret = read_uid(nfc_dev, &tag);
		if (ret < 0) {
			LOG_ERR("failed to read UID");
			goto poll_next;
		}

		LOG_HEXDUMP_INF(tag.nfc_id, tag.nfc_id_len, "UID");

poll_next:
		k_msleep(POLL_FREQ_MS);
	}

	return 0;
}
