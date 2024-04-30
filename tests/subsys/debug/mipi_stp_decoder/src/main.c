/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/debug/mipi_stp_decoder.h>

static int cnt;
static enum mipi_stp_decoder_ctrl_type exp_type[10];
static union mipi_stp_decoder_data exp_data[10];
static size_t exp_data_len[10];
static uint64_t exp_ts[10];
static bool exp_marked[10];
static int d_cnt;

static void cb(enum mipi_stp_decoder_ctrl_type type, union mipi_stp_decoder_data data, uint64_t *ts,
	       bool marked)
{
	zassert_equal(exp_type[d_cnt], type, "Expected: %d got:%d", exp_type[d_cnt], type);

	if (exp_ts[d_cnt] == UINT64_MAX) {
		zassert_equal(ts, NULL, NULL);
	} else {
		zassert_true(ts != NULL, NULL);
		zassert_equal(exp_ts[d_cnt], *ts, "exp:%llx got:%llx", exp_ts[d_cnt], *ts);
	}

	zassert_equal(exp_marked[d_cnt], marked, NULL);
	zassert_equal(
		memcmp((uint8_t *)&exp_data[d_cnt], (uint8_t *)&data.data, exp_data_len[d_cnt]), 0,
		NULL);
	d_cnt++;
}

static const struct mipi_stp_decoder_config config = {
	.cb = cb,
};

#define ADD_ITEM(_cnt, _type, _ts, _marked, _data)                                                 \
	do {                                                                                       \
		exp_type[_cnt] = _type;                                                            \
		exp_ts[_cnt] = (uint64_t)_ts;                                                      \
		exp_marked[_cnt] = _marked;                                                        \
		exp_data[_cnt].data = _data;                                                       \
		exp_data_len[_cnt++] = sizeof(_data);                                              \
	} while (0)

ZTEST(mipi_stp_decoder_test, test_chunk_null)
{
	uint8_t data[] = {0x00, 0x00};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(cnt, d_cnt, NULL);
}

ZTEST(mipi_stp_decoder_test, test_chunk_master)
{
	/* 0x1(m8) 0xab 0x0 (null) 0xf1(m16) 0x3412 */
	uint8_t data[] = {0xa1, 0x0b, 0x1f, 0x34, 0x12};

	ADD_ITEM(cnt, STP_DECODER_MASTER, UINT64_MAX, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_MASTER, UINT64_MAX, false, (uint16_t)0x4321);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(cnt, d_cnt, NULL);
}

ZTEST(mipi_stp_decoder_test, test_chunk_channel)
{
	/* 0(null) 1(m8) ab 3(c8) ab f3(c16) 4664 3(c8) bb 1(m8) 0b 3(c8) aa*/
	uint8_t data[] = {0x10, 0xba, 0xa3, 0xfb, 0x63, 0x44, 0x36, 0xbb, 0x01, 0x3b, 0xaa};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_MASTER, UINT64_MAX, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DECODER_CHANNEL, UINT64_MAX, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DECODER_CHANNEL, UINT64_MAX, false, (uint16_t)0x6446);
	/* MSB byte is taken from previous C16 */
	ADD_ITEM(cnt, STP_DECODER_CHANNEL, UINT64_MAX, false, (uint16_t)0x64bb);
	ADD_ITEM(cnt, STP_DECODER_MASTER, UINT64_MAX, false, (uint8_t)0x0b);
	/* M8 resets current channel */
	ADD_ITEM(cnt, STP_DECODER_CHANNEL, UINT64_MAX, false, (uint8_t)0xaa);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_data)
{
	/* 4(d8) ab 5(d16) 0x3456 6(d32) 0x11223344 7(d64) 0x1020304050607080 */
	/* f8(dm8) ab f9(dm16) 0x3456 fa(dm32) 0x11223344 fb(dm64) 0x1020304050607080 */
	uint8_t data[] = {0xa4, 0x5b, 0x43, 0x65, 0x16, 0x21, 0x32, 0x43, 0x74, 0x01,
			  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,

			  0x8f, 0xba, 0x9f, 0x43, 0x65, 0xaf, 0x11, 0x22, 0x33, 0x44,
			  0xbf, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

	ADD_ITEM(cnt, STP_DATA8, UINT64_MAX, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DATA16, UINT64_MAX, false, (uint16_t)0x3456);
	ADD_ITEM(cnt, STP_DATA32, UINT64_MAX, false, (uint32_t)0x11223344);
	ADD_ITEM(cnt, STP_DATA64, UINT64_MAX, false, (uint32_t)0x1020304050607080);
	ADD_ITEM(cnt, STP_DATA8, UINT64_MAX, true, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DATA16, UINT64_MAX, true, (uint16_t)0x3456);
	ADD_ITEM(cnt, STP_DATA32, UINT64_MAX, true, (uint32_t)0x11223344);
	ADD_ITEM(cnt, STP_DATA64, UINT64_MAX, true, (uint32_t)0x1020304050607080);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_data_ts)
{
	uint8_t data[] = {
		/*d8ts + 13b TS */ 0x4f,
		0xba,
		0x1d,
		0x21,
		0x32,
		0x43,
		0x54,
		0x65,
		0x76,
		0x07,
		/*d16ts + 3b TS */ 0x5f,
		0xba,
		0xdc,
		0x13,
		0x22,
		/*d32ts + 3b TS */ 0x6f,
		0x11,
		0x22,
		0xba,
		0xdc,
		0x13,
		0x22,
		/*d64ts + 3b TS */ 0x7f,
		0x11,
		0x22,
		0xba,
		0xdc,
		0x11,
		0x22,
		0x33,
		0x44,
		0x13,
		0x22,
		/*d8mts + 14b TS */ 0xa8,
		0xeb,
		0x11,
		0x22,
		0x33,
		0x44,
		0x55,
		0x66,
		0x77,
		0x88,
		/*d16mts + 2b TS */ 0xa9,
		0xcb,
		0x2d,
		0x31,
		/*d32mts + 2b TS */ 0xaa,
		0xcb,
		0x1d,
		0x21,
		0x22,
		0x31,
		/*d64mts + 2b TS */ 0xab,
		0xcb,
		0x1d,
		0x21,
		0x12,
		0x11,
		0x11,
		0x11,
		0x21,
		0x31,
	};

	ADD_ITEM(cnt, STP_DATA8, 0x11223344556677, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DATA16, 0x11223344556122, false, (uint16_t)0xabcd);
	ADD_ITEM(cnt, STP_DATA32, 0x11223344556122, false, (uint32_t)0x1122abcd);
	ADD_ITEM(cnt, STP_DATA64, 0x11223344556122, false, (uint64_t)0x1122abcd11223344);
	ADD_ITEM(cnt, STP_DATA8, 0x1122334455667788, true, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DATA16, 0x1122334455667713, true, (uint16_t)0xabcd);
	ADD_ITEM(cnt, STP_DATA32, 0x1122334455667713, true, (uint32_t)0xabcd1122);
	ADD_ITEM(cnt, STP_DATA64, 0x1122334455667713, true, (uint64_t)0xabcd112211111111);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_multi_chunk_data_ts)
{
	/*d8ts + 13b TS */
	uint8_t data[] = {0x4f, 0xba, 0x1d, 0x21, 0x32};
	uint8_t data2[] = {
		0x43, 0x54, 0x65, 0x76, 0x07,
	};

	ADD_ITEM(cnt, STP_DATA8, 0x11223344556677, false, (uint8_t)0xab);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);

	/* First part without any packet decoded. */
	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, 0, "got:%d exp:%d", d_cnt, 0);

	mipi_stp_decoder_decode(data2, sizeof(data2));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_errors)
{
	uint8_t data[] = {/*merr 0x12 gerr 0x12 null */ 0x12, 0xf2, 0x12, 0x02};

	ADD_ITEM(cnt, STP_DECODER_MERROR, UINT64_MAX, false, (uint8_t)0x12);
	ADD_ITEM(cnt, STP_DECODER_GERROR, UINT64_MAX, false, (uint8_t)0x12);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_freq)
{
	uint8_t data[] = {/* freq 0x11223344 null */ 0x0f,
			  0x18,
			  0x21,
			  0x32,
			  0x43,
			  0x04,
			  /* freq_ts 0x11223344 + 2b TS */ 0x0f,
			  0x19,
			  0x21,
			  0x32,
			  0x43,
			  0x24,
			  0x12};

	ADD_ITEM(cnt, STP_DECODER_FREQ, UINT64_MAX, false, (uint32_t)0x11223344);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_FREQ, 0x21ULL, false, (uint32_t)0x11223344);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_async)
{
	uint8_t data[] = {/* null async null*/
			  0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_ASYNC, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_multi_chunk_async)
{
	/* null async null split into 2 buffers */
	uint8_t data[] = {
		0xf0,
		0xff,
		0xff,
	};
	uint8_t data2[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_ASYNC, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);

	/* First part only null packet is decoded */
	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, 1, "got:%d exp:%d", d_cnt, 1);

	mipi_stp_decoder_decode(data2, sizeof(data2));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_chunk_freq2)
{
	/* null async null split into 2 buffers */
	uint8_t data[] = {0xf0, 0x80, 0x00, 0xc4, 0xb4, 0x04};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_FREQ, UINT64_MAX, false, (uint64_t)5000000);

	mipi_stp_decoder_decode(data, sizeof(data));
	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

ZTEST(mipi_stp_decoder_test, test_sync_loss)
{
	/* null async null split into 2 buffers */
	uint8_t data[] = {0xf0, 0x80, 0x00, 0xc4, 0xff, 0xff, 0xff, 0xff, 0xf5, 0xff, 0xff, 0xff,
			  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x60, 0x11, 0x11, 0x11, 0x11};

	ADD_ITEM(cnt, STP_DECODER_NULL, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DECODER_ASYNC, UINT64_MAX, false, (uint8_t)0);
	ADD_ITEM(cnt, STP_DATA32, UINT64_MAX, false, (uint32_t)0x11111111);

	mipi_stp_decoder_decode(data, 4);
	mipi_stp_decoder_sync_loss();
	mipi_stp_decoder_decode(&data[4], sizeof(data) - 4);

	zassert_equal(d_cnt, cnt, "got:%d exp:%d", d_cnt, cnt);
}

static void before(void *data)
{
	cnt = 0;
	d_cnt = 0;
	mipi_stp_decoder_init(&config);
}

ZTEST_SUITE(mipi_stp_decoder_test, NULL, NULL, before, NULL, NULL);
