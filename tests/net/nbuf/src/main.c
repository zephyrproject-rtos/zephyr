/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <stdio.h>

#include <tc_util.h>

#include <net/nbuf.h>
#include <net/net_ip.h>

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

	linearize(buf, buf_orig, sizeof(orig_len));

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
		     sizeof(struct icmp_hdr), sizeof(buf_orig));
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
			printf("%s %08X ", str, n);
		}

		printf("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
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

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
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

	printk("nbuf tests passed\n");

	TC_END_REPORT(TC_PASS);
	return;

fail:
	TC_END_REPORT(TC_FAIL);
	return;
}
