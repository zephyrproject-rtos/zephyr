/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ipc/ipc_icmsg_buf.h>

/* The buffer size itself would be 199 bytes.
 * 212 - sizeof(struct icmsg_buf) - 1 = 199.
 * -1 because internal rd/wr_idx is reserved to mean the buffer is empty.
 */
static uint8_t memory_area[212] __aligned(4);

static void test_icmsg_buf_ut(void)
{
	static uint8_t rbuf[198];
	static uint8_t message[20] = {'a'};
	struct icmsg_buf *ib;
	int rlen;
	int wlen;

	ib = icmsg_buf_init(memory_area, sizeof(memory_area));
	zassert_equal_ptr(ib, memory_area, NULL);
	zassert_equal(ib->len, (sizeof(memory_area) - sizeof(*ib)), NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	/* Try to write more than buffer can store. */
	rlen = icmsg_buf_write(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, -ENOMEM, NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	/* Read empty buffer. */
	rlen = icmsg_buf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, 0, NULL);

	/* Single write and read. */
	wlen = icmsg_buf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message), NULL);
	zassert_equal(ib->wr_idx, (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	rlen = icmsg_buf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message), NULL);
	zassert_equal(ib->wr_idx, (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, (sizeof(message) + sizeof(uint16_t)), NULL);

	/* Buffer size is 212 - 12 = 200 Bytes for len, wr_idx, rd_idx.
	 * When writing message of 20 Bytes, actually 22 Bytes are stored,
	 * (2 Bytes reserved for message len). Test if after 9 writes, 10th write
	 * would return -ENOMEM. 200 - (9 * 22) = 2 bytes left.
	 *
	 * Reset the buffer first.
	 */
	ib = icmsg_buf_init(memory_area, sizeof(memory_area));
	zassert_equal_ptr(ib, memory_area, NULL);
	zassert_equal(ib->len, (sizeof(memory_area) - sizeof(*ib)), NULL);
	zassert_equal(ib->wr_idx, 0, NULL);
	zassert_equal(ib->rd_idx, 0, NULL);

	for (size_t i = 0; i < 9; i++) {
		wlen = icmsg_buf_write(ib, message, sizeof(message));
		zassert_equal(wlen, sizeof(message), NULL);
	}

	wlen = icmsg_buf_write(ib, message, sizeof(message));
	zassert_equal(wlen, -ENOMEM, NULL);

	/* Test reading with buf == NULL, should return len of the next message to read. */
	rlen = icmsg_buf_read(ib, NULL, 0);
	zassert_equal(rlen, sizeof(message), NULL);

	/* Read with len == 0 and correct buf pointer. */
	rlen = icmsg_buf_read(ib, rbuf, 0);
	zassert_equal(rlen, -ENOMEM, NULL);

	/* Read whole data from the buffer. */
	for (size_t i = 0; i < 9; i++) {
		zassert_equal(ib->rd_idx, i * (sizeof(message) + sizeof(uint16_t)), NULL);
		wlen = icmsg_buf_read(ib, rbuf, sizeof(rbuf));
		zassert_equal(wlen, sizeof(message), NULL);
	}

	zassert_equal(ib->wr_idx, 9 * (sizeof(message) + sizeof(uint16_t)), NULL);
	zassert_equal(ib->rd_idx, 9 * (sizeof(message) + sizeof(uint16_t)), NULL);

	/* Write message that would be wrapped around. */
	wlen = icmsg_buf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message), NULL);
	/* Have to store 22 bytes in total.
	 * 2 Bytes left in tail of the buffer 20 bytes go in front.
	 */
	zassert_equal(ib->wr_idx, 20, NULL);

	/* Read wrapped message. */
	rlen = icmsg_buf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message), NULL);
	zassert_equal(message[0], 'a', NULL);
}

void test_main(void)
{
	ztest_test_suite(icmsg_buf,
			ztest_unit_test(test_icmsg_buf_ut)
			);
	ztest_run_test_suite(icmsg_buf);
}
