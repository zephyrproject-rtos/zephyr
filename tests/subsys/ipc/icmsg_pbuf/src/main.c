/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/ipc/icmsg_pbuf.h>
#include <zephyr/random/rand32.h>


#define MEM_AREA_SZ 256

static uint8_t memory_area[MEM_AREA_SZ] __aligned(32);

#define TEST_SPB_CFG_INIT(len, cache_line_sz, rd_idx_loc, wr_idx_loc, data)	\
{										\
	.len = len,								\
	.cache_line_sz = cache_line_sz,						\
	.rd_idx_loc = rd_idx_loc,						\
	.wr_idx_loc = wr_idx_loc,						\
	.data = data,								\
}

static void print_buf_info(struct icmsg_pbuf *ib)
{
	printk("----------stats start-----------\n");
	printk("cfg->rd_idx_loc: %p, val: %u\n", ib->cfg->rd_idx_loc, *(ib->cfg->rd_idx_loc));
	printk("cfg->wr_idx_loc: %p, val: %u\n", ib->cfg->wr_idx_loc, *(ib->cfg->wr_idx_loc));
	printk("cfg->data_loc:   %p\n", ib->cfg->data_loc);

	printk("data->rd_idx: %u\n", ib->data->rd_idx);
	printk("data->wr_idx: %u\n", ib->data->wr_idx);
	printk("-----------stats end------------\n");
}

ZTEST(test_icmsg_buf, test_icmsg_buf_init)
{
	/* Use API to define configurations. */
	static const struct icmsg_pbuf_cfg cfg0_ok =
		ICMSG_PBUF_CFG_INIT(memory_area, MEM_AREA_SZ, 32);
	static const struct icmsg_pbuf_cfg cfg1_ok =
		ICMSG_PBUF_CFG_INIT(memory_area, MEM_AREA_SZ, 0);
	static const struct icmsg_pbuf_cfg cfg2_nok =
		ICMSG_PBUF_CFG_INIT(memory_area+1, MEM_AREA_SZ-4, 4);
	static struct icmsg_pbuf_data ib_data;

	struct icmsg_pbuf ib = {
		.data = &ib_data,
	};

	uint8_t write_buf[MEM_AREA_SZ];
	for (size_t i = 0; i < MEM_AREA_SZ; i++) {
		write_buf[i] = i+1;
	}

	uint8_t read_buf[MEM_AREA_SZ] = {0};

	int ret;

	memset(memory_area, 0, sizeof(memory_area));

	ib.cfg = &cfg0_ok;
	zassert_equal(icmsg_pbuf_init(&ib), 0);

	ib.cfg = &cfg1_ok;
	zassert_equal(icmsg_pbuf_init(&ib), 0);

	ib.cfg = &cfg2_nok;
	zassert_equal(icmsg_pbuf_init(&ib), -EINVAL);

	ib.cfg = &cfg0_ok;
	zassert_equal(icmsg_pbuf_init(&ib), 0);

	ret = icmsg_pbuf_write(&ib, write_buf, 11);
	zassert_equal(ret, 11);

	ret = icmsg_pbuf_write(&ib, write_buf+5, 25);
	zassert_equal(ret, 25);

	ret = icmsg_pbuf_read(&ib, NULL, 0);
	zassert_equal(ret, 11);

	ret = icmsg_pbuf_read(&ib, read_buf, ret);
	zassert_equal(ret, 11);

	ret = icmsg_pbuf_read(&ib, NULL, 25);
	zassert_equal(ret, 25);

	ret = icmsg_pbuf_read(&ib, read_buf, ret);
	zassert_equal(ret, 25);

	ret = icmsg_pbuf_read(&ib, NULL, 0);
	zassert_equal(ret, 0);

	ret = icmsg_pbuf_write(&ib, write_buf, 212);
	zassert_equal(ret, 212);

	ret = icmsg_pbuf_read(&ib, read_buf, 212);
	zassert_equal(ret, 212);

	print_buf_info(&ib);
}

ZTEST_SUITE(test_icmsg_buf, NULL, NULL, NULL, NULL, NULL);
