/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sys/spsc_pbuf.h>
#include <zephyr/random/random.h>

#define HDR_LEN sizeof(uint32_t)
#define TLEN(len) ROUND_UP(HDR_LEN + len, sizeof(uint32_t))
#define STRESS_TIMEOUT_MS ((CONFIG_SYS_CLOCK_TICKS_PER_SEC < 10000) ? 1000 : 15000)

/* The buffer size itself would be 199 bytes.
 * 212 - sizeof(struct spsc_pbuf) - 1 = 199.
 * -1 because internal rd/wr_idx is reserved to mean the buffer is empty.
 */
static bool use_cache(uint32_t flags)
{
	return IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_ALWAYS) ||
		(IS_ENABLED(CONFIG_SPSC_PBUF_CACHE_FLAG) && (flags & SPSC_PBUF_CACHE));
}

static void test_spsc_pbuf_flags(uint32_t flags)
{
	static uint8_t memory_area[216] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	static uint8_t rbuf[198];
	static uint8_t message[20] = {'a'};
	struct spsc_pbuf *ib;
	int rlen;
	int wlen;
	size_t capacity = (use_cache(flags) ?
		(sizeof(memory_area) - offsetof(struct spsc_pbuf, ext.cache.data)) :
		(sizeof(memory_area) - offsetof(struct spsc_pbuf, ext.nocache.data))) -
		sizeof(uint32_t);

	memset(memory_area, 0, sizeof(memory_area));
	ib = spsc_pbuf_init(memory_area, sizeof(memory_area), flags);
	zassert_equal_ptr(ib, memory_area, NULL);
	zassert_equal(spsc_pbuf_capacity(ib), capacity);

	/* Try writing invalid value. */
	rlen = spsc_pbuf_write(ib, rbuf, 0);
	zassert_equal(rlen, -EINVAL);
	rlen = spsc_pbuf_write(ib, rbuf, SPSC_PBUF_MAX_LEN);
	zassert_equal(rlen, -EINVAL);

	/* Try to write more than buffer can store. */
	rlen = spsc_pbuf_write(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, -ENOMEM);

	/* Read empty buffer. */
	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, 0);

	/* Single write and read. */
	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message));

	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message));

	ib = spsc_pbuf_init(memory_area, sizeof(memory_area), flags);
	zassert_equal_ptr(ib, memory_area, NULL);

	int repeat = capacity / (sizeof(message) + sizeof(uint32_t));

	for (int i = 0; i < repeat; i++) {
		wlen = spsc_pbuf_write(ib, message, sizeof(message));
		zassert_equal(wlen, sizeof(message));
	}

	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, -ENOMEM);

	/* Test reading with buf == NULL, should return len of the next message to read. */
	rlen = spsc_pbuf_read(ib, NULL, 0);
	zassert_equal(rlen, sizeof(message));

	/* Read with len == 0 and correct buf pointer. */
	rlen = spsc_pbuf_read(ib, rbuf, 0);
	zassert_equal(rlen, -ENOMEM);

	/* Read whole data from the buffer. */
	for (size_t i = 0; i < repeat; i++) {
		wlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
		zassert_equal(wlen, sizeof(message));
	}

	/* Buffer is empty */
	rlen = spsc_pbuf_read(ib, NULL, 0);
	zassert_equal(rlen, 0);

	/* Write message that would be wrapped around. */
	wlen = spsc_pbuf_write(ib, message, sizeof(message));
	zassert_equal(wlen, sizeof(message));

	/* Read wrapped message. */
	rlen = spsc_pbuf_read(ib, rbuf, sizeof(rbuf));
	zassert_equal(rlen, sizeof(message));
	zassert_equal(message[0], 'a');
}

ZTEST(test_spsc_pbuf, test_spsc_pbuf_ut)
{
	test_spsc_pbuf_flags(0);
}

ZTEST(test_spsc_pbuf, test_spsc_pbuf_ut_cache)
{
	test_spsc_pbuf_flags(SPSC_PBUF_CACHE);
}

static int check_buffer(char *buf, uint16_t len, char exp)
{
	for (uint16_t i = 0; i < len; i++) {
		if (buf[i] != exp) {
			return -EINVAL;
		}
	}

	return 0;
}

static void packet_write(struct spsc_pbuf *pb,
			 uint16_t len,
			 uint16_t outlen,
			 uint8_t id,
			 int exp_rv,
			 int line)
{
	int rv;
	char *buf;

	rv = spsc_pbuf_alloc(pb, len, &buf);
	zassert_equal(rv, exp_rv, "%d: Unexpected rv:%d (exp:%d)", line, rv, exp_rv);
	if (rv < 0) {
		return;
	}
	zassert_equal((uintptr_t)buf % sizeof(uint32_t), 0, "%d: Expected aligned buffer", line);
	zassert_true(rv >= outlen, "%d: Unexpected rv (bigger than %d)", line, rv, outlen);

	for (uint16_t i = 0; i < outlen; i++) {
		buf[i] = id + i;
	}

	if (outlen > 0) {
		spsc_pbuf_commit(pb, outlen);
	}
}
#define PACKET_WRITE(_pb, _len, _outlen, _id, _exp_rv) \
	packet_write(_pb, _len, _outlen, _id, _exp_rv, __LINE__)

static void packet_consume(struct spsc_pbuf *pb,
			   uint16_t exp_rv,
			   uint8_t exp_id,
			   int line)
{
	uint16_t rv;
	char *buf;

	rv = spsc_pbuf_claim(pb, &buf);
	zassert_equal(rv, exp_rv, "%d: Unexpected rv:%d (exp:%d)", line, rv, exp_rv);
	if (rv == 0) {
		return;
	}

	for (int i = 0; i < rv; i++) {
		zassert_equal(buf[i], exp_id + i, "%d: Unexpected value %d (exp:%d) at %d",
				line, buf[i], exp_id + i, i);
	}

	spsc_pbuf_free(pb, rv);
}

#define PACKET_CONSUME(_pb, _exp_rv, _exp_id) packet_consume(_pb, _exp_rv, _exp_id, __LINE__)

ZTEST(test_spsc_pbuf, test_0cpy)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	uint32_t capacity = spsc_pbuf_capacity(pb);
	uint16_t len1;
	uint16_t len2;

	/* Writing 0 length returns error. */
	PACKET_WRITE(pb, 0, 0, 0, -EINVAL);
	spsc_pbuf_commit(pb, 0);

	PACKET_WRITE(pb, SPSC_PBUF_MAX_LEN, 0, 0, capacity - sizeof(uint32_t));

	len1 = capacity - 8 - 2 * sizeof(uint32_t);
	PACKET_WRITE(pb, len1, len1, 0, len1);

	/* Remaining space. */
	len2 = capacity - TLEN(len1) - HDR_LEN;
	/* Request exceeding capacity*/
	PACKET_WRITE(pb, len2 + 1, 0, 1, len2);

	PACKET_WRITE(pb, len2, len2, 1, len2);

	/* Consume packets. */
	PACKET_CONSUME(pb, len1, 0);
	PACKET_CONSUME(pb, len2, 1);

	/* No more packets. */
	PACKET_CONSUME(pb, 0, 0);
}

ZTEST(test_spsc_pbuf, test_0cpy_smaller)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	uint32_t capacity = spsc_pbuf_capacity(pb);
	uint16_t len1;
	uint16_t len2;

	len1 = capacity - 10 - sizeof(uint16_t);
	PACKET_WRITE(pb, len1, len1 - 5, 0, len1);

	len2 = 10 - sizeof(uint16_t) - 1;
	PACKET_WRITE(pb, len2, len2, 1, len2);

	/* Consume packets. */
	PACKET_CONSUME(pb, len1 - 5, 0);
	PACKET_CONSUME(pb, len2, 1);
	PACKET_CONSUME(pb, 0, 0);
}

ZTEST(test_spsc_pbuf, test_0cpy_discard)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	uint32_t capacity = spsc_pbuf_capacity(pb);
	int len1, len2;

	len1 = 14;
	PACKET_WRITE(pb, len1, len1, 0, len1);

	len2 = capacity - TLEN(len1) - 10;
	PACKET_WRITE(pb, len2, len2, 1, len2);

	/* Consume first packet */
	PACKET_CONSUME(pb, len1, 0);

	/* Consume next packet. At this point buffer shall be completely empty. */
	PACKET_CONSUME(pb, len2, 1);

	/* Allocate but then discard by committing 0 length. Alloc will add padding. */
	PACKET_WRITE(pb, len1, 0, 0, len1);

	/* No packet in the buffer. */
	PACKET_CONSUME(pb, 0, 0);

	/* Buffer is empty except for the padding added by alloc. */
	len2 = len1 + len2 - sizeof(uint16_t);
	PACKET_WRITE(pb, len2, 0, 0, len2);
}

ZTEST(test_spsc_pbuf, test_0cpy_corner1)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb;
	uint32_t capacity;
	char *buf;
	uint16_t len;
	uint16_t len1;
	uint16_t len2;

	pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	capacity = spsc_pbuf_capacity(pb);

	/* Commit 5 byte packet. */
	len1 = 5;
	PACKET_WRITE(pb, len1, len1, 0, len1);

	/* Attempt to allocate packet till the end of the buffer. */
	len2 = capacity;
	len2 = spsc_pbuf_alloc(pb, len2, &buf);

	uint16_t exp_len2 = capacity - TLEN(len1) - HDR_LEN;

	zassert_equal(len2, exp_len2, "got %d, exp: %d", len2, exp_len2);

	len = spsc_pbuf_claim(pb, &buf);
	zassert_equal(len1, len);
	spsc_pbuf_free(pb, len);

	spsc_pbuf_commit(pb, len2);

	len = spsc_pbuf_claim(pb, &buf);
	zassert_equal(len2, len);
	spsc_pbuf_free(pb, len);
}

ZTEST(test_spsc_pbuf, test_0cpy_corner2)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb;
	uint32_t capacity;
	uint16_t len1;
	uint16_t len2;

	pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	capacity = spsc_pbuf_capacity(pb);

	/* Commit 16 byte packet. */
	len1 = 16;
	PACKET_WRITE(pb, len1, len1, 0, len1);

	/* Attempt to allocate packet that will leave 5 bytes at the end. */
	len2 = capacity - TLEN(len1) - HDR_LEN - 5;
	PACKET_WRITE(pb, len2, len2, 1, len2);

	/* Free first packet. */
	PACKET_CONSUME(pb, len1, 0);

	/* Allocate something that does not fit at the end. */
	len1 = 8;
	PACKET_WRITE(pb, len1, len1, 2, len1);

	/* There should be no place in the buffer now, only length field would fill.*/
	PACKET_WRITE(pb, 1, 0, 2, 0);

	/* Free second packet. */
	PACKET_CONSUME(pb, len2, 1);

	/* Get longest available. As now there is only one packet at the beginning
	 * that should be remaining space decremented by length fields.
	 */
	uint16_t exp_len = capacity - TLEN(len1) - HDR_LEN;

	PACKET_WRITE(pb, capacity, 0, 2, exp_len);
}

ZTEST(test_spsc_pbuf, test_largest_alloc)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb;
	uint32_t capacity;
	uint16_t len1;
	uint16_t len2;

	pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	capacity = spsc_pbuf_capacity(pb);

	len1 = 15;
	PACKET_WRITE(pb, len1, len1, 0, len1);
	PACKET_CONSUME(pb, len1, 0);

	len2 = capacity - TLEN(len1) - TLEN(10);
	PACKET_WRITE(pb, len2, len2, 1, len2);

	PACKET_WRITE(pb, SPSC_PBUF_MAX_LEN, 0, 1, 12);

	PACKET_WRITE(pb, SPSC_PBUF_MAX_LEN - 1, 0, 1, 12);

	pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	capacity = spsc_pbuf_capacity(pb);

	len1 = 15;
	PACKET_WRITE(pb, len1, len1, 0, len1);
	PACKET_CONSUME(pb, len1, 0);

	len2 = capacity - TLEN(len1) - TLEN(12);
	PACKET_WRITE(pb, len2, len2, 1, len2);

	PACKET_WRITE(pb, SPSC_PBUF_MAX_LEN - 1, 0, 1, 12);
}

ZTEST(test_spsc_pbuf, test_utilization)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	struct spsc_pbuf *pb;
	uint32_t capacity;
	uint16_t len1, len2, len3;
	int u;

	pb = spsc_pbuf_init(buffer, sizeof(buffer), 0);

	if (!IS_ENABLED(CONFIG_SPSC_PBUF_UTILIZATION)) {
		zassert_equal(spsc_pbuf_get_utilization(pb), -ENOTSUP);
		return;
	}
	capacity = spsc_pbuf_capacity(pb);

	len1 = 10;
	PACKET_WRITE(pb, len1, len1, 0, len1);
	u = spsc_pbuf_get_utilization(pb);
	zassert_equal(u, 0);

	PACKET_CONSUME(pb, len1, 0);
	u = spsc_pbuf_get_utilization(pb);
	zassert_equal(u, TLEN(len1));

	len2 = 11;
	PACKET_WRITE(pb, len2, len2, 1, len2);
	PACKET_CONSUME(pb, len2, 1);
	u = spsc_pbuf_get_utilization(pb);
	zassert_equal(u, TLEN(len2));

	len3 = capacity - TLEN(len1) - TLEN(len2);
	PACKET_WRITE(pb, SPSC_PBUF_MAX_LEN, len3, 2, len3);
	PACKET_CONSUME(pb, len3, 2);

	u = spsc_pbuf_get_utilization(pb);
	int exp_u = TLEN(len3);

	zassert_equal(u, exp_u);
}

struct stress_data {
	struct spsc_pbuf *pbuf;
	uint32_t capacity;
	uint32_t write_cnt;
	uint32_t read_cnt;
	uint32_t wr_err;
};

bool stress_read(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	char buf[128];
	int len;
	int rpt = (sys_rand32_get() & 3) + 1;

	for (int i = 0; i < rpt; i++) {
		len = spsc_pbuf_read(ctx->pbuf, buf, (uint16_t)sizeof(buf));
		if (len == 0) {
			return true;
		}

		if (len < 0) {
			zassert_true(false, "Unexpected error: %d, cnt:%d", len, ctx->read_cnt);
		}

		zassert_ok(check_buffer(buf, len, ctx->read_cnt));
		ctx->read_cnt++;
	}

	return true;
}

bool stress_write(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	char buf[128];
	uint16_t len = 1 + (sys_rand32_get() % (ctx->capacity / 4));
	int rpt = (sys_rand32_get() & 1) + 1;

	zassert_true(len < sizeof(buf), "len:%d %d", len, ctx->capacity);

	for (int i = 0; i < rpt; i++) {
		memset(buf, (uint8_t)ctx->write_cnt, len);
		if (spsc_pbuf_write(ctx->pbuf, buf, len) == len) {
			ctx->write_cnt++;
		} else {
			ctx->wr_err++;
		}
	}

	return true;
}

ZTEST(test_spsc_pbuf, test_stress)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	static struct stress_data ctx = {};
	uint32_t repeat = 0;

	ctx.pbuf = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	ctx.capacity = spsc_pbuf_capacity(ctx.pbuf);

	ztress_set_timeout(K_MSEC(STRESS_TIMEOUT_MS));
	TC_PRINT("Reading from an interrupt, writing from a thread\n");
	ZTRESS_EXECUTE(ZTRESS_TIMER(stress_read, &ctx, repeat, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_write, &ctx, repeat, 2000, Z_TIMEOUT_TICKS(4)));
	TC_PRINT("Writes:%d failures: %d\n", ctx.write_cnt, ctx.wr_err);

	TC_PRINT("Writing from an interrupt, reading from a thread\n");
	ZTRESS_EXECUTE(ZTRESS_TIMER(stress_write, &ctx, repeat, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_read, &ctx, repeat, 1000, Z_TIMEOUT_TICKS(4)));
	TC_PRINT("Writes:%d failures: %d\n", ctx.write_cnt, ctx.wr_err);
}

bool stress_claim_free(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	char *buf;
	uint16_t len;
	int rpt = sys_rand32_get() % 0x3;

	for (int i = 0; i < rpt; i++) {
		len = spsc_pbuf_claim(ctx->pbuf, &buf);

		if (len == 0) {
			return true;
		}

		zassert_ok(check_buffer(buf, len, ctx->read_cnt));

		spsc_pbuf_free(ctx->pbuf, len);

		ctx->read_cnt++;
	}

	return true;
}

bool stress_alloc_commit(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	uint32_t rnd = sys_rand32_get();
	uint16_t len = 1 + (rnd % (ctx->capacity / 4));
	int rpt = rnd % 0x3;
	char *buf;
	int err;

	for (int i = 0; i < rpt; i++) {
		err = spsc_pbuf_alloc(ctx->pbuf, len, &buf);
		zassert_true(err >= 0);
		if (err != len) {
			return true;
		}

		memset(buf, (uint8_t)ctx->write_cnt, len);

		spsc_pbuf_commit(ctx->pbuf, len);
		ctx->write_cnt++;
	}

	return true;
}

ZTEST(test_spsc_pbuf, test_stress_0cpy)
{
	static uint8_t buffer[128] __aligned(MAX(Z_SPSC_PBUF_DCACHE_LINE, 4));
	static struct stress_data ctx;
	uint32_t repeat = 0;

	ctx.write_cnt = 0;
	ctx.read_cnt = 0;
	ctx.pbuf = spsc_pbuf_init(buffer, sizeof(buffer), 0);
	ctx.capacity = spsc_pbuf_capacity(ctx.pbuf);

	ztress_set_timeout(K_MSEC(STRESS_TIMEOUT_MS));
	ZTRESS_EXECUTE(ZTRESS_THREAD(stress_claim_free, &ctx, repeat, 0, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_alloc_commit, &ctx, repeat, 1000, Z_TIMEOUT_TICKS(4)));

	ZTRESS_EXECUTE(ZTRESS_THREAD(stress_alloc_commit, &ctx, repeat, 0, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_claim_free, &ctx, repeat, 1000, Z_TIMEOUT_TICKS(4)));
}

ZTEST_SUITE(test_spsc_pbuf, NULL, NULL, NULL, NULL, NULL);
