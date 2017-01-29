/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include <tc_util.h>

#include <net/nbuf.h>
#include <net/net_ip.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define LL_RESERVE 28

struct ipv6_hdr {
	uint8_t vtc;
	uint8_t tcflow;
	uint16_t flow;
	uint8_t len[2];
	uint8_t nexthdr;
	uint8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __packed;

struct udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t chksum;
} __packed;

struct icmp_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
} __packed;

static const char example_data[] =
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?";

static int test_ipv6_multi_frags(void)
{
	struct net_buf *buf, *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int bytes, remaining = strlen(example_data), pos = 0;

	/* Example of multi fragment scenario with IPv6 */
	buf = net_nbuf_get_reserve_rx(0);
	frag = net_nbuf_get_reserve_data(LL_RESERVE);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_nbuf_set_appdata(buf, (void *)udp + sizeof(*udp));
		net_nbuf_set_appdatalen(buf, 0);
	}

	net_buf_frag_add(buf, frag);

	/* Put some data to rest of the fragments */
	frag = net_nbuf_get_reserve_data(LL_RESERVE);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_NBUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_NBUF_DATA_SIZE - LL_RESERVE);
		return -EINVAL;
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(example_data));
		return -EINVAL;
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &example_data[pos], copy);

		printk("Remaining %d left %d copy %d\n", remaining, bytes,
		       copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			return -EINVAL;
		}

		net_buf_frag_add(buf, frag);
		if (remaining > 0) {
			frag = net_nbuf_get_reserve_data(LL_RESERVE);
		}
	}

	bytes = net_buf_frags_len(buf->frags);
	if (bytes != strlen(example_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(example_data), bytes);
		return -EINVAL;
	}

	/* Normally one should not unref the fragment list like this
	 * because it will leave the buf->frags pointing to already
	 * freed fragment.
	 */
	net_nbuf_unref(buf->frags);
	if (!buf->frags) {
		printk("Fragment list should not be empty.\n");
		return -EINVAL;
	}
	buf->frags = NULL; /* to prevent double free */

	net_nbuf_unref(buf);

	return 0;
}

static char buf_orig[200];
static char buf_copy[200];

static void linearize(struct net_buf *buf, char *buffer, int len)
{
	char *ptr = buffer;

	buf = buf->frags;

	while (buf && len > 0) {

		memcpy(ptr, buf->data, buf->len);
		ptr += buf->len;
		len -= buf->len;

		buf = buf->frags;
	}
}

static int test_fragment_copy(void)
{
	struct net_buf *buf, *frag, *new_buf, *new_frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	size_t orig_len;
	int pos;

	buf = net_nbuf_get_reserve_rx(0);
	frag = net_nbuf_get_reserve_data(LL_RESERVE);

	/* Place the IP + UDP header in the first fragment */
	if (net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(*ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(*ipv6));

		if (net_buf_tailroom(frag) < sizeof(*udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_buf_add(frag, sizeof(*udp));

		memcpy(net_buf_add(frag, 15), example_data, 15);

		net_nbuf_set_appdata(buf, (void *)udp + sizeof(*udp) + 15);
		net_nbuf_set_appdatalen(buf, 0);
	}

	net_buf_frag_add(buf, frag);

	orig_len = net_buf_frags_len(buf);

	printk("Total copy data len %zd\n", orig_len);

	linearize(buf, buf_orig, orig_len);

	/* Then copy a fragment list to a new fragment list */
	new_frag = net_nbuf_copy_all(buf->frags, sizeof(struct ipv6_hdr) +
				     sizeof(struct icmp_hdr));
	if (!new_frag) {
		printk("Cannot copy fragment list.\n");
		return -EINVAL;
	}

	new_buf = net_nbuf_get_reserve_tx(0);
	net_buf_frag_add(new_buf, new_frag);

	printk("Total new data len %zd\n", net_buf_frags_len(new_buf));

	if (net_buf_frags_len(buf) != 0) {
		printk("Fragment list missing data, %zd bytes not copied\n",
		       net_buf_frags_len(buf));
		return -EINVAL;
	}

	if (net_buf_frags_len(new_buf) != (orig_len + sizeof(struct ipv6_hdr) +
					   sizeof(struct icmp_hdr))) {
		printk("Fragment list missing data, new buf len %zd "
		       "should be %zd\n", net_buf_frags_len(new_buf),
		       orig_len + sizeof(struct ipv6_hdr) +
		       sizeof(struct icmp_hdr));
		return -EINVAL;
	}

	linearize(new_buf, buf_copy, sizeof(buf_copy));

	if (!memcmp(buf_orig, buf_copy, sizeof(buf_orig))) {
		printk("Buffer copy failed, buffers are same!\n");
		return -EINVAL;
	}

	pos = memcmp(buf_orig, buf_copy + sizeof(struct ipv6_hdr) +
		     sizeof(struct icmp_hdr), orig_len);
	if (pos) {
		printk("Buffer copy failed at pos %d\n", pos);
		return -EINVAL;
	}

	return 0;
}

/* Empty data and test data must be the same size in order the test to work */
static const char test_data[] = { '0', '1', '2', '3', '4',
				  '5', '6', '7' };
static const char empty_data[] = { 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x00, 0x00 };

static void hexdump(const char *str, const uint8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}

static int test_fragment_push(void)
{
#define FRAG_COUNT 7
	struct net_buf *buf, *frags[FRAG_COUNT], *frag;
	uint8_t *ptr;
	int i, bytes;

	buf = net_nbuf_get_reserve_rx(0);
	frag = NULL;

	for (i = 0; i < FRAG_COUNT; i++) {
		frags[i] = net_nbuf_get_reserve_data(12);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		/* Copy character test data in front of the fragment */
		memcpy(net_buf_add(frags[i], sizeof(test_data)),
		       test_data, sizeof(test_data));

		/* Followed by bytes of zeroes */
		memset(net_buf_add(frags[i], sizeof(test_data)), 0,
		       sizeof(test_data));
	}

	net_buf_frag_add(buf, frags[0]);

	bytes = net_buf_frags_len(buf);
	if (bytes != FRAG_COUNT * sizeof(test_data) * 2) {
		printk("Push test failed, fragments had %d bytes but "
		       "should have had %zd\n", bytes,
		       FRAG_COUNT * sizeof(test_data) * 2);
		return -1;
	}

	if (net_nbuf_is_compact(buf->frags)) {
		printk("The buf->frags is not compact. Test fails\n");
		return -1;
	}

	if (net_nbuf_is_compact(buf)) {
		printk("The buf is definitely not compact. Test fails\n");
		return -1;
	}

	buf = net_nbuf_compact(buf);

	if (!net_nbuf_is_compact(buf)) {
		printk("The buf should be in compact form. Test fails\n");
		return -1;
	}

	/* Try compacting again, nothing should happen */
	buf = net_nbuf_compact(buf);

	if (!net_nbuf_is_compact(buf)) {
		printk("The buf should be compacted now. Test fails\n");
		return -1;
	}

	buf = net_nbuf_push(buf, buf->frags, sizeof(empty_data));
	if (!buf) {
		printk("push test failed, even with fragment pointer\n");
		return -1;
	}

	/* Clear the just allocated area */
	memcpy(buf->frags->data, empty_data, sizeof(empty_data));

	/* The data should look now something like this (frag->data will point
	 * into this part).
	 *     empty
	 *     test data
	 *     empty
	 *     test data
	 *     empty
	 *     test data
	 *     empty
	 *     test data
	 *     empty
	 *     test data
	 *     empty
	 *
	 * Then the second fragment:
	 *     test data
	 *     empty
	 *     test data
	 *     empty
	 */

	frag = buf->frags;

	hexdump("frag 1", frag->data, frag->len);

	ptr = frag->data;

	for (i = 0; i < frag->len / (sizeof(empty_data) * 2); i++) {
		if (memcmp(ptr, empty_data, sizeof(empty_data))) {
			printk("%d: No empty data at pos %p\n",
			       __LINE__, ptr);
			return -1;
		}

		ptr += sizeof(empty_data);

		if (memcmp(ptr, test_data, sizeof(test_data))) {
			printk("%d: No test data at pos %p\n",
			       __LINE__, ptr);
			return -1;
		}

		ptr += sizeof(empty_data);
	}

	/* One empty data at the end of first fragment */
	if (memcmp(ptr, empty_data, sizeof(empty_data))) {
		printk("%d: No empty data at pos %p\n",
		       __LINE__, ptr);
		return -1;
	}

	frag = frag->frags;

	hexdump("frag 2", frag->data, frag->len);

	ptr = frag->data;

	for (i = 0; i < frag->len / (sizeof(empty_data) * 2); i++) {
		if (memcmp(ptr, test_data, sizeof(test_data))) {
			printk("%d: No test data at pos %p\n",
			       __LINE__, ptr);
			return -1;
		}

		ptr += sizeof(test_data);

		if (memcmp(ptr, empty_data, sizeof(empty_data))) {
			printk("%d: No empty data at pos %p\n",
			       __LINE__, ptr);
			return -1;
		}

		ptr += sizeof(empty_data);
	}

	net_nbuf_unref(buf);

	return 0;
}

static int test_fragment_pull(void)
{
	struct net_buf *buf, *newbuf, *frags[FRAG_COUNT], *frag;
	int i, bytes_before, bytes_after, amount = 10, bytes_before2;

	buf = net_nbuf_get_reserve_tx(0);
	frag = NULL;

	for (i = 0; i < FRAG_COUNT; i++) {
		frags[i] = net_nbuf_get_reserve_data(12);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		/* Copy character test data in front of the fragment */
		memcpy(net_buf_add(frags[i], sizeof(test_data)),
		       test_data, sizeof(test_data));
	}

	net_buf_frag_add(buf, frags[0]);

	bytes_before = net_buf_frags_len(buf);

	newbuf = net_nbuf_pull(buf, amount / 2);
	if (newbuf != buf) {
		printk("First fragment is wrong\n");
		return -1;
	}

	bytes_after = net_buf_frags_len(buf);
	if (bytes_before != (bytes_after + amount / 2)) {
		printk("Wrong amount of data in fragments, should be %d "
		       "but was %d\n", bytes_before - amount / 2, bytes_after);
		return -1;
	}

	newbuf = net_nbuf_pull(buf, amount);
	if (newbuf != buf) {
		printk("First fragment is wrong\n");
		return -1;
	}

	newbuf = net_nbuf_pull(buf, amount * 100);
	if (newbuf != buf) {
		printk("First fragment is wrong\n");
		return -1;
	}

	bytes_after = net_buf_frags_len(buf);
	if (bytes_after != 0) {
		printk("Fragment list should be empty (left %d bytes)\n",
		       bytes_after);
		return -1;
	}

	net_nbuf_unref(buf);

	/* Trying without TX or RX buf as a first element */
	frags[0] = net_nbuf_get_reserve_data(12);
	frag = frags[0];
	memcpy(net_buf_add(frags[0], sizeof(test_data)),
	       test_data, sizeof(test_data));

	for (i = 1; i < FRAG_COUNT; i++) {
		frags[i] = net_nbuf_get_reserve_data(12);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		memcpy(net_buf_add(frags[i], sizeof(test_data)),
		       test_data, sizeof(test_data));
	}

	buf = frags[0];

	bytes_before2 = net_buf_frags_len(buf);

	if (bytes_before != bytes_before2) {
		printk("Invalid number of bytes in fragments (%d vs %d)\n",
		       bytes_before, bytes_before2);
		return -1;
	}

	bytes_before = net_buf_frags_len(buf);

	newbuf = net_nbuf_pull(buf, amount / 2);
	if (newbuf != buf) {
		printk("First fragment is wrong\n");
		return -1;
	}

	bytes_after = net_buf_frags_len(buf);
	if (bytes_before != (bytes_after + amount / 2)) {
		printk("Wrong amount of data in fragments2, should be %d "
		       "but was %d\n", bytes_before - amount / 2, bytes_after);
		return -1;
	}

	newbuf = net_nbuf_pull(buf, amount);
	if (newbuf == buf || newbuf != frags[1]) {
		printk("First fragment2 is wrong\n");
		return -1;
	}

	newbuf = net_nbuf_pull(buf, amount * 100);
	if (newbuf == buf || newbuf != NULL) {
		printk("First fragment2 is not correct\n");
		return -1;
	}

	return 0;
}

static const char sample_data[] =
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz";

static char test_rw_short[] =
	"abcdefghijklmnopqrstuvwxyz";

static char test_rw_long[] =
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz";

static int test_nbuf_read_append(void)
{
	int remaining = strlen(sample_data);
	uint8_t verify_rw_short[sizeof(test_rw_short)];
	uint8_t verify_rw_long[sizeof(test_rw_long)];
	struct net_buf *buf;
	struct net_buf *frag;
	struct net_buf *tfrag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	uint8_t data[10];
	int pos = 0;
	int bytes;
	uint16_t off;
	uint16_t tpos;
	uint16_t fail_pos;

	/* Example of multi fragment read, append and skip APS's */
	buf = net_nbuf_get_reserve_rx(0);
	frag = net_nbuf_get_reserve_data(LL_RESERVE);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_nbuf_set_appdata(buf, (void *)udp + sizeof(*udp));
		net_nbuf_set_appdatalen(buf, 0);
	}

	net_buf_frag_add(buf, frag);

	/* Put some data to rest of the fragments */
	frag = net_nbuf_get_reserve_data(LL_RESERVE);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_NBUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_NBUF_DATA_SIZE - LL_RESERVE);
		return -EINVAL;
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(sample_data));
		return -EINVAL;
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &sample_data[pos], copy);

		printk("Remaining %d left %d copy %d\n", remaining, bytes,
		       copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			return -EINVAL;
		}

		net_buf_frag_add(buf, frag);
		if (remaining > 0) {
			frag = net_nbuf_get_reserve_data(LL_RESERVE);
		}
	}

	bytes = net_buf_frags_len(buf->frags);
	if (bytes != strlen(sample_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(sample_data), bytes);
		return -EINVAL;
	}

	/* Failure cases */
	/* Invalid buffer */
	tfrag = net_nbuf_skip(NULL, 10, &fail_pos, 10);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case NULL buffer\n");
		return -EINVAL;
	}

	/* Invalid: Skip more than a buffer length.*/
	tfrag = net_buf_frag_last(buf->frags);
	tfrag = net_nbuf_skip(tfrag, tfrag->len - 1, &fail_pos, tfrag->len + 2);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len - 1, tfrag->len + 2, tfrag->len);
		return -EINVAL;
	}

	/* Invalid offset */
	tfrag = net_buf_frag_last(buf->frags);
	tfrag = net_nbuf_skip(tfrag, tfrag->len + 10, &fail_pos, 10);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len + 10, 10, tfrag->len);
		return -EINVAL;
	}

	/* Valid cases */

	/* Offset is more than single fragment length */
	/* Get the first data fragment */
	tfrag = buf->frags;
	tfrag = tfrag->frags;
	off = tfrag->len;
	tfrag = net_nbuf_read(tfrag, off + 10, &tpos, 10, data);
	if (!tfrag ||
	    memcmp(sample_data + off + 10, data, 10)) {
		printk("Failed to read from offset %d, frag length %d"
		       "read length %d\n",
		       tfrag->len + 10, tfrag->len, 10);
		return -EINVAL;
	}

	/* Skip till end of all fragments */
	/* Get the first data fragment */
	tfrag = buf->frags;
	tfrag = tfrag->frags;
	tfrag = net_nbuf_skip(tfrag, 0, &tpos, strlen(sample_data));
	if (!(!tfrag && tpos == 0)) {
		printk("Invalid skip till end of all fragments");
		return -EINVAL;
	}

	/* Short data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append short data
	 * 3) Append short data again
	 * 4) Skip first short data from cached frag or offset
	 * 5) Read short data and compare
	 */
	tfrag = net_buf_frag_last(buf->frags);
	off = tfrag->len;

	if (!net_nbuf_append(buf, sizeof(test_rw_short), test_rw_short)) {
		printk("net_nbuf_append failed\n");
		return -EINVAL;
	}

	if (!net_nbuf_append(buf, sizeof(test_rw_short), test_rw_short)) {
		printk("net_nbuf_append failed\n");
		return -EINVAL;
	}

	tfrag = net_nbuf_skip(tfrag, off, &tpos, sizeof(test_rw_short));
	if (!tfrag) {
		printk("net_nbuf_skip failed\n");
		return -EINVAL;
	}

	tfrag = net_nbuf_read(tfrag, tpos, &tpos, sizeof(test_rw_short),
			      verify_rw_short);
	if (memcmp(test_rw_short, verify_rw_short, sizeof(test_rw_short))) {
		printk("net_nbuf_read failed with mismatch data");
		return -EINVAL;
	}

	/* Long data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append long data
	 * 3) Append long data again
	 * 4) Skip first long data from cached frag or offset
	 * 5) Read long data and compare
	 */
	tfrag = net_buf_frag_last(buf->frags);
	off = tfrag->len;

	if (!net_nbuf_append(buf, sizeof(test_rw_long), test_rw_long)) {
		printk("net_nbuf_append failed\n");
		return -EINVAL;
	}

	if (!net_nbuf_append(buf, sizeof(test_rw_long), test_rw_long)) {
		printk("net_nbuf_append failed\n");
		return -EINVAL;
	}

	/* Try to pass fragment to net_nbuf_append(), this should fail
	 * as we always need to pass the first buf into it.
	 */
	if (net_nbuf_append(buf->frags, sizeof(test_rw_short), test_rw_short)) {
		printk("net_nbuf_append succeed but should have failed\n");
		return -EINVAL;
	}

	tfrag = net_nbuf_skip(tfrag, off, &tpos, sizeof(test_rw_long));
	if (!tfrag) {
		printk("net_nbuf_skip failed\n");
		return -EINVAL;
	}

	tfrag = net_nbuf_read(tfrag, tpos, &tpos, sizeof(test_rw_long),
			      verify_rw_long);
	if (memcmp(test_rw_long, verify_rw_long, sizeof(test_rw_long))) {
		printk("net_nbuf_read failed with mismatch data");
		return -EINVAL;
	}

	net_nbuf_unref(buf);

	return 0;
}

static int test_nbuf_read_write_insert(void)
{
	struct net_buf *read_frag;
	struct net_buf *temp_frag;
	struct net_buf *buf;
	struct net_buf *frag;
	uint8_t read_data[100];
	uint16_t read_pos;
	uint16_t len;
	uint16_t pos;

	/* Example of multi fragment read, append and skip APS's */
	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	/* 1) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data).
	 */
	frag = net_nbuf_write(buf, frag, NET_IPV6UDPH_LEN, &pos, 10,
			      (uint8_t *)sample_data);
	if (!frag || pos != 58) {
		printk("Usecase 1: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 1: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 10)) {
		printk("Usecase 1: Read data mismatch\n");
		return -EINVAL;
	}

	/* 2) Write IPv6 and UDP header at offset 0. (Empty space is created
	 * already in Usecase 1, just need to fill the header, at this point
	 * there shouldn't be any length change).
	 */
	frag = net_nbuf_write(buf, frag, 0, &pos, NET_IPV6UDPH_LEN,
			      (uint8_t *)sample_data);
	if (!frag || pos != 48) {
		printk("Usecase 2: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, 0, &read_pos, NET_IPV6UDPH_LEN,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 2: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, NET_IPV6UDPH_LEN)) {
		printk("Usecase 2: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	/* 3) Offset is in next to next fragment.
	 * Write app data after 2 fragments. (If the offset far away, api will
	 * create empty fragments(space) till offset and write data).
	 */
	frag = net_nbuf_write(buf, buf->frags, 200, &pos, 10,
			      (uint8_t *)sample_data + 10);
	if (!frag) {
		printk("Usecase 3: Write failed");
	}

	read_frag = net_nbuf_read(frag, pos - 10, &read_pos, 10,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 3: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 10, 10)) {
		printk("Usecase 3: Read data mismatch\n");
		return -EINVAL;
	}

	/* 4) Offset is in next to next fragment (overwrite).
	 * Write app data after 2 fragments. (Space is already available from
	 * Usecase 3, this scenatio doesn't create any space, it just overwrites
	 * the existing data.
	 */
	frag = net_nbuf_write(buf, buf->frags, 190, &pos, 10,
			      (uint8_t *)sample_data);
	if (!frag) {
		printk("Usecase 4: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, pos - 10, &read_pos, 20,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 4: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("Usecase 4: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	/* 5) Write 20 bytes in fragment which has only 10 bytes space.
	 *    API should overwrite on first 10 bytes and create extra 10 bytes
	 *    and write there.
	 */
	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	/* Create 10 bytes space. */
	net_buf_add(frag, 10);

	frag = net_nbuf_write(buf, frag, 0, &pos, 20, (uint8_t *)sample_data);
	if (!frag && pos != 20) {
		printk("Usecase 5: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, 0, &read_pos, 20, read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 5: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("USecase 5: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	/* 6) First fragment is full, second fragment has 10 bytes tail room,
	 *    third fragment has 5 bytes.
	 *    Write data (30 bytes) in second fragment where offset is 10 bytes
	 *    before the tailroom.
	 *    So it should overwrite 10 bytes and create space for another 10
	 *    bytes and write data. Third fragment 5 bytes overwritten and space
	 *    for 5 bytes created.
	 */
	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len);

	/* 2nd fragment last 10 bytes tailroom, rest occupied */
	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len - 10);

	read_frag = temp_frag = frag;
	read_pos = frag->len - 10;

	/* 3rd fragment, only 5 bytes occupied */
	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);
	net_buf_add(frag, 5);

	temp_frag = net_nbuf_write(buf, temp_frag, temp_frag->len - 10, &pos,
				   30, (uint8_t *) sample_data);
	if (!temp_frag) {
		printk("Use case 6: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(read_frag, read_pos, &read_pos, 30,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 6: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 30)) {
		printk("Usecase 6: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	/* 7) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data.
	 */

	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	frag = net_nbuf_write(buf, frag, NET_IPV6UDPH_LEN, &pos, 10,
			      (uint8_t *)sample_data + 10);
	if (!frag || pos != 58) {
		printk("Usecase 7: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 7: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 10, 10)) {
		printk("Usecase 7: Read data mismatch\n");
		return -EINVAL;
	}

	if (!net_nbuf_insert(buf, frag, NET_IPV6UDPH_LEN, 10,
			     (uint8_t *)sample_data)) {
		printk("Usecase 7: Insert failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, NET_IPV6UDPH_LEN, &read_pos, 20,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 7: Read after failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("Usecase 7: Read data mismatch after insertion\n");
		return -EINVAL;
	}

	/* Insert data outside input fragment length, error case. */
	if (net_nbuf_insert(buf, frag, 70, 10, (uint8_t *)sample_data)) {
		printk("Usecase 7: False insert failed\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	/* 8) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data. Insertion data is long which will
	 * take two fragments.
	 */
	buf = net_nbuf_get_reserve_rx(0);
	net_nbuf_set_ll_reserve(buf, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	net_buf_frag_add(buf, frag);

	frag = net_nbuf_write(buf, frag, NET_IPV6UDPH_LEN, &pos, 10,
			      (uint8_t *)sample_data + 60);
	if (!frag || pos != 58) {
		printk("Usecase 8: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 8: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 60, 10)) {
		printk("Usecase 8: Read data mismatch\n");
		return -EINVAL;
	}

	if (!net_nbuf_insert(buf, frag, NET_IPV6UDPH_LEN, 60,
			     (uint8_t *)sample_data)) {
		printk("Usecase 8: Insert failed\n");
		return -EINVAL;
	}

	read_frag = net_nbuf_read(frag, NET_IPV6UDPH_LEN, &read_pos, 70,
				  read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 8: Read after failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 70)) {
		printk("Usecase 8: Read data mismatch after insertion\n");
		return -EINVAL;
	}

	/* Unref */
	net_nbuf_unref(buf);

	return 0;
}


void main(void)
{
	if (test_ipv6_multi_frags() < 0) {
		goto fail;
	}

	if (test_fragment_copy() < 0) {
		goto fail;
	}

	if (test_fragment_push() < 0) {
		goto fail;
	}

	if (test_fragment_pull() < 0) {
		goto fail;
	}

	if (test_nbuf_read_append() < 0) {
		goto fail;
	}

	if (test_nbuf_read_write_insert() < 0) {
		goto fail;
	}

	printk("nbuf tests passed\n");

	TC_END_REPORT(TC_PASS);
	return;

fail:
	TC_END_REPORT(TC_FAIL);
	return;
}
