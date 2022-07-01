/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/smp.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <mgmt/mgmt.h>
#include "../../../../../subsys/mgmt/mcumgr/smp_reassembly.h"

static struct zephyr_smp_transport zst;
static uint8_t buff[CONFIG_MCUMGR_BUF_SIZE];
#define TEST_FRAME_SIZE 256

static struct net_buf *backup;

/* The function is called by zephyr_smp_reassembly_complete to pass a completed packet
 * for further processing; since there is nothing to process, this stub will only backup
 * buffer the pointer to allow a test case to free it with use of the mcumgr net_buf
 * management.
 */
void zephyr_smp_rx_req(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	backup = nb;
}

ZTEST(smp_reassembly, test_first)
{
	zephyr_smp_reassembly_init(&zst);
	struct mgmt_hdr *mh = (struct mgmt_hdr *)buff;
	int frag_used;
	int ret;
	int expected;

	/** First fragment errors **/
	/* Len longer than netbuf error */
	zassert_equal(-ENOSR, zephyr_smp_reassembly_collect(&zst, buff, CONFIG_MCUMGR_BUF_SIZE + 1),
		      "Expected -ENOSR error");
	/* Len not enough to read expected size from header */
	zassert_equal(-ENODATA,
		      zephyr_smp_reassembly_collect(&zst, buff, sizeof(struct mgmt_hdr) - 1),
		      "Expected -ENODATA error");
	/* Length extracted from header, plus size of header, is bigger than buffer */
	mh->nh_len = sys_cpu_to_be16(CONFIG_MCUMGR_BUF_SIZE - sizeof(struct mgmt_hdr) + 1);
	zassert_equal(-ENOSR,
		      zephyr_smp_reassembly_collect(&zst, buff, sizeof(struct mgmt_hdr) + 1),
		      "Expected -ENOSR error");

	/* Successfully alloc buffer */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct mgmt_hdr));
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used;
	ret = zephyr_smp_reassembly_collect(&zst, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	/* Force complete it, expected returned number of bytes missing */
	ret = zephyr_smp_reassembly_complete(&zst, true);
	zassert_equal(expected, ret,
		      "Forced completion ret %d, but expected was %d\n", ret, expected);

	/* Check fail due to lack of buffers: there is only one buffer and it already got passed
	 * for processing by complete
	 */
	ret = zephyr_smp_reassembly_collect(&zst, buff, frag_used);
	zassert_equal(-ENOMEM, ret,
		      "Expected is %d should be %d\n", ret, expected);

	/* This will normally be done by packet processing and should not be done by hand:
	 * release the buffer to the pool
	 */
	mcumgr_buf_free(backup);

}

ZTEST(smp_reassembly, test_drops)
{
	struct mgmt_hdr *mh = (struct mgmt_hdr *)buff;
	int frag_used;
	int ret;
	int expected;

	/* Collect one buffer and drop it */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct mgmt_hdr));
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used;
	ret = zephyr_smp_reassembly_collect(&zst, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	ret = zephyr_smp_reassembly_drop(&zst);
	zassert_equal(0, ret,
		      "Expected %d from drop, got %d", ret, expected);
}

ZTEST(smp_reassembly, test_collection)
{
	struct mgmt_hdr *mh = (struct mgmt_hdr *)buff;
	int pkt_used;
	int ret;
	int expected;
	int frag;
	void *p;

	for (int i = 0; i < ARRAY_SIZE(buff); i++) {
		buff[i] = (i % 255) + 1;
	}

	/** Collect fragments **/
	/* First fragment with header */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct mgmt_hdr));
	frag = 40;
	ret = zephyr_smp_reassembly_collect(&zst, buff, frag);
	expected = TEST_FRAME_SIZE - frag;
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);
	pkt_used = frag;

	/* Next fragment */
	frag = 40;
	ret = zephyr_smp_reassembly_collect(&zst, &buff[pkt_used], frag);
	pkt_used += frag;
	expected = TEST_FRAME_SIZE - pkt_used;
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	/* Try to complete incomplete, no force */
	ret = zephyr_smp_reassembly_complete(&zst, false);
	zassert_equal(-ENODATA, ret,
		      "Expected -ENODATA when completing incomplete buffer");

	/* Last fragment */
	ret = zephyr_smp_reassembly_collect(&zst, &buff[pkt_used], expected);
	zassert_equal(0, ret,
		      "Expected is %d should be %d\n", ret, 0);

	/* And overflow */
	ret = zephyr_smp_reassembly_collect(&zst, buff, 1);
	zassert_equal(-EOVERFLOW, ret,
		      "Expected -EOVERFLOW, got %d\n", ret);

	/* Complete successfully complete buffer */
	ret = zephyr_smp_reassembly_complete(&zst, false);
	zassert_equal(0, ret,
		      "Expected 0 from complete, got %d\n", ret);

	p = net_buf_pull_mem(backup, TEST_FRAME_SIZE);

	ret = memcmp(p, buff, TEST_FRAME_SIZE);
	zassert_equal(ret, 0, "Failed to assemble packet");

	/* This will normally be done by packet processing and should not be done by hand:
	 * release the buffer to the pool
	 */
	mcumgr_buf_free(backup);
}

ZTEST(smp_reassembly, test_no_packet_started)
{
	int ret;

	/** Complete on non-started packet **/
	ret = zephyr_smp_reassembly_complete(&zst, false);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL from complete, got %d", ret);
	ret = zephyr_smp_reassembly_complete(&zst, true);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL from complete, got %d", ret);

	/* Try to drop packet when there is none yet */
	ret = zephyr_smp_reassembly_drop(&zst);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL, there is no packet started yet");
}

ZTEST(smp_reassembly, test_ud)
{
	struct mgmt_hdr *mh = (struct mgmt_hdr *)buff;
	int frag_used;
	int ret;
	int expected;
	void *p;

	/* No packet started yet */
	p = zephyr_smp_reassembly_get_ud(&zst);
	zassert_equal(p, NULL, "Expect NULL ud poiner");

	/* After collecting first fragment */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE);
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used + sizeof(struct mgmt_hdr);
	ret = zephyr_smp_reassembly_collect(&zst, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	p = zephyr_smp_reassembly_get_ud(&zst);
	zassert_not_equal(p, NULL, "Expect non-NULL ud poiner");
	zephyr_smp_reassembly_drop(&zst);
}

ZTEST_SUITE(smp_reassembly, NULL, NULL, NULL, NULL, NULL);
