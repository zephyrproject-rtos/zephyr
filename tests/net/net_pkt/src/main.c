/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include <ztest.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>

#if defined(CONFIG_NET_DEBUG_NET_PKT)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define NET_LOG_ENABLED 1
#else
#define DBG(fmt, ...)
#endif

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define LL_RESERVE 28
#define FRAG_COUNT 7

struct ipv6_hdr {
	u8_t vtc;
	u8_t tcflow;
	u16_t flow;
	u8_t len[2];
	u8_t nexthdr;
	u8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __packed;

struct udp_hdr {
	u16_t src_port;
	u16_t dst_port;
	u16_t len;
	u16_t chksum;
} __packed;

struct icmp_hdr {
	u8_t type;
	u8_t code;
	u16_t chksum;
} __packed;

static const char example_data[] =
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?";

static void test_ipv6_multi_frags(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int bytes, remaining = strlen(example_data), pos = 0;

	/* Example of multi fragment scenario with IPv6 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			zassert_true(false, "No space for IPv6 header");
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			zassert_true(false, "No space for UDP header");
		}

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp));
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	/* Put some data to rest of the fragments */
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE);
		zassert_true(false, "Invalid byte count");
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(example_data));
		zassert_true(false, "Still space");
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &example_data[pos], copy);

		DBG("Remaining %d left %d copy %d\n", remaining, bytes, copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			zassert_true(false, "There is still tailroom left");
		}

		net_pkt_frag_add(pkt, frag);
		if (remaining > 0) {
			frag = net_pkt_get_reserve_rx_data(LL_RESERVE,
							   K_FOREVER);
		}
	}

	bytes = net_pkt_get_len(pkt);
	if (bytes != strlen(example_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(example_data), bytes);
		zassert_true(false, "Invalid number of bytes");
	}

	/* Normally one should not unref the fragment list like this
	 * because it will leave the pkt->frags pointing to already
	 * freed fragment.
	 */
	net_pkt_frag_unref(pkt->frags);

	zassert_not_null(pkt->frags, "Frag list empty");

	pkt->frags = NULL; /* to prevent double free */

	net_pkt_unref(pkt);
}

static char buf_orig[200];
static char buf_copy[200];

static void linearize(struct net_pkt *pkt, char *buffer, int len)
{
	struct net_buf *frag;
	char *ptr = buffer;

	frag = pkt->frags;

	while (frag && len > 0) {

		memcpy(ptr, frag->data, frag->len);
		ptr += frag->len;
		len -= frag->len;

		frag = frag->frags;
	}
}

static void test_fragment_copy(void)
{
	struct net_pkt *pkt, *new_pkt;
	struct net_buf *frag, *new_frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	size_t orig_len, reserve;
	int pos;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(*ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			zassert_true(false, "No space for IPv6 header");
		}
		net_buf_add(frag, sizeof(*ipv6));

		if (net_buf_tailroom(frag) < sizeof(*udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			zassert_true(false, "No space for UDP header");
		}

		net_buf_add(frag, sizeof(*udp));

		memcpy(net_buf_add(frag, 15), example_data, 15);

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp) + 15);
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	orig_len = net_pkt_get_len(pkt);

	DBG("Total copy data len %zd\n", orig_len);

	linearize(pkt, buf_orig, orig_len);

	/* Then copy a fragment list to a new fragment list.
	 * Reserve some space in front of the buffers.
	 */
	reserve = sizeof(struct ipv6_hdr) + sizeof(struct icmp_hdr);
	new_frag = net_pkt_copy_all(pkt, reserve, K_FOREVER);
	zassert_not_null(new_frag, "Cannot copy fragment list");

	new_pkt = net_pkt_get_reserve_tx(0, K_FOREVER);
	new_pkt->frags = net_buf_frag_add(new_pkt->frags, new_frag);

	DBG("Total new data len %zd\n", net_pkt_get_len(new_pkt));

	if ((net_pkt_get_len(pkt) + reserve) != net_pkt_get_len(new_pkt)) {
		int diff;

		diff = net_pkt_get_len(new_pkt) - reserve -
			net_pkt_get_len(pkt);

		printk("Fragment list missing data, %d bytes not copied "
		       "(%zd vs %zd)\n", diff,
		       net_pkt_get_len(pkt) + reserve,
		       net_pkt_get_len(new_pkt));
		zassert_true(false, "Frag list missing");
	}

	if (net_pkt_get_len(new_pkt) != (orig_len + sizeof(struct ipv6_hdr) +
					   sizeof(struct icmp_hdr))) {
		printk("Fragment list missing data, new pkt len %zd "
		       "should be %zd\n", net_pkt_get_len(new_pkt),
		       orig_len + sizeof(struct ipv6_hdr) +
		       sizeof(struct icmp_hdr));
		zassert_true(false, "Frag list missing data");
	}

	linearize(new_pkt, buf_copy, sizeof(buf_copy));

	zassert_true(memcmp(buf_orig, buf_copy, sizeof(buf_orig)),
		     "Buffer copy failed, buffers are same");

	pos = memcmp(buf_orig, buf_copy + sizeof(struct ipv6_hdr) +
		     sizeof(struct icmp_hdr), orig_len);
	if (pos) {
		printk("Buffer copy failed at pos %d\n", pos);
		zassert_true(false, "Buf copy failed");
	}
}

/* Empty data and test data must be the same size in order the test to work */
static const char test_data[] = { '0', '1', '2', '3', '4',
				  '5', '6', '7' };

static const char sample_data[] =
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz";

static char test_rw_short[] =
	"abcdefghijklmnopqrstuvwxyz";

static char test_rw_long[] =
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz "
	"abcdefghijklmnopqrstuvwxyz ";

static void test_pkt_read_append(void)
{
	int remaining = strlen(sample_data);
	u8_t verify_rw_short[sizeof(test_rw_short)];
	u8_t verify_rw_long[sizeof(test_rw_long)];
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_buf *tfrag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	u8_t data[10];
	int pos = 0;
	int bytes;
	u16_t off;
	u16_t tpos;
	u16_t fail_pos;

	/* Example of multi fragment read, append and skip APS's */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			zassert_true(false, "No space for IPv6 header");
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			zassert_true(false, "No space for UDP header");
		}

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp));
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	/* Put some data to rest of the fragments */
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE);
		zassert_true(false, "Invalid number of bytes avail");
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(sample_data));
		zassert_true(false, "Not out of space");
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &sample_data[pos], copy);

		DBG("Remaining %d left %d copy %d\n", remaining, bytes, copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			zassert_true(false, "Still tailroom left");
		}

		net_pkt_frag_add(pkt, frag);
		if (remaining > 0) {
			frag = net_pkt_get_reserve_rx_data(LL_RESERVE,
							   K_FOREVER);
		}
	}

	bytes = net_pkt_get_len(pkt);
	if (bytes != strlen(sample_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(sample_data), bytes);
		zassert_true(false, "Message size wrong");
	}

	/* Failure cases */
	/* Invalid buffer */
	tfrag = net_frag_skip(NULL, 10, &fail_pos, 10);
	zassert_true(!tfrag && fail_pos == 0xffff, "Invalid case NULL buffer");

	/* Invalid: Skip more than a buffer length.*/
	tfrag = net_buf_frag_last(pkt->frags);
	tfrag = net_frag_skip(tfrag, tfrag->len - 1, &fail_pos, tfrag->len + 2);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len - 1, tfrag->len + 2, tfrag->len);
		zassert_true(false, "Invalid offset");
	}

	/* Invalid offset */
	tfrag = net_buf_frag_last(pkt->frags);
	tfrag = net_frag_skip(tfrag, tfrag->len + 10, &fail_pos, 10);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len + 10, 10, tfrag->len);
		zassert_true(false, "Invalid offset");
	}

	/* Valid cases */

	/* Offset is more than single fragment length */
	/* Get the first data fragment */
	tfrag = pkt->frags;
	tfrag = tfrag->frags;
	off = tfrag->len;
	tfrag = net_frag_read(tfrag, off + 10, &tpos, 10, data);
	if (!tfrag ||
	    memcmp(sample_data + off + 10, data, 10)) {
		printk("Failed to read from offset %d, frag length %d "
		       "read length %d\n",
		       tfrag->len + 10, tfrag->len, 10);
		zassert_true(false, "Fail offset read");
	}

	/* Skip till end of all fragments */
	/* Get the first data fragment */
	tfrag = pkt->frags;
	tfrag = tfrag->frags;
	tfrag = net_frag_skip(tfrag, 0, &tpos, strlen(sample_data));
	zassert_true(!tfrag && tpos == 0,
		     "Invalid skip till end of all fragments");

	/* Short data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append short data
	 * 3) Append short data again
	 * 4) Skip first short data from cached frag or offset
	 * 5) Read short data and compare
	 */
	tfrag = net_buf_frag_last(pkt->frags);
	off = tfrag->len;

	zassert_true(net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_short),
					test_rw_short, K_FOREVER),
		     "net_pkt_append failed");

	zassert_true(net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_short),
					test_rw_short, K_FOREVER),
		     "net_pkt_append failed");

	tfrag = net_frag_skip(tfrag, off, &tpos,
			     (u16_t)sizeof(test_rw_short));
	zassert_not_null(tfrag, "net_frag_skip failed");

	tfrag = net_frag_read(tfrag, tpos, &tpos,
			     (u16_t)sizeof(test_rw_short),
			     verify_rw_short);
	zassert_true(!tfrag && tpos == 0, "net_frag_read failed");
	zassert_false(memcmp(test_rw_short, verify_rw_short,
			     sizeof(test_rw_short)),
		      "net_frag_read failed with mismatch data");

	/* Long data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append long data
	 * 3) Append long data again
	 * 4) Skip first long data from cached frag or offset
	 * 5) Read long data and compare
	 */
	tfrag = net_buf_frag_last(pkt->frags);
	off = tfrag->len;

	zassert_true(net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_long),
					test_rw_long, K_FOREVER),
		     "net_pkt_append failed");

	zassert_true(net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_long),
					test_rw_long, K_FOREVER),
		     "net_pkt_append failed");

	tfrag = net_frag_skip(tfrag, off, &tpos,
			      (u16_t)sizeof(test_rw_long));
	zassert_not_null(tfrag, "net_frag_skip failed");

	tfrag = net_frag_read(tfrag, tpos, &tpos,
			     (u16_t)sizeof(test_rw_long),
			     verify_rw_long);
	zassert_true(!tfrag && tpos == 0, "net_frag_read failed");
	zassert_false(memcmp(test_rw_long, verify_rw_long,
			     sizeof(test_rw_long)),
		      "net_frag_read failed with mismatch data");

	net_pkt_unref(pkt);

	DBG("test_pkt_read_append passed\n");
}

static void test_pkt_read_write_insert(void)
{
	struct net_buf *read_frag;
	struct net_buf *temp_frag;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t read_data[100];
	u16_t read_pos;
	u16_t len;
	u16_t pos;

	/* Example of multi fragment read, append and skip APS's */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	/* 1) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data).
	 */
	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data, K_FOREVER);
	zassert_false(!frag || pos != 58, "Usecase 1: Write failed");

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 1: Read failed");

	zassert_false(memcmp(read_data, sample_data, 10),
		      "Usecase 1: Read data mismatch");

	/* 2) Write IPv6 and UDP header at offset 0. (Empty space is created
	 * already in Usecase 1, just need to fill the header, at this point
	 * there shouldn't be any length change).
	 */
	frag = net_pkt_write(pkt, frag, 0, &pos, NET_IPV6UDPH_LEN,
			     (u8_t *)sample_data, K_FOREVER);
	zassert_false(!frag || pos != 48, "Usecase 2: Write failed");

	read_frag = net_frag_read(frag, 0, &read_pos, NET_IPV6UDPH_LEN,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		     "Usecase 2: Read failed");

	zassert_false(memcmp(read_data, sample_data, NET_IPV6UDPH_LEN),
		      "Usecase 2: Read data mismatch");

	net_pkt_unref(pkt);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* 3) Offset is in next to next fragment.
	 * Write app data after 2 fragments. (If the offset far away, api will
	 * create empty fragments(space) till offset and write data).
	 */
	frag = net_pkt_write(pkt, pkt->frags, 200, &pos, 10,
			     (u8_t *)sample_data + 10, K_FOREVER);
	zassert_not_null(frag, "Usecase 3: Write failed");

	read_frag = net_frag_read(frag, pos - 10, &read_pos, 10,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		     "Usecase 3: Read failed");

	zassert_false(memcmp(read_data, sample_data + 10, 10),
		      "Usecase 3: Read data mismatch");

	/* 4) Offset is in next to next fragment (overwrite).
	 * Write app data after 2 fragments. (Space is already available from
	 * Usecase 3, this scenatio doesn't create any space, it just overwrites
	 * the existing data.
	 */
	frag = net_pkt_write(pkt, pkt->frags, 190, &pos, 10,
			     (u8_t *)sample_data, K_FOREVER);
	zassert_not_null(frag, "Usecase 4: Write failed");

	read_frag = net_frag_read(frag, pos - 10, &read_pos, 20,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 4: Read failed");

	zassert_false(memcmp(read_data, sample_data, 20),
		      "Usecase 4: Read data mismatch");

	net_pkt_unref(pkt);

	/* 5) Write 20 bytes in fragment which has only 10 bytes space.
	 *    API should overwrite on first 10 bytes and create extra 10 bytes
	 *    and write there.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	/* Create 10 bytes space. */
	net_buf_add(frag, 10);

	frag = net_pkt_write(pkt, frag, 0, &pos, 20, (u8_t *)sample_data,
			     K_FOREVER);
	zassert_false(!frag && pos != 20, "Usecase 5: Write failed");

	read_frag = net_frag_read(frag, 0, &read_pos, 20, read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		     "Usecase 5: Read failed");

	zassert_false(memcmp(read_data, sample_data, 20),
		      "USecase 5: Read data mismatch");

	net_pkt_unref(pkt);

	/* 6) First fragment is full, second fragment has 10 bytes tail room,
	 *    third fragment has 5 bytes.
	 *    Write data (30 bytes) in second fragment where offset is 10 bytes
	 *    before the tailroom.
	 *    So it should overwrite 10 bytes and create space for another 10
	 *    bytes and write data. Third fragment 5 bytes overwritten and space
	 *    for 5 bytes created.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len);

	/* 2nd fragment last 10 bytes tailroom, rest occupied */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len - 10);

	read_frag = temp_frag = frag;
	read_pos = frag->len - 10;

	/* 3rd fragment, only 5 bytes occupied */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	net_buf_add(frag, 5);

	temp_frag = net_pkt_write(pkt, temp_frag, temp_frag->len - 10, &pos,
				  30, (u8_t *) sample_data, K_FOREVER);
	zassert_not_null(temp_frag, "Use case 6: Write failed");

	read_frag = net_frag_read(read_frag, read_pos, &read_pos, 30,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 6: Read failed");

	zassert_false(memcmp(read_data, sample_data, 30),
		      "Usecase 6: Read data mismatch");

	net_pkt_unref(pkt);

	/* 7) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data.
	 */

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data + 10, K_FOREVER);
	zassert_false(!frag || pos != 58, "Usecase 7: Write failed");

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 7: Read failed");

	zassert_false(memcmp(read_data, sample_data + 10, 10),
		     "Usecase 7: Read data mismatch");

	zassert_true(net_pkt_insert(pkt, frag, NET_IPV6UDPH_LEN, 10,
				    (u8_t *)sample_data, K_FOREVER),
		     "Usecase 7: Insert failed");

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 20,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 7: Read after failed");

	zassert_false(memcmp(read_data, sample_data, 20),
		      "Usecase 7: Read data mismatch after insertion");

	/* Insert data outside input fragment length, error case. */
	zassert_false(net_pkt_insert(pkt, frag, 70, 10, (u8_t *)sample_data,
				     K_FOREVER),
		      "Usecase 7: False insert failed");

	net_pkt_unref(pkt);

	/* 8) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data. Insertion data is long which will
	 * take two fragments.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data + 60, K_FOREVER);
	zassert_false(!frag || pos != 58, "Usecase 8: Write failed");

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 8: Read failed");

	zassert_false(memcmp(read_data, sample_data + 60, 10),
		      "Usecase 8: Read data mismatch");

	zassert_true(net_pkt_insert(pkt, frag, NET_IPV6UDPH_LEN, 60,
				    (u8_t *)sample_data, K_FOREVER),
		     "Usecase 8: Insert failed");

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 70,
				 read_data);
	zassert_false(!read_frag && read_pos == 0xffff,
		      "Usecase 8: Read after failed");

	zassert_false(memcmp(read_data, sample_data, 70),
		      "Usecase 8: Read data mismatch after insertion");

	net_pkt_unref(pkt);

	DBG("test_pkt_read_write_insert passed\n");
}

static int calc_fragments(struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;
	int count = 0;

	while (frag) {
		frag = frag->frags;
		count++;
	}

	return count;
}

static bool net_pkt_is_compact(struct net_pkt *pkt)
{
	struct net_buf *frag, *last;
	size_t total = 0, calc;
	int count = 0;

	last = NULL;
	frag = pkt->frags;

	while (frag) {
		total += frag->len;
		count++;

		last = frag;
		frag = frag->frags;
	}

	NET_ASSERT(last);

	if (!last) {
		return false;
	}

	calc = count * (last->size) - net_buf_tailroom(last) -
		count * (net_buf_headroom(last));

	if (total == calc) {
		return true;
	}

	NET_DBG("Not compacted total %zu real %zu", total, calc);

	return false;
}

static void test_fragment_compact(void)
{
	struct net_pkt *pkt;
	struct net_buf *frags[FRAG_COUNT], *frag;
	int i, bytes, total, count;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = NULL;

	for (i = 0, total = 0; i < FRAG_COUNT; i++) {
		frags[i] = net_pkt_get_reserve_rx_data(12, K_FOREVER);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		/* Copy character test data in front of the fragment */
		memcpy(net_buf_add(frags[i], sizeof(test_data)),
		       test_data, sizeof(test_data));

		/* Followed by bytes of zeroes */
		(void)memset(net_buf_add(frags[i], sizeof(test_data)), 0,
			     sizeof(test_data));

		total++;
	}

	if (total != FRAG_COUNT) {
		printk("There should be %d fragments but was %d\n",
		       FRAG_COUNT, total);
		zassert_true(false, "Invalid fragment count");
	}

	DBG("step 1\n");

	pkt->frags = net_buf_frag_add(pkt->frags, frags[0]);

	bytes = net_pkt_get_len(pkt);
	if (bytes != FRAG_COUNT * sizeof(test_data) * 2) {
		printk("Compact test failed, fragments had %d bytes but "
		       "should have had %zd\n", bytes,
		       FRAG_COUNT * sizeof(test_data) * 2);
		zassert_true(false, "Invalid fragment bytes");
	}

	zassert_false(net_pkt_is_compact(pkt),
		      "The pkt is definitely not compact");

	DBG("step 2\n");

	net_pkt_compact(pkt);

	zassert_true(net_pkt_is_compact(pkt),
		     "The pkt should be in compact form");

	DBG("step 3\n");

	/* Try compacting again, nothing should happen */
	net_pkt_compact(pkt);

	zassert_true(net_pkt_is_compact(pkt),
		     "The pkt should be compacted now");

	total = calc_fragments(pkt);

	/* Add empty fragment at the end and compact, the last fragment
	 * should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	DBG("step 4\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 1)) {
		printk("Last fragment removal failed, chain should have %d "
		       "fragments but had %d\n", i-1, i);
		zassert_true(false, "Last frag rm fails");
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		zassert_true(false, "Frags missing");
	}

	/* Add two empty fragments at the end and compact, the last two
	 * fragment should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	DBG("step 5\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 2)) {
		printk("Last two fragment removal failed, chain should have "
		       "%d fragments but had %d\n", i-2, i);
		zassert_true(false, "Last two frag rm fails");
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		zassert_true(false, "Frags missing");
	}

	/* Add empty fragment at the beginning and at the end, and then
	 * compact, the two fragment should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_insert(pkt, frag);

	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	DBG("step 6\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 2)) {
		printk("Two fragment removal failed, chain should have "
		       "%d fragments but had %d\n", i-2, i);
		zassert_true(false, "Two frag rm fails");
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		zassert_true(false, "Frags missing");
	}

	DBG("test_fragment_compact passed\n");
}

static const char frag_data[512] = { 42 };
static void test_fragment_split(void)
{
	struct net_pkt *pkt;
	struct net_buf *rest;
	int ret;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);

	ret = net_pkt_append(pkt, 50, (u8_t *) frag_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	ret = net_pkt_split(pkt, pkt->frags, 10, &rest, K_FOREVER);

	zassert_false(ret, "Failed to split net_pkt at offset 10");
	zassert_false(!pkt->frags, "Failed to split net_pkt at offset 10");
	zassert_false(!rest, "Failed to split net_pkt at offset 10");

	net_pkt_unref(pkt);
	net_buf_unref(rest);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);

	ret = net_pkt_append(pkt, 100, (u8_t *) frag_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	ret = net_pkt_split(pkt, pkt->frags, 100, &rest, K_FOREVER);

	zassert_false(ret, "Failed to split net_pkt at offset 100");
	zassert_false(!pkt->frags,
		      "Failed to split net_pkt at offset 100");
	zassert_false(rest, "Failed to split net_pkt at offset 100");

	net_pkt_unref(pkt);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);

	ret = net_pkt_append(pkt, 100, (u8_t *) frag_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	ret = net_pkt_split(pkt, pkt->frags, 50, &rest, K_FOREVER);

	zassert_false(ret, "Failed to split net_pkt at offset 50");
	zassert_false(!pkt->frags,
		      "Failed to split net_pkt at offset 50");
	zassert_false(!rest, "Failed to split net_pkt at offset 50");

	zassert_false(net_pkt_get_len(pkt) != 50,
		      "Failed to split net_pkt at offset 50");
	zassert_false(net_buf_frags_len(rest) != 50,
		      "Failed to split net_pkt at offset 50");

	net_pkt_unref(pkt);
	net_buf_unref(rest);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);

	ret = net_pkt_append(pkt, 350, (u8_t *) frag_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	ret = net_pkt_split(pkt, pkt->frags, 150, &rest, K_FOREVER);

	zassert_false(ret, "Failed to split net_pkt at offset 150");
	zassert_false(!pkt->frags,
		      "Failed to split net_pkt at offset 150");
	zassert_false(!rest, "Failed to split net_pkt at offset 150");

	zassert_false(net_pkt_get_len(pkt) != 150,
		      "Failed to split net_pkt at offset 150");
	zassert_false(net_buf_frags_len(rest) != 200,
		      "Failed to split net_pkt at offset 150");

	net_pkt_unref(pkt);
	net_buf_unref(rest);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);

	ret = net_pkt_append(pkt, 512, (u8_t *) frag_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	ret = net_pkt_split(pkt, pkt->frags, 500, &rest, K_FOREVER);

	zassert_false(ret, "Failed to split net_pkt at offset 500");
	zassert_false(!pkt->frags,
		      "Failed to split net_pkt at offset 500");
	zassert_false(!rest, "Failed to split net_pkt at offset 500");

	zassert_false(net_pkt_get_len(pkt) != 500,
		      "Failed to split net_pkt at offset 500");
	zassert_false(net_buf_frags_len(rest) != 12,
		      "Failed to split net_pkt at offset 500");

	net_pkt_unref(pkt);
	net_buf_unref(rest);
}

static const char pull_test_data[] =
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz "
	"abcdefghijklmnopqrstuvxyz ";

static void test_pkt_pull(void)
{
	struct net_pkt *pkt;
	u16_t ret;
	int res;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	ret = net_pkt_append(pkt, sizeof(pull_test_data), (u8_t *)
			     pull_test_data, K_FOREVER);

	zassert_false(!ret, "Failed to append data");

	res = net_pkt_pull(pkt, 0, 10);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");

	res = net_pkt_pull(pkt, 10, 20);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");

	res = net_pkt_pull(pkt, 140, 150);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");

	res = net_pkt_pull(pkt, 42, 72);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");

	res = net_pkt_pull(pkt, 0, 42);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");

	res = net_pkt_pull(pkt, 138, 69);

	zassert_true(res == 0, "Failed to pull 10 bytes from offset 0");
}

static void test_net_pkt_append_memset(void)
{
	struct net_pkt *pkt;
	u8_t read_data[128];
	u16_t cur_pos;
	int ret;

	int size = sizeof(read_data);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	ret = net_pkt_append_memset(pkt, 50, 0, K_FOREVER);
	zassert_true(ret == 50, "Failed to append data");

	ret = net_pkt_append_memset(pkt, 128, 255, K_FOREVER);
	zassert_true(ret == 128, "Failed to append data");

	ret = net_pkt_append_memset(pkt, 128, 0, K_FOREVER);
	zassert_true(ret == 128, "Failed to append data");

	ret = net_frag_linearize(read_data, size, pkt, 0, 50);
	zassert_true(ret == 50, "Linearize failed failed");
	for (cur_pos = 0; cur_pos < 50; ++cur_pos) {
		zassert_true(read_data[cur_pos] == 0,
			     "Byte was expected to read 0");
	}

	ret = net_frag_linearize(read_data, size, pkt, 50, 128);
	zassert_true(ret == 128, "Linearize failed failed");
	for (cur_pos = 0; cur_pos < 128; ++cur_pos) {
		zassert_true(read_data[cur_pos] == 255,
			     "Byte was expected to read 255");
	}

	ret = net_frag_linearize(read_data, size, pkt, 50 + 128, 128);
	zassert_true(ret == 128, "Linearize failed failed");
	for (cur_pos = 0; cur_pos < 128; ++cur_pos) {
		zassert_true(read_data[cur_pos] == 0,
			     "Byte was expected to read 0");
	}

	net_pkt_unref(pkt);
}

void test_main(void)
{
	ztest_test_suite(net_pkt_tests,
			 ztest_unit_test(test_ipv6_multi_frags),
			 ztest_unit_test(test_fragment_copy),
			 ztest_unit_test(test_pkt_read_append),
			 ztest_unit_test(test_pkt_read_write_insert),
			 ztest_unit_test(test_fragment_compact),
			 ztest_unit_test(test_fragment_split),
			 ztest_unit_test(test_pkt_pull),
			 ztest_unit_test(test_net_pkt_append_memset)
			 );

	ztest_run_test_suite(net_pkt_tests);
}
