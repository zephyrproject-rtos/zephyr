/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include "mgmt/mcumgr/transport/smp_reassembly.h"
#include "mgmt/mcumgr/transport/smp_internal.h"

#define TRANSPORT_NETBUF_SIZE CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE
static struct smp_transport smpt;
static uint8_t buff[TRANSPORT_NETBUF_SIZE];
#define TEST_FRAME_SIZE 256

static struct net_buf *backup;

/* The function is called by smp_reassembly_complete to pass a completed packet
 * for further processing; since there is nothing to process, this stub will only backup
 * buffer the pointer to allow a test case to free it with use of the mcumgr net_buf
 * management.
 */
void smp_rx_req(struct smp_transport *smpt, struct net_buf *nb)
{
	backup = nb;
}

ZTEST(smp_reassembly, test_first)
{
	smp_reassembly_init(&smpt);
	struct smp_hdr *mh = (struct smp_hdr *)buff;
	int frag_used;
	int ret;
	int expected;

	/** First fragment errors **/
	/* Len longer than netbuf error */
	zassert_equal(-ENOSR, smp_reassembly_collect(&smpt, buff, TRANSPORT_NETBUF_SIZE + 1),
		      "Expected -ENOSR error");
	/* Len not enough to read expected size from header */
	zassert_equal(-ENODATA,
		      smp_reassembly_collect(&smpt, buff, sizeof(struct smp_hdr) - 1),
		      "Expected -ENODATA error");
	/* Length extracted from header, plus size of header, is bigger than buffer */
	mh->nh_len = sys_cpu_to_be16(TRANSPORT_NETBUF_SIZE - sizeof(struct smp_hdr) + 1);
	zassert_equal(-ENOSR,
		      smp_reassembly_collect(&smpt, buff, sizeof(struct smp_hdr) + 1),
		      "Expected -ENOSR error");

	/* Successfully alloc buffer */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct smp_hdr));
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used;
	ret = smp_reassembly_collect(&smpt, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	/* Force complete it, expected returned number of bytes missing */
	ret = smp_reassembly_complete(&smpt, true);
	zassert_equal(expected, ret,
		      "Forced completion ret %d, but expected was %d\n", ret, expected);

	/* Check fail due to lack of buffers: there is only one buffer and it already got passed
	 * for processing by complete
	 */
	ret = smp_reassembly_collect(&smpt, buff, frag_used);
	zassert_equal(-ENOMEM, ret,
		      "Expected -ENOMEM, got %d\n", ret);

	/* This will normally be done by packet processing and should not be done by hand:
	 * release the buffer to the pool
	 */
	smp_packet_free(backup);
}

ZTEST(smp_reassembly, test_drops)
{
	struct smp_hdr *mh = (struct smp_hdr *)buff;
	int frag_used;
	int ret;
	int expected;

	/* Collect one buffer and drop it */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct smp_hdr));
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used;
	ret = smp_reassembly_collect(&smpt, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	ret = smp_reassembly_drop(&smpt);
	zassert_equal(0, ret,
		      "Expected %d from drop, got %d", ret, expected);
}

ZTEST(smp_reassembly, test_collection)
{
	struct smp_hdr *mh = (struct smp_hdr *)buff;
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
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE - sizeof(struct smp_hdr));
	frag = 40;
	ret = smp_reassembly_collect(&smpt, buff, frag);
	expected = TEST_FRAME_SIZE - frag;
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);
	pkt_used = frag;

	/* Next fragment */
	frag = 40;
	ret = smp_reassembly_collect(&smpt, &buff[pkt_used], frag);
	pkt_used += frag;
	expected = TEST_FRAME_SIZE - pkt_used;
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	/* Try to complete incomplete, no force */
	ret = smp_reassembly_complete(&smpt, false);
	zassert_equal(-ENODATA, ret,
		      "Expected -ENODATA when completing incomplete buffer");

	/* Last fragment */
	ret = smp_reassembly_collect(&smpt, &buff[pkt_used], expected);
	zassert_equal(0, ret,
		      "Expected 0, got %d\n", ret);

	/* And overflow */
	ret = smp_reassembly_collect(&smpt, buff, 1);
	zassert_equal(-EOVERFLOW, ret,
		      "Expected -EOVERFLOW, got %d\n", ret);

	/* Complete successfully complete buffer */
	ret = smp_reassembly_complete(&smpt, false);
	zassert_equal(0, ret,
		      "Expected 0 from complete, got %d\n", ret);

	p = net_buf_pull_mem(backup, TEST_FRAME_SIZE);

	ret = memcmp(p, buff, TEST_FRAME_SIZE);
	zassert_equal(ret, 0, "Failed to assemble packet");

	/* This will normally be done by packet processing and should not be done by hand:
	 * release the buffer to the pool
	 */
	smp_packet_free(backup);
}

ZTEST(smp_reassembly, test_no_packet_started)
{
	int ret;

	/** Complete on non-started packet **/
	ret = smp_reassembly_complete(&smpt, false);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL from complete, got %d", ret);
	ret = smp_reassembly_complete(&smpt, true);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL from complete, got %d", ret);

	/* Try to drop packet when there is none yet */
	ret = smp_reassembly_drop(&smpt);
	zassert_equal(-EINVAL, ret,
		      "Expected -EINVAL, there is no packet started yet");
}

ZTEST(smp_reassembly, test_ud)
{
	struct smp_hdr *mh = (struct smp_hdr *)buff;
	int frag_used;
	int ret;
	int expected;
	void *p;

	/* No packet started yet */
	p = smp_reassembly_get_ud(&smpt);
	zassert_equal(p, NULL, "Expect NULL ud poiner");

	/* After collecting first fragment */
	mh->nh_len = sys_cpu_to_be16(TEST_FRAME_SIZE);
	frag_used = 40;
	expected = TEST_FRAME_SIZE - frag_used + sizeof(struct smp_hdr);
	ret = smp_reassembly_collect(&smpt, buff, frag_used);
	zassert_equal(expected, ret,
		      "Expected is %d should be %d\n", ret, expected);

	p = smp_reassembly_get_ud(&smpt);
	zassert_not_equal(p, NULL, "Expect non-NULL ud poiner");
	smp_reassembly_drop(&smpt);
}

ZTEST_SUITE(smp_reassembly, NULL, NULL, NULL, NULL, NULL);
