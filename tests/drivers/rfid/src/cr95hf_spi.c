/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/drivers/rfid.h>

#define CR95HF_SND_BUF_SIZE 50
#define CR95HF_RCV_BUF_SIZE 258

struct rfid_cr95hf_data {
	enum rfid_mode current_mode;
	uint64_t cm_timestamp;
	struct gpio_callback irq_callback;
	struct spi_buf spi_snd_buffer;
	struct spi_buf spi_rcv_buffer;
	struct spi_buf_set spi_snd_buffer_arr;
	struct spi_buf_set spi_rcv_buffer_arr;
	uint8_t rcv_buffer[CR95HF_RCV_BUF_SIZE];
	uint8_t snd_buffer[CR95HF_SND_BUF_SIZE];
	struct k_sem irq_out_sem;
	gpio_callback_handler_t cb_handler;
};

static const struct device *rfid_dev;

ZTEST(cr95hf_test, test_cr95hf_read_uid)
{
	rfid_dev = DEVICE_DT_GET(DT_ALIAS(rfid));
	zassert_true(device_is_ready(rfid_dev), "CR95HF device not ready/init failed");

	rfid_select_mode(rfid_dev, TAG_DETECTOR);
	struct rfid_cr95hf_data *data = rfid_dev->data;
	zassert_equal(data->current_mode, READY, "Current Mode is not READY as expected");

	rfid_protocol_select(rfid_dev, ISO_14443A);
	zassert_equal(data->current_mode, READER, "Current Mode is not READER as expected");

	uint8_t uid[10];
	size_t uid_len = 10;
	uint8_t expected_uid[4] = { 0x08, 0x19, 0x2D, 0xA2};
	rfid_get_uid(rfid_dev, uid, &uid_len);
	zassert_equal(uid_len,4,"Length of TAG UID is not 4 as expected");
	zassert_mem_equal(uid,expected_uid,4,"Received TAG ID is not the expected 0x192DA29E");
}

ZTEST_SUITE(cr95hf_test, NULL, NULL, NULL, NULL, NULL);
