/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/sys/spsc_pbuf.h>

/* The buffer size itself would be 199 bytes.
 * 212 - sizeof(struct spsc_pbuf) - 1 = 199.
 * -1 because internal rd/wr_idx is reserved to mean the buffer is empty.
 */
static uint8_t memory_area[216] __aligned(4);

ZTEST(test_spsc_pbuf, test_spsc_pbuf_ut)
{
	static uint8_t rbuf[198];
	static uint8_t message[20] = {'a'};
	struct spsc_pbuf *ib;
	int rlen;
	int wlen;

	ib = spsc_pbuf_init(memory_area, sizeof(memory_area), 0);
	zassert_equal_ptr(ib, memory_area, NULL);
	zassert_equal(ib->len, (sizeof(memory_area) - sizeof(*ib)), NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	/* Try to write more than buffer can store. */
	rlen = spsc_pbuf_write(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, -ENOMEM, NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	/* Read empty buffer. */
	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, 0, NULL);

	/* Single write and read. */
	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message), NULL);
	zassert_equal(ib->wr_idx, (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message), NULL);
	zassert_equal(ib->wr_idx, (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, (sizeof(message) + sizeof(uint16_t)), NULL);

	/* Buffer size is 216 - 16 = 200 Bytes for len, wr_idx, rd_idx and flags.
	 * When writing message of 20 Bytes, actually 22 Bytes are stored,
	 * (2 Bytes reserved for message len). Test if after 9 writes, 10th write
	 * would return -ENOMEM. 200 - (9 * 22) = 2 bytes left.
	 *
	 * Reset the buffer first.
	 */
	zassert_equal(SPSC_PBUF_CAPACITY(sizeof(memory_area)), 200, NULL);

	ib = spsc_pbuf_init(memory_area, sizeof(memory_area), 0);
	zassert_equal_ptr(ib, memory_area, NULL);
	zassert_equal(ib->len, (sizeof(memory_area) - sizeof(*ib)), NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	for (size_t i = 0; i < 9; i++) {
		wlen = spsc_pbuf_write(ib, message, sizeof(message));
		zassert_equal(wlen, sizeof(message), NULL);
	}

	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, -ENOMEM, NULL);

	/* Test reading with buf == NULL, should return len of the next message to read. */
	rlen = spsc_pbuf_read(ib, NULL, 0);
	zassert_equal(rlen, sizeof(message), NULL);

	/* Read with len == 0 and correct buf pointer. */
	rlen = spsc_pbuf_read(ib, rbuf, 0);
	zassert_equal(rlen, -ENOMEM, NULL);

	/* Read whole data from the buffer. */
	for (size_t i = 0; i < 9; i++) {
		zassert_equal(ib->rd_idx, i * (sizeof(message) + sizeof(uint16_t)), NULL);
		wlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
		zassert_equal(wlen, sizeof(message), NULL);
	}

	zassert_equal(ib->wr_idx, 9 * (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, 9 * (sizeof(message) + sizeof(uint16_t)), NULL);

	/* Write message that would be wrapped around. */
	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message), NULL);

	/* Read wrapped message. */
	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message), NULL);
	zassert_equal(message[0], 'a', NULL);
}

ZTEST_SUITE(test_spsc_pbuf, NULL, NULL, NULL, NULL, NULL);
