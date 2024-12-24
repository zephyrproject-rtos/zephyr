/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/sys/mpsc_pbuf.h>

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#define PUT_EXT_LEN \
	((sizeof(union mpsc_pbuf_generic) + sizeof(void *)) / sizeof(uint32_t))

#define LEN_BITS 9

struct test_data {
	MPSC_PBUF_HDR;
	uint32_t len : LEN_BITS;
	uint32_t data : 32 - MPSC_PBUF_HDR_BITS - LEN_BITS;
};

struct test_data_ext {
	struct test_data hdr;
	void *data;
} __packed;

struct test_data_var {
	struct test_data hdr;
	uint32_t data[];
};

union test_item {
	struct test_data data;
	struct test_data_ext data_ext;
	union mpsc_pbuf_generic item;
};

static uint32_t get_wlen(const union mpsc_pbuf_generic *item)
{
	union test_item *t_item = (union test_item *)item;

	return t_item->data.len;
}

static uint32_t drop_cnt;
static uint32_t exp_drop_cnt;
static uintptr_t exp_dropped_data[10];
static uint32_t exp_dropped_len[10];

static void drop(const struct mpsc_pbuf_buffer *buffer, const union mpsc_pbuf_generic *item)
{
	struct test_data_var *packet = (struct test_data_var *)item;

	zassert_true(drop_cnt < exp_drop_cnt);
	zassert_equal(packet->hdr.len, exp_dropped_len[drop_cnt],
			"(%d) Got:%08x, Expected: %08x",
			drop_cnt, packet->hdr.len, exp_dropped_len[drop_cnt]);
	zassert_equal(packet->hdr.data, exp_dropped_data[drop_cnt],
			"(%d) Got:%08x, Expected: %08x",
			drop_cnt, packet->hdr.data, exp_dropped_data[drop_cnt]);
	for (int i = 0; i < exp_dropped_len[drop_cnt] - 1; i++) {
		int err = memcmp(packet->data, &exp_dropped_data[drop_cnt],
				 sizeof(uint32_t));


		zassert_equal(err, 0);
	}

	drop_cnt++;
}

static uint32_t buf32[512];

static struct mpsc_pbuf_buffer_config mpsc_buf_cfg = {
	.buf = buf32,
	.size = ARRAY_SIZE(buf32),
	.notify_drop = drop,
	.get_wlen = get_wlen
};

static void init(struct mpsc_pbuf_buffer *buffer, uint32_t wlen, bool overwrite)
{
	drop_cnt = 0;
	exp_drop_cnt = 0;
	mpsc_buf_cfg.flags = overwrite ? MPSC_PBUF_MODE_OVERWRITE : 0;
	mpsc_buf_cfg.size = wlen;
	mpsc_pbuf_init(buffer, &mpsc_buf_cfg);

#if CONFIG_SOC_SERIES_NRF52X
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	DWT->CYCCNT = 0;
#endif
}

static inline uint32_t get_cyc(void)
{
#if CONFIG_SOC_SERIES_NRF52X
	return DWT->CYCCNT;
#else
	return k_cycle_get_32();
#endif
}

void item_put_no_overwrite(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 4 - !pow2, false);

	int repeat = buffer.size*2;
	union test_item test_1word = {.data = {.valid = 1, .len = 1 }};

	for (int i = 0; i < repeat; i++) {
		union test_item *t;

		test_1word.data.data = i;
		mpsc_pbuf_put_word(&buffer, test_1word.item);

		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data.data, i);
		mpsc_pbuf_free(&buffer, &t->item);

	}

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_item_put_no_overwrite)
{
	item_put_no_overwrite(true);
	item_put_no_overwrite(false);
}

void item_put_overwrite(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 4 - !pow2, true);

	union test_item test_1word = {.data = {.valid = 1, .len = 1 }};

	exp_dropped_data[0] = 0;
	exp_dropped_len[0] = 1;
	exp_drop_cnt = 1;

	for (int i = 0; i < buffer.size + 1; i++) {
		test_1word.data.data = i;
		mpsc_pbuf_put_word(&buffer, test_1word.item);
	}

	zassert_equal(drop_cnt, exp_drop_cnt,
			"Unexpected number of dropped messages: %d", drop_cnt);
}

ZTEST(log_buffer, test_item_put_overwrite)
{
	item_put_overwrite(true);
	item_put_overwrite(false);
}

void item_put_saturate(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 4 - !pow2, false);

	int repeat = buffer.size;
	union test_item test_1word = {.data = {.valid = 1, .len = 1 }};
	union test_item *t;

	zassert_false(mpsc_pbuf_is_pending(&buffer));

	for (int i = 0; i < repeat / 2; i++) {
		test_1word.data.data = i;
		mpsc_pbuf_put_word(&buffer, test_1word.item);

		zassert_true(mpsc_pbuf_is_pending(&buffer));

		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data.data, i);
		mpsc_pbuf_free(&buffer, &t->item);
	}

	for (int i = 0; i < repeat + 1; i++) {
		test_1word.data.data = i;
		mpsc_pbuf_put_word(&buffer, test_1word.item);
	}

	for (int i = 0; i < repeat; i++) {
		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data.data, i);
		mpsc_pbuf_free(&buffer, &t->item);
	}

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_item_put_saturate)
{
	item_put_saturate(true);
	item_put_saturate(false);
}

void benchmark_item_put(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, ARRAY_SIZE(buf32) - !pow2, true);

	int repeat = buffer.size - 1;
	union test_item test_1word = {.data = {.valid = 1, .len = 1 }};
	uint32_t t = get_cyc();

	for (int i = 0; i < repeat; i++) {
		test_1word.data.data = i;
		mpsc_pbuf_put_word(&buffer, test_1word.item);
	}

	t = get_cyc() - t;
	PRINT("%s buffer\n", pow2 ? "pow2" : "non-pow2");
	PRINT("single word put time: %d cycles\n", t/repeat);

	t = get_cyc();
	for (int i = 0; i < repeat; i++) {
		union test_item *ti;

		ti = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(ti);
		zassert_equal(ti->data.data, i);
		mpsc_pbuf_free(&buffer, &ti->item);
	}

	t = get_cyc() - t;
	PRINT("single word item claim,free: %d cycles\n", t/repeat);

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_benchmark_item_put)
{
	benchmark_item_put(true);
	benchmark_item_put(false);
}

void item_put_ext_no_overwrite(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 8 - !pow2, false);

	int repeat = buffer.size * 2;
	union test_item test_ext_item = {
		.data = {
			.valid = 1,
			.len = PUT_EXT_LEN
		}
	};
	void *data;

	for (uintptr_t i = 0; i < repeat; i++) {
		union test_item *t;

		data = (void *)i;
		test_ext_item.data.data = i;
		mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, data);

		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data_ext.hdr.data, i);
		zassert_equal(t->data_ext.data, (void *)i);
		mpsc_pbuf_free(&buffer, &t->item);
	}

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_item_put_ext_no_overwrite)
{
	item_put_ext_no_overwrite(true);
	item_put_ext_no_overwrite(false);
}

void item_put_word_ext_overwrite(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 8 - !pow2, true);

	size_t w = (sizeof(uint32_t) + sizeof(void *)) / sizeof(uint32_t);
	int repeat = 1 + buffer.size / w;
	union test_item test_ext_item = {
		.data = {
			.valid = 1,
			.len = PUT_EXT_LEN
		}
	};

	exp_dropped_data[0] = 0;
	exp_dropped_len[0] = w;
	exp_drop_cnt = 1;

	for (uintptr_t i = 0; i < repeat; i++) {
		test_ext_item.data.data = i;
		mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, (void *)i);
	}

	zassert_equal(drop_cnt, exp_drop_cnt,
			"Unexpected number of dropped messages: %d (exp: %d)",
			drop_cnt, exp_drop_cnt);
}

ZTEST(log_buffer, test_item_put_word_ext_overwrite)
{
	item_put_word_ext_overwrite(true);
	item_put_word_ext_overwrite(false);
}

void item_put_ext_saturate(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 8 - !pow2, false);

	int repeat = buffer.size / PUT_EXT_LEN;
	union test_item test_ext_item = {
		.data = {
			.valid = 1,
			.len = PUT_EXT_LEN
		}
	};
	void *data;
	union test_item *t;

	for (uintptr_t i = 0; i < repeat/2; i++) {
		test_ext_item.data.data = i;
		data = (void *)i;
		mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, data);

		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data.data, i);
		mpsc_pbuf_free(&buffer, &t->item);
	}

	for (uintptr_t i = 0; i < repeat; i++) {
		test_ext_item.data.data = i;
		data = (void *)i;
		mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, data);
	}

	for (uintptr_t i = 0; i < repeat; i++) {
		t = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(t);
		zassert_equal(t->data_ext.data, (void *)i);
		zassert_equal(t->data_ext.hdr.data, i);
		mpsc_pbuf_free(&buffer, &t->item);
	}

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_item_put_ext_saturate)
{
	item_put_ext_saturate(true);
	item_put_ext_saturate(false);
}

void benchmark_item_put_ext(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, ARRAY_SIZE(buf32) - !pow2, false);

	int repeat = (buffer.size - 1) / PUT_EXT_LEN;
	union test_item test_ext_item = {
		.data = {
			.valid = 1,
			.len = PUT_EXT_LEN
		}
	};
	void *data = NULL;
	uint32_t t = get_cyc();

	for (int i = 0; i < repeat; i++) {
		test_ext_item.data.data = i;
		mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, data);
	}

	t = get_cyc() - t;
	PRINT("%spow2 buffer\n", pow2 ? "" : "non-");
	PRINT("put_ext time: %d cycles\n", t/repeat);

	t = get_cyc();
	for (int i = 0; i < repeat; i++) {
		union test_item *ti;

		ti = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(ti);
		zassert_equal(ti->data.data, i);
		mpsc_pbuf_free(&buffer, &ti->item);
	}

	t = get_cyc() - t;
	PRINT("ext item claim,free: %d cycles\n", t/repeat);

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_benchmark_item_put_ext)
{
	benchmark_item_put_ext(true);
	benchmark_item_put_ext(false);
}

void benchmark_item_put_data(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, ARRAY_SIZE(buf32) - !pow2, false);

	int repeat = (buffer.size - 1) / PUT_EXT_LEN;
	union test_item test_ext_item = {
		.data_ext = {
			.hdr = {
				.valid = 1,
				.len = PUT_EXT_LEN
			},
			.data = NULL
		}
	};
	uint32_t cyc = get_cyc();

	for (uintptr_t i = 0; i < repeat; i++) {
		test_ext_item.data_ext.hdr.data = i;
		test_ext_item.data_ext.data = (void *)i;
		mpsc_pbuf_put_data(&buffer, (uint32_t *)&test_ext_item,
				    PUT_EXT_LEN);
	}

	cyc = get_cyc() - cyc;
	PRINT("%spow2 buffer\n", pow2 ? "" : "non-");
	PRINT("put_ext time: %d cycles\n", cyc/repeat);

	cyc = get_cyc();
	for (int i = 0; i < repeat; i++) {
		union test_item *ti;

		ti = (union test_item *)mpsc_pbuf_claim(&buffer);
		zassert_true(ti);
		zassert_equal(ti->data.data, i);
		mpsc_pbuf_free(&buffer, &ti->item);
	}

	cyc = get_cyc() - cyc;
	PRINT("ext item claim,free: %d cycles\n", cyc/repeat);

	zassert_is_null(mpsc_pbuf_claim(&buffer));
}

ZTEST(log_buffer, test_benchmark_item_put_data)
{
	benchmark_item_put_data(true);
	benchmark_item_put_data(false);
}

void item_put_data_overwrite(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 8 - !pow2, true);

	size_t w = (sizeof(uint32_t) + sizeof(void *)) / sizeof(uint32_t);
	int repeat = 1 + buffer.size / w;
	static const int len = sizeof(struct test_data_ext) / sizeof(uint32_t);
	struct test_data_ext item = {
		.hdr = {
			.valid = 1,
			.len = len
		}
	};

	exp_dropped_data[0] = 0;
	exp_dropped_len[0] = w;
	exp_drop_cnt = 1;

	for (uintptr_t i = 0; i < repeat; i++) {
		void *vitem;
		item.data = (void *)i;
		item.hdr.data = i;
		vitem = (uint32_t *)&item;
		zassert_true(IS_PTR_ALIGNED(vitem, uint32_t), "unaligned ptr");
		mpsc_pbuf_put_data(&buffer, (uint32_t *)vitem, len);
	}

	zassert_equal(drop_cnt, exp_drop_cnt,
			"Unexpected number of dropped messages: %d", drop_cnt);
}

ZTEST(log_buffer, test_put_data_overwrite)
{
	item_put_data_overwrite(true);
	item_put_data_overwrite(false);
}

void item_alloc_commit(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 16 - !pow2, false);

	struct test_data_var *packet;
	uint32_t len = 5;
	int repeat = 1024;

	for (int i = 0; i < repeat; i++) {
		packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len,
								 K_NO_WAIT);
		packet->hdr.len = len;
		for (int j = 0; j < len - 1; j++) {
			packet->data[j] = i + j;
		}

		mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)packet);

		packet = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
		zassert_true(packet);
		zassert_equal(packet->hdr.len, len);

		for (int j = 0; j < len - 1; j++) {
			zassert_equal(packet->data[j], i + j);
		}

		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)packet);
	}
}

ZTEST(log_buffer, test_item_alloc_commit)
{
	item_alloc_commit(true);
	item_alloc_commit(false);
}

void item_max_alloc(bool overwrite)
{
	struct mpsc_pbuf_buffer buffer;
	struct test_data_var *packet;

	init(&buffer, 8, overwrite);

	/* First try to allocate the biggest possible packet. */
	for (int i = 0; i < 2; i++) {
		packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer,
								 buffer.size,
								 K_NO_WAIT);
		zassert_true(packet != NULL);
		packet->hdr.len = buffer.size;
		mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)packet);

		packet = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)packet);
	}

	/* Too big packet cannot be allocated. */
	packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer,
							 buffer.size + 1,
							 K_NO_WAIT);
	zassert_true(packet == NULL);
}

ZTEST(log_buffer, test_item_max_alloc)
{
	item_max_alloc(true);
	item_max_alloc(false);
}

static uint32_t saturate_buffer_uneven(struct mpsc_pbuf_buffer *buffer,
					uint32_t len)
{
	struct test_data_var *packet;
	uint32_t uneven = 3;
	uint32_t cnt = 0;
	int repeat =
		uneven + ((buffer->size - (uneven * len)) / len);

	/* Put some data to include wrapping */
	for (int i = 0; i < uneven; i++) {
		packet = (struct test_data_var *)mpsc_pbuf_alloc(buffer, len,
								 K_NO_WAIT);
		packet->hdr.len = len;
		mpsc_pbuf_commit(buffer, (union mpsc_pbuf_generic *)packet);

		packet = (struct test_data_var *)mpsc_pbuf_claim(buffer);
		zassert_true(packet);
		mpsc_pbuf_free(buffer, (union mpsc_pbuf_generic *)packet);
	}

	for (int i = 0; i < repeat; i++) {
		packet = (struct test_data_var *)mpsc_pbuf_alloc(buffer, len,
								 K_NO_WAIT);
		zassert_true(packet);
		packet->hdr.len = len;
		packet->hdr.data = i;
		for (int j = 0; j < len - 1; j++) {
			packet->data[j] = i + j;
		}

		mpsc_pbuf_commit(buffer, (union mpsc_pbuf_generic *)packet);
		cnt++;
	}

	return cnt;
}

void item_alloc_commit_saturate(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 32 - !pow2, false);

	saturate_buffer_uneven(&buffer, 5);

	struct test_data_var *packet;
	uint32_t len = 5;

	packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len,
							 K_NO_WAIT);
	zassert_is_null(packet);

	/* Get one packet from the buffer. */
	packet = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(packet);
	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)packet);

	/* and try to allocate one more time, this time with success. */
	packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len,
							 K_NO_WAIT);
	zassert_true(packet);
}

ZTEST(log_buffer, test_item_alloc_commit_saturate)
{
	item_alloc_commit_saturate(true);
	item_alloc_commit_saturate(false);
}

void item_alloc_preemption(bool pow2)
{
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, ARRAY_SIZE(buf32) - !pow2, false);

	struct test_data_var *p0;
	struct test_data_var *p1;
	struct test_data_var *p;

	p0 = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, 10, K_NO_WAIT);
	zassert_true(p0);
	p0->hdr.len = 10;

	/* Check that no packet is yet available */
	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p);

	p1 = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, 20, K_NO_WAIT);
	zassert_true(p1);
	p1->hdr.len = 20;

	/* Commit p1, p0 is still not committed, there should be no packets
	 * available for reading.
	 */
	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p1);

	/* Check that no packet is yet available */
	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p);

	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p0);

	/* Validate that p0 is the first one. */
	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p);
	zassert_equal(p->hdr.len, 10);
	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p);

	/* Validate that p1 is the next one. */
	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p);
	zassert_equal(p->hdr.len, 20);
	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p);

	/* No more packets. */
	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p);
}

ZTEST(log_buffer, test_item_alloc_preemption)
{
	item_alloc_preemption(true);
	item_alloc_preemption(false);
}

void overwrite(bool pow2)
{
	struct test_data_var *p;
	uint32_t fill_len = 5;
	uint32_t len0, len1;
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 32 - !pow2, true);
	uint32_t packet_cnt = saturate_buffer_uneven(&buffer, fill_len);

	zassert_equal(drop_cnt, exp_drop_cnt, NULL);

	exp_dropped_data[0] = 0;
	exp_dropped_len[0] = fill_len;
	exp_drop_cnt++;
	exp_dropped_data[1] = 1;
	exp_dropped_len[1] = fill_len;
	exp_drop_cnt++;

	len0 = 6;
	p = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len0, K_NO_WAIT);

	p->hdr.len = len0;
	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p);
	zassert_equal(drop_cnt, exp_drop_cnt);

	/* Request allocation which will require dropping 2 packets. */
	len1 = 9;
	exp_dropped_data[1] = 1;
	exp_dropped_len[1] = fill_len;
	exp_dropped_data[2] = 2;
	exp_dropped_len[2] = fill_len;
	exp_drop_cnt = 3;

	p = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len1, K_NO_WAIT);

	p->hdr.len = len1;
	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p);
	zassert_equal(drop_cnt, exp_drop_cnt);

	for (int i = 0; i < (packet_cnt - drop_cnt); i++) {
		p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
		zassert_true(p);
		zassert_equal(p->hdr.len, fill_len);
		zassert_equal(p->hdr.data, i + drop_cnt);
		for (int j = 0; j < fill_len - 1; j++) {
			zassert_equal(p->data[j], p->hdr.data + j);
		}

		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p);
	}

	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p);
	zassert_equal(p->hdr.len, len0);
	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p);

	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p);
	zassert_equal(p->hdr.len, len1);
	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p);

	p = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p);
}

ZTEST(log_buffer, test_overwrite)
{
	overwrite(true);
	overwrite(false);
}

void overwrite_while_claimed(bool pow2)
{
	struct test_data_var *p0;
	struct test_data_var *p1;
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 32 - !pow2, true);

	uint32_t fill_len = 5;
	uint32_t len = 6;
	uint32_t packet_cnt = saturate_buffer_uneven(&buffer, fill_len);

	/* Start by claiming a packet. Buffer is now full. Allocation shall
	 * skip claimed packed and drop the next one.
	 */
	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p0);
	zassert_equal(p0->hdr.len, fill_len);

	exp_dropped_data[0] = p0->hdr.data + 1; /* next packet is dropped */
	exp_dropped_len[0] = fill_len;
	exp_dropped_data[1] = p0->hdr.data + 2; /* next packet is dropped */
	exp_dropped_len[1] = fill_len;
	exp_drop_cnt = 2;
	p1 = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, 6, K_NO_WAIT);
	zassert_equal(drop_cnt, exp_drop_cnt);
	p1->hdr.len = len;
	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p1);

	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p0);

	for (int i = 0; i < packet_cnt - drop_cnt - 1; i++) {
		p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
		zassert_true(p0);
		zassert_equal(p0->hdr.len, fill_len);
		zassert_equal(p0->hdr.data, i + drop_cnt + 1);
		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p0);
	}

	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p0);
	zassert_equal(p0->hdr.len, len);

	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p0);
}

ZTEST(log_buffer, test_overwrite_while_claimed)
{
	overwrite_while_claimed(true);
	overwrite_while_claimed(false);
}

void overwrite_while_claimed2(bool pow2)
{
	struct test_data_var *p0;
	struct test_data_var *p1;
	struct mpsc_pbuf_buffer buffer;

	init(&buffer, 32 - !pow2, true);

	uint32_t fill_len = 1;
	uint32_t len = 3;
	uint32_t packet_cnt = saturate_buffer_uneven(&buffer, fill_len);

	/* Start by claiming a packet. Buffer is now full. Allocation shall
	 * skip claimed packed and drop the next one.
	 */
	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p0);
	zassert_equal(p0->hdr.len, fill_len);

	exp_dropped_data[0] = p0->hdr.data + 1; /* next packet is dropped */
	exp_dropped_len[0] = fill_len;
	exp_dropped_data[1] = p0->hdr.data + 2; /* next packet is dropped */
	exp_dropped_len[1] = fill_len;
	exp_dropped_data[2] = p0->hdr.data + 3; /* next packet is dropped */
	exp_dropped_len[2] = fill_len;
	exp_drop_cnt = 3;
	p1 = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len, K_NO_WAIT);

	zassert_equal(drop_cnt, exp_drop_cnt);
	p1->hdr.len = len;
	mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)p1);

	mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p0);

	for (int i = 0; i < packet_cnt - drop_cnt - 1; i++) {
		p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
		zassert_true(p0);
		zassert_equal(p0->hdr.len, fill_len);
		zassert_equal(p0->hdr.data, i + drop_cnt + 1);
		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)p0);
	}

	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_true(p0);
	zassert_equal(p0->hdr.len, len);

	p0 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
	zassert_is_null(p0);
}

ZTEST(log_buffer, test_overwrite_while_claimed2)
{
	overwrite_while_claimed2(true);
	overwrite_while_claimed2(false);
}

static uintptr_t current_rd_idx;

static void validate_packet(struct test_data_var *packet)
{
	zassert_equal((uintptr_t)packet->hdr.data, current_rd_idx,
			"Got %d, expected: %d",
			(uintptr_t)packet->hdr.data, current_rd_idx);
	current_rd_idx++;
}

static void consistent_drop(const struct mpsc_pbuf_buffer *buffer,
			    const union mpsc_pbuf_generic *item)
{
	validate_packet((struct test_data_var *)item);
}

uint32_t rand_get(uint32_t min, uint32_t max)
{
	return min + (sys_rand32_get() % max);
}

ZTEST(log_buffer, test_overwrite_consistency)
{
	struct mpsc_pbuf_buffer buffer;
	static struct mpsc_pbuf_buffer_config cfg = {
		.buf = buf32,
		.size = ARRAY_SIZE(buf32),
		.notify_drop = consistent_drop,
		.get_wlen = get_wlen,
		.flags = MPSC_PBUF_MODE_OVERWRITE
	};

	mpsc_pbuf_init(&buffer, &cfg);
	int repeat = 50000;
	int id = 0;

	while (id < repeat) {
		struct test_data_var *tdv = NULL;
		bool alloc_during_claim = (rand_get(1, 5) <= 2);

		/* Occasionally claim buffer to simulate that claiming is
		 * interrupted by allocation.
		 */
		if (alloc_during_claim) {
			tdv = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
			if (tdv) {
				validate_packet(tdv);
			}
		}

		uint32_t wr_cnt = rand_get(1, 15);

		for (int i = 0; i < wr_cnt; i++) {
			uint32_t wlen = rand_get(1, 15);
			struct test_data_var *tdv2;

			tdv2 = (struct test_data_var *)mpsc_pbuf_alloc(&buffer,
								       wlen,
								       K_NO_WAIT);
			tdv2->hdr.len = wlen;
			tdv2->hdr.data = id++;
			mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)tdv2);
		}

		/* Put back item claimed before committing new items. */
		if (tdv) {
			mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)tdv);
		}

		uint32_t rd_cnt = rand_get(1, 30);

		for (int i = 0; i < rd_cnt; i++) {
			struct test_data_var *tdv2;

			tdv2 = (struct test_data_var *)mpsc_pbuf_claim(&buffer);
			if (!tdv2) {
				continue;
			}

			validate_packet(tdv2);
			mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)tdv2);
		}
	}
}

K_THREAD_STACK_DEFINE(t1_stack, 1024);
K_THREAD_STACK_DEFINE(t2_stack, 1024);

static k_thread_stack_t *stacks[2] = {t1_stack, t2_stack};
static struct k_thread threads[2];
static k_tid_t tids[2];

void t_entry(void *p0, void *p1, void *p2)
{
	struct mpsc_pbuf_buffer *buffer = p0;
	uintptr_t wait_ms = (uintptr_t)p1;
	struct test_data_ext *t;
	void *vt;

	t = (struct test_data_ext *)mpsc_pbuf_alloc(buffer,
						    sizeof(*t) / sizeof(uint32_t),
						    K_MSEC(1));
	zassert_is_null(t);

	t = (struct test_data_ext *)mpsc_pbuf_alloc(buffer,
						    sizeof(*t) / sizeof(uint32_t),
						    K_MSEC(wait_ms));
	t->hdr.len = PUT_EXT_LEN;
	t->data = k_current_get();

	vt = t;
	zassert_true(IS_PTR_ALIGNED(vt, union mpsc_pbuf_generic), "unaligned ptr");
	mpsc_pbuf_commit(buffer, (union mpsc_pbuf_generic *)vt);
	while (1) {
		k_sleep(K_MSEC(10));
	}
}

void start_threads(struct mpsc_pbuf_buffer *buffer)
{
	int prio = 2;
	uintptr_t wait_ms = 1000;

	for (int i = 0; i < ARRAY_SIZE(threads); i++) {
		tids[i] = k_thread_create(&threads[i], stacks[i], 1024, t_entry,
					  buffer, (void *)wait_ms, NULL,
					  prio--,
					  0, K_NO_WAIT);
	}

	k_sleep(K_MSEC(10));

	for (int i = 0; i < ARRAY_SIZE(threads); i++) {
		k_ticks_t t = k_thread_timeout_remaining_ticks(tids[i]);
		k_ticks_t exp_wait = k_ms_to_ticks_ceil32(wait_ms);

		/* Threads shall be blocked, waiting for available space. */
		zassert_within(t, exp_wait, k_ms_to_ticks_ceil32(20));
	}
}

/* Test creates two threads which pends on the buffer until there is a space
 * available. When enough buffers is released threads are woken up and they
 * allocate packets.
 */
ZTEST(log_buffer, test_pending_alloc)
{
	int prio = k_thread_priority_get(k_current_get());
	struct mpsc_pbuf_buffer buffer;
	void *vt;

	k_thread_priority_set(k_current_get(), 3);

	init(&buffer, ARRAY_SIZE(buf32) - 1, true);

	uint32_t fill_len = 1;
	uint32_t packet_cnt = saturate_buffer_uneven(&buffer, fill_len);

	start_threads(&buffer);

	k_sleep(K_MSEC(1));

	for (int i = 0; i < packet_cnt; i++) {
		union test_item *t = (union test_item *)mpsc_pbuf_claim(&buffer);

		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)t);
	}


	for (int i = 0; i < 2; i++) {
		struct test_data_ext *t =
			(struct test_data_ext *)mpsc_pbuf_claim(&buffer);

		zassert_true(t);
		zassert_equal(t->data, tids[ARRAY_SIZE(tids) - 1 - i]);
		vt = t;
		zassert_true(IS_PTR_ALIGNED(vt, union mpsc_pbuf_generic), "unaligned ptr");
		mpsc_pbuf_free(&buffer, (union mpsc_pbuf_generic *)vt);
	}

	zassert_equal(mpsc_pbuf_claim(&buffer), NULL, "No more packets.");
	k_thread_priority_set(k_current_get(), prio);
}

static void check_packet(struct mpsc_pbuf_buffer *buffer, char exp_c)
{
	union test_item claimed_item;
	const union mpsc_pbuf_generic *claimed;

	claimed = mpsc_pbuf_claim(buffer);
	zassert_true(claimed, NULL);
	claimed_item.item = *claimed;
	zassert_equal(claimed_item.data.data, exp_c, NULL);

	mpsc_pbuf_free(buffer, claimed);
}

ZTEST(log_buffer, test_put_while_claim)
{
	struct mpsc_pbuf_buffer buffer;
	uint32_t buffer_storage[4];
	const union mpsc_pbuf_generic *claimed;
	struct mpsc_pbuf_buffer_config buffer_config = {
		.buf = buffer_storage,
		.size = 4,
		.notify_drop = drop,
		.get_wlen = get_wlen,
		.flags = MPSC_PBUF_SIZE_POW2 | MPSC_PBUF_MODE_OVERWRITE
	};
	union test_item claimed_item;
	union test_item item = {
		.data = {
			.valid = 1,
			.busy = 0,
			.len = 1,
			.data = (uint32_t)'a'
		}
	};

	exp_drop_cnt = 0;
	drop_cnt = 0;
	mpsc_pbuf_init(&buffer, &buffer_config);
	/* Expect buffer = {} */

	for (int i = 0; i < buffer.size; ++i) {
		mpsc_pbuf_put_word(&buffer, item.item);
		item.data.data++;
	}

	/* Expect buffer = {a, b, c, d}. Adding new word will drop 'a'. */
	exp_dropped_data[exp_drop_cnt] = (uint32_t)'a';
	exp_dropped_len[exp_drop_cnt] = 1;
	exp_drop_cnt++;

	item.data.data = 'e';
	mpsc_pbuf_put_word(&buffer, item.item);
	zassert_equal(drop_cnt, exp_drop_cnt, NULL);
	/* Expect buffer = {e, b, c, d} */

	claimed = mpsc_pbuf_claim(&buffer);
	zassert_true(claimed, NULL);
	claimed_item.item = *claimed;
	zassert_equal(claimed_item.data.data, (uint32_t)'b', NULL);

	/* Expect buffer = {e, B, c, d}. Adding new will drop 'c'. */
	exp_dropped_data[exp_drop_cnt] = (int)'c';
	exp_dropped_len[exp_drop_cnt] = 1;
	exp_drop_cnt++;

	item.data.data = 'f';
	mpsc_pbuf_put_word(&buffer, item.item);
	zassert_equal(drop_cnt, exp_drop_cnt, NULL);
	/* Expect buffer = {e, B, f, d}, Adding new will drop 'd'. */

	exp_dropped_data[exp_drop_cnt] = (int)'d';
	exp_dropped_len[exp_drop_cnt] = 1;
	exp_drop_cnt++;
	item.data.data = 'g';
	mpsc_pbuf_put_word(&buffer, item.item);
	zassert_equal(drop_cnt, exp_drop_cnt, NULL);
	/* Expect buffer = {e, B, f, g} */

	mpsc_pbuf_free(&buffer, claimed);
	/* Expect buffer = {e -, f, g} */

	check_packet(&buffer, 'e');
	/* Expect buffer = {-, -, f, g} */

	check_packet(&buffer, 'f');
	/* Expect buffer = {-, -, -, g} */

	check_packet(&buffer, 'g');
	/* Expect buffer = {-, -, -, -} */

	claimed = mpsc_pbuf_claim(&buffer);
	zassert_equal(claimed, NULL, NULL);
}

static void check_usage(struct mpsc_pbuf_buffer *buffer,
			uint32_t now, int exp_err, uint32_t max, uint32_t line)
{
	uint32_t size;
	uint32_t usage;
	int err;

	mpsc_pbuf_get_utilization(buffer, &size, &usage);
	zassert_equal(size / sizeof(int), buffer->size - 1, "%d: got:%d, exp:%d",
			line, size / sizeof(int), buffer->size - 1);
	zassert_equal(usage, now, "%d: got:%d, exp:%d", line, usage, now);

	err = mpsc_pbuf_get_max_utilization(buffer, &usage);
	zassert_equal(err, exp_err);
	if (err == 0) {
		zassert_equal(usage, max, "%d: got:%d, exp:%d", line, usage, max);
	}
}

#define CHECK_USAGE(buffer, now, max) \
	check_usage(buffer, (now) * sizeof(int), 0, (max) * sizeof(int), __LINE__)

static void ignore_drop(const struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic *item)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(item);
}

ZTEST(log_buffer, test_utilization)
{
	struct mpsc_pbuf_buffer buffer;
	struct mpsc_pbuf_buffer_config config = {
		.buf = buf32,
		.size = ARRAY_SIZE(buf32),
		.notify_drop = ignore_drop,
		.get_wlen = get_wlen,
		.flags = 0 /* Utilization not supported. */
	};

	mpsc_pbuf_init(&buffer, &config);

	check_usage(&buffer, 0, -ENOTSUP, 0, __LINE__);

	/* Initialize with max utilization support. */
	config.flags = MPSC_PBUF_MAX_UTILIZATION;
	mpsc_pbuf_init(&buffer, &config);

	CHECK_USAGE(&buffer, 0, 0);

	union test_item test_1word = {.data = {.valid = 1, .len = 1 }};
	union test_item test_ext_item = {
		.data = {
			.valid = 1,
			.len = PUT_EXT_LEN
		}
	};
	union test_item *t;

	mpsc_pbuf_put_word(&buffer, test_1word.item);

	CHECK_USAGE(&buffer, 1, 1);

	mpsc_pbuf_put_word_ext(&buffer, test_ext_item.item, NULL);

	CHECK_USAGE(&buffer, 1 + PUT_EXT_LEN, 1 + PUT_EXT_LEN);

	t = (union test_item *)mpsc_pbuf_claim(&buffer);

	zassert_true(t != NULL);
	CHECK_USAGE(&buffer, 1 + PUT_EXT_LEN, 1 + PUT_EXT_LEN);
	mpsc_pbuf_free(&buffer, &t->item);

	t = (union test_item *)mpsc_pbuf_claim(&buffer);
	zassert_true(t != NULL);

	CHECK_USAGE(&buffer, PUT_EXT_LEN, 1 + PUT_EXT_LEN);

	mpsc_pbuf_free(&buffer, &t->item);

	CHECK_USAGE(&buffer, 0, 1 + PUT_EXT_LEN);

	union test_item test_ext_item2 = {
		.data_ext = {
			.hdr = {
				.valid = 1,
				.len = PUT_EXT_LEN
			},
			.data = NULL
		}
	};

	mpsc_pbuf_put_data(&buffer, (uint32_t *)&test_ext_item2, PUT_EXT_LEN);

	CHECK_USAGE(&buffer, PUT_EXT_LEN, 1 + PUT_EXT_LEN);

	t = (union test_item *)mpsc_pbuf_claim(&buffer);
	zassert_true(t != NULL);
	mpsc_pbuf_free(&buffer, &t->item);

	CHECK_USAGE(&buffer, 0, 1 + PUT_EXT_LEN);

	memset(&buffer, 0, sizeof(buffer));
	/* Initialize to reset indexes. */
	mpsc_pbuf_init(&buffer, &config);

	struct test_data_var *packet;
	uint32_t len = 5;
	uint32_t i;

	for (i = 0; i < (buffer.size - 1) / len; i++) {
		packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len, K_NO_WAIT);
		packet->hdr.len = len;

		mpsc_pbuf_commit(&buffer, (union mpsc_pbuf_generic *)packet);
		CHECK_USAGE(&buffer, len * (i + 1), len * (i + 1));
	}

	packet = (struct test_data_var *)mpsc_pbuf_alloc(&buffer, len, K_NO_WAIT);

	zassert_true(packet == NULL);
}

/* Make sure that `mpsc_pbuf_alloc()` works in spinlock-held context when buf is not available */
ZTEST(log_buffer, test_alloc_in_spinlock)
{
	struct mpsc_pbuf_buffer buffer;
	struct test_data_var *packet;
	struct k_spinlock l = {0};

	init(&buffer, 32, false);

	/* Allocate all available buffer */
	packet = (struct test_data_var *)mpsc_pbuf_alloc(
		&buffer, 32, K_MSEC(10));
	zassert_not_null(packet);

	K_SPINLOCK(&l) {
		/* Try to allocate another buf */
		packet = (struct test_data_var *)mpsc_pbuf_alloc(
			&buffer, 32, K_MSEC(10));
		/* No buf is available this time */
		zassert_is_null(packet);
	}
}

/*test case main entry*/
ZTEST_SUITE(log_buffer, NULL, NULL, NULL, NULL, NULL);
