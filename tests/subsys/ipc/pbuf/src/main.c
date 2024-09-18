/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/ipc/pbuf.h>
#include <zephyr/random/random.h>


#define MEM_AREA_SZ	256
#define MPS		240
#define MSGA_SZ		11
#define MSGB_SZ		25

static char memory_area[MEM_AREA_SZ] __aligned(32);

static void print_pbuf_info(struct pbuf *pb)
{
	printk("----------stats start-----------\n");
	printk("cfg->rd_idx_loc: %p, val: %u\n", pb->cfg->rd_idx_loc, *(pb->cfg->rd_idx_loc));
	printk("cfg->wr_idx_loc: %p, val: %u\n", pb->cfg->wr_idx_loc, *(pb->cfg->wr_idx_loc));
	printk("cfg->data_loc:   %p\n", pb->cfg->data_loc);
	printk("cfg->len:              %u\n", pb->cfg->len);
	printk("cfg->dcache_alignment: %u\n", pb->cfg->dcache_alignment);

	printk("data.rd_idx: %u\n", pb->data.rd_idx);
	printk("data.wr_idx: %u\n", pb->data.wr_idx);
	printk("-----------stats end------------\n");
}

/* Read/write tests. */
ZTEST(test_pbuf, test_rw)
{
	uint8_t read_buf[MEM_AREA_SZ] = {0};
	uint8_t write_buf[MEM_AREA_SZ];
	int ret;

	BUILD_ASSERT(MSGA_SZ < MEM_AREA_SZ);
	BUILD_ASSERT(MSGB_SZ < MEM_AREA_SZ);
	BUILD_ASSERT(MPS < MEM_AREA_SZ);

	/* TODO: Use PBUF_DEFINE().
	 * The user should use PBUF_DEFINE() macro to define the buffer,
	 * however for the purpose of this test PBUF_CFG_INIT() is used in
	 * order to avoid clang complains about memory_area not being constant
	 * expression.
	 */
	static const struct pbuf_cfg cfg = PBUF_CFG_INIT(memory_area, MEM_AREA_SZ, 0);

	static struct pbuf pb = {
		.cfg = &cfg,
	};

	for (size_t i = 0; i < MEM_AREA_SZ; i++) {
		write_buf[i] = i+1;
	}

	zassert_equal(pbuf_tx_init(&pb), 0);

	/* Write MSGA_SZ bytes packet. */
	ret = pbuf_write(&pb, write_buf, MSGA_SZ);
	zassert_equal(ret, MSGA_SZ);

	/* Write MSGB_SZ bytes packet. */
	ret = pbuf_write(&pb, write_buf+MSGA_SZ, MSGB_SZ);
	zassert_equal(ret, MSGB_SZ);

	/* Get the number of bytes stored. */
	ret = pbuf_read(&pb, NULL, 0);
	zassert_equal(ret, MSGA_SZ);
	/* Attempt to read with too small read buffer. */
	ret = pbuf_read(&pb, read_buf, ret-1);
	zassert_equal(ret, -ENOMEM);
	/* Read the packet. */
	ret = pbuf_read(&pb, read_buf, ret);
	zassert_equal(ret, MSGA_SZ);
	/* Check data corectness. */
	zassert_mem_equal(read_buf, write_buf, ret);

	/* Get the number of bytes stored. */
	ret = pbuf_read(&pb, NULL, 0);
	zassert_equal(ret, MSGB_SZ);
	/* Read the packet. */
	ret = pbuf_read(&pb, read_buf, ret);
	zassert_equal(ret, MSGB_SZ);
	/* Check data corectness. */
	zassert_mem_equal(read_buf, write_buf+MSGA_SZ, ret);

	/* Get the number of bytes stored. */
	ret = pbuf_read(&pb, NULL, 0);
	zassert_equal(ret, 0);

	/* Write max packet size with wrapping around. */
	ret = pbuf_write(&pb, write_buf, MPS);
	zassert_equal(ret, MPS);
	/* Get the number of bytes stored. */
	ret = pbuf_read(&pb, NULL, 0);
	zassert_equal(ret, MPS);
	/* Read  max packet size with wrapp around. */
	ret = pbuf_read(&pb, read_buf, ret);
	zassert_equal(ret, MPS);
	/* Check data corectness. */
	zassert_mem_equal(write_buf, read_buf, MPS);
}

/* API ret codes tests. */
ZTEST(test_pbuf, test_retcodes)
{
	/* TODO: Use PBUF_DEFINE().
	 * The user should use PBUF_DEFINE() macro to define the buffer,
	 * however for the purpose of this test PBUF_CFG_INIT() is used in
	 * order to avoid clang complains about memory_area not being constant
	 * expression.
	 */
	static const struct pbuf_cfg cfg0 = PBUF_CFG_INIT(memory_area, MEM_AREA_SZ, 32);
	static const struct pbuf_cfg cfg1 = PBUF_CFG_INIT(memory_area, MEM_AREA_SZ, 0);
	static const struct pbuf_cfg cfg2 = PBUF_CFG_INIT(memory_area, 20, 4);

	static struct pbuf pb0 = {
		.cfg = &cfg0,
	};

	static struct pbuf pb1 = {
		.cfg = &cfg1,
	};

	static struct pbuf pb2 = {
		.cfg = &cfg2,
	};

	/* Initialize buffers. */
	zassert_equal(pbuf_tx_init(&pb0), 0);
	zassert_equal(pbuf_tx_init(&pb1), 0);
	zassert_equal(pbuf_tx_init(&pb2), 0);

	print_pbuf_info(&pb0);
	print_pbuf_info(&pb1);
	print_pbuf_info(&pb2);

	uint8_t read_buf[MEM_AREA_SZ];
	uint8_t write_buf[MEM_AREA_SZ];

	for (size_t i = 0; i < MEM_AREA_SZ; i++) {
		write_buf[i] = i+1;
	}

	/* pbuf_write incorrect params tests. */
	zassert_equal(pbuf_write(NULL, write_buf, 10), -EINVAL);
	zassert_equal(pbuf_write(&pb2, NULL, 10), -EINVAL);
	zassert_equal(pbuf_write(&pb2, write_buf, 0), -EINVAL);
	zassert_equal(pbuf_read(NULL, read_buf, 10), -EINVAL);

	/* Attempt to write more than the buffer can fit. */
	zassert_equal(pbuf_write(&pb2, write_buf, 5), -ENOMEM);

	/* Write maximal amount, the buffer fit. */
	zassert_equal(pbuf_write(&pb2, write_buf, 4), 4);

	/* Attempt to write to full buffer. */
	zassert_equal(pbuf_write(&pb2, write_buf, 1), -ENOMEM);

	/* Get the bytes stored. */
	zassert_equal(pbuf_read(&pb2, NULL, 1), 4);

	/* Attempt to read with too small read buffer. */
	zassert_equal(pbuf_read(&pb2, read_buf, 1), -ENOMEM);

	/* Get the bytes stored. */
	zassert_equal(pbuf_read(&pb2, NULL, 0), 4);

	/* Read the data with correct buffer size. */
	zassert_equal(pbuf_read(&pb2, read_buf, 4), 4);

	/* Check data correctness. */
	zassert_mem_equal(read_buf, write_buf, 4);

	/* Read from empty buffer. */
	zassert_equal(pbuf_read(&pb2, read_buf, 10), 0);
	zassert_equal(pbuf_read(&pb2, read_buf, 10), 0);
	zassert_equal(pbuf_read(&pb2, read_buf, 10), 0);
}

#define STRESS_LEN_MOD (44)
#define STRESS_LEN_MIN (20)
#define STRESS_LEN_MAX (STRESS_LEN_MIN + STRESS_LEN_MOD)

struct stress_data {
	struct pbuf *pbuf;
	uint32_t wr_cnt;
	uint32_t rd_cnt;
	uint32_t wr_err;
};

/* Check if buffer of len contains exp values. */
static int check_buffer(char *buf, uint16_t len, char exp)
{
	for (uint16_t i = 0; i < len; i++) {
		if (buf[i] != exp) {
			return -EINVAL;
		}
	}

	return 0;
}

bool stress_read(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	char buf[STRESS_LEN_MAX];
	int len;
	int rpt = (sys_rand8_get() & 3) + 1;

	for (int i = 0; i < rpt; i++) {
		len = pbuf_read(ctx->pbuf, buf, (uint16_t)sizeof(buf));
		if (len == 0) {
			return true;
		}

		if (len < 0) {
			zassert_true(false, "Unexpected error: %d, cnt:%d", len, ctx->rd_cnt);
		}

		zassert_ok(check_buffer(buf, len, ctx->rd_cnt));
		ctx->rd_cnt++;
	}

	return true;
}

bool stress_write(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct stress_data *ctx = (struct stress_data *)user_data;
	char buf[STRESS_LEN_MAX];

	uint16_t len = STRESS_LEN_MIN + (sys_rand8_get() % STRESS_LEN_MOD);
	int rpt = (sys_rand8_get() & 1) + 1;

	zassert_true(len < sizeof(buf));

	for (int i = 0; i < rpt; i++) {
		memset(buf, (uint8_t)ctx->wr_cnt, len);
		int ret = pbuf_write(ctx->pbuf, buf, len);

		if (ret == len) {
			ctx->wr_cnt++;
		} else if (ret == -ENOMEM) {
			ctx->wr_err++;
		} else {
			zassert_unreachable();
		}
	}

	return true;
}

ZTEST(test_pbuf, test_stress)
{
	static uint8_t buffer[MEM_AREA_SZ] __aligned(32);
	static struct stress_data ctx = {};
	uint32_t repeat = 0;

	/* TODO: Use PBUF_DEFINE().
	 * The user should use PBUF_DEFINE() macro to define the buffer,
	 * however for the purpose of this test PBUF_CFG_INIT() is used in
	 * order to avoid clang complains about buffer not being constant
	 * expression.
	 */
	static const struct pbuf_cfg cfg = PBUF_CFG_INIT(buffer, MEM_AREA_SZ, 4);

	static struct pbuf pb = {
		.cfg = &cfg,
	};

	zassert_equal(pbuf_tx_init(&pb), 0);
	ctx.pbuf = &pb;
	ctx.wr_cnt = 0;
	ctx.rd_cnt = 0;

	ztress_set_timeout(K_MSEC(1500));
	TC_PRINT("Reading from an interrupt, writing from a thread\n");
	ZTRESS_EXECUTE(ZTRESS_TIMER(stress_read, &ctx, repeat, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_write, &ctx, repeat, 2000, Z_TIMEOUT_TICKS(4)));
	TC_PRINT("Writes:%d unsuccessful: %d\n", ctx.wr_cnt, ctx.wr_err);

	TC_PRINT("Writing from an interrupt, reading from a thread\n");
	ZTRESS_EXECUTE(ZTRESS_TIMER(stress_write, &ctx, repeat, Z_TIMEOUT_TICKS(4)),
		       ZTRESS_THREAD(stress_read, &ctx, repeat, 1000, Z_TIMEOUT_TICKS(4)));
	TC_PRINT("Writes:%d unsuccessful: %d\n", ctx.wr_cnt, ctx.wr_err);
}

ZTEST_SUITE(test_pbuf, NULL, NULL, NULL, NULL, NULL);
