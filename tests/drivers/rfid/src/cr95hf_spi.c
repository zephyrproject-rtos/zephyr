/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/drivers/rfid.h>
#include <zephyr/rfid/iso14443.h>

#define CR95HF_SND_BUF_SIZE 50
#define CR95HF_RCV_BUF_SIZE 258

struct rfid_cr95hf_data {
	struct gpio_callback irq_callback;
	struct spi_buf spi_snd_buffer;
	struct spi_buf spi_rcv_buffer;
	struct spi_buf_set spi_snd_buffer_arr;
	struct spi_buf_set spi_rcv_buffer_arr;
	uint8_t rcv_buffer[CR95HF_RCV_BUF_SIZE];
	uint8_t snd_buffer[CR95HF_SND_BUF_SIZE];
	struct k_sem irq_out_sem;
	gpio_callback_handler_t cb_handler;
	struct k_mutex data_mutex;
	bool hw_tx_crc;
	uint32_t timeout_us;
};

static const struct device *rfid_dev;
struct rfid_iso14443a_info info = {0};

ZTEST(cr95hf_test, test_cr95hf_read_uid)
{
	rfid_dev = DEVICE_DT_GET(DT_NODELABEL(cr95hf));
	zassert_true(device_is_ready(rfid_dev), "CR95HF device not ready/init failed");

	zassert_equal(rfid_load_protocol(rfid_dev, RFID_PROTO_ISO14443A,
					 RFID_MODE_INITIATOR | RFID_MODE_TX_106 | RFID_MODE_RX_106),
		      0, "Failed to load protocol");

	zassert_equal(rfid_iso14443a_request(rfid_dev, info.atqa, true), 0,
		      "Failed to request ATQA");

	zassert_equal(rfid_iso14443a_sdd(rfid_dev, &info), 0, "Failed to select device");

	uint8_t expected_uid[4] = {0x08, 0x19, 0x2D, 0xA2};

	zassert_equal(info.uid_len, 4, "Length of TAG UID is not 4 as expected");
	zassert_mem_equal(info.uid, expected_uid, 4,
			  "Received TAG ID is not the expected 0x192DA29E");
}

ZTEST_SUITE(cr95hf_test, NULL, NULL, NULL, NULL, NULL);
